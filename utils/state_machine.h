#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    MACHINE_STOP
} machine_state;

int state_machine(char rec_byte);

#endif