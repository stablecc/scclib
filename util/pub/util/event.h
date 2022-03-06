/*
BSD 3-Clause License

Copyright (c) 2022, Stable Cloud Computing, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _SYS_UTIL_EVENT_H
#define _SYS_UTIL_EVENT_H

#include <stdint.h>

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \defgroup util_event Signaling kernel event counter
	@{

	A wrapper for Linux eventfd. Provides a signallable event counter, which is readable and writable across process boundaries.
	
	See https://man7.org/linux/man-pages/man2/eventfd.2.html

	Test examples from \ref scclib/util/unittest/event.cc

	Blocking event:
	\snippet scclib/util/unittest/event.cc Blocking event

	Non-blocking event:
	\snippet scclib/util/unittest/event.cc Non-blocking event

	Non-blocking semaphore event:
	\snippet scclib/util/unittest/event.cc Non-blocking semaphore event

	Example from \ref scclib/net/unittest/inet.cc using a Poller to wait for Event
	\snippet scclib/net/unittest/inet.cc Inet client server
*/

/** Signaling kernel event counter
	\file
*/

/** Signaling kernel event counter.

	Writes increment the event counter.
	
	Sempahore mode means reads will decrement the counter by 1. Otherwise, reads reset the counter to 0.

	Non-blocking mode means the event can throw an exception (EAGAIN) if called when not signalled. The
	event should probably be polled in this case. See: \ref util_poller
*/
class Event
{
	int m_fd;
	int m_flags;

	void close();
public:
	/** Event flags. */
	enum EventFlag
	{
		nonblocking = 1,	///< Non-blocking mode. If an event is read while unsignalled, throws and exception.
		semaphore   = 2		///< Semaphore mode. Event reads decrement the counter by 1. Otherwise, reads set the counter to 0.
	};

	/** Construct an event.
		\param flags Event flags, see: \ref EventFlag
	*/
	Event(int = 0);
	virtual ~Event();

	/** No copy. */
	Event(const Event&) = delete;
	/** No copy. */
	void operator=(const Event&) = delete;

	/** Move constructor. */
	Event(Event&& other) : m_fd{other.m_fd}, m_flags{other.m_flags}
	{
		other.m_fd = -1;
		other.m_flags = 0;
	}
	/** Move assignment. */
	const Event& operator=(Event&& other)
	{
		m_fd = other.m_fd;
		m_flags = other.m_flags;
		other.m_fd = -1;
		other.m_flags = 0;
		return *this;
	}

	/** Cast object to int. */
	operator int() const { return m_fd; }
	/** Return file descriptor. -1 for invalid object. */
	int fd() const { return m_fd; }

	/** Reset the event.

		Reset the event to unsignalled state (counter 0).
		
		\param flags if >= 0, override the event flags, otherwise, reset to original flags. See: \ref EventFlag
	*/
	void reset(int = -1);

	/** Read from (decrement) the event counter.

		For blocking event, blocks until counter becomes signalled (non-zero).
		For a non-blocking event, throws an exception if the counter is not signalled.

		For a semaphore, returns 1 and decrements counter by 1.
		Otherwise, returns the counter value and sets counter to 0.
	*/
	uint64_t read();

	/** Write to (increment) the event counter.
		\param v Increments the event counter by the value.
	*/
	void write(uint64_t);
};
/** @} */
/** @} */
}

#endif
