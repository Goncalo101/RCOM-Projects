#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef enum {
    TYPE,
    TLV_LENGTH,
    VALUE
} tlv_state;

typedef enum {
    CTRL_FLD,
    SEQ_NO,
    TLV,
    LENGTH,
    DATA,
    BCC2_CHECK,
} data_state;

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    ESC,
    CHECK_END_FLAG,
    MACHINE_STOP
} machine_state;

int state_machine(unsigned char rec_byte);

#endif