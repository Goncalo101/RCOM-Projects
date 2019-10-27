#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "application.h"
#include "connection.h"
#include "flags.h"

static int fd = 0;

off_t get_file_size(char *filename) {
    struct stat file_stat;
    stat(filename, &file_stat);

    return file_stat.st_size;
}

int send_file(char *filename) {
    off_t file_size = get_file_size(filename);
    int file_desc = open(filename, O_RDONLY, 0777);

    if(file_desc == ERROR){
        perror("file descriptor");
        return ERROR;
    }

    frame_t frame;

    file_t file_info = {.file_size = file_size, .filename = filename};

    frame.file_info = &file_info;

    send_packet(fd, &frame);

    
    off_t bytes_read = 0;
    int bytes_written = 0;

    char pinguim[64+1];
    char control[2] = {0, 0x40};
    int counter = 0;
    while(bytes_read < file_size){
        bytes_read += read(file_desc, pinguim, 64);
        if(bytes_read == ERROR) perror("ERRO");
        printf("BYTES READ: %d\n", bytes_read);

        packet_t packet = {.fragment = pinguim, .addr = SENDER_CMD, .ctrl = control[counter%2], .length = 64};
        frame.packet = &packet;

        bytes_written = send_packet(fd, &frame);
        ++counter;
    }

    return 0;
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

