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
#include "net/unix.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <string>
#include <sstream>
#include <cstring>

using namespace scc::net;

void UnixAddr::init()
{
	m_addr->sun_family = AF_UNIX;
	m_addr->sun_path[0] = 0;
}

/**  IPv6 SocketBase address, initialized with "any" address
*/
UnixAddr::UnixAddr() : m_addr(new sockaddr_un)
{
	init();
}

UnixAddr::~UnixAddr()
{
	delete m_addr;
}

UnixAddr::UnixAddr(const std::string& n) : UnixAddr()
{
	host(n);
}

UnixAddr::UnixAddr(const sockaddr* a) : UnixAddr()
{
	if (a->sa_family == AF_UNIX)
	{
		memcpy(m_addr, a, sizeof(sockaddr_un));
	}
}

UnixAddr::UnixAddr(const UnixAddr& b) : UnixAddr()
{
	strcpy(m_addr->sun_path, b.m_addr->sun_path);
}

UnixAddr& UnixAddr::operator=(const UnixAddr& b)
{
	strcpy(m_addr->sun_path, b.m_addr->sun_path);
	return *this;
}

UnixAddr::UnixAddr(UnixAddr&& b) : UnixAddr()
{
	strcpy(m_addr->sun_path, b.m_addr->sun_path);
	b.init();
}

UnixAddr& UnixAddr::operator=(UnixAddr&& b)
{
	strcpy(m_addr->sun_path, b.m_addr->sun_path);
	b.init();
	return *this;
}

UnixAddr::operator const sockaddr*() const
{
	return reinterpret_cast<const sockaddr*>(m_addr);
}

UnixAddr::operator sockaddr*()
{
	return reinterpret_cast<sockaddr*>(m_addr);
}

unsigned UnixAddr::len() const
{
	return sizeof(sockaddr_un);
}

/**
	Set the unix socket filename. Must be < 108 chars.

	This file file be created in the filesystem with type "socket".
*/
void UnixAddr::host(const std::string& h)
{
	if (h.size() < sizeof(m_addr->sun_path))			// sun_path is 108 bytes long
	{
		strcpy(m_addr->sun_path, h.c_str());
	}
	else
	{
		strncpy(m_addr->sun_path, h.c_str(), sizeof(m_addr->sun_path)-1);
		m_addr->sun_path[sizeof(m_addr->sun_path)-1] = 0;
	}
}

std::string UnixAddr::host() const
{
	return std::string(m_addr->sun_path);
}

std::ostream& operator<<(std::ostream& os, const UnixAddr& sa)
{
	return os.write(sa.str().c_str(), sa.str().size());
}

std::string UnixAddr::str() const
{
	std::stringstream s;
	s << "unix domain name=" << host();
	return s.str();
}

UnixTcpSock::UnixTcpSock() : TcpSocket(AF_UNIX, SOCK_STREAM, 0) { }

UnixTcpSock::UnixTcpSock(int fd) : TcpSocket(fd) { }

void UnixTcpSock::reset()
{
	SocketBase::reset(AF_UNIX, SOCK_STREAM, 0);
}

UnixAddr UnixTcpSock::get_addr()
{
	sockaddr a;
	get_sockaddr(a);
	return UnixAddr(&a);
}

UnixTcpSock UnixTcpSock::accept()
{
	std::error_code ec;
	UnixTcpSock s(TcpSocket::accept(nullptr, 0, ec));
	if (ec.value())
	{
		std::stringstream st;
		st << "accept()";
		throw std::system_error(ec, st.str());
	}
	return s;
}

UnixTcpSock UnixTcpSock::accept(UnixAddr& peer)
{
	std::error_code ec;
	UnixTcpSock s(TcpSocket::accept(peer, peer.len(), ec));
	if (ec.value())
	{
		std::stringstream st;
		st << "accept(peer)";
		throw std::system_error(ec, st.str());
	}
	return s;
}

UnixUdpSock::UnixUdpSock() : UdpSocket(AF_UNIX, SOCK_DGRAM, 0) { }

void UnixUdpSock::reset()
{
	SocketBase::reset(AF_UNIX, SOCK_DGRAM, 0);
}

UnixAddr UnixUdpSock::get_addr()
{
	sockaddr a;
	get_sockaddr(a);
	return UnixAddr(&a);
}
