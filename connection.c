#include "connection.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

struct termios oldtio;

int llopen(int port, int mode) {
  func_ptr functions[] = {send_set, send_ack};
  char dev[20];
  struct termios newtio;

  sprintf(dev, "/dev/ttyS%d", port);

  int fd = open(dev, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(dev);
    return -1;
  }

  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 5;  /* blocking read until 5 chars received */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  int func_ret = functions[mode](fd);

  if (func_ret != 0)
    return -1;

  return fd;
}

int llclose(int fd) {
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    return -1;
  }

  return close(fd);
}

machine_state_ret check_cmd(char rec_cmd) {
  if (rec_cmd == SET_CMD)
    return SET_RET;
  else if (rec_cmd == UACK_CMD)
    return UACK_RET;
  else
    return FAIL;
}

int state_machine(char rec_byte) {
  machine_state_ret state_ret;
  static machine_state state = START;
  static int addr = 0, cmd = 0;

  switch (state) {
  case START:
    if (rec_byte == FLAG) {
      state = FLAG_RCV;
      break;
    }
    break;
  case FLAG_RCV:
    if (rec_byte == FLAG)
      break;
    else if (rec_byte == RECEIVER_ANS) {
      state = A_RCV;
      addr = rec_byte;
    } else
      state = START;
    break;
  case A_RCV:
    state_ret = check_cmd(rec_byte);
    if (rec_byte == FLAG)
      state = FLAG_RCV;
    else if (state_ret != FAIL) {
      state = C_RCV;
      cmd = rec_byte;
    } else
      state = START;
    break;
  case C_RCV:
    if (rec_byte == FLAG)
      state = FLAG_RCV;
    else if (rec_byte == BCC(addr, cmd))
      state = BCC_OK;
    else
      state = START;
    break;
  case BCC_OK:
    if (rec_byte == FLAG) {
      state = MACHINE_STOP;
      return 1;
    } else
      state = START;
    break;
  default:
    return -1;
  }
  return 0;
}

int llread(int fd, char *buffer) {
  int nbytes = 0, accept = 0, res;

  bzero(buffer, strlen(buffer));
  do {
    res = read(fd, &buffer[nbytes], sizeof(char));
    if (res == -1) {
      if (errno == EINTR) {
        printf("read timeout\n");
        return -2;
      }
      printf("read failed\n");
      return -1;
    }

    accept = state_machine(buffer[nbytes]);
    printf("read hex: 0x%x ascii:%u\n", buffer[nbytes], buffer[nbytes]);
    nbytes++;
  } while (!accept);

  printf("> %s\n", buffer);
  printf("read %d bytes\n", nbytes);

  return nbytes;
}

int llwrite(int fd, char *buffer, int length) {
  int nbytes = 0, res;

  for (; nbytes < length; ++nbytes) {
    res = write(fd, &buffer[nbytes], sizeof(char));
    if (res == -1) {
      printf("write failed\n");
      return -1;
    }
    printf("wrote hex: 0x%x ascii:%u\n", buffer[nbytes], buffer[nbytes]);
  }
  printf("< %s\n", buffer);
  printf("wrote %d bytes\n", nbytes);

  return nbytes;
}

int send_file(int fd, char *filename) {
  int file_desc = open(filename, O_RDONLY);
  int filename_len = strlen(filename);

  if (file_desc == -1) {
    perror("Unable to open file");
    return -1;
  }

  struct stat file_stat;
  fstat(file_desc, &file_stat);
  off_t sizeFile = file_stat.st_size;

  printf("TAMANHO: %d 0x%x\n", file_stat.st_size, file_stat.st_size);

  int total_size = 7 * sizeof(unsigned char) + filename_len;
  unsigned char *start_packet = malloc(total_size);

  start_packet[0] = START_PACKET;    // C
  start_packet[1] = FILE_SIZE_PARAM; // T1
  start_packet[2] = L1;              // L1
  start_packet[3] = sizeFile >> 8;   // V1
  start_packet[4] = sizeFile;        // V1
  start_packet[5] = FILE_NAME_PARAM; // T2
  start_packet[6] = filename_len;    // L2

  for (int i = 0; i < filename_len; i++) // V2 (NOME DO FICHEIRO)
    start_packet[7 + i] = filename[i];

  for (int i = 0; i < total_size; i++)
    printf("start_packet[%d]:   0x%x\n", i, start_packet[i]);

  printf("Nome ficheiro: %s\n", &start_packet[7]);

  /*
char size_param[SIZE_LENGTH];
sprintf(size_param, "%c%c%lu", FILE_SIZE_PARAM, sizeof(off_t),
file_stat.st_size); printf("size: %lx\n", file_stat.st_size); for (int i = 0; i
< SIZE_LENGTH; ++i) { printf("0x%x\n", strlen(size_param));
}
printf("--------\n");

char *filename_param = malloc(strlen(filename) + 2);
sprintf(filename_param, "%c%c%s", FILE_NAME_PARAM, strlen(filename), filename);

char start_packet[] = {START_PACKET, '\0'};
strcat(start_packet, size_param);
for (int i = 0; i < strlen(start_packet); ++i) {
printf("0x%x\n", start_packet[i]);
}
strcat(start_packet, filename_param);
// sprintf(start_packet, "%c%s%s", START_PACKET, size_param, filename_param);
*/
  // free(filename_param);

  llwrite(fd, start_packet, strlen(start_packet));
}

int send_set(int fd) {
  char set_sequence[TYPE_A_PACKET_LENGTH];

  sprintf(set_sequence, "%c%c%c%c%c", FLAG, SENDER_CMD, SET_CMD,
          BCC(SENDER_CMD, SET_CMD), FLAG);

  int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
  if (bytes_written == -1)
    return bytes_written;

  char ack_sequence[TYPE_A_PACKET_LENGTH];
  int count = 0, bytes_read;

  do {
    alarm(TIMEOUT);
    bytes_read = llread(fd, ack_sequence);
    // TODO: VERIFICAR SE RECEBEU O ACK_CMD

    if (bytes_read != -2)
      break;
  } while (count++ < MAX_ATTEMPTS && bytes_read < 0);

  alarm(0);

  send_file(fd, "pinguim.gif");

  return 0;
}

int send_ack(int fd) {
  char set_sequence[TYPE_A_PACKET_LENGTH];

  int bytes_read = llread(fd, set_sequence);
  if (bytes_read == -1)
    return bytes_read;
  // TODO: VERIFICAR SE RECEBEU O SET_CMD

  sprintf(set_sequence, "%c%c%c%c%c", FLAG, RECEIVER_ANS, UACK_CMD,
          BCC(RECEIVER_ANS, UACK_CMD), FLAG);

  int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
  if (bytes_written == -1)
    return bytes_written;

  return 0;
}
