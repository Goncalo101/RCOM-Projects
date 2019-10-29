#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strmanip.h"

char *str_replace(char *target, char needle, const char *replacement, size_t* length) {

    unsigned char *buffer = malloc(*length + 1);
    memcpy(buffer, target, *length);
    int counter = 0;
    printf("LENGTH BEFORE REPL: %ld\n", *length);

    for (size_t i = 4; i < *length; i++) {

        if (buffer[i] == needle) {
             ++counter;
            printf("LENGTH: %d\n", *length);
            buffer = (char *) realloc(buffer, ++(*length));
            memcpy(buffer+i + 1, buffer + i, (*length) - i - 4);

            buffer[i] = 0x7D;

            i++;
        }
    
    }
    return buffer;
}