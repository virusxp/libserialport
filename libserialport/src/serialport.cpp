#include "serialport.h"

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

SerialPortException::SerialPortException(int errorNumber, SerialPort* thrower) : exception() {
    throwingInstance = thrower;
    errorCode = errorNumber;
}

const char* SerialPortException::what() const throw() {
    string exceptionText;

    exceptionText.clear();
    exceptionText += "[ERROR]: An exception occured with error number [" + to_string(errorCode) + "]";
    if(throwingInstance != NULL) exceptionText += " using serial port [" + throwingInstance->getPortName() + "].";
    else exceptionText += " using an unknown or not initialized serial port.";

    return exceptionText.c_str();
}

SerialPortWThread::SerialPortWThread(int fp, std::mutex* wrMutex, SyncCharFiFo* rwFiFo, volatile bool* killSig) {
	serialPortFP = fp;
	rw = wrMutex;
	FiFo = rwFiFo;
	breakThis = killSig;
}

void SerialPortWThread::operator()() const {
    char buf;
    while(!breakThis) {
        int j = FiFo->getSize();
        if(j > 0) {
            rw->lock();
            for(int i = 0; i < j; i++) {
                buf = FiFo->pop();
                write(serialPortFP,(void*)(&buf),sizeof(char));
            }
            rw->unlock();
        }
    }
}

SerialPortRThread::SerialPortRThread(int fp, std::mutex* wrMutex, SyncCharFiFo* rwFiFo, volatile bool* killSig) {
	serialPortFP = fp;
	rw = wrMutex;
	FiFo = rwFiFo;
	breakThis = killSig;
}

void SerialPortRThread::operator()() const {
    char chr;
    while(!breakThis) {
        rw->lock();
        if(read(serialPortFP,&chr,sizeof(char)) != 0) FiFo->push(chr);
        rw->unlock();
    }
}

