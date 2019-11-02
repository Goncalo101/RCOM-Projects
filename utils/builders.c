#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../flags.h"
#include "builders.h"
#include "strmanip.h"

int calc_bcc2(unsigned char *packet, size_t length){
    int bcc2 = BCC(packet[0], packet[1]);

    for(int i = 2; i < length; ++i){
        bcc2 = BCC(bcc2, packet[i]);
    }
    return bcc2;
}

void int_to_string(off_t integer, unsigned char string[8]) {
    off_t mask = 0xff;

    bzero(string, 8);
    for (int i = 7; i >= 0; --i) {
        string[7 - i] = mask & (integer >> (8 * i));
    }
}

unsigned char *build_data_packet(unsigned char *fragment, size_t *length) {
    static unsigned char seq_no = 0;

    unsigned char *packet = malloc((*length) * sizeof(unsigned char) + PACKET_HEAD_LEN + 1);
    printf ("LENGTH / 255: %d, length: %d\n", (unsigned char) ((*length) / 255), (unsigned char)(*length));
    sprintf(packet, "%c%c%c%c", DATA_PACKET, seq_no++, (unsigned char) ((*length) / 255), (unsigned char) ((*length) % 255));
    memcpy(&packet[4], fragment, *length);
    *length += PACKET_HEAD_LEN;

    return packet;
}


unsigned char *build_control_packet(file_t *file_info, size_t *length, unsigned char ctrl) {
    size_t filename_len = strlen(file_info->filename);

    *length = 5 + filename_len + 8;
    unsigned char *ctrl_packet = malloc(*length);
    bzero(ctrl_packet, *length);

    sprintf(ctrl_packet, "%c%c%c", ctrl, FILE_SIZE_PARAM, 8);

    unsigned char file_size_buf[8];
    int_to_string(file_info->file_size, file_size_buf);

    memcpy(&ctrl_packet[3], file_size_buf, 8);

    unsigned char *filename_tlv = malloc(filename_len + 2);
    sprintf(filename_tlv, "%c%c", FILE_NAME_PARAM, (unsigned char) filename_len);
    memcpy(&filename_tlv[2], file_info->filename, filename_len);
    memcpy(&ctrl_packet[11], filename_tlv, filename_len + 2);

    free(filename_tlv);

    return ctrl_packet;
}

unsigned char *build_frame(frame_t *frame) {
    unsigned char *packet;
    printf("building control packet with initial length %ld\n", frame->length);
    switch (frame->request_type) {
        case DATA_REQ:
            printf("building data packet\n");
            packet = build_data_packet(frame->packet->fragment, &(frame->length));
            break;
        case CTRL_REQ:
            printf("building control packet\n");
            packet = build_control_packet(frame->file_info, &(frame->length), frame->packet_ctrl);
        default:break;
    }

    printf("control packet built\n");
    printf("new length %ld\n", frame->length);

    int bcc2 = calc_bcc2(packet, frame->length);

    unsigned char *frame_str = malloc(frame->length + FRAME_I_LENGTH);
    sprintf(frame_str, "%c%c%c%c", FLAG, frame->addr, frame->frame_ctrl, BCC(frame->addr, frame->frame_ctrl));

    if (frame->request_type == DATA_REQ) {
      printf("BEFORE BYTE STUFFING: %ld\n", frame->length);
      unsigned char esc_esc[] = {ESCAPE, 0x5d};
      packet = str_replace(packet, ESCAPE, esc_esc, &(frame->length));

      unsigned char esc_flag[] = {ESCAPE, 0x5e};
      packet = str_replace(packet, FLAG, esc_flag, &(frame->length));

      unsigned char esc_bcc[] = {ESCAPE, (unsigned char)(bcc2) ^ 0x20};
      packet = str_replace(packet, bcc2, esc_bcc, &(frame->length));

      printf("AFTER BYTE STUFFING: %ld\n", frame->length);
    }

    memcpy(&frame_str[4], packet, frame->length);
    sprintf(&frame_str[4 + (frame->length)], "%c%c", bcc2, FLAG);
    frame->length += FRAME_I_LENGTH;

    return frame_str;
}

void prepare_control_frame(frame_t *frame, off_t file_size, size_t filename_len, unsigned char *filename, unsigned char addr, request_t req, unsigned char packet_ctrl, unsigned char frame_ctrl) {
    file_t *file_info = malloc(sizeof(file_t));
    file_info->file_size = file_size;
    file_info->filename = malloc(filename_len);
    strcpy(file_info->filename, filename);

    frame->request_type = req;
    frame->length = filename_len;
    frame->file_info = file_info;
    frame->packet_ctrl = packet_ctrl;
    frame->frame_ctrl = frame_ctrl;
    frame->addr = addr;
}
