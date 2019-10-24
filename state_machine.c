#include "state_machine.h"
#include "flags.h"

#include <limits.h>
#include <stdio.h>

static machine_state state = START;
static machine_ret mach_ret;

machine_ret check_cmd(char rec_cmd) {
  if (rec_cmd == SET_CMD || rec_cmd == 0x00 || rec_cmd == 0x0b || rec_cmd == 0x07 || rec_cmd == 0x05 || rec_cmd == 0xffffff85 || rec_cmd == 0x01 || rec_cmd == 0x81)
    return SET_RET;
  return FAIL;
}

int sub_machine(char rec_byte) {
  static machine_state sub_machine_state = TYPE_FIELD;
  static char counter = 0;

  switch (sub_machine_state) {
  case TYPE_FIELD:
    if (rec_byte == FILE_SIZE_PARAM || rec_byte == FILE_NAME_PARAM) {
      sub_machine_state = LENGTH_FIELD;
      --counter;
      return 0;
    }
    break;
  case LENGTH_FIELD:
    counter = rec_byte;
    sub_machine_state = VALUE_FIELD;
  case VALUE_FIELD:
    if (counter == 0) {
      sub_machine_state = TYPE_FIELD;
      return 1;
    }
    --counter;
    break;
  }
  return -1;
}

int data_machine(char rec_byte) {
  static machine_state sub_machine_state = PACK_DATA;
  static char counter = 0;
  static char data_len_counter = 2;
  static char seq_no = 0;
  static char oct_len[2];
  static int  oct_num = 0, index = 0;

  switch (sub_machine_state) {
  case PACK_DATA:
    if (rec_byte == DATA_PACKET) {
      state = SEQUENCE_NO;
    }
    break;
  case SEQUENCE_NO:
    seq_no = rec_byte;
    state = OCT_LEN;
    break;
  case OCT_LEN:
    if (data_len_counter == 0) {
      oct_num = 256 * oct_len[0] + oct_len[1];
      state = PACKET;
    }
    oct_len[2-data_len_counter] = rec_byte;
    --data_len_counter;
    break;
  case PACKET:
    if (oct_num == 0) {
      // RESET
      sub_machine_state = PACK_DATA;
      counter = 0;
      data_len_counter = 2;
      seq_no = 0;
      oct_len[2];
      oct_num = 0, index = 0;
      return 1;
    }
    --oct_num;
    break;
  default: break;
  }
}

int state_machine(char rec_byte) {
  machine_ret mach_ret;
  static int addr = 0, cmd = 0;

  printf("STATE: %d\n", state);

  switch (state) {
  case START:
    if (rec_byte == FLAG) {
      state = FLAG_RCV;
      break;
    }
    break;
  case FLAG_RCV:
    if (rec_byte == FLAG)
      break;
    else if (rec_byte == RECEIVER_ANS) {
      state = A_RCV;
      addr = rec_byte;
    } else
      state = START;
    break;
  case A_RCV:
    mach_ret = check_cmd(rec_byte);
    printf("MACH RET: %d\n", mach_ret);
    if (rec_byte == FLAG)
      state = FLAG_RCV;
    else if (mach_ret != FAIL) {
      state = C_RCV;
      cmd = rec_byte;
    } else if( rec_byte == SET_CMD)
      state = START;
    break;
  case C_RCV:
    if (rec_byte == FLAG)
      state = FLAG_RCV;
    else if (rec_byte == (BCC(addr, cmd)))
      state = BCC_OK;
    else
      state = START;
    break;
  case BCC_OK:
    if (rec_byte == FLAG) {
      state = START;
      addr = 0, cmd = 0;
      return 1;
    } else if (rec_byte == START_PACKET || rec_byte == END_PACKET)
      state = PACK_CTRL;
    else if (rec_byte == 0x01)
      state = SEQUENCE_NO;
    else
      state = START;
    break;
  case PACK_CTRL:
    if (sub_machine(rec_byte) == 1) {
      state = CHECK_INTERMEDIATE;
    }
    break;
  case CHECK_INTERMEDIATE:
    if (rec_byte == ESCAPE)
      break;
    else if (rec_byte == DATA_PACKET)
      state = PACK_DATA;

    state = BCC_OK;
    // corrigir isso
    break;
  case PACK_DATA:
    if (data_machine(rec_byte)) {
      state = CHECK_INTERMEDIATE;
    }
    break;
  default:
    return -1;
  }
  return 0;
}

machine_ret get_machine_ret() { return mach_ret; }