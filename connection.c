#include "connection.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int llread(int fd, char* buffer){
	int nbytes = 0, res;
	
	bzero(buffer, strlen(buffer));
  	do {
  		res = read(fd, &buffer[nbytes], sizeof(char));
  		printf("cenas\n");
  		if (res == -1) {
			if (errno == EINTR) {
				printf("read timeout\n");
				return -2;
			}
  			printf("read failed\n");
  			return -1;
  		}
		printf("read hex: 0x%x ascii:%u\n", buffer[nbytes], buffer[nbytes]);
  	} while(buffer[nbytes++] != '\0');
  	
  	printf("> %s\n", buffer);
	printf("read %d bytes\n", nbytes);

  	return nbytes;
}

int state_machine(char rec_byte) {
	static machine_state state = START;
	static int addr = 0, cmd = 0;

	switch(state) {
		case START: 
			if (rec_byte == FLAG) { 
				state = FLAG_RCV;
				break;
			}	
			break;
		case FLAG_RCV:
			if (rec_byte == FLAG) break;
			else if (rec_byte == RECEIVER_ANS) {
				state = A_RCV;
				addr = rec_byte;
			} else state = START;
			break;
		case A_RCV:
			if (rec_byte == FLAG) state = FLAG_RCV;
			else if (rec_byte == SET_CMD) {
				state = C_RCV;
				cmd = rec_byte;
			} else state = START;
			break;
		case C_RCV:
			if (rec_byte == FLAG) state = FLAG_RCV;
			else if (rec_byte == BCC(addr, cmd)) state = BCC_OK;
			else state = START;
			break;
		case BCC_OK:
			if (rec_byte == FLAG) state = MACHINE_STOP;
			else state = START;
			break;
		case MACHINE_STOP:
			return 1;
		default:return -1;
	}

	return 0;
}

int llread2(int fd, char* buffer){
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
  	} while (!accept);
  	
  	printf("> %s\n", buffer);
	printf("read %d bytes\n", nbytes);

  	return nbytes;
}

int llwrite(int fd, char* buffer, int length){
	int nbytes = 0, res;

    for (; nbytes < length; ++nbytes)
    {
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

int send_set(int fd){
  char set_sequence[5+1];

  sprintf(set_sequence, "%c%c%c%c%c", FLAG, SENDER_CMD, SET_CMD, BCC(SENDER_CMD, SET_CMD), FLAG);

  int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
  if(bytes_written == -1) return bytes_written;

  char ack_sequence[TYPE_A_PACKET_LENGTH];
  int count = 0, bytes_read;

  do {
	alarm(TIMEOUT);  
	bytes_read = llread2(fd, ack_sequence);
	if (bytes_read != -2) break;
  } while(count++ < MAX_ATTEMPTS && bytes_read < 0);
  
  alarm(0);
   
  return 0;
}

int send_ack(int fd){
  char set_sequence[TYPE_A_PACKET_LENGTH];

  int bytes_read = llread2(fd, set_sequence);
  if(bytes_read == -1) return bytes_read;


  sprintf(set_sequence, "%c%c%c%c%c", FLAG, RECEIVER_ANS, UACK_CMD, BCC(RECEIVER_ANS, UACK_CMD), FLAG);

  int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
  if(bytes_written == -1) return bytes_written;

  return 0;
}
