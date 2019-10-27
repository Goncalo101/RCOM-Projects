#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../flags.h"
#include "builders.h"

int calc_bcc2(char *packet, size_t length){
    int bcc2 = BCC(packet[0], packet[1]);

    for(int i = 2; i < length; ++i){
        bcc2 = BCC(bcc2, packet[i]); 
    }
    return bcc2;
}

void int_to_string(int integer, char string[8]) {
    off_t mask = 0xff;

    for (int i = 7; i >= 0; --i) {
        string[7 - i] = mask & integer >> (8 * i);
    }
}

char *build_data_packet(char *fragment, size_t *length) {
    static char seq_no = 0;

    *length += PACKET_HEAD_LEN + 1;

    char *packet = malloc((*length) * sizeof(char));
    sprintf(packet, "%c%c%c%c", DATA_PACKET, seq_no++, (*length) / 255, (*length) % 255);
    memcpy(&packet[4], fragment, *length);

    return packet;
}


char *build_control_packet(file_t *file_info, size_t *length, char ctrl) {
    size_t filename_len = strlen(file_info->filename);
    
    *length = 5 + filename_len + 8;
    char *ctrl_packet = malloc(*length);
    bzero(ctrl_packet, *length);

    sprintf(ctrl_packet, "%c%c%c", ctrl, FILE_SIZE_PARAM, 8);
    
    char file_size_buf[8];
    int_to_string(file_info->file_size, file_size_buf);

    memcpy(&ctrl_packet[3], file_size_buf, 8);

    char *filename_tlv = malloc(filename_len + 2);
    sprintf(filename_tlv, "%c%c", FILE_NAME_PARAM, filename_len);
    memcpy(&filename_tlv[2], file_info->filename, filename_len);
    memcpy(&ctrl_packet[11], filename_tlv, filename_len + 2);

    return ctrl_packet;
}

char *build_frame(frame_t *frame) {
    char *packet;
    printf("building control packet with initial length %d\n", frame->length);
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
    printf("new length %d\n", frame->length);

    int bcc2 = calc_bcc2(packet, frame->length);
    
    char *frame_str = malloc(frame->length + FRAME_I_LENGTH);
    sprintf(frame_str, "%c%c%c%c", FLAG, frame->addr, frame->frame_ctrl, BCC(frame->addr, frame->frame_ctrl));
    memcpy(&frame_str[4], packet, frame->length);
    sprintf(&frame_str[4 + (frame->length)], "%c%c", bcc2, FLAG);

    frame->length += FRAME_I_LENGTH;
    return frame_str;
}