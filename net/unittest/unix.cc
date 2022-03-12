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
#include <net/unix.h>
#include <gtest/gtest.h>
#include <iostream>
#include <system_error>
#include <cerrno>
#include <thread>
#include <future>
#include <sstream>
#include <util/event.h>
#include <util/poller.h>
#include <net/inet.h>
#include <util/iostream.h>
#include <util/fs.h>
#include <util/logger.h>

/** \addtogroup net_unix
	@{ */
/** Tests for \ref net_unix \file */
/** \example net/unittest/unix.cc */
/** @} */

using std::cout;
using std::endl;
using std::string;
using std::async;
using std::stringstream;
using scc::util::IoStream;
using scc::util::Logger;
using scc::util::Event;
using scc::util::Poller;
using scc::net::UnixAddr;
using scc::net::UnixTcpSock;
using scc::net::UnixUdpSock;

std::string server_addrname("/tmp/server.sock");
std::string client_addrname("/tmp/client.sock");

struct unix_networking : public testing::Test
{
	std::string snd, got, testgot;
	scc::util::Logger log;
	UnixAddr server_addr, client_addr;
	scc::util::Event startev, quitev;

	unix_networking() : snd("this is a test line\nthis is the second\nanother\n\nlast one")
	{
		Logger log;
		log.add_cout();
		log.id("test");

		std::system_error err;
		scc::util::Filesystem::remove(server_addrname, &err);
		server_addr.host(server_addrname);
		scc::util::Filesystem::remove(client_addrname, &err);
		client_addr.host(client_addrname);
	}
	virtual ~unix_networking()
	{
		std::system_error err;
		scc::util::Filesystem::remove(server_addrname, &err);
		scc::util::Filesystem::remove(client_addrname, &err);
	}

	void tcp_stream_test(UnixTcpSock& sock)
	{
		Logger log;
		log.add_cout();
		log.id("unix tcp stream client");

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

		log << "tcp_stream_test: end" << endl;
	}

	void tcp_writeread_test(UnixTcpSock& sock)
	{
		Logger log;
		log.add_cout();
		log.id("unix tcp readwrite client");

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

	void udp_writeread_test(UnixUdpSock& sock, UnixAddr& addr)
	{
		Logger log;
		log.add_cout();
		log.id("unix udp client");

		log << "sending " << snd.size() << " to " << addr.str() << endl;
		ASSERT_EQ(sock.send(snd.data(), snd.size(), addr), snd.size());

		char buf[snd.size()];
		size_t got;
		UnixAddr from;
		got = sock.recv(buf, snd.size(), from);
		log << "got reply " << got << " from " << from.host() << endl;
		ASSERT_EQ(got, snd.size());
		ASSERT_EQ(memcmp(buf, snd.data(), snd.size()), 0);

		log << "end" << endl;
	}

	static int udp_readwrite_server(Event& startev, UnixUdpSock& sock, UnixAddr& addr, UnixAddr& from, Event& quitev)
	{
		Logger log;
		log.add_cout();
		log.id("unix udp server");

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

	static int tcp_iostream_server(Event& startev, UnixTcpSock& sock, UnixAddr& addr, UnixAddr& from)
	{
		Logger log;
		log.add_cout();
		log.id("unix tcp stream server");

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

	static int tcp_readwrite_server(Event& startev, UnixTcpSock& sock, UnixAddr& addr, UnixAddr& from)
	{
		Logger log;
		log.add_cout();
		log.id("unix tcp readwrite server");

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

TEST_F(unix_networking, addrnames)
{
	ASSERT_EQ(server_addr.host(), server_addrname);
	ASSERT_EQ(client_addr.host(), client_addrname);
}

TEST_F(unix_networking, udp_readwrite)
{
	UnixUdpSock servsock;
	UnixAddr from;
	
	auto fut = async(udp_readwrite_server, std::ref(startev), std::ref(servsock), std::ref(server_addr), std::ref(from), std::ref(quitev));

	startev.read();	  // wait for the server to listen

	UnixUdpSock sock;

	// bind to receive replies
	log << "binding to " << client_addr << endl;
	sock.bind(client_addr); 

	udp_writeread_test(sock, server_addr);

	quitev.write(1);		// signal server to quit (we don't have a connection)
	
	int res = fut.get();
	log << "return code: " << res << endl;
	
	ASSERT_EQ(res, 0);
	ASSERT_EQ(from.host(), client_addr.host());
}

TEST_F(unix_networking, tcp_iostream)
{
	UnixTcpSock servsock;

	UnixAddr from;
	auto fut = async(tcp_iostream_server, std::ref(startev), std::ref(servsock), std::ref(server_addr), std::ref(from));

	startev.read();

	UnixTcpSock sock;

	// binding will allow the server to see our "address"; it is optional for connected protocol
	log << "binding to " << client_addr << endl;
	sock.bind(client_addr);

	log << "connect" << endl;
	sock.connect(server_addr);

	log << "connected to addr name=" << server_addr.host() << endl;

	tcp_stream_test(sock);

	sock.close();		// close the socket to shut down the remote server connection
	
	int res = fut.get();
	log << "return code: " << res << endl;
	
	ASSERT_EQ(res, 0);
	ASSERT_EQ(from.host(), client_addr.host());
}

TEST_F(unix_networking, tcp_readwrite)
{
	UnixTcpSock servsock;
	UnixAddr from;

	auto fut = async(tcp_readwrite_server, std::ref(startev), std::ref(servsock), std::ref(server_addr), std::ref(from));

	startev.read();

	UnixTcpSock sock;

	log << "binding to " << client_addr << endl;
	sock.bind(client_addr);

	log << "connect" << endl;
	sock.connect(server_addr);

	log << "connected to addr name=" << server_addr.host() << endl;

	tcp_writeread_test(sock);

	sock.close();
	int res = fut.get();
	log << "return code: " << res << endl;
	ASSERT_EQ(res, 0);
	ASSERT_EQ(from.host(), client_addr.host());
}
