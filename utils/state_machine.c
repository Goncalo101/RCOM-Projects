#include "../flags.h"
#include "state_machine.h"



int state_machine(char rec_byte) {
    static machine_state state = START;
    static char cmd = 0, addr = 0; 

    switch (state)
    {
    case START:
        if(rec_byte == FLAG) 
            state = FLAG_RCV;
        break;
    case FLAG_RCV:
        if(rec_byte == SENDER_CMD || rec_byte == RECEIVER_CMD){
            state = A_RCV;
            addr = rec_byte;
        }
        else if(rec_byte == FLAG)
            state = FLAG_RCV;
        else state = START;
        break;
    case A_RCV:
        if(rec_byte == SET_CMD || rec_byte == UACK_CMD){
            state = C_RCV;
            cmd = rec_byte;
        }
        else if(rec_byte == FLAG)
            state = FLAG_RCV;
        else state = START;
        break;
    case C_RCV:
        if(rec_byte == BCC(addr,cmd))
            state = BCC_OK;
        else if(rec_byte == FLAG)
            state = FLAG_RCV;
        else state = START;
        break;
    case BCC_OK:
        state = START;
        if(rec_byte == FLAG) return 1;
        
        break;
    default:
        break;
    }
    return 0;
}