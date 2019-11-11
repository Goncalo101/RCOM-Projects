#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "utils/builders.h"
#include "utils/state_machine.h"
#include "utils/strmanip.h"
#include "connection.h"
#include "flags.h"

static struct termios oldtio;
static int connection_mode = 0;

int llread(int fd, unsigned char *buffer) {
    int bytes_read = 0, accept = 0, res = 0, alarm_count = MAX_ALARM_COUNT;
    
    do {
        alarm(TIMEOUT);
        res = read(fd, &buffer[bytes_read], sizeof(unsigned char));
        if (res == ERROR) {
            if (errno == EINTR) {
                --alarm_count;
                printf("read timeout %d\n", MAX_ALARM_COUNT - alarm_count);
                continue;
            }

            perror("read failed");
            return ERROR;
        }
        #ifdef debug
        printf(" %02x ", buffer[bytes_read]);
        #endif
        accept = state_machine(buffer[bytes_read]);
        if (accept == -5) continue;
        bytes_read++;

    } while (!accept && alarm_count > 0);

    if(accept == -2) return accept;
    alarm(0);
    printf("\nread %d bytes, accept %d\n", bytes_read, accept);

    if (alarm_count <= 0) {
        printf("Alarm limit reached.\n");
        return ERROR;
    }

    return bytes_read;
}

int llwrite(int fd, unsigned char *buffer, int length) {
    int bytes_written = 0, res;

    for (; bytes_written < length; ++bytes_written) {
        res = write(fd, &buffer[bytes_written], sizeof(unsigned char));

        if (res == ERROR) {
            perror("write error");
            return ERROR;
        }
        #ifdef debug
        printf(" %02x ", buffer[bytes_written]);
        #endif
    }

    printf("\nwrote %d bytes\n", bytes_written);

    return bytes_written;
}

int check_cmd(int fd, unsigned char cmd_byte, unsigned char *cmd) {
    int bytes_read = 0;
    do { 
        bytes_read = llread(fd, cmd);
        if (bytes_read == ERROR) {
            return ERROR;
        }
        if (cmd[bytes_read - 3] == 0x01 || cmd[bytes_read - 3] == 0x81) return -2;
    } while (cmd[bytes_read - 3] != cmd_byte);

    return bytes_read;
}

int send_packet(int fd, frame_t *frame) {
    unsigned char *frame_str = NULL;
    build_frame(frame, &frame_str);

    unsigned char cmd;
    unsigned char buf[TYPE_A_PACKET_LENGTH + 1];
    bzero(buf, TYPE_A_PACKET_LENGTH + 1);
    if (frame->frame_ctrl == 0) {
        cmd = 0x85;
    } else if (frame->frame_ctrl == 0x40) {
        cmd = 0x05;
    }

    int bytes_written = llwrite(fd, frame_str, frame->length), counter = MAX_RETRIES;
    int cmd_stat = 0;
    while ((cmd_stat = check_cmd(fd, cmd, buf)) < 0 && counter > 0) {
        if(cmd_stat == -1) --counter;
        bytes_written = llwrite(fd, frame_str, frame->length);
		if (frame->frame_ctrl == 0) {
        	cmd = 0x85;
    	} else if (frame->frame_ctrl == 0x40) {
        	cmd = 0x05;
   		}
    }
    if (counter <= 0) exit(-1);
    
    free(frame_str);

    return bytes_written;
}

int string_to_int(unsigned char *string){
    off_t num = 0;

    for(int i = 0; i < SIZE_LENGTH; ++i){
        num += string[i] << ((7-i) * 8);
    }

    return num;
}

int get_packet(int fd, frame_t *frame) {
    unsigned char *buffer = malloc(2 * MAX_FRAGMENT_SIZE + 14);
    char buf[TYPE_A_PACKET_LENGTH + 1];
    int bytes_read = 0, counter = MAX_RETRIES;
    unsigned char cmd;

    while ((bytes_read = llread(fd, buffer)) < 0 && --counter > 0) {
        if (bytes_read == ERROR) {
            return ERROR;
        } else if(bytes_read == -2) {
            if (buffer[2] == 0) {
                cmd = 0x81;
            } else if (buffer[2] == 0x40) {
                cmd = 0x01;
            }
            sprintf(buf, "%c%c%c%c%c", FLAG, RECEIVER_ANS, cmd, BCC(RECEIVER_ANS, cmd), FLAG);
            llwrite(fd, (unsigned char *) buf, TYPE_A_PACKET_LENGTH);
        }
    }

    if (counter <= 0) {
        printf("number of tries exceeded (connection lost?), exiting\n");
        exit(-1);
    }

    buffer = realloc(buffer, bytes_read);

    size_t len;

    switch (buffer[CTRL_POS]) {
        case DATA_PACKET:
            len = buffer[6] * 255 + buffer[7];
            rm_stuffing(&buffer, len+8);
            frame->length = len;
            frame->packet->fragment = malloc(len);
            memcpy(frame->packet->fragment, &buffer[8], len);
            bytes_read = len;
            break;
        case END_PACKET:
        case START_PACKET:
            frame->file_info->file_size = string_to_int(&buffer[CTRL_POS+3]);
            frame->file_info->filename = malloc(frame->file_info->file_size + 1);
            strncpy((char *) (frame->file_info->filename), (char *) (&buffer[CTRL_POS + 13]), bytes_read - CTRL_POS - 1 - SIZE_LENGTH - 4 - 2);
            break;
    }


    if (buffer[2] == 0) {
        cmd = 0x85;
    } else if (buffer[2] == 0x40) {
        cmd = 0x05;
    }

    sprintf(buf, "%c%c%c%c%c", FLAG, RECEIVER_ANS, cmd, BCC(RECEIVER_ANS, cmd), FLAG);
    llwrite(fd, (unsigned char *) buf, TYPE_A_PACKET_LENGTH);
    
    free(buffer);

    return bytes_read;
}

