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
#include <system_error>
#include <thread>
#include <future>
#include <sstream>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include "util/event.h"
#include "util/poller.h"
#include "net/inet.h"
#include "net/unix.h"
#include "net/net_if.h"
#include "util/iostream.h"
#include "util/logger.h"

/** \addtogroup net_inet
	@{
*/
/** Test file for \ref net_inet
	\file
*/
/** \example scclib/net/unittest/inet.cc
	
	Test for internet networking.
*/
/** @} */

using std::cout;
using std::endl;
using std::string;
using std::unique_lock;
using std::lock_guard;
using std::mutex;
using std::exception;
using std::condition_variable;
using std::async;
using std::stringstream;
using std::error_code;
using std::to_string;
using scc::util::IoStream;
using scc::util::Logger;
using scc::util::Event;
using scc::util::Poller;
using scc::net::NetIfFlag;
using scc::net::InetAddrFlag;
using scc::net::InetAddr;
using scc::net::InetTcpSock;
using scc::net::InetUdpSock;

#ifndef GTEST_SKIP
#define GTEST_SKIP(dum) return
#endif

// look for an address on a network interface that satisfies InetAddrFlag input flag
// Set the address if provided, and return the interface number
static int find_interface(int flag, InetAddr* ad = nullptr)
{
	for (auto& i : scc::net::NetIf::all_interfaces())
	{
		//cout << "if=" << i << endl;
		if (!i.test_flags(NetIfFlag::if_up))		continue;

		for (auto& j : i.addrs())
		{
			//cout << "  addr=" << i << endl;
			if ((j.test_flags(flag)))
			{
				if (ad)
				{
					*ad = j;
				}
				return i.index();
			}
		}
	}
	return 0;
}

struct inet_networking : public testing::Test
{
	std::string snd, got, testgot;
	scc::util::Logger log;
	InetAddr server_addr, client_addr, servfrom;
	scc::util::Event startev, quitev;
	InetTcpSock tcp_serv_sock, tcp_client_sock;
	InetUdpSock udp_serv_sock, udp_client_sock;

	inet_networking() : snd("this is a test line\nthis is the second\nanother\n\nlast one")
	{
		Logger log;
		log.add_cout();
		log.id("test");
	}
	virtual ~inet_networking()
	{
	}
	void reset()
	{
		tcp_serv_sock.reset();
		udp_serv_sock.reset();
		tcp_client_sock.reset();
		udp_client_sock.reset();
		startev.reset();
		quitev.reset();
	}

	void udp_readwrite_client()
	{
		auto fut = async(udp_readwrite_server, std::ref(startev), std::ref(udp_serv_sock), std::ref(server_addr), std::ref(servfrom), std::ref(quitev));

		startev.read();	  // wait for the server to listen

		log << "started" << endl;

		udp_writeread_test(udp_client_sock, client_addr);

		quitev.write(1);		// signal server to quit

		int res = fut.get();
		log << "return code: " << res << endl;
		ASSERT_EQ(res, 0);
	};

	void tcp_iostream_client()
	{
		auto fut = async(tcp_iostream_server, std::ref(startev), std::ref(tcp_serv_sock), std::ref(server_addr), std::ref(servfrom));

		startev.read();	  // wait for the server to listen

		log << "connect" << endl;
		tcp_client_sock.connect(server_addr);

		log << "connected to addr=" << server_addr.str() << endl;

		tcp_stream_test(tcp_client_sock);

		tcp_client_sock.close();		// closed socket will stop the server

		int res = fut.get();
		log << "return code: " << res << endl;
		ASSERT_EQ(res, 0);
	}

	void tcp_readwrite_client()
	{
		auto fut = async(tcp_readwrite_server, std::ref(startev), std::ref(tcp_serv_sock), std::ref(server_addr), std::ref(servfrom));

		log << "read start event" << endl;
		startev.read();	  // wait for the server to listen

		InetTcpSock sock;
		log << "connect" << endl;
		tcp_client_sock.connect(server_addr);

		log << "connected to addr=" << server_addr.str() << endl;

		tcp_writeread_test(tcp_client_sock);

		tcp_client_sock.close();
		int res = fut.get();
		log << "return code: " << res << endl;
		ASSERT_EQ(res, 0);
	}

	void tcp_stream_test(InetTcpSock& sock)
	{
		Logger log;
		log.add_cout();
		log.id("inet tcp stream client");

		IoStream stream(sock, sock);
		stringstream testst(snd);

		// first send and recieve using getline

		log << "sending " << snd.size() << endl;
		ASSERT_TRUE(stream << snd << endl);
		stream.flush();

		while (std::getline(testst, testgot))
		{
			ASSERT_TRUE(std::getline(stream, got));
			log << "got reply " << got.size() << endl;
			ASSERT_EQ(got, testgot);
		}

		// now send and receive using stream operators

		ASSERT_TRUE(stream << snd << endl);
		stream.flush();

		stringstream testst2(snd);
		while (testst2 >> testgot)
		{
			ASSERT_TRUE(stream >> got);
			log << "got reply " << got.size() << endl;
			ASSERT_EQ(got, testgot);
		}

		log << "end" << endl;
	}

