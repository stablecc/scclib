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
#include <gtest/gtest.h>
#include <system_error>
#include <future>
#include <sstream>
#include <chrono>
#include <thread>

#include "util/event.h"
#include "util/poller.h"
#include "net/inet.h"
#include "util/iostream.h"
#include "util/logger.h"

/** \addtogroup net_inet
	@{ */
/** Tests for \ref net_inet streams \file */
/** \example net/unittest/inet_stream.cc */
/** @} */

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::ios;
using scc::util::IoStream;
using scc::util::Logger;
using scc::util::Event;
using scc::util::Poller;
using scc::net::InetAddr;
using scc::net::InetTcpSock;

struct InetServerStreamTest : public testing::Test
{
	Event startev, quitev;
	InetTcpSock ssock, csock;
	InetAddr caddr, saddr;
	Logger log;

	InetServerStreamTest()
	{
		log.add_cout();
		log.max_line(2048);
		saddr.host("::");
		saddr.port(9876);
		ssock.reuse_addr(true);
		ssock.bind(saddr);
		caddr.host("::1");
		caddr.port(9876);
	}
	virtual ~InetServerStreamTest()
	{
	}

	static void set_all_polls(Poller& p, int fd)
	{
		p.set(fd, Poller::PollFlag::input|Poller::PollFlag::output|Poller::PollFlag::read_hup|Poller::PollFlag::priority);
	}
	static std::string str_polls(int ev)
	{
		stringstream s;
		if (ev & Poller::PollFlag::input)		s << " input";
		if (ev & Poller::PollFlag::output)		s << " output";
		if (ev & Poller::PollFlag::read_hup)	s << " read_hup";
		if (ev & Poller::PollFlag::priority)	s << " priority";
		if (ev & Poller::PollFlag::hup)			s << " hup";
		if (ev & Poller::PollFlag::error)		s << " error";
		return s.str();
	}

	static std::string server(InetTcpSock& sock, Event& start, Event& quit)
	{
		using scc::util::Poller;
		Logger log;
		log.add_cout();
		log.id("*serv*");

		stringstream s;

		try
		{
			sock.listen();
			log << "signal start" << endl;
			start.write(1);

			Poller p;
			p.set(quit, Poller::PollFlag::input);
			set_all_polls(p, sock);

			//sock.non_blocking(true);
			//error_code ec;

			log << "poll before accept" << endl;
			s << "*accept_poll:";
			while (1)
			{
				p.wait();
				if (p.event(quit))
				{
					log << "quit event" << endl;
					s << " quit";
					return s.str();
				}
				
				int ev = p.event(sock);
				
				if (ev & ~Poller::PollFlag::output)
				{
					cout << "sock events: " << str_polls(ev) << endl;
					s << str_polls(ev);
				}
				
				if (ev & Poller::PollFlag::input)		break;
			}

			log << "calling accept" << endl;
			InetAddr from;
			auto conn = sock.accept(from);
			log << "connect from " << from << endl;

			IoStream st(conn, conn);
			st.exceptions(ios::failbit);		// throw on eof or bad
			try
			{
				set_all_polls(p, conn);
				while (1)
				{
					s << " *conn_read_poll:";
					while (1)
					{
						p.wait();
						if (p.event(quit))
						{
							log << "quit event" << endl;
							s << " quit";
							return s.str();
						}

						int ev = p.event(conn);
						if (ev & ~Poller::PollFlag::output)
						{
							cout << "conn events: " << str_polls(ev) << endl;
							s << " *conn_ev:" << str_polls(ev);
						}

						if (ev & Poller::PollFlag::read_hup)
						{
							log << "connection sock peer hung up" << endl;
							s << " *conn_peer_hup";
							return s.str();
						}

						if (ev & Poller::PollFlag::input)		break;
					}

					log << "getline waiting" << endl;
					string got;
					if (!std::getline(st, got))
					{
						log << "getline returned failed" << endl;
						s << " *getline_failed";
						return s.str();
					}
					log << "got=" << got << endl;

					s << " *conn_write_poll:";
					while (1)
					{
						p.wait();
						if (p.event(quit))
						{
							log << "quit event" << endl;
							s << " quit";
							return s.str();
						}

						int ev = p.event(conn);
						if (ev & ~Poller::PollFlag::input)
						{
							cout << "conn events: " << str_polls(ev) << endl;
							s << " *conn_ev:" << str_polls(ev);
						}

						if (ev & Poller::PollFlag::read_hup)
						{
							log << "connection sock peer hung up" << endl;
							s << " *conn_peer_hup";
							return s.str();
						}

						if (ev & Poller::PollFlag::output)		break;
					}

					st << got << endl;
					if (got == "quit")
					{
						log << "getline stream got quit" << endl;
						s << " *getline_quit";
						return s.str();
					}
				}
				log << "returned from stream loop" << endl;
				s << " *stream_loop_returned";
			}
			catch (const std::exception& e)
			{
				log << "stream exception: " << e.what() << endl;

				if (st.eof())
				{
					log << "stream eof" << endl;
					s << " *stream_eof";
				}
				if (st.bad())
				{
					log << "stream eof" << endl;
					s << " *stream_bad";
				}
				if (st.send_fail().size())
				{
					log << "stream send failmsg=" << st.send_fail() << endl;
					s << " *stream_send_fail=" << st.send_fail();
				}
				if (st.recv_fail().size())
				{
					log << "stream recv failmsg=" << st.recv_fail() << endl;
					s << " *stream_recv_fail=" << st.recv_fail();
				}
				return s.str();
			}
		}
		catch (const std::exception& e)
		{
			log << "pre-accept ex: " << e.what() << endl;
			start.write(1);
			return s.str()+" ex="+e.what();
		}
		start.write(1);
		return s.str();
	}

	void clientstart()
	{
		log << "wait for start" << endl;
		startev.read();

		//this_thread::sleep_for(chrono::milliseconds(100));

		log << "connect to " << endl;
		csock.connect(caddr);
	}
};

TEST_F(InetServerStreamTest, send_quit)
{
	log.id("send quit and recv");

	auto fut = async(server, std::ref(ssock), std::ref(startev), std::ref(quitev));

	clientstart();

	IoStream st(csock, csock);

	st << "quit" << endl;
	string got;
	getline(st, got);
	log << "got=" << got << endl;

	auto ret = fut.get();
	log << "server response: " << ret << endl;
}

TEST_F(InetServerStreamTest, reset)
{
	log.id("send recv and reset");

	auto fut = async(server, std::ref(ssock), std::ref(startev), std::ref(quitev));

	clientstart();

	csock.reset();

	auto ret = fut.get();
	log << "server response: " << ret << endl;
}

TEST_F(InetServerStreamTest, send_and_reset_clientsock)
{
	log.id("send recv and reset client socket");

	auto fut = async(server, std::ref(ssock), std::ref(startev), std::ref(quitev));

	clientstart();

	IoStream st(csock, csock);

	st << "don't quit" << endl;
	string got;
	getline(st, got);
	log << "got=" << got << endl;

	csock.reset();

	auto ret = fut.get();
	log << "server response: " << ret << endl;
}

TEST_F(InetServerStreamTest, quit)
{
	log.id("send recv and reset");

	auto fut = async(server, std::ref(ssock), std::ref(startev), std::ref(quitev));

	log << "wait for start" << endl;
	startev.read();

	quitev.write(1);

	auto ret = fut.get();
	log << "server response: " << ret << endl;
}
