#ifndef _MICROBOX_PLATFORM_H_
#define _MICROBOX_PLATFORM_H_

#include <stdint.h>
#include <stddef.h>

void SerialPrint(const char* s);
size_t SerialWrite(uint8_t c);
int SerialRead();
int SerialAvailable();

#endif