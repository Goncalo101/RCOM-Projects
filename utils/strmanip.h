#ifndef STRMANIP_H
#define STRMANIP_H

char *str_replace(char *target, char needle, const char *replacement, size_t* length);
char *rm_stuffing(char *str, size_t length);

#endif
