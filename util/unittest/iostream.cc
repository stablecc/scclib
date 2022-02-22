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
#include "util/iostream.h"
#include "util/rwloopbuf.h"
#include "util/rwcounter.h"
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

/** \addtogroup util_iostream
	@{ */
/** Tests for \ref util_iostream
	\file */
/** \example scclib/util/unittest/iostream.cc
	Tests for \ref util_iostream */
/** @} */

using namespace std;
using scc::util::RwLoopBuffer;
using scc::util::InStream;
using scc::util::OutStream;
using scc::util::IoStream;
using scc::util::RwCounter;
using scc::util::Reader;
using scc::util::Writer;

//! [I/O stream decorator stack]
static string test_word = "QuotesFromOscarWilde";
static string test_line = "It is always a silly thing to give advice, but to give good advice is fatal.";
static string test_long =
	"One can survive everything, nowadays, except death, and live down everything except a good reputation.\n"
	"One should always play fairly when one has the winning cards.\n"
	"Patriotism is the virtue of the vicious.\n"
	"Selfishness is not living as one wishes to live, it is asking others to live as one wishes to live.";

TEST(iostream_test, example_decorator_stack)
{
	RwLoopBuffer rwbuf;					// loopback stream buffer
	RwCounter rwcount(rwbuf, rwbuf);	// decorator provides read() and write() for the stream buffer
	IoStream rws(rwcount, rwcount);		// provides std::iostream

	std::stringstream sstr;				// std::stringstream is a std::iostream

	// verify stream write behavior

	rws << test_line << endl;			// std::endl writes a newline char and flushes the stream
	rws << test_long << endl;
	sstr << test_line << endl;
	sstr << test_long << endl;

	ASSERT_EQ(rwbuf.str(), sstr.str());

	ASSERT_EQ(rwcount.write_count(), sstr.str().size());
	ASSERT_EQ(rwcount.read_count(), 0);

	// verify stream read behavior

	string reads, tests;
	while (sstr >> reads)
	{
		ASSERT_TRUE(rws >> tests);
		ASSERT_EQ(reads, tests);
	}

	ASSERT_FALSE(rws >> tests);		// verify the RwLoopBuffer stream is done as well

	ASSERT_EQ(rwcount.read_count(), sstr.str().size());

	ASSERT_EQ(rwbuf.str().size(), 0); 			// the loopback buffer is now empty
}

struct iostreamTest : public testing::Test
{
	std::vector<string> all;
	iostreamTest()
	{
		all.push_back(test_word);
		all.push_back(test_line);
		all.push_back(test_long);
	}
	virtual ~iostreamTest() {}

