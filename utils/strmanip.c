#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../flags.h"
#include "strmanip.h"

void str_replace(unsigned char **target, unsigned char needle, const char *replacement, size_t* length) {
    size_t original_length = *length;
    unsigned char *buffer = calloc(2 * original_length, 1);
    memcpy(buffer, *target, 4);

    for (size_t i = 4, j = 4; i < original_length; ++i, ++j) {
      if (*target[i] == needle) {
          ++(*length);
          memcpy((char *) (&buffer[j]), replacement, strlen(replacement));
          ++j;
      } else {
        memcpy(&buffer[j], &(*target[i]), 1);
      }
    }

    buffer = realloc(buffer, *length);
    *target = realloc(*target, *length);
    memcpy(*target, buffer, *length);
    free(buffer);
}


void rm_stuffing(unsigned char **str, size_t length) {
    unsigned char *buf = malloc(length);

    for (size_t i = 0, j = 0; i < length; i++,j++) {
        if (*str[j] == ESCAPE) {
            buf[i] = *str[++j] ^ 0x20;
        } else buf[i] = *str[j];
    }

    *str = realloc(*str, length);
    memcpy(*str, buf, length);
    free(buf);
}
