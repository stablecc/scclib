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
#ifndef _SCC_NET_SOCKET_H
#define _SCC_NET_SOCKET_H

#include <string>
#include <system_error>
#include <chrono>
#include <util/iobase.h>

struct sockaddr;

namespace scc::net {

/** \addtogroup net
	@{
*/

/** \defgroup net_sock Low-level tcp and udp sockets
	@{

	Low-level socket interface.
*/

/** Low-level tcp and udp sockets.
	\file
*/

/** Socket address base class.
*/
class SockaddrBase
{
public:
	SockaddrBase() {}
	virtual ~SockaddrBase() {}
	virtual operator const sockaddr*() const { return nullptr; }
	virtual operator sockaddr*() { return nullptr; }
	virtual unsigned len() const { return 0; }
	virtual std::string str() const { return ""; }
	virtual std::string host() const { return ""; }
};

/** Socket base class.

	In general most errors are returned via exception. Some of the interfaces can be called with an error_code reference, in which case the
	error return must be checked by the caller. \see SocketBase::error_code
*/
class SocketBase
{
	int	m_fd;

	/** Copy creation not allowed. */
	SocketBase(const SocketBase&) = delete;
	/** Copy assignment not allowed. */
	void operator=(const SocketBase&) = delete;

protected:
	explicit SocketBase(int fd) : m_fd{fd} {}
	SocketBase(int, int, int);
	void reset(int, int, int);
	void move(SocketBase& other)
	{
		m_fd = other.m_fd;
		other.m_fd = -1;
	}
	void get_sockaddr(sockaddr&);

public:
	virtual ~SocketBase();
	/** Move construct socket. */
	SocketBase(SocketBase&& other)
	{
		move(other);
	}
	/** Move assign socket. */
	const SocketBase& operator=(SocketBase&& other)
	{
		close();
		move(other);
		return *this;
	}

	/** Return the underlying socket handle. */
	int fd() const { return m_fd; }
	/** Allow object to be cast to a socket handle. */
	operator int() const { return m_fd; }

	/**
		Set address reusable.

		Allows a bind to an address if there is not an active listener on the address.

		The system may delay releasing a non-reusable address after it has been used.

		The system default is non-reusable.
	*/
	void reuse_addr(bool r = true);

	/**
		Set port reusable.

		Allows multiple sockets to be bound to an identical address.
		
		This must be set on each socket before calling bind().

		For TCP sockets, multiple threads or processes can now have a distinct listening socket.

		This may be more efficient than using a single listening thread to distribute connections, or having multiple threads that
		compete to accept from the same socket.

		For UDP sockets, having a distinct socket per thread or process may be more efficient than having multiple threads compete
		for datagrams from the same socket.

		The system default is non-reuseable.
	*/
	void reuse_port(bool r = true);
	
	/**
		Set the socket non-blocking.

		Most socket operations that would normally block will now return with EAGAIN error code.
		
		connect() calls will return the EINPROGRESS error.

		Allows polling of the socket. \see scc::util::Poller

		From the system documentation, the following polling flags can be used for the socket:
		┌────────────────────────────────────────────────────────────────────┐
		│                            I/O events                              │
		├───────────┬───────────┬────────────────────────────────────────────┤
		│Event      │ Poll flag │ Occurrence                                 │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Read       │ POLLIN    │ New data arrived.                          │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Read       │ POLLIN    │ A connection setup has been completed (for │
		│           │           │ connection-oriented sockets)               │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Read       │ POLLHUP   │ A disconnection request has been initiated │
		│           │           │ by the other end.                          │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Read       │ POLLHUP   │ A connection is broken (only  for  connec‐ │
		│           │           │ tion-oriented protocols).  When the socket │
		│           │           │ is written SIGPIPE is also sent.           │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Write      │ POLLOUT   │ Socket has enough send  buffer  space  for │
		│           │           │ writing new data.                          │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Read/Write │ POLLIN |  │ An outgoing connect(2) finished.           │
		│           │ POLLOUT   │                                            │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Read/Write │ POLLERR   │ An asynchronous error occurred.            │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Read/Write │ POLLHUP   │ The other end has shut down one direction. │
		├───────────┼───────────┼────────────────────────────────────────────┤
		│Exception  │ POLLPRI   │ Urgent data arrived.  SIGURG is sent then. │
		└───────────┴───────────┴────────────────────────────────────────────┘

		Default is false.
	*/
	void non_blocking(bool b = true);
	/**
		Get the current error code (status).

		In general, error category() is std::error_code::system_category
		
		Error value() 0 means success, otherwise this is a system error.

		<cerror> header contains system errors, e.g. EINPROGRESS means a call on a non-blocking socket cannot be completed immediately.
	*/
	std::error_code error_code();
	/**
		Set receive buffer size.

		Minimum input value is 128. The system will double the value input to account for
		bookeeping overhead.
	*/
	void recv_bufsize(unsigned);
	/**
		Get total receive buffer size including overhead.
	*/
	unsigned recv_bufsize();
	/**
		Set send buffer size.

		Minimum input value is 1024. The system will double the value input to account for
		bookeeping overhead.
	*/
	void send_bufsize(unsigned);
	/**
		Get total send buffer size including overhead.
	*/
	unsigned send_bufsize();
	/**
		Set the send timeout.

		When a send() is called on a blocking socket:

		If the timeout elapses and there is data available, the send() will return the amount of data available.
		If no data is availble, the system will return an error of EAGAIN or EWOULDBLOCK as if the socket were non-blocking.

		The default is 0, meaning that the send() has no timeout.
	*/
	void send_timeout(std::chrono::milliseconds);
	/**
		Set the receive timeout.

		When a recv() is called on a blocking socket:

		If the timeout elapses and there is data available, the recv() will return the amount of data available.
		If no data is availble, the system will return an error of EAGAIN or EWOULDBLOCK as if the socket were non-blocking.

		The default is 0, meaning that the recv() has no timeout.
	*/
	void recv_timeout(std::chrono::milliseconds);