	void tcp_writeread_test(InetTcpSock& sock)
	{
		Logger log;
		log.add_cout();
		log.id("inet tcp readwrite client");

		log << "sending " << snd.size() << endl;
		ASSERT_EQ(sock.send(snd.data(), snd.size()), snd.size());

		char buf[snd.size()];
		size_t sz = 0, got;
		do
		{
			got = sock.recv(buf+sz, snd.size()-sz);
			log << "got reply " << got << endl;
			ASSERT_GT(got, 0);
			sz += got;
		}
		while (sz < snd.size());
		ASSERT_EQ(sz, snd.size());
		ASSERT_EQ(memcmp(buf, snd.data(), sz), 0);

		log << "end" << endl;
	}

	void udp_writeread_test(InetUdpSock& sock, InetAddr& addr)
	{
		Logger log;
		log.add_cout();
		log.id("inet udp client");

		log << "sending " << snd.size() << " to " << addr.str() << endl;
		ASSERT_EQ(sock.send(snd.data(), snd.size(), addr), snd.size());

		char buf[snd.size()];
		size_t got;
		InetAddr from;
		got = sock.recv(buf, snd.size(), from);
		log << "got reply " << got << " from " << from.host() << endl;
		ASSERT_EQ(got, snd.size());
		ASSERT_EQ(memcmp(buf, snd.data(), snd.size()), 0);

		log << "end" << endl;
	}

	static int udp_readwrite_server(Event& startev, InetUdpSock& sock, InetAddr& addr, InetAddr& from, Event& quitev)
	{
		Logger log;
		log.add_cout();
		log.id("inet udp server");

		try
		{
			log << "reuse_addr" << endl;
			sock.reuse_addr(true);
			log << "bind to " << addr.host() << endl;
			sock.bind(addr);

			startev.write(1);		// signal the other thread it can start

			// now poll to see when we have new data (or are signalled to quit)
			Poller pin;
			pin.set(sock.fd(), Poller::input);
			pin.set(quitev.fd(), Poller::input);

			while (1)
			{
				pin.wait();
				if (pin.event(quitev.fd()))
				{
					startev.write(1);	// no blocking allowed
					return 0;
				}
				else if (pin.event(sock.fd()))
				{
					auto sz = sock.recv_next();	 // how many bytes are waiting?
					char buf[sz];
					sock.recv(buf, sz, from);
					log << "got " << sz << " from " << from.host() << endl;
					sock.send(buf, sz, from);
				}
			}
		}
		catch (const std::exception& e)
		{
			log << "exception: " << e.what() << endl;
			startev.write(1);	// no blocking allowed
			return errno;
		}
		startev.write(1);	// no blocking allowed
		return 0;
	}

	static int tcp_iostream_server(Event& startev, InetTcpSock& sock, InetAddr& addr, InetAddr& from)
	{
		Logger log;
		log.add_cout();
		log.id("inet tcp stream server");

		try
		{
			log << "reuse_addr" << endl;
			sock.reuse_addr(true);
			log << "bind to " << addr.host() << endl;
			sock.bind(addr);
			log << "listen" << endl;
			sock.listen();

			startev.write(1);		// signal the other thread it can start

			auto conn = sock.accept(from);
			log << "connect from " << from << endl;

			IoStream stream(conn, conn);

			for (string got; std::getline(stream, got);)	 // loop until eof (socket closed)
			{
				log << "got " << got.size() << endl;
				stream << got << endl;
			}

		}
		catch (const std::exception& e)
		{
			log << "exception: " << e.what() << endl;
			startev.write(1);	// no blocking allowed
			return errno;
		}
		startev.write(1);	// no blocking allowed
		return 0;
	}

	static int tcp_readwrite_server(Event& startev, InetTcpSock& sock, InetAddr& addr, InetAddr& from)
	{
		Logger log;
		log.add_cout();
		log.id("inet tcp readwrite server");

		try
		{
			log << "reuse_addr" << endl;
			sock.reuse_addr(true);
			log << "bind to " << addr.host() << endl;
			sock.bind(addr);
			log << "listen" << endl;
			sock.listen();

			startev.write(1);		// signal the other thread it can start

			auto conn = sock.accept(from);
			log << "connect from " << from << endl;

			char buf[16];
			int sz;
			while ((sz = conn.recv(buf, 16)) > 0)		// loop until we get 0 (socket closed)
			{
				log << "got " << sz << endl;
				conn.send(buf, sz);
			}
		}
		catch (const std::exception& e)
		{
			log << "exception: " << e.what() << endl;
			startev.write(1);							// make sure we don't block
			return errno;
		}
		startev.write(1);
		return 0;
	}
};

