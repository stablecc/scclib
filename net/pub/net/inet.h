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
#ifndef _SCC_NET_INET_H
#define _SCC_NET_INET_H

#include <string>
#include <cstdint>
#include <memory>
#include <net/socket.h>

// forward declarations
struct sockaddr;
struct sockaddr_in6;

namespace scc::net {

/** \addtogroup net
	@{
*/

/** \defgroup net_inet Internet tcp and udp networking
	@{

	Tcp/ip and udp/ip networking.

	Test examples from \ref scclib/net/unittest/inet.cc

	Streaming tcp client/server, also showing non-blocking socket connect:
	\snippet scclib/net/unittest/inet.cc Inet client server

	Multicast udp client/server:
	\snippet scclib/net/unittest/inet.cc Multicast client server

	Port and address reuse with a multithreaded server:
	\snippet scclib/net/unittest/inet.cc TCP multiserver
*/

/** Internet tcp and udp networking.
	\file
*/

/**
	Internet address flags.
*/
enum InetAddrFlag
{
	prot_mask=		0xf,				///< Protocol mask.
	ipv4=			0x1,				///< IPv4.
	ipv6=			0x2,				///< IPv6.

	type_mask=		0xf0,				///< Address mask.
	any=			0x10,				///< Any address.
	loopback=		0x20,				///< Loopback address.
	unicast=		0x40,				///< Unicast address.
	multicast=		0x80,				///< Multicast address.

	scope_mask=		0xff00,			 	///< Scope for multicast addresses mask.
	if_local=		0x0100,				///< Traffic is restricted to the local interface.
	link_local=		0x0200,				///< Traffic is restricted to the local link.
	realm_local=	0x0400,				///< Traffic is restricted to the local realm.
	admin_local=	0x0800,				///< Traffic is restricted to the local admin.
	site_local=		0x1000,				///< Traffic is restricted to the local site.
	org_local=		0x2000,				///< Traffic is restricted to the local organization.
	global=			0x4000,				///< Global traffic is allowed.

	mcast_flags_mask=		0xf0000,		///< Multicast flags mask.
	mcast_rendezvous =		0x10000,		///< Address has a rendezvous point embedded.
	mcast_prefix=			0x20000,		///< Prefix-based address.
	mcast_dynamic=			0x40000,		///< Dynamic (temporary) address, otherwise permanent (assigned).
	
	mcast_reserved_mask=	0xff00000,		///< Some reserved multicast addresses.
	mcast_all_nodes=		0x0100000,		///< Reaches all nodes in the scope, e.g. ff0X::1
	mcast_all_routers=		0x0200000,		///< Reaches all routers in the scope, e.g. ff0X::2

	unicast_special_mask=	0xf0000000,		///< Some special unicast addresses.
	unique_local_address =	0x10000000,		///< Address which can be used freely within a site: e.g. fd00::/8
};

/** Ipv6 internet address.

	An ipv4 address can be embedded in ipv6 for example, with ipv4/ipv6
	
	Example:

	  
	  ::ffff:192.168.1.2
	  
	
	The default address is any host (all zeroes or ::)
*/
class InetAddr : public SockaddrBase
{
	sockaddr_in6* m_addr;

	void init();

public:
	/**
		ipv6 internet address, initialized with "any" address
	*/
	InetAddr();
	/**
		ipv6 internet address, with specified port (address set to any).
	*/
	InetAddr(unsigned);
	/**
		ipv6 internet address, with specified host address and port.
	*/
	InetAddr(const std::string&, unsigned);
	/**
		ipv6 internet address, initialized with socket address.
	*/
	InetAddr(const sockaddr*);
	/**
		ipv6 internet address, copy constructed.
	*/
	InetAddr(const InetAddr&);
	/**
		ipv6 internet address, copy assigned.
	*/
	InetAddr& operator=(const InetAddr&);
	/**
		ipv6 internet address, move constructed. The original address is set to any.
	*/
	InetAddr(InetAddr&&);
	/**
		ipv6 internet address, move assigned. The original address is set to any.
	*/
	InetAddr& operator=(InetAddr&&);
	/**
		ipv6 internet address destructor.
	*/
	virtual ~InetAddr();

	/**
		Socket address const pointer.
	*/
	virtual operator const sockaddr*() const;
	/**
		Socket address pointer.
	*/
	virtual operator sockaddr*();
	/**
		Socket address length in bytes.
	*/
	virtual unsigned len() const;
	/**
		Readable address string.
	*/
	virtual std::string str() const;
	/**
		Get host.
	*/
	virtual std::string host() const;