	void readwords_test(const string s, int recvbuf_size=10)
	{
		stringstream sstr(s);
		RwLoopBuffer rw(s);
		InStream ins(rw);
		ins.recvbuf_size(recvbuf_size);

		ASSERT_EQ(ins.recvbuf_size(), recvbuf_size);

		cout << "readwords_test: " << s.substr(0,10) << "... size=" << s.size() << " recvbuf_size= " << ins.recvbuf_size() << endl;

		string reads, tests;
		while (sstr >> reads)
		{
			ASSERT_TRUE(ins >> tests);
			ASSERT_EQ(reads, tests);
		}
	}
	void readlines_test(const string s, int recvbuf_size=10)
	{
		stringstream sstr(s);
		RwLoopBuffer rw(s);
		InStream ins(rw);
		ins.recvbuf_size(recvbuf_size);

		ASSERT_EQ(ins.recvbuf_size(), recvbuf_size);

		cout << "readlines_test: " << s.substr(0,10) << "... size=" << s.size() << " recvbuf_size= " << ins.recvbuf_size() << endl;

		string reads, tests;
		while (std::getline(sstr, reads))
		{
			ASSERT_TRUE(std::getline(ins, tests));
			ASSERT_EQ(reads, tests);
		}
	}
	void writewords_test(const string s, int sendbuf_size=10)
	{
		stringstream sstr(s);
		RwLoopBuffer rw;
		OutStream ous(rw);
		ous.sendbuf_size(sendbuf_size);

		ASSERT_EQ(ous.sendbuf_size(), sendbuf_size);

		cout << "writewords_test: " << s.substr(0,10) << "... size=" << s.size() << " sendbuf_size= " << ous.sendbuf_size() << endl;

		string reads;
		stringstream writess;
		while (sstr >> reads)
		{
			writess << reads;
			ASSERT_TRUE(ous << reads);
		}
		writess.flush();
		ous.flush();
		ASSERT_EQ(writess.str(), rw.str());
	}
	void writelines_test(const string s, int sendbuf_size=10)
	{
		stringstream sstr(s);
		RwLoopBuffer rw;
		OutStream ous(rw);
		ous.sendbuf_size(sendbuf_size);

		ASSERT_EQ(ous.sendbuf_size(), sendbuf_size);

		cout << "writelines_test: " << s.substr(0,10) << "... size=" << s.size() << " sendbuf_size= " << ous.sendbuf_size() << endl;

		string reads;
		stringstream writess;
		while (std::getline(sstr, reads))
		{
			writess << reads << endl;
			ASSERT_TRUE(ous << reads << endl);
		}
		writess.flush();
		ous.flush();
		ASSERT_EQ(writess.str(), rw.str());
	}
	void readchunks_test(const string s, int rbuf1=10, int rsz1=1024, int rbuf2=10)
	{
		RwLoopBuffer rw(s);
		InStream ins(rw);

		ins.recvbuf_size(rbuf1);
		ASSERT_EQ(ins.recvbuf_size(), rbuf1);

		int toread = s.size();
		cout << "readchunks_test1: " << s.substr(0,10) << "... read=" << rsz1 << " toread=" << toread
			<< " recvbuf_size= " << ins.recvbuf_size() << endl;
		char bin1[rsz1];
		ins.read(bin1, rsz1);
		auto lastread = ins.gcount();
		ASSERT_EQ(lastread, std::min(rsz1, toread));
		ASSERT_EQ(memcmp(s.data(), bin1, lastread), 0);
		toread -= lastread;
		if (lastread < rsz1)
		{
			ASSERT_EQ((ins.rdstate()&std::ios_base::eofbit), std::ios_base::eofbit);
		}

		if (toread)
		{
			ins.recvbuf_size(rbuf2);
			ASSERT_EQ(ins.recvbuf_size(), rbuf2);

			cout << "readchunks_test2: " << s.substr(0,10) << "... read=" << toread << " toread=" << toread
				<< " recvbuf_size= " << ins.recvbuf_size() << endl;
			char bin2[toread];
			ins.read(bin2, toread);
			auto lastread2 = ins.gcount();
			ASSERT_EQ(lastread2, toread);
			ASSERT_EQ(memcmp(s.data()+lastread, bin2, lastread2), 0);
			ASSERT_NE((ins.rdstate()&std::ios_base::eofbit), std::ios_base::eofbit);
		}
	}
	void writechunks_test(const string s, int wbuf1=10, int wsz1=1024, int wbuf2=10)
	{
		RwLoopBuffer rw;
		OutStream ous(rw);

		ous.sendbuf_size(wbuf1);
		ASSERT_EQ(ous.sendbuf_size(), wbuf1);

		int towrite = s.size();
		wsz1 = std::min(wsz1, towrite);
		cout << "writechunks_test1: " << s.substr(0,10) << "... write=" << wsz1 << " towrite=" << towrite
			<< " sendbuf_size= " << ous.sendbuf_size() << endl;
		ASSERT_TRUE(ous.write(s.data(), wsz1));

		towrite -= wsz1;

		if (towrite)
		{
			ous.sendbuf_size(wbuf2);
			ASSERT_EQ(ous.sendbuf_size(), wbuf2);

			cout << "writechunks_test2: " << s.substr(0,10) << "... write=" << towrite << " towrite=" << towrite
				<< " sendbuf_size= " << ous.sendbuf_size() << endl;

			ASSERT_TRUE(ous.write(s.data()+wsz1, towrite));
		}
		ous.flush();
		ASSERT_EQ(s, rw.str());
	}
};

