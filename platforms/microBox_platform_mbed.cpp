/*

#ifdef MBED

#include <mbed.h>

extern BufferedSerial serialPort;

void SerialPrint(const char* s)
{
    serialPort.write(s, strlen(s));
}

size_t SerialWrite(uint8_t c)
{
    return serialPort.write(&c, 1);
}

int SerialRead()
{
    char c;
    serialPort.read(&c, 1);
    return c;
}

int SerialAvailable()
{
    return serialPort.readable();
}

#endif

*/