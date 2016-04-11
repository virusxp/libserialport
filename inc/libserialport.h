/*
 * libserialport.h
 *
 *  Created on: 11.04.2016
 *      Author: virusxp
 */

#ifndef INC_LIBSERIALPORT_H_
#define INC_LIBSERIALPORT_H_

#include "utils/syncBuffer.h"

#include <thread>
#include <atomic>

class CharacterDevice
{
	public:
		virtual CharacterDevice() = 0;
		virtual ~CharacterDevice() {}

		virtual void write() = 0;
		virtual char read() = 0;
};

class SerialPort : public CharacterDevice
{
	private:
		SyncedFIFO<char> writeBuffer;
		SyncedFIFO<char> readBuffer;

		std::atomic_bool destroyThread;
		std::thread readerThread;
		std::thread writerThread;

	public:
		SerialPort();
		virtual ~SerialPort() {};

		void write(char chr);
		char read();
		char read_b();
};



#endif /* INC_LIBSERIALPORT_H_ */
