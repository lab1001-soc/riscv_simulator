#ifndef __MYPRINTF_H__
#define __MYPRINTF_H__
#include "../types.h"
void MyPrintf(const char *format, ...);
void printv(unsigned char *buf, unsigned int len, char *s);
void printfsend(uint8_t *buf, int len);
#endif