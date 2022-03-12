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
#ifndef _SCC_NET_IF_H
#define _SCC_NET_IF_H

#include <vector>
#include <string>
#include <net/inet.h>

namespace scc::net {

/** \addtogroup net
	@{
*/

/** \defgroup net_if Internet network interface utility
	@{

	Scans network interfaces and enumerates address types.
*/

/** Internet network interface utility.
	\file
*/

/** Named address within an interface.
*/
class NetIfAddr : public InetAddr
{
	std::string m_name;
public:
	NetIfAddr(const std::string& name, const InetAddr& addr) : InetAddr(addr), m_name(name) {}

	/** The interface address name. May be a subclass of the interface name. */
	const std::string& if_addr_name() const { return m_name; }

	std::string str() const;
};

/** Interface flags.
*/
enum NetIfFlag
{
	if_up=				0x001,  ///< Interface is running.
	if_broadcast=		0x002,  ///< Valid broadcast address set.
	if_loopback=		0x004,  ///< Interface is a loopback interface.
	if_pointtopoint=	0x008,  ///< Interface is a point-to-point link.
	if_running=			0x010,  ///< Resources allocated.
	if_noarp=			0x020,  ///< No arp protocol, L2 destination address not set.
	if_promisc=			0x040,  ///< Interface is in promiscuous mode.
	if_allmulti=		0x080,  ///< Receives all multicast packets.
	if_multicast=		0x100,  ///< Supports multicast.
	if_dynamic=			0x200,  ///< The addresses are lost when the interface goes down.
	if_echo=			0x400   ///< Echoes sent packets.
};

/** A network interface.

	Contains zero or more internet addresses.
*/
class NetIf
{
	std::string m_name;
	std::string m_hwaddr;
	int m_index;
	size_t m_speed;
	size_t m_mtu;
	int m_flags;
	std::vector<NetIfAddr> m_addrs;

	static int parse_flags(unsigned);
	static int find_index(const std::string&);

	NetIf(const std::string&, const std::string&, int);
	friend class NetIfIterator;

public:
	NetIf() {}

	/** List network interfaces on the local system.
	*/
	static std::vector<NetIf> all_interfaces();
	
	enum class SocketType
	{
		any,
		tcp_stream,
		udp_datagram,
	};

	/** List addresses for the specified host (do a name lookup).
	
		Returns a list of host addresses to that can used with bind() or connect().

		Can specify a filter for a specific capability (tcp or udp).

		This will carry out a name resolution and return the list of addresses that map to the host name.

		If both ipv4 and ipv6 are specified, ipv6 addresses are preferred.
	*/
	static std::vector<InetAddr> host_addrs(const std::string& name, SocketType=SocketType::any);

	/** Name of the interface. */
	const std::string& name() const { return m_name; }

	/** The interface index. */
	const int index() const { return m_index; }

	/** Hardware address for the interface. */
	const std::string& hw_addr() const { return m_hwaddr; }

	/** Link speed in bytes / second, 0 means loopback. */
	const size_t speed() const { return m_speed; }

	/** Maximum transmission unit. */
	const size_t mtu() const { return m_mtu; }

	/**	Flags for this interface.
		\returns NetIfFlag bit mask.
	*/
	int flags() const { return m_flags; }
	bool test_flags(int f) const { return (m_flags&f)==f; }

	/** Zero or more addresses associated with this interface. */
	const std::vector<NetIfAddr>& addrs() const { return m_addrs; }

	std::string str() const;
};

/** @} */
/** @} */

}	// namespace

std::ostream& operator<<(std::ostream&, const scc::net::NetIf&);
std::ostream& operator<<(std::ostream&, const scc::net::NetIfAddr&);

#endif
