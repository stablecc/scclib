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
#include <iostream>
#include <cstring>
#include <system_error>
#include <getopt.h>
#include <chrono>
#include <thread>
#include "net/net_if.h"
#include "net/inet.h"
#include "util/poller.h"
#include "util/event.h"
#include "net/inet.h"
#include "util/iostream.h"
#include "util/logger.h"

/** \addtogroup examples
	@{ */
/** Networking example \file */
/** \example examples/net/netmain.cc */
/** @} */

using std::exception;
using std::getline;
using std::string;
using std::thread;
using std::runtime_error;
using std::endl;
using std::chrono::milliseconds;
using std::chrono::seconds;
using scc::util::Logger;
using scc::util::Event;
using scc::util::Poller;
using scc::util::InStream;
using scc::util::OutStream;
using scc::util::IoStream;
using scc::net::NetIf;
using scc::net::InetAddr;
using scc::net::InetTcpSock;
using scc::net::InetUdpSock;

Logger lout;

void print_available_addrs()
{
	lout << endl;
	lout << "Any addrs:" << endl;
	InetAddr sa;
	sa.host("::ffff:0.0.0.0");
	lout << "    " << sa << endl;
	sa.host("::");
	lout << "    " << sa << endl;

	auto ifs = NetIf::all_interfaces();
	for (auto& i : ifs)
	{
		lout << "Interface " << i.name() << " (index " << i.index() << "):" << endl;
		for (auto& s : i.addrs())
		{
			lout << "    " << s << endl;
		}
	}
}

void print_interfaces()
{
	lout << endl;
	lout << "Interfaces: " << endl;
	auto ifs = NetIf::all_interfaces();
	for (auto& i : ifs)
	{
		lout << i.str() << endl;
	}
}

string resolve(const string& host, bool udp=false)
{
	auto hads = NetIf::host_addrs(host, udp ? NetIf::SocketType::udp_datagram : NetIf::SocketType::tcp_stream);
	if (hads.size())
	{
		lout << "* " << host << " --> " << hads[0] << endl;
		return hads[0].host();
	}
	return host;
}

void test_tcp(const string& host, int port, unsigned scope, int timeout)
{
	InetAddr addr;
	addr.host(host);
	if (scope)			addr.scope_id(scope);
	addr.port(port);

	InetTcpSock s;

	if (timeout >= 0)
	{
		lout << "* connecting with " << timeout << " second timeout..." << endl;
		
		std::error_code ec;
		s.non_blocking(true);
		s.connect(addr, ec);			// don't let EINPROGRESS throw an exception
		
		if (ec.value() != 0 && ec.value() != EINPROGRESS)
		{
			throw std::system_error(ec.value(), std::system_category());
		}
		
		Poller pout;
		pout.set(s.fd(), Poller::output);
		pout.wait(seconds(timeout));

		if (pout.event(s.fd()) == 0)
		{
			throw runtime_error("timed out");
			return;
		}

		s.non_blocking(false);
		s.connect(addr);			// this will either connect or throw
	}
	else
	{
		lout << "* connecting with no timeout" << endl;
		s.connect(addr);
	}
	lout << "* connected OK" << endl;
}


void listen_tcp(const string& host, int port, unsigned scope)
{
	InetAddr addr;
	addr.host(host);
	if (scope)			addr.scope_id(scope);
	addr.port(port);

	lout << "* server tcp address: " << addr << endl;

	InetTcpSock s;
	s.reuse_addr(true);
	
	lout << "* tcp binding to address" << endl;
	s.bind(addr);
	
	lout << "* tcp listening" << endl;	
	s.listen();

	for (;;)
	{
		InetAddr from;

		lout << "* tcp waiting for connection" << endl;
		auto conn = s.accept(from);
		lout << "* connection from " << from << endl;

		IoStream st(conn, conn);

		string got;
		while (getline(st, got))	 // loop until eof (socket closed)
		{
			lout << "echo > " << got << endl;
			st << got << endl;
		}
		lout << "* input stream failed: eof()=" << st.eof() << endl;
	}
}

