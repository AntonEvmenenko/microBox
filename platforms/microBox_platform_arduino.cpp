#ifdef ARDUINO

#include <Arduino.h>

void SerialPrint(const char* s)
{
    Serial.print(s);
}

size_t SerialWrite(uint8_t c)
{
    return Serial.write(c);
}

int SerialRead()
{
    return Serial.read();
}

int SerialAvailable()
{
    return Serial.available();
}

#endif