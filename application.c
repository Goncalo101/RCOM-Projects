#include <stdlib.h>
#include <stdio.h>

#include "application.h"
#include "connection.h"

int send_file(char *filename) {

}

int receive_file(char *filename) {
    
}

void start_app(int port, int mode, char *filename) {
    int fd = llopen(port, mode);
    if (fd == -1) {
        perror("llopen error");
        exit(-1);
    }

    func_ptr functions[] = {send_file, receive_file};

}

