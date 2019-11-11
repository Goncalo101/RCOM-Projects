#ifndef APPLICATION_H
#define APPLICATION_H
#include <termios.h>


void start_app(int port, int mode, speed_t baudrate);
int send_file(char *filename);
int receive_file();

#endif