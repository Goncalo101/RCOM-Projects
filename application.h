#ifndef APPLICATION_H
#define APPLICATION_H

typedef int (*func_ptr)(unsigned char*);

void start_app(int port, int mode, unsigned char *filename);
int send_file(unsigned char *filename);
int receive_file(unsigned char *filename);

#endif