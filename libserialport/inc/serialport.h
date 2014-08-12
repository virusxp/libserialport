#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <string>
#include <exception>
#include <asm/termios.h>

#define SP_1_STOP_BIT   0x01
#define SP_1_5_STOP_BIT 0x02
#define SP_2_STOP_BIT   0x03

#define SP_NO_FLOWCNTRL     0x00
#define SP_DTR_FLOWCNTRL    0x01
#define SP_CTS_FLOWCNTRL    0x02
#define SP_SOFT_FLOWCNTRL   0x04

#define SP_NO_PARITY 0x00
#define SP_ODD_PARITY 0x01
#define SP_EVEN_PARITY 0x02
#define SP_MARK_PARITY 0x03
#define SP_SPACE_PARITY 0x04

#define SP_PORT_NOT_FOUND_EXCP  0xFFFE
#define SP_PORT_NOT_OPENED      0xFFFD

typedef struct SerialPortConfig_t {
    int baudRate;
    int byteSize;
    int stopBits;
    int parity;
    int flowCntrl;
    char xx[2];
} SerialPortConfig;

class SerialPort {
private:
    int serialPortFptr = 0;
    SerialPortConfig serialConfig;
    std::string portName;
    struct termios2 oldtio;

public:
    SerialPort(std::string portname, SerialPortConfig* config = NULL);
    virtual ~SerialPort();

    std::string getPortName();
    SerialPortConfig getPortConfiguration();
};

class SerialPortException : public std::exception {
private:
    int errorCode;
    SerialPort* throwingInstance;

public:
    SerialPortException(int errorNumber, SerialPort* thrower);
    virtual const char* what() const throw();
};

#endif // SERIALPORT_H
