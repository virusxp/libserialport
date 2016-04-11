/*
 * syncBuffer.h
 *
 *  Created on: 11.04.2016
 *      Author: virusxp
 */

#ifndef INC_UTILS_SYNCBUFFER_H_
#define INC_UTILS_SYNCBUFFER_H_

#include <stdlib.h>

#include <mutex>

template<typename T>
class SyncedFIFOElement
{
	private:
		T container;
		SyncedFIFOElement* nextElement;

	public:
		SyncedFIFOElement()
		{
			this->container = T();
			this->nextElement = NULL;
		}

		SyncedFIFOElement(T value, SyncedFIFOElement* next = NULL)
		{
			this->container = value;
			this->nextElement = next;
		}

		~SyncedFIFOElement()
		{

		}

		void setValue(T value)
		{
			this->container = value;
		}

		void setNext(SyncedFIFOElement* next)
		{
			this->nextElement = next;
		}

		bool hasNext()
		{
			return (this->nextElement != NULL);
		}

		T getValue()
		{
			return this->container;
		}

		SyncedFIFOElement* getNext()
		{
			return this->nextElement;
		}
};

template<typename T>
class SyncedFIFO
{
	private:
		SyncedFIFOElement<T> head;
		SyncedFIFOElement<T>* tail;

		unsigned int stackSize;
		std::mutex fifoMtx;

	public:
		SyncedFIFO()
		{
			this->tail = &this->head;
			this->stackSize = 0;
		}

		SyncedFIFO(SyncedFIFO& ref)
		{
			this->tail = &this->head;
			this->stackSize = 0;

			SyncedFIFOElement<T>* tmp = ref.head.getNext();
			while(tmp != NULL)
			{
				this->push(tmp->getValue());
				tmp = tmp->getNext();
			}
		}

		SyncedFIFO(T firstElement)
		{
			this->tail = &this->head;
			this->stackSize = 0;

			this->push(firstElement);

		}

		~SyncedFIFO()
		{
			std::lock_guard<std::mutex> lock(this->fifoMtx);
			if(stackSize != 0)
			{
				SyncedFIFOElement<T>* tmp = this->head.getNext();
				SyncedFIFOElement<T>* tmp2 = tmp->getNext();
				while(tmp != NULL)
				{
					delete tmp;
					tmp = tmp2;
					tmp2 = (tmp != NULL) ? tmp->getNext() : NULL;
				}
			}
		}

		void push(T newElement)
		{
			std::lock_guard<std::mutex> lock(this->fifoMtx);
			this->tail = this->tail.setNext(new SyncedFIFOElement<T>(newElement));

			this->tail = this->tail.getNext();
			this->stackSize++;
		}

		T pop(int* status = NULL)
		{
			std::lock_guard<std::mutex> lock(this->fifoMtx);

			if(this->stackSize == 0)
			{
				if(status != NULL)
					status = -1;
				return T();
			}

			this->stackSize--;
			SyncedFIFOElement<T>* tmp = this->head.getNext();
			T buffer = tmp->getValue();

			this->tail = (this->tail == tmp) ? &this->head : this->tail;
			this->head.setNext(tmp->getNext());

			delete tmp;

			if(status != NULL)
				status = 0;
			return buffer;
		}

		unsigned int size()
		{
			std::lock_guard<std::mutex> lock(this->fifoMtx);
			return this->stackSize;
		}
};

#endif /* INC_UTILS_SYNCBUFFER_H_ */