TEST_F(iostreamTest, read)
{
	for (auto& s : all)
	{
		readwords_test(s);
	}
	for (auto& s : all)
	{
		readlines_test(s);
	}
}

TEST_F(iostreamTest, write)
{
	for (auto& s : all)
	{
		writewords_test(s);
	}
	for (auto& s : all)
	{
		writelines_test(s);
	}
}

TEST_F(iostreamTest, readchunks)
{
	for (auto& s : all)
	{
		readchunks_test(s); // buf 10, read all
		readchunks_test(s, 10, 5, 20); // buf 10, read 5, buf 20, read all
		readchunks_test(s, 10, 8, 5); // buf 10, read 8, buf 5, read all
	}
}

TEST_F(iostreamTest, writechunks)
{
	for (auto& s : all)
	{
		writechunks_test(s); // buf 10, write all
		writechunks_test(s, 10, 5, 20); // buf 10, write 5, buf 20, write all
		writechunks_test(s, 10, 8, 5); // buf 10, write 8, buf 5, write all
	}
}

TEST(iostream_test, write_read)
{
	string val("this\nis\na\ntest\n");
	RwLoopBuffer rw;
	IoStream ios(rw, rw);

	ASSERT_TRUE(ios.write(val.data(), val.size()));
	ios.flush();
	char buf[sizeof(val)];
	ASSERT_TRUE(ios.read(buf, val.size()));
	ASSERT_EQ(ios.gcount(), val.size());
	ASSERT_EQ(memcmp(buf, val.data(), val.size()), 0);
	ASSERT_FALSE(ios.read(buf, val.size()));
}

TEST(iostream_test, stream_write_read)
{
	string val("this\nis\na\ntest\n");
	RwLoopBuffer rw;
	IoStream ios(rw, rw);

	ASSERT_TRUE(ios << val);
	ios.flush();
	string s;
	ASSERT_TRUE(ios >> s);
	ASSERT_EQ(s, "this");
	ASSERT_TRUE(ios >> s);
	ASSERT_EQ(s, "is");
	ASSERT_TRUE(ios >> s);
	ASSERT_EQ(s, "a");
	ASSERT_TRUE(ios >> s);
	ASSERT_EQ(s, "test");
}
string printstate(int state)
{
	stringstream st;
	st << "state: ";
	if (state == 0) st << "good ";
	if ((state&std::ios_base::badbit)==std::ios_base::badbit) st << "bad ";
	if ((state&std::ios_base::failbit)==std::ios_base::failbit) st << "fail ";
	if ((state&std::ios_base::eofbit)==std::ios_base::eofbit) st << "eof ";
	return st.str();
}

TEST(iostream_test, write_buffer_states_test)
{
	RwLoopBuffer rw;
	OutStream st(rw, 10);

	string s1 = "This is a test", s2 = " of the stream bit stuff.";

	cout << "***OutStream test" << endl;
	cout << "Before " << printstate(st.rdstate()) << endl;
	ASSERT_EQ(st.rdstate(), 0);
	st << s1;
	cout << "write '"<<s1<<"' string: '" << rw.str() << "' " << printstate(st.rdstate()) << endl;
	ASSERT_EQ(st.rdstate(), 0);
	ASSERT_EQ(rw.str(), s1.substr(0, 10));
	st.setstate(std::ios_base::eofbit);
	cout << "seteof string: '" << rw.str() << "' " << printstate(st.rdstate()) << endl;
	ASSERT_EQ(st.rdstate(), std::ios_base::eofbit);
	st << s2;
	cout << "write '"<<s2<<"' string: '" << rw.str() << "' " << printstate(st.rdstate()) << endl;
	ASSERT_EQ(st.rdstate(), std::ios_base::eofbit|std::ios_base::failbit);
	ASSERT_EQ(rw.str(), s1.substr(0, 10));
	st.clear();
	cout << "clear string: '" << rw.str() << "' "  << printstate(st.rdstate()) << endl;
	st << s2;
	cout << "write '"<<s2<<"' string: '" << rw.str() << "' " << printstate(st.rdstate()) << endl;
	st.flush();
	cout << "flush string: '" << rw.str() << "' " << printstate(st.rdstate()) << endl;
	ASSERT_EQ(st.rdstate(), 0);
	ASSERT_EQ(rw.str(), s1+s2);
}

