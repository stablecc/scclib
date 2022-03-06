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
#include "net/inet.h"
#include "net/socket.h"
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <string>
#include <sstream>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

/** \addtogroup net_inet
	@{ */
/** \ref net_inet implementation \file */
/** @} */

using namespace scc::net;

void InetAddr::init()
{
	m_addr->sin6_family = AF_INET6;
	m_addr->sin6_port = 0;
	m_addr->sin6_flowinfo = 0;
	m_addr->sin6_addr = in6addr_any;
	m_addr->sin6_scope_id = 0;
}

InetAddr::InetAddr() : m_addr(new sockaddr_in6)
{
	init();
}

InetAddr::~InetAddr()
{
	delete m_addr;
}

InetAddr::InetAddr(unsigned p) : InetAddr()
{
	port(p);
}

InetAddr::InetAddr(const sockaddr* a) : InetAddr()
{
	if (a->sa_family == AF_INET)
	{
		const sockaddr_in* x = reinterpret_cast<const sockaddr_in*>(a);
		// convert to mapped IPv4 address
		//uint32_t h = ntohl(x->sin_addr.s_addr);
		uint32_t h = x->sin_addr.s_addr;
		unsigned char* b = reinterpret_cast<unsigned char*>(&h);
		m_addr->sin6_addr.s6_addr[10] = 0xff;
		m_addr->sin6_addr.s6_addr[11] = 0xff;
		m_addr->sin6_addr.s6_addr[12] = b[0];
		m_addr->sin6_addr.s6_addr[13] = b[1];
		m_addr->sin6_addr.s6_addr[14] = b[2];
		m_addr->sin6_addr.s6_addr[15] = b[3];

		m_addr->sin6_port = x->sin_port;
	}
	else if (a->sa_family == AF_INET6)
	{
		memcpy(m_addr, a, sizeof(sockaddr_in6));
	}
}

InetAddr::InetAddr(const InetAddr& b) : InetAddr()
{
	memcpy(m_addr, b.m_addr, sizeof(sockaddr_in6));
}

InetAddr& InetAddr::operator=(const InetAddr& b)
{
	memcpy(m_addr, b.m_addr, sizeof(sockaddr_in6));
	return *this;
}

InetAddr::InetAddr(InetAddr&& b) : InetAddr()
{
	memcpy(m_addr, b.m_addr, sizeof(sockaddr_in6));
	b.init();
}

InetAddr& InetAddr::operator=(InetAddr&& b)
{
	memcpy(m_addr, b.m_addr, sizeof(sockaddr_in6));
	b.init();
	return *this;
}

InetAddr::InetAddr(const std::string& h, unsigned p) : InetAddr()
{
	host(h);
	port(p);
}

InetAddr::operator const sockaddr*() const
{
	return reinterpret_cast<const sockaddr*>(m_addr);
}

InetAddr::operator sockaddr*()
{
	return reinterpret_cast<sockaddr*>(m_addr);
}

unsigned InetAddr::len() const
{
	return sizeof(sockaddr_in6);
}

void InetAddr::any_host()
{
	m_addr->sin6_addr = in6addr_any;
}

void InetAddr::local_host()
{
	m_addr->sin6_addr = in6addr_loopback;
}

void InetAddr::port(unsigned p)
{
	m_addr->sin6_port = htons(p);
}

unsigned InetAddr::port() const
{
	return ntohs(m_addr->sin6_port);
}


void InetAddr::scope_id(uint32_t s)
{
	m_addr->sin6_scope_id = s;
}

uint32_t InetAddr::scope_id() const
{
	return m_addr->sin6_scope_id;
}

void InetAddr::host(const std::string& h)
{
	in6_addr a;

	if (inet_pton(AF_INET6, h.c_str(), &a) <= 0) {		  // set the address
		std::ostringstream os;
		os << "InetAddr::host('" << h << "'): invalid address";
		throw std::runtime_error(os.str());
	}

	m_addr->sin6_addr = a;
}

std::string InetAddr::host() const
{
	char buf[INET6_ADDRSTRLEN];

	if (inet_ntop(AF_INET6, &m_addr->sin6_addr, buf, INET6_ADDRSTRLEN) == nullptr)   // copy address to string
	{
		std::stringstream st;
		st << "inet_ntop()";
		throw std::system_error(errno, std::system_category(), st.str());
	}

	return std::string(buf);
}

// test len bytes to see if they are equal to t
static bool btest(const unsigned char* b, unsigned t, int len)
{
	for (; len > 0; len--, b++)
	{
		if (*b != t)
		{
			return false;
		}
	}
	return true;
}

