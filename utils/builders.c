#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../flags.h"
#include "builders.h"
#include "strmanip.h"

int calc_bcc2(unsigned char *packet, size_t length){
    int bcc2 = BCC(packet[0], packet[1]);

    for (size_t i = 2; i < length; ++i){
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

void build_data_packet(unsigned char *fragment, unsigned char **packet, size_t *length) {
    static unsigned char seq_no = 0;

    #ifdef debug
    printf ("LENGTH / 255: %ld, length: %ld\n", ((*length) / 255), (*length));
    #endif
    *packet = realloc(*packet, (*length) + PACKET_HEAD_LEN);
    sprintf((char *) (*packet), "%c%c%c%c", DATA_PACKET, seq_no++, (unsigned char) ((*length) / 255), (unsigned char) ((*length) % 255));
    memcpy(&((*packet)[4]), fragment, *length);
    *length += PACKET_HEAD_LEN;
}


void build_control_packet(file_t *file_info, size_t *length, unsigned char **packet, unsigned char ctrl) {
    size_t filename_len = strlen(file_info->filename);

    *length = 5 + filename_len + 8;
    *packet = realloc(*packet, *length);

    sprintf((char *) (*packet), "%c%c%c", ctrl, FILE_SIZE_PARAM, 8);

    unsigned char file_size_buf[8];
    int_to_string(file_info->file_size, file_size_buf);

    memcpy(&((*packet)[3]), file_size_buf, 8);

    unsigned char *filename_tlv = malloc(filename_len + 2);
    sprintf((char *) filename_tlv, "%c%c", FILE_NAME_PARAM, (unsigned char) filename_len);
    memcpy(&filename_tlv[2], file_info->filename, filename_len);
    memcpy(&((*packet)[11]), filename_tlv, filename_len + 2);
    free(filename_tlv);
}

void build_frame(frame_t *frame, unsigned char **frame_str) {
    unsigned char *packet = malloc(1);
    switch (frame->request_type) {
        case DATA_REQ:
            #ifdef debug
            printf("building data packet\n");
            #endif
            build_data_packet(frame->packet->fragment, &packet, &(frame->length));
            break;
        case CTRL_REQ:
            #ifdef debug
            printf("building control packet\n");
            #endif
            build_control_packet(frame->file_info, &(frame->length), &packet, frame->packet_ctrl);
        default:break;
    }

    int bcc2 = calc_bcc2(packet, frame->length);

    #ifdef debug
    printf("alloc memory for packet with length %ld\n", frame->length);
    #endif
    *frame_str = (unsigned char *) malloc(frame->length + FRAME_I_LENGTH + 40000000);
    sprintf((char *) (*frame_str), "%c%c%c%c", FLAG, frame->addr, frame->frame_ctrl, BCC(frame->addr, frame->frame_ctrl));

    if (frame->request_type == DATA_REQ) {
      char esc_esc[] = {ESCAPE, 0x5d};
      str_replace(&packet, ESCAPE, esc_esc, &(frame->length));

      char esc_flag[] = {ESCAPE, 0x5e};
      str_replace(&packet, FLAG, esc_flag, &(frame->length));

      char esc_bcc[] = {ESCAPE, (unsigned char)(bcc2) ^ 0x20};
      str_replace(&packet, bcc2, esc_bcc, &(frame->length));
    }

    memcpy(&((*frame_str)[4]), packet, frame->length);
    sprintf((char*) (&((*frame_str)[4 + (frame->length)])), "%c%c", bcc2, FLAG);
    frame->length += FRAME_I_LENGTH;
}

void prepare_control_frame(frame_t *frame, off_t file_size, size_t filename_len, char *filename, unsigned char addr, request_t req, unsigned char packet_ctrl, unsigned char frame_ctrl) {
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
