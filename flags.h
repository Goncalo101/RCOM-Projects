#define FLAG  0x7e
#define SENDER_CMD 0x03
#define RECEIVER_ANS SENDER_CMD
#define SET_CMD 0x03
#define UACK_CMD 0x07
#define BCC(ADDR, CMD) (ADDR) ^ (CMD)
#define ESCAPE 0x7d

#define SIZE_LENGTH 8
#define FILE_SIZE_PARAM 0x00
#define FILE_NAME_PARAM 0x01
#define START_PACKET 0x02
#define END_PACKET 0x03

