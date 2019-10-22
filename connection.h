#ifndef CONNECTION_H
#define CONNECTION_H

#define READ 0
#define WRITE 1

#define FLAG  0x7e
#define SENDER_CMD 0x03
#define RECEIVER_ANS SENDER_CMD
#define SET_CMD 0x03
#define UACK_CMD 0x07
#define BCC(ADDR, CMD) ADDR ^ CMD

#define TIMEOUT 3
#define MAX_ATTEMPTS 3

#define SIZE_LENGTH 8
#define FILE_SIZE_PARAM 0x00
#define FILE_NAME_PARAM 0x01
#define START_PACKET 0x02
#define END_PACKET 0x03
#define START_PACKET_LENGTH 11

#define TYPE_A_PACKET_LENGTH 5

#define BAUDRATE B38400


typedef enum {
	SET_RET,
	UACK_RET,
	FAIL
} machine_state_ret;

typedef enum {
	START,
	FLAG_RCV,
	A_RCV,
	C_RCV,
	BCC_OK,
	MACHINE_STOP 
} machine_state;

typedef int (*func_ptr)(int);

int llread(int fd, char* buffer); 

int llwrite(int fd, char* buffer, int length);

int llopen(int port, int mode);

int llclose(int fd);

int send_set(int fd);

int send_ack(int fd);

#endif 
