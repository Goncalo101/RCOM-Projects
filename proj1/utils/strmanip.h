#ifndef STRMANIP_H
#define STRMANIP_H

void str_replace(unsigned char **target, unsigned char needle, const char *replacement, size_t* length);
void rm_stuffing(unsigned char **str, size_t length);

#endif