int InetAddr::flags() const
{
	int flags = 0;

	const unsigned char* ad = m_addr->sin6_addr.s6_addr;
	if (btest(ad, 0, 10) && ad[10] == 0xff && ad[11] == 0xff)
	{
		flags |= InetAddrFlag::ipv4;

		if (btest(ad+12, 0, 4))
		{
			flags |= InetAddrFlag::any;
		}
		else if (ad[12] == 127)
		{
			flags |= InetAddrFlag::loopback;
		}
		else if (ad[12] >= 224 && ad[12] <= 239)
		{
			flags |= InetAddrFlag::multicast;
			flags |= InetAddrFlag::global;
		}
		else
		{
			flags |= InetAddrFlag::unicast;
			flags |= InetAddrFlag::global;
		}
	}
	else
	{
		flags |= InetAddrFlag::ipv6;

		if (btest(ad, 0, 16))	// ::
		{
			flags |= InetAddrFlag::any;
		}
		else if (btest(ad, 0, 15) && ad[15] == 1)	// ::1
		{
			flags |= InetAddrFlag::loopback;
		}
		else if (ad[0] == 0xff)
		{
			flags |= InetAddrFlag::multicast;
			// flags
			if ((ad[1] & 0x10) == 0x10)
			{
				flags |= InetAddrFlag::mcast_dynamic;
			}
			if ((ad[1] & 0x20) == 0x10)
			{
				flags |= InetAddrFlag::mcast_prefix;
			}
			if ((ad[1] & 0x40) == 0x10)
			{
				flags |= InetAddrFlag::mcast_rendezvous;
			}
			// scope
			switch (ad[1] & 0xf)
			{
				case 0x1: flags |= InetAddrFlag::if_local; break;
				case 0x2: flags |= InetAddrFlag::link_local; break;
				case 0x3: flags |= InetAddrFlag::realm_local; break;
				case 0x4: flags |= InetAddrFlag::admin_local; break;
				case 0x5: flags |= InetAddrFlag::site_local; break;
				case 0x8: flags |= InetAddrFlag::org_local; break;
				case 0xe: flags |= InetAddrFlag::global; break;
			}
			// reserved addresses ff00::/24
			if ((ad[1] & 0xf0) == 0x00)
			{
				if (btest(ad+2, 0, 13) && ad[15] == 1)		// ff0X::1
				{
					flags |= InetAddrFlag::mcast_all_nodes;
				}
				if (btest(ad+2, 0, 13) && ad[15] == 2)		// ff0X::2
				{
					flags |= InetAddrFlag::mcast_all_routers;
				}
			}
		}
		else
		{
			flags |= InetAddrFlag::unicast;
			
			// fe80::/64
			if (ad[0] == 0xfe && ad[1] == 0x80 && btest(ad+2, 0, 6))	// 8 bytes total
			{
				flags |= InetAddrFlag::link_local;
			}
			else
			{
				flags |= InetAddrFlag::global;
			}

			// fc00::/7; currently fd00::/8 is defined for /48 prefixes
			if ((ad[0] & 0xfe) == 0xfc)
			{
				flags |= InetAddrFlag::unique_local_address;
			}
		}
	}

	return flags;
}

std::ostream& operator<<(std::ostream& os, const InetAddr& sa)
{
	return os.write(sa.str().c_str(), sa.str().size());
}

std::string InetAddr::str() const
{
	std::stringstream s;
	int f = flags();
	switch (f & InetAddrFlag::prot_mask)
	{
		case InetAddrFlag::ipv4:
			s << "ipv4";
			break;
		case InetAddrFlag::ipv6:
			s << "ipv6";
			break;
	}
	s << " " << host();
	s << " port: " << port() << " scope_id: " << scope_id();
	s << " flags:";
	switch (f & InetAddrFlag::type_mask)
	{
		case InetAddrFlag::any:
			s << " type-any";
			break;
		case InetAddrFlag::loopback:
			s << " type-loop";
			break;
		case InetAddrFlag::multicast:
			s << " type-mcast";
			break;
		case InetAddrFlag::unicast:
			s << " type-unicast";
			break;
	}

	switch (f & scope_mask)
	{
		case InetAddrFlag::if_local:
			s << " scope-iface-local";
			break;
		case InetAddrFlag::link_local:
			s << " scope-link-local";
			break;
		case InetAddrFlag::realm_local:
			s << " scope-realm-local";
			break;
		case InetAddrFlag::admin_local:
			s << " scope-admin-local";
			break;
		case InetAddrFlag::site_local:
			s << " scope-site-local";
			break;
		case InetAddrFlag::org_local:
			s << " scope-org-local";
			break;
		case InetAddrFlag::global:
			s << " scope-global";
			break;
	}

	switch (f & InetAddrFlag::mcast_flags_mask)
	{
		case InetAddrFlag::mcast_rendezvous:
			s << " mcast-flags-rendezvous";
			break;
		case InetAddrFlag::mcast_prefix:
			s << " mcast-flags-prefix";
			break;
		case InetAddrFlag::mcast_dynamic:
			s << " mcast-flags-dynamic";
			break;
	}

	switch (f & InetAddrFlag::mcast_reserved_mask)
	{
		case InetAddrFlag::mcast_all_nodes:
			s << " mcast-all-nodes";
			break;
		case InetAddrFlag::mcast_all_routers:
			s << " mcast-all-routers";
			break;
	}

	switch (f & InetAddrFlag::unicast_special_mask)
	{
		case InetAddrFlag::unique_local_address:
			s << " unique-local-address";
			break;
	}

	return s.str();
}

