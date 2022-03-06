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
#ifndef _SYS_UTIL_POLLER_H
#define _SYS_UTIL_POLLER_H

#include <map>
#include <chrono>

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \defgroup util_poller Kernel file descriptor event notification poller
	@{
	
	Linux kernel file descriptor event notification, using Linux epoll.
	
	See https://man7.org/linux/man-pages/man7/epoll.7.html
*/

/** Linux kernel i/o event notification (poller).
	\file
*/

/** Poller which allows polling of generic file descriptors for various events.

	Examples of valid descriptors are pipes and stream (TCP) sockets.

	After a wait(), the poller can be tested to see which events have been polled.

	Example from \ref scclib/net/unittest/inet.cc using a Poller to wait for Event
	\snippet scclib/net/unittest/inet.cc Inet client server
*/
class Poller
{
	int m_fd;
	std::map<int, int> m_polls;
	std::map<int, int> m_events;
	void wait(int);
	int converteflags(int);

public:
	/** Polling flags. */
	enum PollFlag
	{
		input		= 0x1,		///< File descriptor is available for read operations
		output		= 0x2,		///< File descriptor is available for write operations
		read_hup	= 0x4,		///< For stream sockets, peer connection (or local connection) has been shut down.
		priority	= 0x8,		///< There is some exceptional condition on the file descriptor. E.g. There is out-of-band data on a TCP socket.
		hup			= 0x10,		///< Hang up. The local or peer connection has been closed.
		error		= 0x20		///< Error condition.
	};

	Poller();
	virtual ~Poller();

	Poller(const Poller&) = delete;				 // no copying
	void operator=(const Poller&) = delete;

	Poller(Poller&&) = delete;					  // no move
	const Poller& operator=(Poller&&) = delete;

	/** Add a file desriptor to poller.

		Only input, output, read_hup or priority must be specified, hup and error are automatically polled.

		\param fd File descriptor
		\param flags Logical OR of polling flags, see: \ref PollFlag
	*/
	void set(int, int);

	/** Remove a file descriptor from poller.

		\param fd File descriptor
	*/
	void remove(int);
	
	/** Wait forever for an event. */
	void wait()
	{
		wait(-1);
	}
	
	/** Wait for a number of milliseconds for an event. */
	void wait(std::chrono::milliseconds t)
	{
		wait(t.count());
	}

	/** Return flags which were polled for this file descriptor.

		\param fd File descriptor
		\returns 0 No event was polled for this descriptor
		\returns event Logical OR of poll flags, see: \ref PollFlag
	*/
	int event(int);
};
/** @} */
/** @} */
}

#endif
