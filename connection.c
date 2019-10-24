#include "connection.h"
#include "flags.h"
#include "state_machine.h"

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

int llread(int fd, char *buffer) {
  int nbytes = 0, accept = 0, res;

  bzero(buffer, strlen(buffer));
  do {
    if (nbytes >= 64) {
      buffer = realloc(buffer, 64);
    }
    
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

int process_msg(char *to_read, packet_t packet[], int bytes_read) {
  int p = 0;
  for (int i = 4; i < bytes_read - 2; i += 5, ++p) {
    packet[p].ctrl = to_read[i];
    packet[p].seq_no = to_read[i + 1];
    packet[p].oct_num = to_read[i + 2] * 256 + to_read[i + 3];
    packet[p].data = malloc(packet[p].oct_num);
    memcpy(packet[p].data, &to_read[i + 4], packet[p].oct_num);
    
    i += packet[p].oct_num;
  }

  printf("packet data %#04x\n", to_read[0]);

  return p;
}

int receive_file(int fd) {
  packet_t packet[5];
  char *to_read = malloc(PACKET_SIZE);
  int nbytes;
  char file_size_buf[8];
  off_t file_size;

  int pinguim = open("pinguim1.gif", O_WRONLY | O_CREAT, 0777);

  while ((nbytes = llread(fd, to_read))) {
    if (to_read[5] == 0x02 && to_read[3] == 0x00) {
      sscanf(&to_read[8], "%ld", &file_size);
      printf("file size %ld\n", file_size);
      continue;
    }
    
    int packet_num = process_msg(to_read, packet, nbytes);
    int written = write(pinguim, packet[0].data, nbytes - 6 - 4 * packet_num + packet[0].oct_num);
    perror("ERRO");
    printf("ERRNO %d\n", errno);

    printf("wrote %d bytes on receive_file", written);
  }
}

int send_file(int fd, char *filename) {
  // open serial port for reading
  int file_desc = open(filename, O_RDONLY);

  // open file to be sent
  if (file_desc == -1) {
    perror("Unable to open file");
    return -1;
  }

  // use stat to find the length of the file
  struct stat file_stat;
  if (fstat(file_desc, &file_stat) == -1) {
    perror("Unable to stat file");
  }

  off_t file_size = file_stat.st_size;

  char start_packet[START_PACKET_LENGTH+8], file_size_buf[8];
  sprintf(start_packet, "%c%c%c%ld", START_PACKET, FILE_SIZE_PARAM, 8, file_size);

  off_t mask = 0xff;

  // for (int i = 7; i >= 0; --i) {
  //   file_size_buf[7 - i] = mask & file_size >> (8 * i);
  // }

  // memcpy(&start_packet[3], file_size_buf, 8);

  char *to_send = build_packet(SENDER_CMD, 0, start_packet);

  return llwrite(fd, to_send, 5 + 11 + 1);
}

int send_set(int fd) {
  char *set_sequence = build_packet(SENDER_CMD, SET_CMD, NULL);

  int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH + 1);
  if (bytes_written == -1)
    return bytes_written;

  char ack_sequence[TYPE_A_PACKET_LENGTH + 1];
  int count = 0, bytes_read;

  do {
    alarm(TIMEOUT);
    bytes_read = llread(fd, ack_sequence);

    if (bytes_read != -2)
      break;
  } while (count++ < MAX_ATTEMPTS && bytes_read < 0 &&
           ack_sequence[2] != UACK_CMD);

  alarm(0);

  send_file(fd, "pinguim.gif");

  return 0;
}

int send_ack(int fd) {
  char set_sequence[TYPE_A_PACKET_LENGTH + 1];

  do {
    int bytes_read = llread(fd, set_sequence);
    if (bytes_read == -1)
      return bytes_read;
  } while (set_sequence[2] != SET_CMD);

  char *ack_sequence = build_packet(RECEIVER_ANS, UACK_CMD, NULL);

  if (ack_sequence == NULL)
    return -1;

  int bytes_written = llwrite(fd, ack_sequence, TYPE_A_PACKET_LENGTH);
  if (bytes_written == -1)
    return bytes_written;
  
  receive_file(fd);

  return 0;
}

char *build_packet(char address, char control, char *data) {
  size_t mem_size = TYPE_A_PACKET_LENGTH;
  char *packet;
  if (data != NULL) {
    mem_size += 11;
    packet = malloc(mem_size + 1);

    sprintf(packet, "%c%c%c%c", FLAG, address, control, BCC(address, control));
    memcpy(&packet[4], data, 11);
    sprintf(&packet[5 + 11], "%c", FLAG);

  } else {
    packet = malloc(mem_size + 1);

    sprintf(packet, "%c%c%c%c%c", FLAG, address, control, BCC(address, control),
            FLAG);
  }

  return packet;
}