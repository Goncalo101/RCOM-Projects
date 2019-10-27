#include <stdio.h>

#include "../flags.h"
#include "state_machine.h"

int data_machine(char rec_byte){
    static data_state state = CTRL_FLD;
    static int bcc2 = 0;
    static int length_counter = 2, counter = 0;
    static int length = 0;

    printf("DATA MACHINE STATE: %d\n", state);

    switch (state)
    {
    case CTRL_FLD:
        if(rec_byte == DATA_PACKET){
            bcc2 = DATA_PACKET;
            state = SEQ_NO;
        }
        break;
    case SEQ_NO:
        bcc2 = BCC(bcc2, rec_byte);
        state = LENGTH;
        break;
    case LENGTH:
        if(length_counter == 2){
            bcc2 = BCC(bcc2, rec_byte);
            length += rec_byte*256;
            --length_counter;
        }
        else if(length_counter == 1){
            bcc2 = BCC(bcc2, rec_byte);
            length += rec_byte;
            state = DATA;
        }
        break;
    case DATA:
        bcc2 = BCC(bcc2, rec_byte);
        ++counter;
        if(counter >= length) state = BCC2_CHECK;
        
        break;
    case BCC2_CHECK:
        if(bcc2 != rec_byte)
            return -1;
        state = CTRL_FLD;
        bcc2 = 0;
        length_counter = 2;
        counter = 0;
        length = 0;
        return 1;
    default:
        break;
    }

    return 0;
}

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
        if(rec_byte == FLAG) return 1;

        if(data_machine(rec_byte) == 1)
            state = CHECK_END_FLAG;
        break;
    case CHECK_END_FLAG:
        if(rec_byte == FLAG) return 1;
        else return -1;
    default:
        break;
    }
    return 0;
}