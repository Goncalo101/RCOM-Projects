#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strmanip.h"

void strreplace(char *string, const char *find, const char *replaceWith){
    if(strstr(string, replaceWith) != NULL){
        char *temporaryString = malloc(strlen(strstr(string, find) + strlen(find)) + 1);
        strcpy(temporaryString, strstr(string, find) + strlen(find));    //Create a string with what's after the replaced part
        *strstr(string, find) = '\0';    //Take away the part to replace and the part after it in the initial string
        strcat(string, replaceWith);    //Concat the first part of the string with the part to replace with
        strcat(string, temporaryString);    //Concat the first part of the string with the part after the replaced part
        free(temporaryString);    //Free the memory to avoid memory leaks
    }
}

// str_replace(packet, "0x7d", "0x7d0x7d");
void str_replace(char *target, const char *needle, const char *replacement, size_t* length) {
  char buffer[1024] = {0};
  char *insert_point = &buffer[0];
  const char *tmp = target;
  size_t needle_len = strlen(needle);
  size_t repl_len = strlen(replacement);
  size_t original_length = *length;

  printf("realloc double size\n");
  target = realloc(target, original_length * 2);

  while (1) {
    const char *p = strstr(tmp, needle);
    
    printf("strstr check null\n");
    if (p == NULL) {
      strcpy(insert_point, tmp);
      break;
    }

    *length += repl_len - needle_len;

    printf("memcpy\n");
    memcpy(insert_point, tmp, p - tmp);
    insert_point += p - tmp;

    memcpy(insert_point, replacement, repl_len);
    insert_point += repl_len;
    printf("memcpy123\n");

    tmp = p + needle_len;
  }

  printf("final realloc\n");
  target = realloc(target, *length);
  printf("final memcpy\n");
  memcpy(target, buffer, *length);
}