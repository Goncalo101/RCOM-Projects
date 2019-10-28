#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strmanip.h"

// void strreplace(char *string, const char *find, const char *replaceWith){
//     if(strstr(string, replaceWith) != NULL){
//         char *temporaryString = malloc(strlen(strstr(string, find) + strlen(find)) + 1);
//         strcpy(temporaryString, strstr(string, find) + strlen(find));    //Create a string with what's after the replaced part
//         *strstr(string, find) = '\0';    //Take away the part to replace and the part after it in the initial string
//         strcat(string, replaceWith);    //Concat the first part of the string with the part to replace with
//         strcat(string, temporaryString);    //Concat the first part of the string with the part after the replaced part
//         free(temporaryString);    // the memory to avoid memory leaks
//     }
// }

// str_replace(packet, "0x7d", "0x7d0x7d");
void str_replace(char *target, char needle, const char *replacement, size_t* length) {
  // char buffer[1024] = {0};
  // char *insert_point = &buffer[0];
  // const char *tmp = target;
  // size_t needle_len = strlen(needle);
  // size_t repl_len = strlen(replacement);
    char *buffer = malloc(*length);
    memcpy(buffer, target, *length);

    for (size_t i = 4; i < *length; i++) {
        if (buffer[i] == needle) {
            buffer = (char *) realloc(buffer, ++(*length));
            memmove(&buffer[i + 1], &buffer[i], *length - i);

            buffer[i] = 0x7d;
            printf("buffer[%d] = 0x%02x\n", i, buffer[i]);

            i++;
        }
        printf("buffer[%d] = 0x%02x\n", i, buffer[i]);
  }

//   printf("ciclo for do str replace com needle: 0x%02x\n", needle);
//   for (int i = 4; i < *length; ++i) {
//     printf("target[%d]: 0x%02x\n", i, target[i]);
//     if (target[i] == 0x7e || target[i] == 0x7d || target[i] == 0xd5) {
//       printf("detetou: 0x%02x\n", target[i]);
//     }
//     if (target[i] == 0x7e || target[i] == 0x7d || target[i] == 0xd5) {
//       target = (char*) realloc(target, ++(*length));
//       // memmove(target + i + 1, target + i, *length - i);
//       char asdasd[] = {0x7d};
//       strcpy(&target[i], asdasd);
//       for (int j = i - 3; j < i + 6; ++j) {
//         printf("after addition target[%d]: 0x%02x\n", j, target[j]);
//       }
//       printf("target[%d]: 0x%02x\n", i, target[i]);
//       i++;
//     }
//   }
//   printf("fim do ciclo for\n");

  // while (1) {
  //   char *p = strstr(tmp, needle);
    
  //   printf("strstr check null\n");
  //   if (p == NULL) {
  //     strcpy(insert_point, tmp);
  //     break;
  //   }

  //   modified = 1;

  //   *length += repl_len - needle_len;

  //   printf("memcpy\n");
  //   memcpy(insert_point, tmp, p - tmp);
  //   insert_point += p - tmp;

  //   memcpy(insert_point, replacement, repl_len);
  //   insert_point += repl_len;
  //   printf("memcpy123\n");

  //   tmp = p + needle_len;
  // }

  // if (modified) {
  //   printf("final realloc\n");
  //   target = realloc(target, *length);
  //   printf("final memcpy\n");
  //   memcpy(target, buffer, *length);
  // }
}