TEST_F(inet_networking, udp_readwrite_loopback)
{
	server_addr.host("::1");
	server_addr.port(9999);

	client_addr.host("::1");
	client_addr.port(9999);

	udp_readwrite_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());			// the port will be ephemeral, but the address will be the same

	reset();

	server_addr.host("::ffff:127.0.0.1");
	server_addr.port(9999);

	client_addr.host("::ffff:127.0.0.1");
	client_addr.port(9999);

	udp_readwrite_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());
}

TEST_F(inet_networking, udp_readwrite_unicast)
{
	// look for an ipv4 global interface (not the loopback)
	ASSERT_GT(find_interface(InetAddrFlag::unicast|InetAddrFlag::ipv4|InetAddrFlag::global, &server_addr), 0);
	server_addr.port(9999);

	cout << "ipv4 server addr: " << server_addr << endl;

	client_addr.host(server_addr.host());
	client_addr.port(9999);

	udp_readwrite_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());

	reset();

	// look for interface with link local ipv6 address (any interface with ipv6 enabled)
	int inum = find_interface(InetAddrFlag::unicast|InetAddrFlag::ipv6|InetAddrFlag::link_local, &server_addr);
	
	if (!inum)
	{
		cout << "NOTE: no interface local address found, skipping" << endl;
		GTEST_SKIP();
	}

	cout << "ipv6 link local server addr: " << server_addr << endl;

	server_addr.port(9999);

	client_addr.host(server_addr.host());
	client_addr.port(9999);

	udp_readwrite_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());

	reset();

	// look for interface with global ipv6 address
	inum = find_interface(InetAddrFlag::unicast|InetAddrFlag::ipv6|InetAddrFlag::global, &server_addr);

	if (!inum)
	{
		cout << "NOTE: no interface local address found, skipping" << endl;
		GTEST_SKIP();
	}

	cout << "ipv6 global server addr: " << server_addr << endl;

	server_addr.port(9999);

	client_addr.host(server_addr.host());
	client_addr.port(9999);

	udp_readwrite_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());
}

TEST_F(inet_networking, tcp_iostream_loopback)
{
	server_addr.host("::1");
	server_addr.port(9999);

	tcp_iostream_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());

	reset();

	server_addr.host("::ffff:127.0.0.1");
	server_addr.port(9999);

	tcp_iostream_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());
}

TEST_F(inet_networking, tcp_readwrite_loopback)
{
	server_addr.host("::1");
	server_addr.port(9999);

	tcp_readwrite_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());

	reset();

	server_addr.host("::ffff:127.0.0.1");
	server_addr.port(9999);

	tcp_readwrite_client();
	log << "server connect from " << servfrom << endl;
	ASSERT_EQ(server_addr.host(), servfrom.host());
}

