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
#include "net/net_if.h"
#include <cassert>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <fstream>
#include <sys/types.h>
#include <netdb.h>
#include "util/filedesc.h"
#include "net/inet.h"

using namespace scc::net;

std::vector<InetAddr> NetIf::host_addrs(const std::string& name, NetIf::SocketType type)
{
	std::vector<InetAddr> ret_vect;

	addrinfo* ai = nullptr;
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_INET6;

	if (type == NetIf::SocketType::tcp_stream)		hints.ai_socktype |= SOCK_STREAM;
	if (type == NetIf::SocketType::udp_datagram)	hints.ai_socktype |= SOCK_DGRAM;

	hints.ai_flags = AI_V4MAPPED|AI_ADDRCONFIG;
	
	int res = getaddrinfo(name.c_str(), nullptr, &hints, &ai);

	if (res != 0)
	{
		freeaddrinfo(ai);
		return ret_vect;
	}

	addrinfo* cur = ai;
	while (cur)
	{
		ret_vect.emplace_back(cur->ai_addr);
		cur = cur->ai_next;
	}

	freeaddrinfo(ai);

	return ret_vect;
}

/** String representation of a network interface address.
*/
std::string NetIfAddr::str() const
{
	std::stringstream s;
	s << InetAddr::str() << " name: " << m_name;
	return s.str();
}

std::ostream& operator<<(std::ostream& os, const scc::net::NetIfAddr& nad)
{
	return os.write(nad.str().c_str(), nad.str().size());
}

std::vector<NetIf> NetIf::all_interfaces()
{
	std::map<int, NetIf> xref;

	ifaddrs* ifs;
	int ret;
	do
	{
		ret = getifaddrs(&ifs);
	}
	while (ret == -1 && errno == EINTR);

	if (ret == -1)
	{
		throw std::system_error(errno, std::system_category(), "getifaddrs()");
	}

	for (ifaddrs* ad = ifs; ad != nullptr; ad = ad->ifa_next)
	{
		sockaddr* sa = ad->ifa_addr;	// get the address
		if (sa == nullptr)
			continue;

		if (sa->sa_family == AF_PACKET)		// this is a packet device, it will always come before the associated addresses
		{

			sockaddr_ll* x = (sockaddr_ll*)sa;		// this is the mac address (low level address)
			std::stringstream hwad;
			for (int i = 0; i < 6; i++)
			{
				hwad << std::setw(2) << std::setfill('0') << std::hex;
				hwad << (int)x->sll_addr[i];
				hwad << std::setw(0) << std::setfill(' ') << std::dec;
				if (i != 5)
					hwad << ":";
			}

			NetIf nif(ad->ifa_name, hwad.str(), ad->ifa_flags);
			xref[nif.index()] = std::move(nif);
		}
		else if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)	// these are network addresses associated with packet device
		{
			int idx;
			do
			{
				idx = if_nametoindex(ad->ifa_name);
			}
			while (idx == 0 && errno == EINTR);
			assert(idx);
			assert(xref.find(idx) != xref.end());

			NetIfAddr na(ad->ifa_name, InetAddr(sa));

			xref[idx].m_addrs.push_back(na);
		}
	}

	freeifaddrs(ifs);

	std::vector<NetIf> if_ret;

	// return in index order
	for (auto& i : xref)
	{
		if_ret.push_back(std::move(i.second));
	}

	return if_ret;
}

