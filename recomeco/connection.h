#ifndef CONNECTION_H
#define CONNECTION_H

#define READ 0
#define WRITE 1

#define TIMEOUT 3
#define MAX_ATTEMPTS 3

#define START_PACKET_LENGTH 11

#define PACKET_SIZE 64


#define BAUDRATE B38400

typedef int (*func_ptr)(int);

int llread(int fd, char *buffer);

int llwrite(int fd, char *buffer, int length);

int llopen(int port, int mode);

int llclose(int fd);

int send_set(int fd);

int send_ack(int fd);

char *build_packet(char address, char control, char *data);

#endif
