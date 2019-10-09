#ifndef CONNECTION_H
#define CONNECTION_H

#define READ 0
#define WRITE 1

#define FLAG  0x7e
#define SENDER_CMD 0x03
#define RECEIVER_ANS SENDER_CMD
#define SET_CMD 0x03
#define UACK_CMD 0x07
#define BCC(CMD) FLAG ^ CMD

#define TIMEOUT 3
#define MAX_ATTEMPTS 3

#define TYPE_A_PACKET_LENGTH 6

int llread(int fd, char* buffer); 

int llwrite(int fd, char* buffer, int length);

int send_set(int fd);

int send_ack(int fd);

#endif 
