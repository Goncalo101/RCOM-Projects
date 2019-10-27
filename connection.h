#ifndef CONNECTION_H
#define CONNECTION_H

#define BAUDRATE B38400

#define PACKET_HEAD_LEN 4

#define MAX_ALARM_COUNT 3
#define TIMEOUT 3

#define INTERRUPTED -2
#define ERROR -1

#define TYPE_A_PACKET_LENGTH 5
#define FRAME_I_LENGTH 6

typedef int (*sender_func)(int);

typedef enum {
    DATA_REQ,
    CTRL_REQ
} request_t;

typedef struct {
    off_t file_size;
    char *filename;
    int ctrl;
} file_t;

typedef struct {
    char *fragment;
    int addr;
    int ctrl;
} packet_t;

typedef struct {
    request_t request_type;
    size_t length;
    union {
        file_t *file_info;
        packet_t *packet;
    };
} frame_t;


int llopen(int port, int mode);
int send_packet(int fd, frame_t *frame);

#endif