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

int llwrite(int fd, char* buffer, int length){
	int nbytes = 0, res;

    for (; nbytes <= length; ++nbytes)
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

  sprintf(set_sequence, "%c%c%c%c%c", FLAG, SENDER_CMD, SET_CMD, BCC(SET_CMD), FLAG);

  int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
  if(bytes_written == -1) return bytes_written;

  char ack_sequence[TYPE_A_PACKET_LENGTH];
  int count = 0, bytes_read;

  do {
	alarm(TIMEOUT);  
	bytes_read = llread(fd, ack_sequence);
	if (bytes_read != -2) break;
  } while(count++ < MAX_ATTEMPTS && bytes_read < 0);
  
  alarm(0);
   
  return 0;
}

int send_ack(int fd){
  char set_sequence[TYPE_A_PACKET_LENGTH];

  int bytes_read = llread(fd, set_sequence);
  if(bytes_read == -1) return bytes_read;


  sprintf(set_sequence, "%c%c%c%c%c", FLAG, RECEIVER_ANS, UACK_CMD, BCC(UACK_CMD), FLAG);

  int bytes_written = llwrite(fd, set_sequence, TYPE_A_PACKET_LENGTH);
  if(bytes_written == -1) return bytes_written;

  return 0;
}
