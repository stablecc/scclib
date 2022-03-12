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
#include <net/net_if.h>
#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <net/socket.h>

/** \addtogroup net_if
	@{ */
/** Tests for \ref net_if \file */
/** \example net/unittest/net_if.cc */
/** @} */

using std::cout;
using std::endl;
using std::string;
using std::vector;
using scc::net::NetIf;
using scc::net::NetIfAddr;
using scc::net::InetAddr;
using scc::net::InetAddrFlag;
using scc::net::NetIfFlag;

TEST(interfaces_test, Find_lo)
{
	auto ifs = NetIf::all_interfaces();
	auto i = std::find_if(ifs.begin(), ifs.end(), [](const auto& x) -> bool
	{
		return x.name() == "lo";
	});
	ASSERT_NE(i, ifs.end());
	ASSERT_EQ(i->index(), 1);
	ASSERT_EQ(i->hw_addr(), "00:00:00:00:00:00");
	cout << *i << endl;
}

TEST(interfaces_test, Find_lo_ipv4)
{
	auto ifs = NetIf::all_interfaces();
	auto i = std::find_if(ifs.begin(), ifs.end(), [](const auto& x) -> bool
	{
		return x.name() == "lo";
	});
	ASSERT_NE(i, ifs.end());
	auto sa = std::find_if(i->addrs().begin(), i->addrs().end(), [](const NetIfAddr& ad) -> bool
	{
		return ad.test_flags(InetAddrFlag::ipv4|InetAddrFlag::loopback);
	});
	ASSERT_NE(sa, i->addrs().end());
	cout << *i << endl;
}

TEST(interfaces_test, find_lo_ipv6)
{
	auto ifs = NetIf::all_interfaces();
	auto i = std::find_if(ifs.begin(), ifs.end(), [](const auto& x) -> bool
	{
		return x.name() == "lo";
	});
	ASSERT_NE(i, ifs.end());
	auto sa = std::find_if(i->addrs().begin(), i->addrs().end(), [](const NetIfAddr& ad) -> bool
	{
		return ad.test_flags(InetAddrFlag::ipv6|InetAddrFlag::loopback);
	});
	ASSERT_NE(sa, i->addrs().end());
	cout << *i << endl;
}

TEST(interfaces_test, find_unicast_ipv4)
{
	bool found = false;
	for (auto& i : NetIf::all_interfaces())
	{
		for (auto& j : i.addrs())
		{
			if ((j.test_flags(InetAddrFlag::ipv4|InetAddrFlag::unicast)))
			{
				found = true;
				cout << j << endl;
			}
		}
	}
	EXPECT_TRUE(found);
}

TEST(interfaces_test, find_unicast_ipv6)
{
	bool found = false;
	for (auto& i : NetIf::all_interfaces())
	{
		for (auto& j : i.addrs())
		{
			if ((j.test_flags(InetAddrFlag::ipv6|InetAddrFlag::unicast)))
			{
				found = true;
				cout << j << endl;
			}
		}
	}
	EXPECT_TRUE(found);
}

TEST(interfaces_test, iterate_addrs_test_local)
{
	auto ifs = NetIf::all_interfaces();
	ASSERT_TRUE(ifs.size() > 0);	// there is always a lo
	for (auto& i : ifs)
	{
		cout << i << endl;

		for (auto& j : i.addrs())
		{
			if ((j.test_flags(InetAddrFlag::link_local)))
			{
				cout << "*found link local address: " << j << endl;
				ASSERT_EQ(j.scope_id(), i.index());
			}
		}
	}
}

TEST(host_addrs, local_addrs)
{
	vector<string> hosts({
		"localhost",
		"127.0.0.1",
		"::1",
		"::"
	});

	for (auto& h : hosts)
	{
		auto v = NetIf::host_addrs(h);
		cout << "host " << h << " for all returned " << v.size() << " entries" << endl;
		for (auto& i : v)
		{
			cout << i << endl;
		}
		ASSERT_GT(v.size(), 0);
		v = NetIf::host_addrs(h, NetIf::SocketType::tcp_stream);
		cout << "host " << h << " for tcp returned " << v.size() << " entries" << endl;
		for (auto& i : v)
		{
			cout << i << endl;
		}
		ASSERT_GT(v.size(), 0);
		v = NetIf::host_addrs(h, NetIf::SocketType::udp_datagram);
		cout << "host " << h << " for udp returned " << v.size() << " entries" << endl;
		for (auto& i : v)
		{
			cout << i << endl;
		}
		ASSERT_GT(v.size(), 0);
	}
	auto ifs = NetIf::all_interfaces();
	for (auto& i : ifs)
	{
		if (i.test_flags(NetIfFlag::if_up|NetIfFlag::if_running))
		{
			cout << i << endl;
			for (auto& a : i.addrs())
			{
				auto v = NetIf::host_addrs(a.host());
				ASSERT_GT(v.size(), 0);
			}
		}
	}
}

