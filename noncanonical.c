/*Non-Canonical Input Processing*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;

void alarm_handler() { printf("Received alarm.\n"); }

void register_signal_handler() {
  struct sigaction action;
  action.sa_handler = alarm_handler;
  sigaction(SIGALRM, &action, NULL);
}

int main(int argc, char **argv) {
  if ((argc < 4) ||
        ((strcmp("/dev/ttyS0", argv[1])!=0) &&
        (strcmp("/dev/ttyS1", argv[1])!=0) &&
        (strcmp("/dev/ttyS4", argv[1])!=0))) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  register_signal_handler();

  int mode = atoi(argv[2]), port = atoi(&argv[1][9]);
  char *filename = malloc(strlen(argv[3]) + 1);
  strcpy(filename, argv[3]);

  start_app(port, mode, filename);

  func_ptr functions[] = {send_file, receive_file};
  functions[mode](filename);

  free(filename);


  // Reenvio

  /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o
  */

  return 0;
}
