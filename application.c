#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "application.h"
#include "connection.h"

static int fd = 0;

off_t get_file_size(char *filename) {
    struct stat file_stat;
    stat(filename, &file_stat);

    return file_stat.st_size;
}

int send_file(char *filename) {
    off_t file_size = get_file_size(filename);
    off_t bytes_written = 0;

    int file_desc = open(filename, O_WRONLY, 0777);

    if(file_desc == ERROR){
        perror("file descriptor");
        return ERROR;
    }

    while(bytes_written < file_size){
        
        ++bytes_written;
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