/// [Inet client server]
TEST(inet_example, client_server_stream_test)
{
	Logger log;
	log.add_cout();
	log.id("tcp client");

	// this test sets up a client and server & streams some data between them

	scc::net::InetAddr addr;
	addr.host("::1");			// localhost
	addr.port(9999);

	// listening event
	scc::util::Event listening;

	std::packaged_task<int(void)> serv_task([&addr, &listening]()
	{
		Logger log;
		log.add_cout();
		log.id("tcp server");

		try
		{
			scc::net::InetTcpSock sock;
			sock.reuse_addr(true);

			sock.bind(addr);
			log << "server listening" << endl;
			sock.listen();

			// can play around with timeouts and timing this way...
			//log << "sleep for a second" << endl;
			//std::this_thread::sleep_for(std::chrono::seconds(1));

			log << "signalling client" << endl;
			listening.write(1);

			scc::net::InetAddr from;
			auto conn = sock.accept(from);
			log << "connection from " << from << endl;

			scc::util::IoStream stream(conn, conn);

			int count = 0;
			for (string got; std::getline(stream, got);)	 // loop until eof (socket closed)
			{
				log << "got " << got << endl;
				count += got.size();
				stream << got << endl;
			}
			
			return count;
		}
		catch (exception& ex)
		{
			log << ex.what() << endl;
			listening.write(1);
			return -1;
		}
	});

	auto fut = serv_task.get_future();
	std::thread serv(std::move(serv_task));			// start server thread
 
	Poller pin;
	pin.set(listening, Poller::input);

	while (1)
	{
		pin.wait(std::chrono::milliseconds(700));
		if (pin.event(listening))
		{
			break;
		}
		log << "waiting for listener" << endl;
	}

	listening.read();
	
	scc::net::InetTcpSock sock;

	// use a non-blocking approach to add a timeout to the connect
	sock.non_blocking(true);
	error_code ec;
	sock.connect(addr, ec);
	if (ec.value() != 0 && ec.value() != EINPROGRESS)
	{
		log << "Non blocking connect failed: " << ec.message() << endl;
		ASSERT_EQ(ec.value(), 0);
	}
	
	log << "waiting for 200 ms seconds to connect" << endl;
	Poller pout;
	pout.set(sock, Poller::output);
	pout.wait(std::chrono::milliseconds(200));

	if (!pout.event(sock))
	{
		log << "connect attempt timed out" << endl;
		ASSERT_EQ(pout.event(sock), 0);
	}

	sock.non_blocking(false);
	sock.connect(addr);			// connect will either throw immediately or connect

	log << "connected, sending stuff" << endl;

	string line1("first line");
	string line2("second line");

	scc::util::IoStream stream(sock, sock);

	string got;
	ASSERT_TRUE(stream << line1 << endl);
	ASSERT_TRUE(getline(stream, got));
	ASSERT_EQ(got, line1);
	ASSERT_TRUE(stream << line2 << endl);
	ASSERT_TRUE(getline(stream, got));
	ASSERT_EQ(got, line2);
	sock.close();

	serv.join();

	int res = fut.get();
	log << "got result=" << res << endl;
	ASSERT_EQ(res, line1.size()+line2.size());
}
/// [Inet client server]

/// [TCP multiserver]
TEST(inet_example, tcp_multiserver)
{
	Logger log;
	log.add_cout();
	log.id("tcp client");

	// this test sets up a client and server & streams some data between them

	scc::net::InetAddr addr;
	addr.host("::1");			// localhost
	addr.port(9999);

	condition_variable cv;
	mutex mx;
	int listening = 0;			// how many threads are listening?
	Event shut;

	auto serv_task = [&addr, &mx, &listening, &cv, &shut](int i)
	{
		Logger log;
		log.add_cout();
		log.id(string("tcp server ")+to_string(i));

		try
		{
			scc::net::InetTcpSock sock;
			sock.reuse_addr(true);
			sock.reuse_port(true);

			/*
				Port and address reuse allows us to bind the same address to two distinct sockets and accept connections. This may
				provide better performance than spawning a new thread from a single acceptor, or competing to accept from a single listening socket.
			*/

			sock.bind(addr);
			log << "server listening" << endl;
			sock.listen();

			log << "telling the client I am listening" << endl;
			{
				lock_guard<mutex> lk(mx);
				listening++;
				cv.notify_one();
			}

			int count = 0;

			for (;;)
			{
				Poller p;
				p.set(sock, Poller::input);
				p.set(shut, Poller::input);
				p.wait();

				if (p.event(shut))
				{
					return count;
				}

				InetAddr from;
				auto conn = sock.accept(from);
				log << "connection from " << from << endl;

				IoStream stream(conn, conn);

				for (string got; std::getline(stream, got);)	 // loop until eof (socket closed)
				{
					log << "got size " << got.size() << " : " << got << endl;
					count += got.size();
					stream << got << endl;
				}
			}

			return count;
		}
		catch (exception& ex)
		{
			log << ex.what() << endl;
			lock_guard<mutex> lk(mx);				// just to make sure the client cannot block
			listening++;
			cv.notify_one();
			return 0;
		}
	};

	auto fut1 = async(serv_task, 1);
	auto fut2 = async(serv_task, 2);
	auto fut3 = async(serv_task, 3);
 
	unique_lock<mutex> lk(mx);
	cv.wait(lk, [&listening]
	{
		return listening == 3;			// wait for all the threads to wake up
	});
	
	int count = 0;

	for (int i = 0; i < 10; i++)
	{
		InetTcpSock sock;
		sock.connect(addr);
		log << "connected #" << i << ", sending stuff" << endl;

		string line1("first line");
		string line2("second line");

		IoStream stream(sock, sock);

		string got;
		ASSERT_TRUE(stream << line1 << endl);
		ASSERT_TRUE(getline(stream, got));
		ASSERT_EQ(got, line1);
		ASSERT_TRUE(stream << line2 << endl);
		ASSERT_TRUE(getline(stream, got));
		ASSERT_EQ(got, line2);
		count += line1.size()+line2.size();
	}

	shut.write(1);

	int res = fut1.get();
	res += fut2.get();
	res += fut3.get();
	log << "got result=" << res << endl;
	ASSERT_EQ(res, count);
}
/// [TCP multiserver]
