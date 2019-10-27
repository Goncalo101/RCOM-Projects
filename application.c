#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "application.h"
#include "connection.h"
#include "flags.h"
#include "types.h"

static int fd = 0;

off_t get_file_size(int fd) {
    struct stat file_stat;
    fstat(fd, &file_stat);

    return file_stat.st_size;
}

int send_file(char *filename) {
    // open file and check for errors
    int file_desc = open(filename, O_RDONLY, 0777);

    if (file_desc == ERROR) {
        perror("file descriptor");
        return ERROR;
    }

    // get file properties (file size and filename length)
    off_t file_size = get_file_size(file_desc);
    size_t filename_len = strlen(filename);

    // build frame structure
    frame_t frame;
    file_t file_info = {.file_size = file_size};
    file_info.filename = malloc(filename_len);
    strcpy(file_info.filename, filename);
    
    frame.request_type = CTRL_REQ;
    frame.length = filename_len;
    frame.file_info = &file_info;
    frame.packet_ctrl = START_PACKET;
    frame.frame_ctrl = 0x01; // TODO change to proper value
    frame.addr = SENDER_CMD;
    
    printf("sending %s (name length %d, file size %ld)\n", file_info.filename, frame.length, file_size);

    // send frame
    send_packet(fd, &frame);
    return 0;

    // off_t bytes_read = 0;
    // int bytes_written = 0;

    // char pinguim[64+1];
    // char control[2] = {0, 0x40};
    // int counter = 0;
    // while(bytes_read < file_size){
    //     bytes_read += read(file_desc, pinguim, 64);
    //     if(bytes_read == ERROR) perror("ERRO");
    //     printf("BYTES READ: %d\n", bytes_read);

    //     packet_t packet = {.fragment = pinguim, .addr = SENDER_CMD, .ctrl = control[counter%2]};
    //     frame.packet = &packet;

    //     bytes_written = send_packet(fd, &frame);
    //     ++counter;
    // }

    // return 0;
}

int receive_file(char *filename) {
    return 0;
}

void start_app(int port, int mode, char *filename) {
    fd = llopen(port, mode);
    if (fd == -1) {
        perror("llopen error");
        exit(-1);
    }

}

