#ifndef TYPES_H
#define TYPES_H

typedef enum {
    DATA_REQ,
    CTRL_REQ
} request_t;

typedef struct {
    off_t file_size;
    unsigned char *filename;
} file_t;

typedef struct {
    unsigned char *fragment;
} packet_t;

typedef struct {
    unsigned char packet_ctrl, frame_ctrl, addr;
    request_t request_type;
    size_t length;
    union {
        file_t *file_info;
        packet_t *packet;
    };
} frame_t;

#endif