#ifndef STRMANIP_H
#define STRMANIP_H

unsigned char *str_replace(unsigned char *target, unsigned char needle, const unsigned char *replacement, size_t* length);
unsigned char *rm_stuffing(unsigned char *str, size_t length);

#endif