void connect_tcp(const string& host, int port, unsigned scope)
{
	InetAddr addr;
	addr.host(host);
	if (scope)			addr.scope_id(scope);
	addr.port(port);

	lout << "* address: " << addr << endl;

	InetTcpSock s;
	s.reuse_addr(true);

	lout << "* tcp connect..." << endl;
	s.connect(addr);

	lout << "* connected, sending keyboard input to server" << endl;
	
	Event done;

	thread t([&s, &done]()
	{
		Logger tout;
		tout.add_cout();

		InStream st(s);

		Poller pin;
		pin.set(done, Poller::input);
		pin.set(s.fd(), Poller::input);

		while (1)
		{
			pin.wait(milliseconds(100));
			if (pin.event(done))
			{
				tout << "* done signal, thread exit" << endl;
				break;
			}
			std::string got;
			while (getline(st, got))
			{
				tout << "got > " << got << endl;
			}
		}
	});

	OutStream st(s);
	
	for (;;)
	{
		std::this_thread::sleep_for(milliseconds(100));

		string cmd;
		getline(std::cin, cmd);

		if (!(st << cmd << endl))
		{
			lout << "* output stream failed, exit" << endl;
			break;
		}
		st.flush();
	}

	done.write(1);
	s.close();
	t.join();
}

void listen_udp(const string& host, int port, unsigned scope)
{
	InetAddr addr;
	addr.host(host);
	if (scope)			addr.scope_id(scope);
	addr.port(port);

	lout << "* server udp address: " << addr << endl;

	InetUdpSock s;
	s.reuse_addr(true);

	lout << "* udp binding to address" << endl;
	s.bind(addr);

	Poller pin;
	pin.set(s.fd(), Poller::input);

	string got;
	for (;;)
	{
		pin.wait();

		auto sz = s.recv_next();
		InetAddr from;
		got.resize(sz+1);
		s.recv(&got[0], sz, from);
		got[sz] = '\0';
		
		lout << "echo to " << from.host() << " > " << got;
		s.send(got.data(), sz, from);
	}
}

void connect_udp(const string& host, int port, unsigned scope)
{
	InetAddr addr;
	addr.host(host);
	if (scope)		addr.scope_id(scope);
	addr.port(port);

	lout << "* connect udp address: " << addr << endl;

	InetUdpSock s;
	s.reuse_addr(true);

	Event done;

	thread t([&s, &done]()
	{
		Logger tout;
		tout.add_cout();

		Poller pin;
		pin.set(s.fd(), Poller::input);
		pin.set(done, Poller::input);

		string got;
		for (;;)
		{
			pin.wait();
			if (pin.event(done))
			{
				tout << "* done signal, thread exit" << endl;
				break;
			}
			if (pin.event(s.fd()))
			{
				auto sz = s.recv_next();
				
				InetAddr from;
				got.resize(sz+1);
				if (s.recv(&got[0], sz, from) <= 0)
				{
					tout << "* recv failed, thread exit" << endl;
					break;
				}
				got[sz] = '\0';
				
				tout << "got from " << from << " > " << got;
			}
		}
	});

	lout << "* sending keyboard input to server" << endl;

	for (;;)
	{
		std::this_thread::sleep_for(milliseconds(100));

		string cmd;
		getline(std::cin, cmd);

		cmd += '\n';

		if (s.send(&cmd[0], cmd.size(), addr) <= 0)
		{
			lout << "* send failed, exit" << endl;
			break;
		}
	}

	done.write(1);
	s.close();
	t.join();
}

void test_connection(const string& host, int port, unsigned scope, int timeout)
{
	try
	{
		test_tcp(resolve(host), port, scope, timeout);
	}
	catch (exception& ex)
	{
		lout << "* connection failed: " << ex.what() << endl;
	}
	
	exit(0);
}

void server(const string& host, int port, bool udp, unsigned scope)
{
	try
	{
		if (udp)
		{
			listen_udp(resolve(host, true), port, scope);
		}
		else
		{
			listen_tcp(resolve(host), port, scope);
		}
	}
	catch (exception& ex)
	{
		lout << "* server failed: " << ex.what() << endl;
	}
	
	exit(0);
}

