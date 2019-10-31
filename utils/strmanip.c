#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../flags.h"
#include "strmanip.h"

unsigned char *str_replace(unsigned char *target, char needle, const char *replacement, size_t* length) {
    int original_length = *length;
    unsigned char *buffer = malloc(2 * original_length);
    bzero(buffer, original_length);
    memcpy(buffer, target, 4);

    printf("LENGTH BEFORE REPL: %ld\n", *length);

    for (size_t i = 4, j = 4; i < original_length; ++i, ++j) {
      if (target[i] == needle) {
          ++(*length);
          memcpy(&buffer[j], replacement, strlen(replacement));
          ++j;
      } else {
        memcpy(&buffer[j], &target[i], 1);
      }
    }

    buffer = realloc(buffer, *length);
  // for (size_t i = 4; i < original_length; ++i) {
  //   for (size_t j = i + 1; j < original_length - 1; ++j) {
  //     if (target[j] == needle) {
  //       buffer = realloc(buffer, ++(*length));
  //       memcpy(&buffer[i], &target[i], j - i);
  //       memcpy(&buffer[j], replacement, strlen(replacement));
  //       i = j + 2;
  //       break;
  //     }
  //   }
  // }

	return buffer;
}
//   for (size_t i = 4, j = 0; i < *length; i++, j++) {
//       if (target[i] == needle) {
//           printf("LENGTH: %d\n", *length);
//           buffer = realloc(buffer, ++(*length));
//           memcpy(&buffer[i + 1], &buffer[i], (*length) - i - 1);
//
//           buffer[i] = ESCAPE;
//
//           i++;
//       }
//   }
// //target = realloc(target, *length);


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
