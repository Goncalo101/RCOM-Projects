#include <stdio.h>

#include "../flags.h"
#include "state_machine.h"

static int counter = 0, length = 0;
static int bcc2 = 0;
static int length_counter = 2;
static data_state data_machine_state = CTRL_FLD;

int tlv_machine(unsigned char rec_byte) {
    static tlv_state state = TYPE;
    static unsigned char length_counter = 0;
    static unsigned char tlv_counter = 2;

    switch (state) {
    case TYPE:
        state = TLV_LENGTH;
        break;
    case TLV_LENGTH:
        length_counter = rec_byte;
        state = VALUE;
        break;
    case VALUE:
        --length_counter;
        if (length_counter == 0) {
            state = TYPE;
            --tlv_counter;
            if (tlv_counter == 0) {
                state = TYPE;
                length_counter = 0;
                tlv_counter = 2;
                return 1;
            }
        } 
    default:
        break;
    }

    return 0;
}

int data_machine(unsigned char rec_byte) {
    switch (data_machine_state) {
    case CTRL_FLD:
        bcc2 = rec_byte;
        if (rec_byte == DATA_PACKET) {
            data_machine_state = SEQ_NO;
        } else if (rec_byte == START_PACKET || rec_byte == END_PACKET) {
            data_machine_state = TLV;
        }
        break;
    case TLV:
        bcc2 = (BCC(bcc2, rec_byte));
        if (tlv_machine(rec_byte)) {
            data_machine_state = BCC2_CHECK;
        }
        break;
    case SEQ_NO:
        bcc2 = (BCC(bcc2, rec_byte));
        data_machine_state = LENGTH;
        break;
    case LENGTH:
        if (length_counter == 2) {
            bcc2 = (BCC(bcc2, rec_byte));
            length += ((unsigned int)(rec_byte) * 255);
            --length_counter;
        }
        else if (length_counter == 1) {
            bcc2 = (BCC(bcc2, rec_byte));
            length += (unsigned int) (rec_byte);
            data_machine_state = DATA;
        }
        break;
    case DATA:
        bcc2 = (BCC(bcc2, rec_byte));
        ++counter;
        if (counter == length) data_machine_state = BCC2_CHECK;
        
        break;
    case BCC2_CHECK:
        data_machine_state = CTRL_FLD;
        length_counter = 2;
        counter = 0;
        length = 0;
        if (bcc2 != rec_byte) {
            puts("bcc 2 error");
            bcc2 = 0;
            return -2;
        }
        bcc2 = 0;
        return 1;
    default:
        break;
    }

    return 0;
}

int state_machine(unsigned char rec_byte) {
    static machine_state state = START;
    static unsigned char cmd = 0, addr = 0;
    int data_ret;
    
    switch (state) {
    case START:
        if (rec_byte == FLAG) 
            state = FLAG_RCV;
		else return -5;
        break;
    case FLAG_RCV:
        if (rec_byte == SENDER_CMD || rec_byte == RECEIVER_CMD) {
            state = A_RCV;
            addr = rec_byte;
        }
        else if (rec_byte == FLAG)
            state = FLAG_RCV;
        else state = START;
        break;
    case A_RCV:
        if (rec_byte == SET_CMD || rec_byte == UACK_CMD || rec_byte == 0x0 || rec_byte == 0x40 || rec_byte == 0xb || rec_byte == 0x05 || rec_byte == 0x85 || rec_byte == 0x81 || rec_byte == 0x01) {
            state = C_RCV;
            cmd = rec_byte;
        }
        else if (rec_byte == FLAG)
            state = FLAG_RCV;
        else state = START;
        break;
    case C_RCV:
        if (rec_byte == (BCC(addr, cmd)))
            state = BCC_OK;
        else if (rec_byte == FLAG)
            state = FLAG_RCV;
        else state = START;
        break;
    case BCC_OK:
        if (rec_byte == FLAG)  {
            state = START;
            cmd = 0;
            addr = 0;
            if (counter < length) {
                bcc2 = 0;
                length_counter = 2;
                data_machine_state = CTRL_FLD;
                counter = 0;
                length = 0;
                return -2;
            }
            return 1;
        } else if (rec_byte == ESCAPE) {
            state = ESC;
        } else if ((data_ret = data_machine(rec_byte)) == 1) {
            state = CHECK_END_FLAG;
        } else if(data_ret == -2) {
            state = START;
            cmd = 0;
            addr = 0;
            return -2;
        }

        break;
    case ESC:
        if (data_machine(rec_byte ^ 0x20)) {
            state = CHECK_END_FLAG;
        }
        state = BCC_OK;
        break;
    case CHECK_END_FLAG:
        if (rec_byte == FLAG) {
            state = START;
            cmd = 0;
            addr = 0;
            return 1;
        } else return -1;
    default:
        break;
    }

    return 0;
}
