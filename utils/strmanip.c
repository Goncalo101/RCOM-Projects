#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../flags.h"
#include "strmanip.h"

char *str_replace(char *target, char needle, const char *replacement, size_t* length) {
    char *buffer = malloc(*length + 1);
	bzero(buffer, *length);
    memcpy(buffer, target, *length);
    int counter = 0;
    printf("LENGTH BEFORE REPL: %ld\n", *length);

    for (size_t i = 4; i < *length; i++) {
        if (buffer[i] == needle) {
            ++counter;
            printf("LENGTH: %d\n", *length);
            buffer = (char *) realloc(buffer, ++(*length));
            memcpy(buffer + i + 1, buffer + i, (*length) - i - 1);

            buffer[i] = ESCAPE;

            i++;
        }
    }

    //memcpy(target, buffer, *length);
    //free(buffer);
	return (char*)buffer;
}


char *rm_stuffing(char *str, size_t length){
    char *buf = malloc(length);

    for (size_t i = 0, j = 0; i < length; i++,j++)
    {
        if(str[j] == ESCAPE)
            buf[i] = str[++j];
        else buf[i] = str[j];
    }


    return buf;
}
