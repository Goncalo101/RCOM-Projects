#ifndef APPLICATION_H
#define APPLICATION_H

typedef int (*func_ptr)(char*);

void start_app(int port, int mode, char *filename);

#endif