void client(const string& host, int port, bool udp, unsigned scope)
{
	try
	{
		if (udp)
		{
			connect_udp(resolve(host, true), port, scope);
		}
		else
		{
			connect_tcp(resolve(host), port, scope);
		}
	}
	catch (exception& ex)
	{
		lout << "* client failed: " << ex.what() << endl;
	}
	exit(0);
}

int main(int argc, char **argv)
{
	option lo[32];
	memset(&lo[0], 0, 32*sizeof(option));
	lo[0].name = "help";
	lo[0].val = '?';
	lo[1].name = "ifs";
	lo[1].val = 'I';
	//lo[1].has_arg = 0;
	lo[2].name = "addrs";
	lo[2].val = 'A';
	lo[3].name = "listen";
	lo[3].val = 'l';
	lo[4].name = "udp";
	lo[4].val = 'u';
	lo[5].name = "test";
	lo[5].val = 'T';
	lo[6].name = "resolve";
	lo[6].val = 'R';
	lo[7].name = "scope";
	lo[7].val = 's';
	lo[7].has_arg = 1;

	lout.add_cout();

	bool usage=false, listen=false, udp=false, test=false, resolve=false;
	unsigned scope=0;
	string mcast;
	while (1)
	{
		int opt = getopt_long(argc, argv, "?IAluTRs:", &lo[0], nullptr);
		if (opt == -1)
		{
			break;
		}
		switch (opt)
		{
		case '?':
			usage = true;
			break;
		case 'I':
			print_interfaces();
			exit(0);
			break;
		case 'A':
			print_available_addrs();
			exit(0);
			break;
		case 'l':
			listen = true;
			break;
		case 'u':
			udp = true;
			break;
		case 'T':
			test = true;
			break;
		case 'R':
			resolve = true;
			break;
		case 's':
			if (!optarg)	usage = true;
			else			scope = atoi(optarg);
			break;

		default:
			usage = true;
		}
	}

	int nargs = argc-optind;

	if (resolve)
	{
		if (nargs < 1)	usage = true;
	}
	else
	{
		if (nargs < 2)	usage = true;
	}

	if (usage)
	{
		using std::cerr;
		cerr << argv[0] << endl;
		cerr << "  scclib net module example" << endl;
		
		cerr << "  For ipv4 addresses, use ipv4/6 syntax, e.g. ::ffff:192.168.1.1" << endl;
		cerr << endl;
		cerr << "  Informational:" << endl;
		cerr << "    -I|--ifs                    print out interfaces and return" << endl;
		cerr << "    -A|--addrs                  print out addrs and return" << endl;
		cerr << "    -R|--resolve HOST1 HOST2 .. resolve host(s) and return" << endl;
		cerr << "  Test:" << endl;
		cerr << "    -T|--test HOST PORT [secs]  perform tcp connection test and return" << endl;
		cerr << "  Client:" << endl;
		cerr << "    HOST PORT                   connect and send keyboard input" << endl;
		cerr << "  Server:" << endl;
		cerr << "    -l HOST PORT                listen and echo" << endl;
		cerr << "  Common params for client/server:" << endl;
		cerr << "    -u|--udp                    udp mode" << endl;
		cerr << "    -s|--scope <NUM>            set scope_id for address" << endl;
		exit(1);
	}

	if (resolve)
	{
		while (optind < argc)
		{
			lout << "* resolving " << argv[optind] << endl;
			auto hads = NetIf::host_addrs(argv[optind], udp ? NetIf::SocketType::udp_datagram : NetIf::SocketType::tcp_stream);
			if (!hads.size())
			{
				lout << "* not resolved" << endl;
			}
			else for (auto& i : hads)
			{
				lout << "* " << i << endl;
			}
			optind++;
		}
	}
	else if (test)
	{
		test_connection(argv[optind], atoi(argv[optind+1]), scope, optind+2 < argc ? atoi(argv[optind+2]) : -1);
	}
	else if (listen)
	{
		server(argv[optind], atoi(argv[optind+1]), udp, scope);
	}
	else
	{
		client(argv[optind], atoi(argv[optind+1]), udp, scope);
	}
	return 0;
}
