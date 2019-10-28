#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strmanip.h"

// str_replace(packet, "0x7d", "0x7d0x7d");
char *str_replace(char *target, char needle, const char *replacement, size_t* length) {

    char *buffer = malloc(*length + 1);
    memcpy(buffer, target, *length);
    int counter = 0;


    for (size_t i = 4; i < *length; i++) {
        if (buffer[i] == needle) {
             ++counter;
            printf("LENGTH: %d\n", *length);
            buffer = (char *) realloc(buffer, ++(*length));
            memcpy(buffer+i + 1, buffer + i, (*length) - i - 4);

            buffer[i] = 0x7d;
            //printf("buffer[%d] = 0x%02x\n", i, buffer[i]);
            //printf("buffer[%d] = 0x%02x\n", i+1, buffer[i+1]);

            i++;
        }
    
    }
    printf("NUM OF 0x%02x: %d\n", needle, counter);
    return buffer;
}