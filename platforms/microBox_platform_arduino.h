#ifdef ARDUINO

#include <Arduino.h>
#include "../port_handler.h"

class ArduinoPortHandler : public PortHandler {
public:
    ArduinoPortHandler(HardwareSerial& port) : port(port)
    {
        
    }

    void begin(unsigned long baudrate)
    {
        port.begin(baudrate);
    }

    virtual size_t write(uint8_t c) override
    {
        return port.write(c);
    }

    virtual int read() override
    {
        return port.read();
    }

    virtual int available() override
    {
        return port.available();
    }

private:
    HardwareSerial& port;
};

#endif