TEST(iostream_test, move_assign)
{
	RwLoopBuffer rw;
	IoStream io1(rw, rw);
	io1 << "test";
	io1.flush();
	IoStream io2 = std::move(io1);
	string s;
	io2 >> s;
	cout << "got: " << s << endl;
	ASSERT_EQ(s, "test");
}

TEST(iostream_test, move_moveconstruct)
{
	RwLoopBuffer rw;
	IoStream io1(rw, rw);
	io1 << "test";
	io1.flush();
	IoStream io2(std::move(io1));
	string s;
	io2 >> s;
	cout << "got: " << s << endl;
	ASSERT_EQ(s, "test");
}

TEST(instream_test, move_assign)
{
	RwLoopBuffer rw("test");
	InStream in1(rw);
	InStream in2 = std::move(in1);
	string s;
	in2 >> s;
	cout << "got: " << s << endl;
	ASSERT_EQ(s, "test");
}

TEST(instream_test, move_construct)
{
	RwLoopBuffer rw("test");
	InStream in1(rw);
	InStream in2(std::move(in1));
	string s;
	in2 >> s;
	cout << "got: " << s << endl;
	ASSERT_EQ(s, "test");
}

TEST(outstream_test, move_assign)
{
	RwLoopBuffer rw;
	OutStream ou1(rw);
	OutStream ou2 = std::move(ou1);
	ou2 << "test";
	ou2.flush();
	cout << "got: " << rw.str() << endl;
	ASSERT_EQ(rw.str(), "test");
}

TEST(outstream_test, move_construct)
{
	RwLoopBuffer rw;
	OutStream ou1(rw);
	OutStream ou2(std::move(ou1));
	ou2 << "test";
	ou2.flush();
	cout << "got: " << rw.str() << endl;
	ASSERT_EQ(rw.str(), "test");
}

TEST(fail_test, outstream_except)
{
	string line("test");

	struct ExReader : public Reader
	{
		bool fail;
		ExReader() : fail(true) {}
		size_t read(void* loc, size_t len)
		{
			if (fail)	throw std::runtime_error("test");
			return len;
		}
	};
	struct ExWriter : public Writer
	{
		bool fail;
		ExWriter() : fail(true) {}
		size_t write(const void*, size_t len)
		{
			if (fail)	throw std::runtime_error("test");
			return len;
		}
	};

	ExReader rd;
	ExWriter wr;
	InStream is(rd);
	OutStream os(wr);
	IoStream io(rd, wr);

	is.read((char*)line.data(), 4);

	ASSERT_TRUE(is.fail());
	ASSERT_EQ(is.rdstate() & ios::badbit, ios::badbit);
	ASSERT_EQ(is.recv_fail(), "test");

	is.clear();
	ASSERT_EQ(is.recv_fail(), "");
	rd.fail = false;
	is.read((char*)line.data(), 4);

	ASSERT_FALSE(is.fail());
	ASSERT_EQ(is.recv_fail(), "");

	os << "test";
	os.flush();

	ASSERT_TRUE(os.fail());
	ASSERT_EQ(os.rdstate() & ios::badbit, ios::badbit);
	ASSERT_EQ(os.send_fail(), "test");

	os.clear();
	ASSERT_EQ(os.send_fail(), "");
	wr.fail = false;
	os << "test";
	os.flush();

	ASSERT_FALSE(os.fail());
	ASSERT_EQ(os.send_fail(), "");

	io.clear();
	rd.fail = true;
	wr.fail = true;

	io.read((char*)line.data(), 4);

	io << "test";
	io.flush();

	ASSERT_TRUE(io.fail());
	ASSERT_EQ(io.rdstate() & ios::badbit, ios::badbit);
	ASSERT_EQ(io.send_fail(), "");
	ASSERT_EQ(io.recv_fail(), "test");

	io.clear();

	io << "test";
	io.flush();

	io.read((char*)line.data(), 4);

	ASSERT_TRUE(io.fail());
	ASSERT_EQ(io.send_fail(), "test");
	ASSERT_EQ(io.recv_fail(), "");

	io.clear();
	rd.fail = false;
	wr.fail = false;

	io << "test";
	io.flush();

	io.read((char*)line.data(), 4);

	ASSERT_FALSE(io.fail());
	ASSERT_EQ(io.send_fail(), "");
	ASSERT_EQ(io.recv_fail(), "");
}

