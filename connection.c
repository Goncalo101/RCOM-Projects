#include "connection.h"
#include "state_machine.h"
#include "flags.h"

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
	// open serial port for reading
  int file_desc = open(filename, O_RDONLY);

	// open file to be sent
  int filename_len = strlen(filename);

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

  printf("TAMANHO: %d 0x%x\n", file_stat.st_size, file_stat.st_size);

  unsigned char start_packet[START_PACKET_LENGTH], file_size_buf[8];
	sprintf(start_packet, "%c%c%c", START_PACKET, FILE_SIZE_PARAM, sizeof(off_t));

	off_t mask = 0xff;

	for (int i = 7; i >= 0; --i) {
		file_size_buf[7-i] = mask & file_size >> (8*i);
	}

	memcpy(&start_packet[3], file_size_buf, 8);

	for (int i = 0; i < 11; ++i) {
		printf("start_packet[%d]: 0x%02x\n", i, start_packet[i]);
	}

  llwrite(fd, start_packet, START_PACKET_LENGTH);
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
