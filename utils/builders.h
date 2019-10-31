#ifndef BUILDERS_H
#define BUILDERS_H

#include "../types.h"

unsigned char *build_frame(frame_t *frame);
void prepare_control_frame(frame_t *frame, off_t file_size, size_t filename_len, unsigned char *filename, unsigned char addr, request_t req, unsigned char packet_ctrl, unsigned char frame_ctrl);

#endif