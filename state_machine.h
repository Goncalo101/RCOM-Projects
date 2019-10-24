#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef enum { SET_RET, UACK_RET, FAIL } machine_ret;

typedef enum {
  START,
  FLAG_RCV,
  A_RCV,
  C_RCV,
  BCC_OK,
  PACK_CTRL,
  TYPE_FIELD,
  LENGTH_FIELD,
  VALUE_FIELD,
  CHECK_INTERMEDIATE,
  SEQUENCE_NO,
  MACHINE_STOP

} machine_state;

int state_machine(char rec_byte);
machine_ret get_machine_ret();

#endif