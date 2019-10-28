#ifndef BUILDERS_H
#define BUILDERS_H

#include "../types.h"

char *build_frame(frame_t *frame);
void prepare_control_frame(frame_t *frame, off_t file_size, size_t filename_len, char *filename, char addr, request_t req, char packet_ctrl, char frame_ctrl);

#endif