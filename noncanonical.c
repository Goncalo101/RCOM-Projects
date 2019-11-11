/*Non-Canonical Input Processing*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "application.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;

void alarm_handler() { // printf("Received alarm.\n"); 
}

void register_signal_handler() {
  struct sigaction action;
  sigaction(SIGALRM, NULL, &action);
  action.sa_handler = alarm_handler;
  
  sigaction(SIGALRM, &action, NULL);
}

int main(int argc, char **argv) {
  int mode;
  speed_t baudrate[] = {B38400, B57600, B115200, B230400};

  if(strcmp("receiver", argv[2]) == 0)
    mode = 1;
  else if(strcmp("sender", argv[2]) == 0)
    mode = 0;
  else {
    mode = -1;
  }

  if ((argc < 2) ||
        ((strcmp("/dev/ttyS0", argv[1])!=0) &&
        (strcmp("/dev/ttyS1", argv[1])!=0) &&
        (strcmp("/dev/ttyS4", argv[1])!=0)) || (mode == -1)) {
    // printf("Usage:\tnserial SerialPort mode filename (opt)\n\tex: nserial /dev/ttyS1 sender pinguim.gif\n\tex: nserial /dev/ttyS1 receiver\n");
    exit(1);
  }

  int port = atoi(&argv[1][9]);

  register_signal_handler();  

  for (int i = 0; i < 4; i++)
  {
    // printf("STARTING TEST %d WITH SPEED %d\n", i, baudrate[i]);
    start_app(port, mode, baudrate[i]);
    if (mode == 0) {
      send_file(argv[3]);
    } else if (mode == 1)
      receive_file();
  }
  


  return 0;
}