SerialPort::SerialPort(string portname, SerialPortConfig* config) {
    if(portname.empty()) throw new SerialPortException(SP_PORT_NOT_FOUND_EXCP,NULL);

    if(config == NULL) {
        serialConfig.baudRate = 9600;
        serialConfig.byteSize = 8;
        serialConfig.flowCntrl = SP_NO_FLOWCNTRL;
        serialConfig.parity = SP_NO_PARITY;
        serialConfig.stopBits = 1;
        serialConfig.xx[0] = '\0';
        serialConfig.xx[1] = '\0';
    } else {
        //analyze the given configuration for plausibility

        //max baudrate == max ftdi defined baud rate
        if((config->baudRate > 3000000) || (config->baudRate <= 0)) serialConfig.baudRate = 9600;
        else serialConfig.baudRate = config->baudRate;

        if((config->stopBits > SP_2_STOP_BIT) || (config->stopBits < SP_1_STOP_BIT)) serialConfig.stopBits = SP_1_STOP_BIT;
        else serialConfig.stopBits = config->stopBits;

        if((config->parity > SP_SPACE_PARITY) || (config->parity < SP_NO_PARITY)) serialConfig.parity = config->parity;
        else serialConfig.parity = SP_NO_PARITY;

        if((config->flowCntrl > SP_SOFT_FLOWCNTRL) || (config->flowCntrl < SP_NO_FLOWCNTRL)) serialConfig.flowCntrl = SP_NO_FLOWCNTRL;
        else serialConfig.flowCntrl = config->flowCntrl;

        if((serialConfig.flowCntrl & SP_SOFT_FLOWCNTRL) > 0) {
            serialConfig.xx[0] = config->xx[0];
            serialConfig.xx[1] = config->xx[1];
        }
    }

    serialPortFptr = open(portName.c_str(),(O_RDWR|O_NOCTTY|O_NONBLOCK));
    if(serialPortFptr < 0) throw new SerialPortException(SP_PORT_NOT_OPENED,NULL);

    //read and save the old configuration to restore it by closing the port
    struct termios2 newtio;
    ioctl(serialPortFptr, TCGETS2, &oldtio);
    ioctl(serialPortFptr, TCGETS2, &newtio);

    //set some general things
    newtio.c_cflag |= (BOTHER| CLOCAL | CREAD);

    //set baud rate
    newtio.c_cflag &= ~CBAUD;
    newtio.c_ispeed = serialConfig.baudRate;
    newtio.c_ospeed = serialConfig.baudRate;

    //timeout settings
    newtio.c_cc[VMIN]=0;
    newtio.c_cc[VTIME]=0;

    //configure parity mode
    switch(config->parity) {
    case(SP_ODD_PARITY):    {
                                newtio.c_iflag &= ~IGNPAR;
                                newtio.c_cflag &= ~CMSPAR;
                                newtio.c_cflag |= PARENB;
                                newtio.c_cflag |= PARODD;
                            }
    break;
    case(SP_EVEN_PARITY):   {
                                newtio.c_iflag &= ~IGNPAR;
                                newtio.c_cflag &= ~CMSPAR;
                                newtio.c_cflag |= PARENB;
                                newtio.c_cflag &= ~PARODD;
                            }
    break;
    case(SP_MARK_PARITY):   {
                                newtio.c_iflag &= ~IGNPAR;
                                newtio.c_cflag |= CMSPAR;
                                newtio.c_cflag |= PARENB;
                                newtio.c_cflag |= PARODD;
                            }
    break;
    case(SP_SPACE_PARITY):  {
                                newtio.c_iflag &= ~IGNPAR;
                                newtio.c_cflag |= CMSPAR;
                                newtio.c_cflag |= PARENB;
                                newtio.c_cflag &= ~PARODD;
                            }
    break;
    default:                {
                                newtio.c_iflag |= IGNPAR;
                                newtio.c_cflag &= ~PARENB;
                            }
    }

    //define byte size
    newtio.c_cflag &= ~CSIZE;
    switch(serialConfig.byteSize) {
    case(5):    newtio.c_cflag |= CS5;
    break;
    case(6):    newtio.c_cflag |= CS6;
    break;
    case(7):    newtio.c_cflag |= CS7;
    break;
    case(8):
    default:    newtio.c_cflag |= CS8;
    }

    //set stop bit count
    if(serialConfig.stopBits == SP_1_STOP_BIT) newtio.c_cflag &= ~CSTOPB;
    else newtio.c_cflag |= CSTOPB;

    //set flow control
    if(serialConfig.flowCntrl & SP_DTR_FLOWCNTRL) newtio.c_cflag &= ~CLOCAL;
    if(serialConfig.flowCntrl & SP_CTS_FLOWCNTRL) newtio.c_cflag |= CRTSCTS;
    else newtio.c_cflag &= ~CRTSCTS;
    if(serialConfig.flowCntrl & SP_SOFT_FLOWCNTRL) {
        newtio.c_iflag |= (IXON | IXOFF);
        newtio.c_cc[VSTART] = serialConfig.xx[0];
        newtio.c_cc[VSTOP] = serialConfig.xx[1];
    } else newtio.c_iflag &= ~(IXON | IXOFF);

    ioctl(serialPortFptr, TCSETS2, &newtio);

    writeTask = new SerialPortWThread(serialPortFptr,writeReadMutex,&serialWriterFiFo,&breakThreads);
    readTask = new SerialPortRThread(serialPortFptr,writeReadMutex,&serialReaderFiFo,&breakThreads);
    writeReadMutex = new mutex();
    writerThreadPtr = new thread(*writeTask);
    readerThreadPtr = new thread(*readTask);
}

SerialPort::~SerialPort() {
    ioctl(serialPortFptr, TCSETS2, &oldtio);
    close(serialPortFptr);
}

SerialPortConfig SerialPort::getPortConfiguration() {
    SerialPortConfig ret;

    ret.baudRate = serialConfig.baudRate;
    ret.byteSize = serialConfig.byteSize;
    ret.flowCntrl = serialConfig.flowCntrl;
    ret.parity = serialConfig.parity;
    ret.stopBits = serialConfig.stopBits;
    ret.xx[0] = serialConfig.xx[0];
    ret.xx[1] = serialConfig.xx[1];

    return ret;
}