int send_set(int fd) {
    char set_command[TYPE_A_PACKET_LENGTH + 1];
    int bcc = BCC(SENDER_CMD, SET_CMD);

    sprintf(set_command, "%c%c%c%c%c", FLAG, SENDER_CMD, SET_CMD, bcc, FLAG);

    int bytes_written = llwrite(fd, (unsigned char *) set_command, TYPE_A_PACKET_LENGTH);
    if (bytes_written < 0) exit(-1);

    char ack_command[TYPE_A_PACKET_LENGTH + 1];

    int bytes_read = check_cmd(fd, UACK_CMD, (unsigned char *) ack_command);

    return bytes_read;
}

int send_ack(int fd) {
    char set_command[TYPE_A_PACKET_LENGTH + 1];

    int bytes_read = check_cmd(fd, SET_CMD, (unsigned char *) set_command);
    if (bytes_read < 0) exit(-1);

    char ack_command[TYPE_A_PACKET_LENGTH + 1];
    int bcc = BCC(RECEIVER_ANS, UACK_CMD);

    sprintf(ack_command, "%c%c%c%c%c", FLAG, RECEIVER_ANS, UACK_CMD, bcc, FLAG);

    int bytes_written = llwrite(fd, (unsigned char *) ack_command, TYPE_A_PACKET_LENGTH);
    return bytes_written;
}

void terminal_setup(int fd, speed_t baudrate) {
    struct termios newtio;

    int tc_attr_status = tcgetattr(fd, &oldtio);
    if (tc_attr_status == ERROR)
    {
        /* save current port settings */
        perror("tcgetattr error");
        exit(ERROR);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-unsigned character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 5 unsigned chars received */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == ERROR)
    {
        perror("tcsetattr error");
        exit(ERROR);
    }

    printf("New termios structure set\n");
}

int llopen(int port, int mode, speed_t baudrate) {
    char device[11];
    connection_mode = mode;

    sprintf(device, "/dev/ttyS%d", port);
    printf("Opened port %s successfully\n", device);

    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == ERROR) {
        perror(device);
        return fd;
    }

    terminal_setup(fd, baudrate);

    sender_func functions[] = {send_set, send_ack};
    functions[mode](fd);

    return fd;
}

int llclose(int fd) {

  char buffer[TYPE_A_PACKET_LENGTH + 1];
  int bytes_written = 0;
  switch (connection_mode) {
    case 0:
      sprintf(buffer, "%c%c%c%c%c", FLAG, SENDER_CMD, 0xb, BCC(SENDER_CMD, 0xb), FLAG);
      bytes_written+=llwrite(fd, (unsigned char *) buffer, TYPE_A_PACKET_LENGTH);
      bzero(buffer, TYPE_A_PACKET_LENGTH+1);
      check_cmd(fd, 0xb, (unsigned char *) buffer);
      sprintf(buffer, "%c%c%c%c%c", FLAG, SENDER_CMD, UACK_CMD, BCC(SENDER_CMD, UACK_CMD), FLAG);
      bytes_written+=llwrite(fd, (unsigned char *) buffer, TYPE_A_PACKET_LENGTH);
      break;
    case 1:
      check_cmd(fd, 0xb, (unsigned char*) buffer);
      sprintf(buffer, "%c%c%c%c%c", FLAG, SENDER_CMD, 0xb, BCC(SENDER_CMD, 0xb), FLAG);
      bytes_written+=llwrite(fd, (unsigned char *) buffer, TYPE_A_PACKET_LENGTH);
      bzero(buffer, TYPE_A_PACKET_LENGTH+1);      
      check_cmd(fd, UACK_CMD, (unsigned char *) buffer);
      break;
  }
  close(fd);
  return bytes_written;
}