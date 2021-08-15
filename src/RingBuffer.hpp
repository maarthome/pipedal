#pragma once
#include <cstddef>
#include "PiPedalException.hpp"
#include <atomic>
#include <mutex>
#include <semaphore.h>


#ifndef NO_MLOCK
#include <sys/mman.h>
#endif /* NO_MLOCK */


namespace pipedal
{
    template <bool MULTI_WRITER = false, bool SEMAPHORE_READER = false>
    class RingBuffer {
        char *buffer;
        bool mlocked = false;
        size_t ringBufferSize;
        size_t ringBufferMask;
        volatile int64_t readPosition = 0;  // volatile = ordering barrier wrt writePosition
        volatile int64_t writePosition = 0; // volatile = ordering barrier wrt/ readPosition
        std::mutex write_mutex;

        sem_t readSemaphore;
        bool semaphore_open = false;

        size_t nextPowerOfTwo(size_t size)
        {
            size_t v = 1;
            while (v < size)
            {
                v *= 2;
            }
            return v;
        }
    public:
        RingBuffer(size_t ringBufferSize = 65536, bool mLock = true) 
        {
            this->ringBufferSize = ringBufferSize = nextPowerOfTwo(ringBufferSize);
            ringBufferMask = ringBufferSize-1;
            buffer = new char[ringBufferSize];

            if (SEMAPHORE_READER) {
                sem_init(&readSemaphore,0,0);
                semaphore_open = true;
            }

            #ifndef NO_MLOCK
            if (mLock)
            {
                if (mlock (buffer, ringBufferSize)) {
		            throw PiPedalStateException("Mlock failed.");
                }
                this->mlocked = true;
            }
            #endif
        }

        void reset() {
            this->readPosition = 0;
            this->writePosition = 0;
            if (SEMAPHORE_READER)
            {
                sem_destroy(&readSemaphore);
                sem_init(&readSemaphore,0,0);
                this->semaphore_open = true;
            }
        }
        void close() {
            if (SEMAPHORE_READER)
            {
                this->semaphore_open = false;
                sem_post(&readSemaphore);
            }
        }
        // 0 -> ready. -1: timed out. -2: closing.
        int readWait(const struct timespec& timeoutMs) {
            if (SEMAPHORE_READER)
            {
                int result = sem_timedwait(&readSemaphore,&timeoutMs);
                if (!semaphore_open) return -2;
                return  (result == 0) ? 0: -1;
            } else {
                throw PiPedalStateException("SEMAPHORE_READER is not set to true.");
            }
        }

        bool readWait() {
            if (SEMAPHORE_READER)
            {
                sem_wait(&readSemaphore);
                return semaphore_open;
            } else {
                throw PiPedalStateException("SEMAPHORE_READER is not set to true.");
            }
        }
        size_t writeSpace() {
            // at most ringBufferSize-1 in order to 
            // to distinguish the empty buffer from the full buffer.
            int64_t size = readPosition-1-writePosition;
            if (size < 0) size += this->ringBufferSize;
            return (size_t)size;
        }
        size_t readSpace() {
            int64_t size = writePosition-readPosition;
            if (size < 0) size += this->ringBufferSize;
            return size_t(size);
        }
        bool write(size_t bytes, uint8_t *data)
        {
            if (MULTI_WRITER)
            {
                std::lock_guard guard(write_mutex);
                if (writeSpace() < bytes) {
                    return false;
                }
                size_t index = this->writePosition;
                for (size_t i = 0; i < bytes; ++i)
                {
                    buffer[(index+i) & ringBufferMask] = data[i];
                }
                this->writePosition = (index+bytes) & ringBufferMask;
                if (SEMAPHORE_READER)
                {
                    sem_post(&readSemaphore);
                }
                return true;
            } else {
                if (writeSpace() < bytes) {
                    return false;
                }
                size_t index = this->writePosition;
                for (size_t i = 0; i < bytes; ++i)
                {
                    buffer[(index+i) & ringBufferMask] = data[i];
                }
                this->writePosition = (index+bytes) & ringBufferMask;
                if (SEMAPHORE_READER)
                {
                    sem_post(&readSemaphore);
                }
                return true;
            }
        }
        bool read(size_t bytes, uint8_t*data)
        {
            if (readSpace() < bytes) return false;
            int64_t readPosition = this->readPosition;
            for (size_t i = 0; i < bytes; ++i)
            {
                data[i] = this->buffer[(readPosition+i) & this->ringBufferMask];
            }
            this->readPosition = (readPosition + bytes) & this->ringBufferMask;
            return true;
        }
        ~RingBuffer() 
        {
            #ifdef USE_MLOCK
                if (this->mlocked)
                {
                    munlock(buffer,ringBufferSize);
                }
            #endif
            if (SEMAPHORE_READER)
            {
                sem_destroy(&this->readSemaphore);
            }

            delete[] buffer;
        }
    };

};