	/**
		IPv6 socket address pointer.
	*/
	operator const sockaddr_in6*() const { return m_addr; }

	/**
		Set to "any" host address ::
	*/
	void any_host();
	/**
		Set to local (loopback) address ::1
	*/
	void local_host();
	/**
		Set the host address.

		Any host ::
		Local host ::1
		ipv4 host ::ffff:192.168.1.2
	*/
	void host(const std::string&);
	/**
		Get the port.
	*/
	unsigned port() const;
	/**
		Set the port.
	*/
	void port(unsigned);
	/**
		Set the scope id of the address.

		Used for "scoped" addresses:
			for link-local (fe80::/10), the scope_id is the interface index (see net::Network_if).
			for site-local (fec0::/10), the scope_id is the site identifier.

		If the scope_id is not set for a scoped address, the routing table may be used to find an address.

		In general link-local unicast (fe80::/64) or link-scope multicast (ff00::/8) may require scope_id to
		route correctly.
	*/
	void scope_id(uint32_t);
	/**
		Get the scope id of the address.
	*/
	uint32_t scope_id() const;

	/**
		Return the address flags.

		The address flags cover commonly used IPv6 address cases and are not exhaustive. \see InetAddrFlag for covered cases.
	*/
	int flags() const;
	/**
		Test if flags are set in this address.
	*/
	bool test_flags(int f) const
	{
		return (flags() & f) == f;
	}
};

/** Internet transmission control protocol (tcp) socket.

	Allows reliable, error-checked delivery of a stream of bytes between two connected peers.
*/
class InetTcpSock : public TcpSocket
{
protected:
	explicit InetTcpSock(int fd) : TcpSocket(fd) {}

public:
	/**
		Create an IPv6 stream socket.
	*/
	InetTcpSock();
	
	/**
		Close the connection and reset the socket.
	*/
	virtual void reset();
	/**
		Get the socket address.
	*/
	InetAddr get_addr();

	/**
		Accept a connection from an anonymous peer.

		Returns a new socket connected to the peer, or throws an exception on error.

		The socket can be polled for Poller::PollFlag::input events, to wait for incoming connections without blocking.
	*/
	InetTcpSock accept();
	std::shared_ptr<InetTcpSock> accept_shared();
	/**
		Accept a connection.

		Returns a new socket connected to the peer and sets the peer address, or throws an exception on error.

		The socket can be polled for Poller::PollFlag::input events, to wait for incoming connections without blocking.
	*/
	InetTcpSock accept(InetAddr&);
	std::shared_ptr<InetTcpSock> accept_shared(InetAddr&);
};

/** Internet user datagram protocol (udp) socket.

	Allows connectionless delivery of messages (datagrams), with a checksum for message integrity, but no guarantee of message ordering or delivery.
*/
class InetUdpSock : public UdpSocket
{
public:
	/**
		Create an IPv6 datagram socket.
	*/
	InetUdpSock();

	/**
		Reset the socket.
	*/
	virtual void reset();

	/**
		Get the socket address.
	*/
	InetAddr get_addr();

	/**
		Join a multicast group.

		Joins a group on the network interface. \see net::Interfaces
		
		All-nodes groups (e.g. ff01::1 and ff02::1) are automatically joined.

		ipv4 mapped multicast addresses are not supported.
	*/
	void mcast_join_group(const InetAddr&, unsigned = 0);
	/**
		Leave a multicast group.
	*/
	void mcast_leave_group(const InetAddr&, unsigned = 0);
	/**
		Set the default interface for outgoing multicast messages.

		Sets the default interface index. \see net::Interfaces

		If interface is 0, removes the default (and lets the system choose an interface).
	*/
	void mcast_interface(unsigned = 0);
	/**
		Enable or disable multicast loopback.

		If enabled, multicast messages sent by the interface will be recieved on the interface.
		
		Default is enabled.
	*/
	void mcast_loopback(bool = true);
	/**
		Hop limit for outgoing multicast messages.

		This is number of times the message will be passed between routers for outgoing traffic.

		The default is 1 (the local subnet router only).
	*/
	void mcast_hops(unsigned = 1);
};

/** @} */
/** @} */

}	// namespace

/** Print the socket address details to an output stream.
*/
std::ostream& operator<<(std::ostream&, const scc::net::InetAddr&);

#endif
