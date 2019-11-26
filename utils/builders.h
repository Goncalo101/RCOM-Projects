#ifndef BUILDERS_H
#define BUILDERS_H

#include "../types.h"

void build_frame(frame_t *frame, unsigned char **frame_str);
void prepare_control_frame(frame_t *frame, off_t file_size, size_t filename_len, char *filename, unsigned char addr, request_t req, unsigned char packet_ctrl, unsigned char frame_ctrl);

#endif