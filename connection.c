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

static frame_t received_frame;
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

        printf("read hex: 0x%x ascii:%u\n", buffer[bytes_read], buffer[bytes_read]);
        accept = state_machine(buffer[bytes_read]);
        bytes_read++;

    } while (!accept && alarm_count > 0);
    alarm(0);
    printf("BYTES READ: 0x%02x %d\n", buffer[bytes_read-1], accept);

    if (alarm_count <= 0) {
        printf("Alarm limit reached.\n");
        return ERROR;
    }
    printf("read %d bytes\n", bytes_read);

    return bytes_read;
}

int llwrite(int fd, unsigned char *buffer, int length) {
    int bytes_written = 0, res, counter;

    for (; bytes_written < length; ++bytes_written) {
        res = write(fd, &buffer[bytes_written], sizeof(unsigned char));

        if (res == ERROR) {
            perror("write error");
            return ERROR;
        }

        printf("wrote hex: 0x%x ascii:%u\n", buffer[bytes_written], buffer[bytes_written]);
    }

    printf("wrote %d bytes\n", bytes_written);

    return bytes_written;
}

int check_cmd(int fd, unsigned char cmd_byte, unsigned char *cmd) {
    static int count = 0;
    printf("CMD COUNT: %d\n", ++count);
    int bytes_read = 0;
    while (cmd[2] != cmd_byte) {
        bytes_read = llread(fd, cmd);
        if (bytes_read == ERROR) {
            exit(ERROR);
        }
    }

    return bytes_read;
}

int send_packet(int fd, frame_t *frame) {
    unsigned char *frame_str = build_frame(frame);

    printf("FRAME: %s\n", frame_str);

    int bytes_written = llwrite(fd, frame_str, frame->length);

    //unsigned char *packet = malloc(TYPE_A_PACKET_LENGTH + 1);

    unsigned char cmd;
    if (frame->frame_ctrl == 0) {
        cmd = 0x40;
    } else if (frame->frame_ctrl == 0x40) {
        cmd = 0x0;
    }
    
    check_cmd(fd, cmd, frame_str);
    //free(frame_str);

    return bytes_written;
}

int string_to_int(unsigned char *string){
    // TODO mudar de sitio vai p builder
    off_t num = 0;

    for(int i = 0; i < SIZE_LENGTH; ++i){
        num += string[i] << ((7-i) * 8);
    }

    return num;
}

int get_packet(int fd, frame_t *frame) {
    unsigned char *buffer = malloc(MAX_FRAGMENT_SIZE + 10);
    int bytes_read = llread(fd, buffer);

    if (bytes_read == ERROR) {
        return ERROR;
    }

    buffer = realloc(buffer, bytes_read);
    size_t len;

    switch (buffer[CTRL_POS]) {
        case DATA_PACKET:
            len = buffer[6] * 255 + buffer[7];
            printf("asdasdasdasdasdasdasd %d\n", len);
            buffer = rm_stuffing(buffer, len+6);
            frame->length = len;
            frame->packet->fragment = malloc(len - 4);
            // verificar aqui
            memcpy(frame->packet->fragment, &buffer[8], len - 4);
            bytes_read = len;
            break;
        case END_PACKET:
        case START_PACKET:
            frame->file_info->file_size = string_to_int(&buffer[CTRL_POS+3]);
            frame->file_info->filename = malloc(frame->file_info->file_size + 1);
            strncpy(frame->file_info->filename, &buffer[CTRL_POS + 13], bytes_read - CTRL_POS - 1 - SIZE_LENGTH - 4 - 2);
            break;
    }

    unsigned char buf[TYPE_A_PACKET_LENGTH];
    unsigned char cmd;
    if (buffer[2] == 0) {
        cmd = 0x40;
    } else if (buffer[2] == 0x40) {
        cmd = 0;
    }

    sprintf(buf, "%c%c%c%c%c", FLAG, RECEIVER_ANS, cmd, BCC(RECEIVER_ANS, cmd), FLAG);

    llwrite(fd, buf, TYPE_A_PACKET_LENGTH);

    return bytes_read;
}

int send_set(int fd) {
    unsigned char set_command[TYPE_A_PACKET_LENGTH + 1];
    int bcc = BCC(SENDER_CMD, SET_CMD);

    sprintf(set_command, "%c%c%c%c%c", FLAG, SENDER_CMD, SET_CMD, bcc, FLAG);

    int bytes_written = llwrite(fd, set_command, TYPE_A_PACKET_LENGTH);

    unsigned char ack_command[TYPE_A_PACKET_LENGTH + 1];
    bzero(ack_command, TYPE_A_PACKET_LENGTH + 1);

    int bytes_read = check_cmd(fd, UACK_CMD, ack_command);

    return bytes_read;
}

int send_ack(int fd) {
    unsigned char set_command[TYPE_A_PACKET_LENGTH + 1];
    bzero(set_command, TYPE_A_PACKET_LENGTH + 1);

    int bytes_read = check_cmd(fd, SET_CMD, set_command);

    unsigned char ack_command[TYPE_A_PACKET_LENGTH + 1];
    int bcc = BCC(RECEIVER_ANS, UACK_CMD);

    sprintf(ack_command, "%c%c%c%c%c", FLAG, RECEIVER_ANS, UACK_CMD, bcc, FLAG);

    int bytes_written = llwrite(fd, ack_command, TYPE_A_PACKET_LENGTH);
    return bytes_written;
}

void terminal_setup(int fd) {
    struct termios newtio;

    int tc_attr_status = tcgetattr(fd, &oldtio);
    if (tc_attr_status == ERROR)
    {
        /* save current port settings */
        perror("tcgetattr error");
        exit(ERROR);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
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

int llopen(int port, int mode) {
    unsigned char device[10];
    connection_mode = mode;

    sprintf(device, "/dev/ttyS%d", port);
    puts(device);

    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == ERROR) {
        perror(device);
        return fd;
    }

    terminal_setup(fd);

    sender_func functions[] = {send_set, send_ack};
    functions[mode](fd);

    return fd;
}

int llclose(int fd) {

  unsigned char buffer[TYPE_A_PACKET_LENGTH + 1];
  int bytes_written = 0;
  switch (connection_mode) {
    case 0:
      sprintf(buffer, "%c%c%c%c%c", FLAG, SENDER_CMD, 0xb, BCC(SENDER_CMD, 0xb), FLAG);
      bytes_written+=llwrite(fd, buffer, TYPE_A_PACKET_LENGTH);
      bzero(buffer, TYPE_A_PACKET_LENGTH+1);
      check_cmd(fd, 0xb, buffer);
      sprintf(buffer, "%c%c%c%c%c", FLAG, SENDER_CMD, UACK_CMD, BCC(SENDER_CMD, UACK_CMD), FLAG);
      bytes_written+=llwrite(fd, buffer, TYPE_A_PACKET_LENGTH);
      break;
    case 1:
      check_cmd(fd, 0xb, buffer);
      sprintf(buffer, "%c%c%c%c%c", FLAG, SENDER_CMD, 0xb, BCC(SENDER_CMD, 0xb), FLAG);
      bytes_written+=llwrite(fd, buffer, TYPE_A_PACKET_LENGTH);
      bzero(buffer, TYPE_A_PACKET_LENGTH+1);      
      check_cmd(fd, UACK_CMD, buffer);
      break;
  }
  close(fd);
  return bytes_written;
}