int NetIf::parse_flags(unsigned flags)
{
	int ret = 0;

	if ((flags&IFF_UP)==IFF_UP)						{ ret |= NetIfFlag::if_up; }
	if ((flags&IFF_LOOPBACK)==IFF_LOOPBACK)			{ ret |= NetIfFlag::if_loopback; }
	if ((flags&IFF_POINTOPOINT)==IFF_POINTOPOINT)	{ ret |= NetIfFlag::if_pointtopoint; }
	if ((flags&IFF_RUNNING)==IFF_RUNNING)			{ ret |= NetIfFlag::if_running; }
	if ((flags&IFF_BROADCAST)==IFF_BROADCAST)		{ ret |= NetIfFlag::if_broadcast; }
	if ((flags&IFF_NOARP)==IFF_NOARP)				{ ret |= NetIfFlag::if_noarp; }
	if ((flags&IFF_PROMISC)==IFF_PROMISC)			{ ret |= NetIfFlag::if_promisc; }
	if ((flags&IFF_ALLMULTI)==IFF_ALLMULTI)			{ ret |= NetIfFlag::if_allmulti; }
	if ((flags&IFF_MULTICAST)==IFF_MULTICAST)		{ ret |= NetIfFlag::if_multicast; }
	if ((flags&IFF_DYNAMIC)==IFF_DYNAMIC)			{ ret |= NetIfFlag::if_dynamic; }
// IFF_ECHO is not available on all kernel versions
#ifdef IFF_ECHO
	if ((flags&IFF_ECHO)==IFF_ECHO)					{ ret |= NetIfFlag::if_echo; }
#endif
	return ret;
}

NetIf::NetIf(const std::string& name, const std::string& hwaddr, int flags)
	: m_name(name), m_hwaddr(hwaddr), m_index(find_index(name)), m_speed(0), m_mtu(0), m_flags(parse_flags(flags))
{
	try
	{
		std::string b("/sys/class/net/");
		auto f = std::fstream(b+m_name+"/mtu", std::ios_base::in);
		f >> m_mtu;
		f.close();
		if (m_name != "lo")
		{
			f = std::fstream(b+m_name+"/speed", std::ios_base::in);
			f >> m_speed;
			m_speed /= 8;	// bits/char
			m_speed *= 1e6;	// mega
			f.close();
		}
	} catch (...) {}	// allow exceptions on optional info
}

int NetIf::find_index(const std::string& name)
{
	assert(name.size());
	int sock = ::socket(AF_ROUTE, SOCK_RAW, 0);	   // get the interface index by most portable method
	assert(sock != -1);
	scc::util::FileDesc fdsock(sock);		// auto close wrapper

	ifreq ifr;									  // from man netdevice
	strcpy(ifr.ifr_name, name.c_str());
	int ret;
	do
	{
		ret = ::ioctl(sock, SIOCGIFINDEX, &ifr);
	}
	while (ret == -1 && errno == EINTR);
	assert(ret != -1);
	return ifr.ifr_ifindex;
}

/** String representation of network interface. Lists all associated addresses.
*/
std::string NetIf::str() const
{
	std::stringstream s;
	s << index() << " " << name() << " hwaddr: " << hw_addr() << " speed: " << speed() << " mtu: " << mtu() << std::endl;
	s << "    flags: ";
	bool sp = false;
	if ((m_flags&if_up)==if_up)						{ s << "up"; sp = true; }
	if ((m_flags&if_broadcast)==if_broadcast)		{ if (sp) s << " "; s << "broadcast"; sp = true; }
	if ((m_flags&if_loopback)==if_loopback)			{ if (sp) s << " "; s << "loopback"; sp = true; }
	if ((m_flags&if_pointtopoint)==if_pointtopoint)	{ if (sp) s << " "; s << "point2point"; sp = true; }
	if ((m_flags&if_multicast)==if_multicast)		{ if (sp) s << " "; s << "multicast"; sp = true; }
	if ((m_flags&if_dynamic)==if_dynamic)			{ if (sp) s << " "; s << "dynamic"; sp = true; }
	if ((m_flags&if_running)==if_running)			{ if (sp) s << " "; s << "running"; sp = true; }
	if ((m_flags&if_noarp)==if_noarp)				{ if (sp) s << " "; s << "noarp"; sp = true; }
	if ((m_flags&if_promisc)==if_promisc)			{ if (sp) s << " "; s << "promisc"; sp = true; }
	if ((m_flags&if_allmulti)==if_allmulti)			{ if (sp) s << " "; s << "allmulti"; sp = true; }
	if ((m_flags&if_echo)==if_echo)					{ if (sp) s << " "; s << "echo"; }

	for (auto& ad : m_addrs)
	{
		s << "\n    " << ad;
	}
	return s.str();
}

std::ostream& operator<<(std::ostream& os, const scc::net::NetIf& nif)
{
	return os.write(nif.str().c_str(), nif.str().size());
}
