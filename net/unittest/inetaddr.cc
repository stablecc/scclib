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
#include <gtest/gtest.h>
#include <iostream>

/** \addtogroup net_inet
	@{
*/
/** Test file for \ref net_inet
	\file
*/
/** \example scclib/net/unittest/sockaddr.cc
	
	Test for internet networking addresses.
*/
/** @} */

using std::cout;
using std::endl;
using scc::net::InetAddr;
using scc::net::InetAddrFlag;

TEST(inetaddr_test, Bad_addr)
{
	InetAddr sa;
	ASSERT_ANY_THROW(sa.host("deadbeef"));
}

TEST(inetaddr_test, init_addr)
{
	InetAddr sa;
	cout << sa << endl;
	ASSERT_EQ(sa.host(), "::");
	ASSERT_EQ(sa.port(), 0);
	ASSERT_EQ(sa.scope_id(), 0);
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6));
	ASSERT_FALSE(sa.test_flags(InetAddrFlag::ipv4));
}

TEST(inetaddr_test, any_addrs)
{
	InetAddr sa;
	sa.host("::ffff:0.0.0.0");
	cout << sa << endl;
	ASSERT_EQ(sa.host(), "::ffff:0.0.0.0");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv4|InetAddrFlag::any));

	sa = InetAddr();
	sa.host("::");
	cout << sa << endl;
	ASSERT_EQ(sa.host(), "::");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::any));
}

TEST(inetaddr_test, loop_addrs)
{
	InetAddr sa;
	sa.host("::ffff:127.0.0.1");
	cout << sa << endl;
	ASSERT_EQ(sa.host(), "::ffff:127.0.0.1");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv4|InetAddrFlag::loopback));

	sa = InetAddr();
	sa.host("::1");
	cout << sa << endl;
	ASSERT_EQ(sa.host(), "::1");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::loopback));
}

TEST(inetaddr_test, unicast_global)
{
	InetAddr sa;
	sa.host("::ffff:192.168.12.24");
	cout << sa << endl;
	ASSERT_EQ(sa.host(), "::ffff:192.168.12.24");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv4|InetAddrFlag::unicast|InetAddrFlag::global));

	sa = InetAddr();
	sa.host("dead::beef:feed");
	cout << sa << endl;
	ASSERT_EQ(sa.host(), "dead::beef:feed");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::unicast|InetAddrFlag::global));
}

TEST(inetaddr_test, unicast_link)
{
	InetAddr sa;
	sa.host("fe80::dead:beef");
	cout << sa << endl;
	ASSERT_EQ(sa.host(),"fe80::dead:beef");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::unicast|InetAddrFlag::link_local));
}

TEST(inetaddr_test, unicast_site)
{
	InetAddr sa;
	sa.host("fd00::dead:beef");
	cout << sa << endl;
	ASSERT_EQ(sa.host(),"fd00::dead:beef");
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::unicast|InetAddrFlag::global|InetAddrFlag::unique_local_address));
}


TEST(inetaddr_test, multicast_ipv4)
{
	InetAddr sa;
	sa.host("::ffff:224.1.2.3");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv4|InetAddrFlag::multicast|InetAddrFlag::global));

	sa = InetAddr();
	sa.host("::ffff:239.1.2.3");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv4|InetAddrFlag::multicast|InetAddrFlag::global));

	sa = InetAddr();
	sa.host("::ffff:223.1.2.3");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv4|InetAddrFlag::unicast|InetAddrFlag::global));

	sa = InetAddr();
	sa.host("::ffff:240.1.2.3");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv4|InetAddrFlag::unicast|InetAddrFlag::global));
}

TEST(inetaddr_test, multicast_ipv6)
{
	InetAddr sa;
	sa.host("ff01::1");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::multicast|InetAddrFlag::if_local|InetAddrFlag::mcast_all_nodes));

	sa = InetAddr();
	sa.host("ff02::1");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::multicast|InetAddrFlag::link_local|InetAddrFlag::mcast_all_nodes));

	sa = InetAddr();
	sa.host("ff01::2");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::multicast|InetAddrFlag::if_local|InetAddrFlag::mcast_all_routers));

	sa = InetAddr();
	sa.host("ff02::2");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::multicast|InetAddrFlag::link_local|InetAddrFlag::mcast_all_routers));

	sa = InetAddr();
	sa.host("ff05::2");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::multicast|InetAddrFlag::site_local|InetAddrFlag::mcast_all_routers));

	sa = InetAddr();
	sa.host("ff18::dead:beef");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::multicast|InetAddrFlag::org_local|InetAddrFlag::mcast_dynamic));

	sa.host("ff03::dead:beef");
	cout << sa << endl;
	ASSERT_TRUE(sa.test_flags(InetAddrFlag::ipv6|InetAddrFlag::multicast|InetAddrFlag::realm_local));
}
