/*
 * libserialport.cpp
 *
 *  Created on: 11.04.2016
 *      Author: virusxp
 */

#include "libserialport.h"

void SerialPortReader(std::atomic_bool* killThread, SyncedFIFO<char>* buffer)
{
	while(!killThread)
	{

	}
}

void SerialPortWriter(std::atomic_bool* killThread, SyncedFIFO<char>* buffer)
{
	while(!killThread)
	{

	}
}

SerialPort::SerialPort()
{
	this->destroyThread = false;
	this->readerThread = std::thread(SerialPortReader,&this->destroyThread,&this->readBuffer);
	this->writerThread = std::thread(SerialPortWriter,&this->destroyThread,&this->writeBuffer);
}

void SerialPort::write(char chr)
{
	this->writeBuffer.push(chr);
}

char SerialPort::read()
{
	int status = 0;
	char ret = this->readBuffer.pop(&status);

	if(status < 0)
		return 0x00;

	return ret;
}

char SerialPort::read_b()
{
	int status = -1;
	char ret;

	while(status < 0)
		ret = this->readBuffer.pop(&status);

	return ret;
}