	/**
		Bind an address to the socket.
	*/
	void bind(const SockaddrBase&);
	/**
		Receive bytes, throwing an exception on error.
		
		Returns the number of bytes received. If a non-zero number of bytes is requested, 0 will be returned if the
		peer has performed an orderly shutdown.
	*/
	size_t recv(void* loc, size_t len);
	/**
		Receive bytes.
		
		Returns the number of bytes received. If a non-zero number of bytes is requested, 0 will be returned if the
		peer has performed an orderly shutdown.

		The caller should check the error code(), with 0 meaning success.

		Non-blocking sockets may return error codes of EAGAIN or EWOULDBLOCK, meaning that there is no data available, and no other error.
	*/
	size_t recv(void* loc, size_t len, std::error_code& e) noexcept;
	/**
		Send bytes, throwing an exception on error.
		
		Returns the number of bytes sent.
	*/
	size_t send(const void* loc, size_t len);
	/**
		Send bytes, setting the error code for the call.
		
		Returns the number of bytes sent.

		The caller should check the error code(), with 0 meaning success.

		Non-blocking sockets may return error codes of EAGAIN or EWOULDBLOCK, meaning that there is no data available, and no other error.
	*/
	size_t send(const void* loc, size_t len, std::error_code& e) noexcept;
	/**
		Close the socket.
	*/
	void close();
};

/** Tcp socket base class.
*/
class TcpSocket : public SocketBase, public scc::util::Reader, public scc::util::Writer
{
protected:
	explicit TcpSocket(int fd) : SocketBase(fd) { }
	TcpSocket(int domain, int stype, int proto) : SocketBase(domain, stype, proto) { }
	/**
		Accept a connection. Returns a file descriptor for the new socket, or return an error code on failure.
		Sets the exception error code for the call.

		If the sockaddr pointer is set, sets it to the address of the connected peer.
	*/
	int accept(sockaddr*, int len, std::error_code&) noexcept;

public:

	virtual void reset() = 0;

	/**
		Set to accept connections.

		After maximum connection backlog is reached (of unaccepted connections),
		system may refuse new connections on the socket.
	*/
	void listen(int maxConnections = 10);

	/**
		Connect to a socket address. Throw an exception on error.
	*/
	void connect(SockaddrBase&);
	/**
		Connect to a socket address. Set the error code for the call. Caller must check code(), which is 0 on success.
	*/
	void connect(SockaddrBase&, std::error_code&) noexcept;

	/**
		Read bytes. Returns number of bytes read, or 0 on peer shutdown (if non-zero bytes requested).

		Throws an exception on error. \see scc::util::Reader
	*/
	virtual size_t read(void* loc, size_t len)
	{
		return recv(loc, len);
	}
	/**
		Write bytes. Returns number of bytes written.

		Throws an exception on error. \see scc::util::Writer
	*/
	virtual size_t write(const void* loc, size_t len)
	{
		return send(loc, len);
	}

	void shutdown();
};

/** Udp socket base class.
*/
class UdpSocket : public SocketBase
{
protected:
	UdpSocket(int, int, int);
public:
	virtual void reset() = 0;
	
	/**
		Receive bytes (a datagram), setting the socket address of the peer.

		/see Socket::recv
	*/
	size_t recv(void* loc, size_t len, SockaddrBase& s);
	/**
		Receive bytes (a datagram), setting the socket address of the peer, and the error code.

		/see Socket::recv
	*/
	size_t recv(void* loc, size_t len, SockaddrBase& s, std::error_code& e) noexcept;
	/**
		Send bytes (a datagram) to a peer address.

		/see Socket::send
	*/
	size_t send(const void* loc, size_t len, const SockaddrBase& d);
	/**
		Send bytes (a datagram) to a peer address, setting the error code.

		/see Socket::send
	*/
	size_t send(const void* loc, size_t len, const SockaddrBase& d, std::error_code& e) noexcept;
	/**
		Return the number of bytes available to read (the size of the next datagram).
	*/
	size_t recv_next();
};

/** @} */
/** @} */

}	// namespace

/**
	Helper to print socket address to output stream.
*/
std::ostream& operator<<(std::ostream&, const scc::net::SockaddrBase&);

#endif
