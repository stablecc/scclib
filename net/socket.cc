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
#include "net/socket.h"
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <sstream>
#include <string>
#include <ostream>
#include <cassert>
#include "util/safe_clib.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

/** \addtogroup net_sock
	@{ */
/** \ref net_sock implementation \file */
/** @} */

using namespace scc::net;

std::ostream& operator<<(std::ostream& os, const SockaddrBase& sa)
{
	return os.write(sa.str().c_str(), sa.str().size());
}

SocketBase::SocketBase(int domain, int stype, int proto) : m_fd{-1}
{
	m_fd = ::socket(domain, stype, proto);
	if (m_fd == -1)
	{
		std::stringstream st;
		st << "socket()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

SocketBase::~SocketBase()
{
	close();
}

void SocketBase::close()
{
	if (m_fd != -1)
	{
		scc::util::safe_close(m_fd);
		m_fd = -1;
	}
}

void SocketBase::reset(int domain, int stype, int proto)
{
	close();

	m_fd = ::socket(domain, stype, proto);
	if (m_fd == -1)
	{
		std::stringstream st;
		st << "socket() from reset()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

std::error_code SocketBase::error_code()
{
	int x;

	socklen_t len = sizeof(x);
	if (::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &x, &len))
	{
		std::stringstream st;
		st << "getsockopt(SO_ERROR)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return std::error_code(x, std::system_category());
}

void SocketBase::recv_bufsize(unsigned s)
{
	unsigned x = s;

	socklen_t len = sizeof(x);
	if (::setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, &x, len))
	{
		std::stringstream st;
		st << "setsockopt(SO_RCVBUF)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

unsigned SocketBase::recv_bufsize()
{
	unsigned x;

	socklen_t len = sizeof(x);
	if (::getsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, &x, &len))
	{
		std::stringstream st;
		st << "getsockopt(SO_RCVBUF)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return x;
}

void SocketBase::send_bufsize(unsigned s)
{
	unsigned x = s;

	socklen_t len = sizeof(x);
	if (::setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &x, len))
	{
		std::stringstream st;
		st << "setsockopt(SO_SNDBUF)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

unsigned SocketBase::send_bufsize()
{
	unsigned x;

	socklen_t len = sizeof(x);
	if (::getsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &x, &len))
	{
		std::stringstream st;
		st << "getsockopt(SO_SNDBUF)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return x;
}

void SocketBase::reuse_addr(bool r)
{
	int x = r ? 1 : 0;

	socklen_t len = sizeof(x);
	if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &x, len))
	{
		std::stringstream st;
		st << "setsockopt(SO_REUSEADDR)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void SocketBase::reuse_port(bool r)
{
	int x = r ? 1 : 0;

	socklen_t len = sizeof(x);
	if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &x, len))
	{
		std::stringstream st;
		st << "setsockopt(SO_REUSEPORT)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void SocketBase::non_blocking(bool b)
{
	int flags = ::fcntl(m_fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::stringstream st;
		st << "fcntl(F_GETFL)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	if (b)
	{
		if (::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			std::stringstream st;
			st << "fcntl(F_SETFL)";
			throw std::system_error(errno, std::system_category(), st.str());
		}
	}
	else
	{
		if (::fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK) == -1)
		{
			std::stringstream st;
			st << "fcntl(F_SETFL)";
			throw std::system_error(errno, std::system_category(), st.str());
		}
	}
}

void SocketBase::send_timeout(std::chrono::milliseconds t)
{
	timeval x;
	x.tv_sec = t.count() / 1000;
	x.tv_usec = (t.count() % 1000) * 1000;  // microseconds

	socklen_t len = sizeof(x);
	if (::setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, &x, len))
	{
		std::stringstream st;
		st << "setsockopt(SO_SNDTIMEO)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void SocketBase::recv_timeout(std::chrono::milliseconds t)
{
	timeval x;
	x.tv_sec = t.count() / 1000;
	x.tv_usec = (t.count() % 1000) * 1000;  // microseconds

	socklen_t len = sizeof(x);
	if (::setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &x, len))
	{
		std::stringstream st;
		st << "setsockopt(SO_RCVTIMEO)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

size_t SocketBase::recv(void* loc, size_t len, std::error_code& ec) noexcept
{
	ec.clear();

	ssize_t sz;
	do
	{
		sz = ::recv(m_fd, loc, len, 0);
	}
	while (sz == -1 && errno == EINTR);

	if (sz == -1)
	{
		ec.assign(errno, std::system_category());
		sz = 0;
	}

	return sz;
}

size_t SocketBase::recv(void* loc, size_t len)
{
	std::error_code ec;
	size_t sz = recv(loc, len, ec);
	if (ec.value())
	{
		std::stringstream st;
		st << "recv()";
		throw std::system_error(ec, st.str());
	}
	return sz;
}

size_t SocketBase::send(const void* loc, size_t len, std::error_code& ec) noexcept
{
	ec.clear();

	ssize_t sz;
	do
	{
		sz = ::send(m_fd, loc, len, MSG_NOSIGNAL); // don't generate a SIGPIPE if the other end drops the connection
	}
	while (sz == -1 && errno == EINTR);

	if (sz == -1)
	{
		ec.assign(errno, std::system_category());
		sz = 0;
	}

	return sz;
}

size_t SocketBase::send(const void* loc, size_t len)
{
	std::error_code ec;
	size_t sz = send(loc, len, ec);
	if (ec.value())
	{
		std::stringstream st;
		st << "send()";
		throw std::system_error(ec, st.str());
	}
	return sz;
}

void SocketBase::bind(const SockaddrBase& a)
{
	if (::bind(m_fd, a, a.len()))
	{
		std::stringstream st;
		st << "bind()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void SocketBase::get_sockaddr(sockaddr& a)
{
	socklen_t len = sizeof(a);
	if (::getsockname(m_fd, &a, &len))
	{
		std::stringstream st;
		st << "getsockname()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void TcpSocket::listen(int max_connection_backlog)
{
	if (::listen(fd(), max_connection_backlog))
	{
		std::stringstream st;
		st << "listen()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

int TcpSocket::accept(sockaddr* peer, int len, std::error_code& ec) noexcept
{
	ec.clear();

	socklen_t sl;
	sockaddr* sa = nullptr;
	socklen_t* sp = nullptr;

	if (peer)
	{
		sl = len;
		sa = peer;
		sp = &sl;
	}

	int nfd;
	do
	{
		nfd = ::accept(fd(), sa, sp); // get our new file descriptor
	}
	while (nfd == -1 && errno == EINTR);

	if (nfd == -1)
	{
		ec.assign(errno, std::system_category());
	}

	//assert(peer == nullptr || sl >= len);		// the unix socket length is variable

	return nfd;
}

void TcpSocket::connect(SockaddrBase& p, std::error_code& ec) noexcept
{
	ec.clear();

	int ret;
	// connect to partner
	do
	{
		ret = ::connect(fd(), p, p.len());
	}
	while (ret == -1 && errno == EINTR);

	if (ret)
	{
		ec.assign(errno, std::system_category());
	}
}

/**
	Make a connection to this address.
*/
void TcpSocket::connect(SockaddrBase& p)
{
	std::error_code ec;
	connect(p, ec);
	if (ec.value())
	{
		std::stringstream st;
		st << "connect()";
		throw std::system_error(ec, st.str());
	}
}

/**
	Shutdown the connection; no further reads or writes will be allowed.

	If the socket is shared (for example, via a fork) then communication will be
	stopped, even if all references to the socket have not been closed.
*/
void TcpSocket::shutdown()
{
	if (::shutdown(fd(), SHUT_RDWR))
	{
		std::stringstream st;
		st << "shutdown()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

UdpSocket::UdpSocket(int domain, int stype, int proto) : SocketBase(domain, stype, proto) { }

size_t UdpSocket::recv(void* loc, size_t len, SockaddrBase& a, std::error_code& ec) noexcept
{
	ec.clear();

	socklen_t alen = a.len();

	ssize_t sz;
	do
	{
		sz = ::recvfrom(fd(), loc, len, 0, a, &alen);
	}
	while (sz == -1 && errno == EINTR);

	if (sz == -1)
	{
		ec.assign(errno, std::system_category());
		sz = 0;
	}
	else
	{
		assert(alen <= a.len());		// in the case of unix domain sockets can return smaller length
	}

	return sz;
}

size_t UdpSocket::recv(void* loc, size_t len, SockaddrBase& a)
{
	std::error_code ec;
	size_t sz = recv(loc, len, a, ec);
	if (ec.value())
	{
		std::stringstream st;
		st << "recv()";
		throw std::system_error(ec, st.str());
	}
	return sz;
}

size_t UdpSocket::send(const void* loc, size_t len, const SockaddrBase& a, std::error_code& ec) noexcept
{
	ec.clear();

	ssize_t sz;
	do
	{
		sz = ::sendto(fd(), loc, len, 0, a, a.len());
	}
	while (sz == -1 && errno == EINTR);

	if (sz == -1)
	{
		ec.assign(errno, std::system_category());
		sz = 0;
	}

	return sz;
}

size_t UdpSocket::send(const void* loc, size_t len, const SockaddrBase& a)
{
	std::error_code ec;
	size_t sz = send(loc, len, a, ec);
	if (ec.value())
	{
		std::stringstream st;
		st << "send()";
		throw std::system_error(ec, st.str());
	}
	return sz;
}

size_t UdpSocket::recv_next()
{
	int v, r;
	do
	{
		r = ::ioctl(fd(), FIONREAD, &v);
	}
	while (r == -1 && errno == EINTR);
	if (r == -1)
	{
		std::stringstream st;
		st << "ioctl(FIONREAD)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return v;
}
