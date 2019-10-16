#include "connection.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>




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

int llopen(int port, int mode){
	func_ptr functions[] = {send_set, send_ack};
	char dev[20];
	struct termios oldtio,newtio;



	sprintf(dev, "/dev/ttyS%d", port);

	int fd = open(dev, O_RDWR | O_NOCTTY );
	if (fd <0) {
		perror(dev); 
		return -1; 
	}

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }


    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
	
    printf("New termios structure set\n");

	int func_ret = functions[mode](fd);


	if(func_ret != 0) return -1;


    tcsetattr(fd,TCSANOW,&oldtio);


	return fd;
}

machine_state_ret check_cmd(char rec_cmd){
	if(rec_cmd == SET_CMD) return SET_RET;
	else if(rec_cmd == UACK_CMD) return UACK_RET;
	else return FAIL;
}

int state_machine(char rec_byte) {
	machine_state_ret state_ret;
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
		state_ret = check_cmd(rec_byte);
		if (rec_byte == FLAG) state = FLAG_RCV;
		else if (state_ret != FAIL) {
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
		if (rec_byte == FLAG) {
			state = MACHINE_STOP;
			return 1;
		}
		else state = START;
		break;
		default: return -1;
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
		nbytes++;
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
	char set_sequence[TYPE_A_PACKET_LENGTH];

	sprintf(set_sequence, "%c%c%c%c%c", FLAG, SENDER_CMD, SET_CMD, BCC(SENDER_CMD, SET_CMD), FLAG);

	int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
	if(bytes_written == -1) return bytes_written;

	char ack_sequence[TYPE_A_PACKET_LENGTH];
	int count = 0, bytes_read;

	do {
		alarm(TIMEOUT);  
		bytes_read = llread2(fd, ack_sequence);
  //TODO: VERIFICAR SE RECEBEU O ACK_CMD

		if (bytes_read != -2) break;
	} while(count++ < MAX_ATTEMPTS && bytes_read < 0);

	alarm(0);

	return 0;
}

int send_ack(int fd){
	char set_sequence[TYPE_A_PACKET_LENGTH];

	int bytes_read = llread2(fd, set_sequence);
	if(bytes_read == -1) return bytes_read;
  	//TODO: VERIFICAR SE RECEBEU O SET_CMD


	sprintf(set_sequence, "%c%c%c%c%c", FLAG, RECEIVER_ANS, UACK_CMD, BCC(RECEIVER_ANS, UACK_CMD), FLAG);

	int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
	if(bytes_written == -1) return bytes_written;

	return 0;
}