InetTcpSock::InetTcpSock() : TcpSocket(AF_INET6, SOCK_STREAM, 0)
{}

void InetTcpSock::reset()
{
	SocketBase::reset(AF_INET6, SOCK_STREAM, 0);
}

InetAddr InetTcpSock::get_addr()
{
	sockaddr a;
	get_sockaddr(a);
	return InetAddr(&a);
}

InetTcpSock InetTcpSock::accept()
{
	std::error_code ec;
	InetTcpSock s(TcpSocket::accept(nullptr, 0, ec));
	if (ec.value())
	{
		std::stringstream st;
		st << "accept()";
		throw std::system_error(ec, st.str());
	}
	return s;
}

InetTcpSock InetTcpSock::accept(InetAddr& peer)
{
	std::error_code ec;
	InetTcpSock s(TcpSocket::accept(peer, peer.len(), ec));
	if (ec.value())
	{
		std::stringstream st;
		st << "accept(peer)";
		throw std::system_error(ec, st.str());
	}
	return s;
}

std::shared_ptr<InetTcpSock> InetTcpSock::accept_shared()
{
	std::error_code ec;
	std::shared_ptr<InetTcpSock> s(new InetTcpSock(TcpSocket::accept(nullptr, 0, ec)));
	if (ec.value())
	{
		std::stringstream st;
		st << "accept()";
		throw std::system_error(ec, st.str());
	}
	return s;
}

std::shared_ptr<InetTcpSock> InetTcpSock::accept_shared(InetAddr& peer)
{
	std::error_code ec;
	std::shared_ptr<InetTcpSock> s(new InetTcpSock(TcpSocket::accept(peer, peer.len(), ec)));
	if (ec.value())
	{
		std::stringstream st;
		st << "accept(peer)";
		throw std::system_error(ec, st.str());
	}
	return s;
}

InetUdpSock::InetUdpSock() : UdpSocket(AF_INET6, SOCK_DGRAM, 0) { }

void InetUdpSock::reset()
{
	SocketBase::reset(AF_INET6, SOCK_DGRAM, 0);
}

InetAddr InetUdpSock::get_addr()
{
	sockaddr a;
	get_sockaddr(a);
	return InetAddr(&a);
}

void InetUdpSock::mcast_join_group(const InetAddr& group_addr, unsigned interface)
{
	const sockaddr_in6* a{group_addr};
	ipv6_mreq mr;
	mr.ipv6mr_multiaddr = a->sin6_addr;
	mr.ipv6mr_interface = interface;

	socklen_t len = sizeof(mr);

	if (::setsockopt(fd(), IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mr, len))
	{
		std::stringstream st;
		st << "mcast join group setsockopt()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void InetUdpSock::mcast_leave_group(const InetAddr& group_addr, unsigned interface)
{
	const sockaddr_in6* a{group_addr};
	ipv6_mreq mr;
	mr.ipv6mr_multiaddr = a->sin6_addr;
	mr.ipv6mr_interface = interface;

	socklen_t len = sizeof(mr);

	if (::setsockopt(fd(), IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mr, len))
	{
		std::stringstream st;
		st << "mcast leave group setsockopt()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void InetUdpSock::mcast_interface(unsigned interface)
{
	unsigned v{interface};
	socklen_t len = sizeof(v);

	if (::setsockopt(fd(), IPPROTO_IPV6, IPV6_MULTICAST_IF, &v, len))
	{
		std::stringstream st;
		st << "mcast interface setsockopt()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void InetUdpSock::mcast_loopback(bool loop)
{
	unsigned x = loop ? 1 : 0;
	socklen_t len = sizeof(x);

	if (::setsockopt(fd(), IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &x, len))
	{
		std::stringstream st;
		st << "mcast loopback setsockopt()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}

void InetUdpSock::mcast_hops(unsigned hops)
{
	unsigned v{hops};
	socklen_t len = sizeof(v);

	if (::setsockopt(fd(), IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &v, len))
	{
		std::stringstream st;
		st << "mcast hops setsockopt()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
}
