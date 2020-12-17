#ifndef MICROBOX_PORT_HANDLER_H
#define MICROBOX_PORT_HANDLER_H

class PortHandler {
public:
    virtual size_t write(uint8_t c) = 0;
    virtual int read()              = 0;
    virtual int available()         = 0;
};

#endif // MICROBOX_PORT_HANDLER_H