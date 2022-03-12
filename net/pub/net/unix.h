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
#ifndef _NET_UNIX_H
#define _NET_UNIX_H

#include <string>
#include <cstdint>
#include <net/socket.h>

struct sockaddr_un;

namespace scc::net {

/** \addtogroup net
	@{
*/

/** \defgroup net_unix Unix domain tcp and udp networking
	@{

	Local networking using socket files.
*/

/** Unix domain tcp and udp networking.
	\file
*/

/** A unix domain address, which is a file of type "socket."
*/
class UnixAddr : public SockaddrBase
{
	sockaddr_un* m_addr;

	void init();

public:
	UnixAddr();
	virtual ~UnixAddr();
	UnixAddr(const std::string&);
	/** Initialize with socket address pointer. */
	UnixAddr(const sockaddr*);
	/** Copy create. */
	UnixAddr(const UnixAddr&);
	/** Copy assign. */
	UnixAddr& operator=(const UnixAddr&);
	/** Move create. */
	UnixAddr(UnixAddr&&);
	/** Move assign. */
	UnixAddr& operator=(UnixAddr&&);

	/** Cast to const opaque socket address. */
	virtual operator const sockaddr*() const;
	/** Cast to opaque socket address. */
	virtual operator sockaddr*();
	/** Length of socket address. */
	virtual unsigned len() const;
	/** Descriptive string for socket address. */
	virtual std::string str() const;
	/** Get host name. For unix domain sockets this is a file name. */
	virtual std::string host() const;

	/** Set host name. For unix domain sockets this is a file name. */
	void host(const std::string&);
};

/** Unix domain tcp (stream) socket.
*/
class UnixTcpSock : public TcpSocket
{
	explicit UnixTcpSock(int);

public:
	UnixTcpSock();

	/**
		Reset the socket.
	*/
	virtual void reset();

	/**
		Get the socket address.
	*/
	UnixAddr get_addr();

	/**
		Accept a connection from an anonymous peer.

		Returns a new socket connected to the peer, or throws an exception on error.

		The socket can be polled for Poller::PollFlag::input events, to wait for incoming connections without blocking.
	*/
	UnixTcpSock accept();
	/**
		Accept a connection.

		Returns a new socket connected to the peer and sets the peer address, or throws an exception on error.

		The socket can be polled for Poller::PollFlag::input events, to wait for incoming connections without blocking.
	*/
	UnixTcpSock accept(UnixAddr&);
};

/** Unix domain udp (datagram) socket.
*/
class UnixUdpSock : public UdpSocket
{
public:
	UnixUdpSock();

	/**
		Reset the socket.
	*/
	virtual void reset();
	
	/**
		Get the socket address.
	*/
	UnixAddr get_addr();
};

/** @} */
/** @} */

}	// namespace

std::ostream& operator<<(std::ostream&, const scc::net::UnixAddr&);	// streaming helper

#endif
