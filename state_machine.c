#include "state_machine.h"
#include "flags.h"

static machine_state state = START;
static machine_ret mach_ret;

machine_ret check_cmd(char rec_cmd) {
  if (rec_cmd == SET_CMD)
    return SET_RET;
  else if (rec_cmd == UACK_CMD)
    return UACK_RET;
  else
    return FAIL;
}

int sub_machine(char rec_byte) {
  static machine_state sub_machine_state = PACK_CTRL;
  static char counter = 0;
  static char tlv_counter = 2;

  switch (sub_machine_state) {
  case PACK_CTRL:
    if (rec_byte == START_PACKET) {
      sub_machine_state = TYPE_FIELD;
      return 0;
    }
    break;
  case TYPE_FIELD:
    if (tlv_counter <= 0) {
      return 1;
    }
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
      return 0;
    }
    --counter;
    break;
  }
}

int state_machine(char rec_byte) {
  machine_ret mach_ret;
  static int addr = 0, cmd = 0;

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
    if (rec_byte == FLAG)
      state = FLAG_RCV;
    else if (mach_ret != FAIL) {
      state = C_RCV;
      cmd = rec_byte;
    } else
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
      state = MACHINE_STOP;
      return 1;
    } else if (rec_byte == 0x02 || rec_byte == 0x03)
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
    state = BCC_OK;
    // corrigir isso
    break;
  case SEQUENCE_NO:
    break;
  default:
    return -1;
  }
  return 0;
}

machine_ret get_machine_ret() { return mach_ret; }