TEST(shared_ptr_test, shared_io)
{
	std::shared_ptr<RwLoopBuffer> buf(new RwLoopBuffer);
	IoStream io(buf, buf);

	for (char ch : test_long)		io.put(ch);
	io.flush();

	ASSERT_EQ(test_long, buf->str());

	string got;
	for (char ch; io.get(ch);)		got.push_back(ch);

	ASSERT_EQ(test_long, got);

	cout << "io buf references: " << buf.use_count() << endl;
	ASSERT_EQ(buf.use_count(), 3);
}

TEST(shared_ptr_test, shared_in)
{
	std::shared_ptr<RwLoopBuffer> buf(new RwLoopBuffer);
	InStream io(buf);

	buf->set(test_long.data(), test_long.size());

	string got;
	for (char ch; io.get(ch);)		got.push_back(ch);

	ASSERT_EQ(test_long, got);

	cout << "in buf references: " << buf.use_count() << endl;
	ASSERT_EQ(buf.use_count(), 2);
}

TEST(shared_ptr_test, shared_out)
{
	std::shared_ptr<RwLoopBuffer> buf(new RwLoopBuffer);
	OutStream io(buf);

	for (char ch : test_long)		io.put(ch);
	io.flush();

	ASSERT_EQ(test_long, buf->str());

	cout << "out buf references: " << buf.use_count() << endl;
	ASSERT_EQ(buf.use_count(), 2);
}

TEST(basic_stream_test, sync_n_flush)
{
	RwLoopBuffer buf;
	RwCounter count(buf, buf);
	IoStream io(count, count, 10, 10);

	for (unsigned i = 0; i < 7; i++)		io.put(test_long[i]);

	cout << "partial put buf size: " << buf.size() << endl;
	cout << "partial put idx: " << buf.idx() << endl;

//	ASSERT_TRUE(buf.empty());
//	ASSERT_EQ(buf.idx(), 0);

	io.flush();			// flush the write buffer

	cout << "partial write buf size: " << buf.size() << endl;
	cout << "partial write idx: " << buf.idx() << endl;

//	ASSERT_EQ(buf.str(), test_long.substr(0, 7));
//	ASSERT_EQ(buf.idx(), 0);

	for (unsigned i = 7; i < test_long.size(); i++)		io.put(test_long[i]);
	io.flush();
	cout << "full write buf size: " << buf.size() << endl;
	cout << "full write idx: " << buf.idx() << endl;

//	ASSERT_EQ(buf.str(), test_long);
//	ASSERT_EQ(buf.idx(), 0);

	string got;
	char ch;
	for (unsigned i = 0; i < 7 && io.get(ch); i++)		got.push_back(ch);
	cout << "partial read got: " << got << endl;
	cout << "partial read buf size: " << buf.size() << endl;
	cout << "partial read idx: " << buf.idx() << endl;

	io.sync();			// sync the read buffer 

	for (unsigned i = 0; i < 2 && io.get(ch); i++)		got.push_back(ch);

	cout << "sync & read 2 more got: " << got << endl;
	cout << "sync & read 2 more buf size: " << buf.size() << endl;
	cout << "sync & read 2 more idx: " << buf.idx() << endl;

	while (io.get(ch))			got.push_back(ch);

	cout << "full read got: " << got << endl;
	cout << "full read buf size: " << buf.size() << endl;
	cout << "full read idx: " << buf.idx() << endl;
}