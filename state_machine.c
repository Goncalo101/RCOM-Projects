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
    } else
      state = START;
    break;
  default:
    return -1;
  }
  return 0;
}

machine_ret get_machine_ret() {
    return mach_ret;
}