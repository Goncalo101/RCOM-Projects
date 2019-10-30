#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/builders.h"
#include "utils/strmanip.h"
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
    char control[2] = {0, 0x40};
    int counter = 0;
    prepare_control_frame(&frame, file_size, filename_len, filename, SENDER_CMD, CTRL_REQ, START_PACKET, control[counter % 2]);
    ++counter;

    printf("sending %s (name length %d, file size %ld)\n", frame.file_info->filename, frame.length, file_size);

    // send frame
    if (send_packet(fd, &frame) == ERROR) return ERROR;

    // send file
    off_t total_read = 0;
    int bytes_written = 0;

    char *file_fragment = malloc(MAX_FRAGMENT_SIZE);

    // prepare data frame
    frame.request_type = DATA_REQ;
    frame.packet_ctrl = DATA_PACKET;
    frame.addr = SENDER_CMD;

    packet_t packet;
    packet.fragment = malloc(MAX_FRAGMENT_SIZE);

    while (total_read < file_size) {
        int bytes_read = read(file_desc, file_fragment, MAX_FRAGMENT_SIZE);
        total_read += bytes_read;

        if (bytes_read < MAX_FRAGMENT_SIZE)
            file_fragment = realloc(file_fragment, bytes_read);

        if (bytes_read == ERROR) perror("ERRO");
        printf("BYTES READ: %d\n", total_read);

        packet.fragment = realloc(packet.fragment, bytes_read);
        memcpy(packet.fragment, file_fragment, bytes_read);

        frame.packet = &packet;
        frame.length = bytes_read;
        frame.frame_ctrl = control[counter % 2];

        bytes_written = send_packet(fd, &frame);
        ++counter;
    }

    // prepare_control_frame(&frame, file_size, filename_len, filename, SENDER_CMD, CTRL_REQ, END_PACKET, control[counter % 2]);
    // if (send_packet(fd, &frame) == ERROR) return ERROR;

    // free(file_fragment);
    // free(packet.fragment);

    return 0;
}

int receive_file(char *filename) {
    frame_t frame;
    frame.file_info = malloc(sizeof(file_t));
    get_packet(fd, &frame);

    printf("received file size: %d, file name: %s\n", frame.file_info->file_size, frame.file_info->filename);

    off_t bytes_read = 0;
    off_t file_size = frame.file_info->file_size;
    free(frame.file_info);

    frame.packet = malloc(sizeof(packet_t));
    int file_desc = open(filename, O_WRONLY | O_CREAT, 0777);

    while (bytes_read < file_size) {
        int read = get_packet(fd, &frame);
        if (read == ERROR) exit(-1);

        write(file_desc, frame.packet->fragment, read);
        bytes_read += read;
    }

    return 0;
}

void start_app(int port, int mode, char *filename) {
    fd = llopen(port, mode);
    if (fd == -1) {
        perror("llopen error");
        exit(-1);
    }
}
