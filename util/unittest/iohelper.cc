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
#include "util/rwloopbuf.h"
#include "util/rwcounter.h"
#include "util/rwtimer.h"
#include "util/iopipeline.h"
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <thread>
#include "util/iostream.h"

/** \addtogroup util_iostream
	@{ */
/** Tests for \ref util_iohelpers
	\file */
/** \example scclib/util/unittest/iohelper.cc
	Tests for \ref util_iohelpers */
/** @} */

using namespace std;
using namespace scc::util;

static string val = "This is a test of the emergency RwLoopBuffer system!";
#define BUF_SZ 1024
static char buf[BUF_SZ];

TEST(RwLoopBuffer, sanity)
{
	RwLoopBuffer rw(val);
	ASSERT_EQ(rw.str(), val);

	rw = val;
	ASSERT_EQ(rw.read(buf, BUF_SZ), val.size());
	ASSERT_EQ(memcmp(buf, val.data(), val.size()), 0);

	rw.clear();
	ASSERT_EQ(rw.write(val.data(), val.size()), val.size());
	ASSERT_EQ(rw.str(), val);
}

TEST(RwLoopBuffer, read_empty)
{
	RwLoopBuffer rw;
	ASSERT_EQ(rw.read(buf, 0), 0);
	ASSERT_EQ(rw.read(buf, BUF_SZ/2), 0);
	ASSERT_EQ(rw.read(buf, BUF_SZ), 0);
}

TEST(RwLoopBuffer, Write_all_read_0)
{
	RwLoopBuffer rw;
	ASSERT_EQ(rw.write(val.data(), val.size()), val.size());
	ASSERT_EQ(rw.read(buf, 0), 0);
}

TEST(RwLoopBuffer, Write_all_read_max)
{
	cout << "val.size()=" << val.size() << endl;
	RwLoopBuffer rw;

	cout << "init readloc=" << rw.read_loc() << " writeloc=" << rw.write_loc() << endl;
	ASSERT_EQ(rw.read_loc(), 0);
	ASSERT_EQ(rw.write_loc(), 0);

	ASSERT_EQ(rw.write(val.data(), val.size()), val.size());
	cout << "after write readloc=" << rw.read_loc() << " writeloc=" << rw.write_loc() << endl;
	ASSERT_EQ(rw.read_loc(), 0);
	ASSERT_EQ(rw.write_loc(), val.size());

	ASSERT_EQ(rw.read(buf, BUF_SZ), val.size());
	cout << "after read readloc=" << rw.read_loc() << " writeloc=" << rw.write_loc() << endl;
	ASSERT_EQ(rw.read_loc(), val.size());
	ASSERT_EQ(rw.write_loc(), val.size());

	ASSERT_EQ(memcmp(buf, val.data(), val.size()), 0);

	ASSERT_EQ(rw.read(buf, BUF_SZ), 0);		// can't read any more
}

TEST(RwLoopBuffer, Write_chunks_read_chunks)
{
	RwLoopBuffer rw;

	auto ptr = (char*)val.data();
	int sz = val.size();
	do
	{
		int towrite = sz < 7 ? sz : 7;
		ASSERT_EQ(rw.write(ptr, towrite), towrite);
		ptr += towrite;
		sz -= towrite;
	}
	while (sz);

	ptr = buf;
	sz = val.size();
	do
	{
		auto got = rw.read(ptr, 11);
		ASSERT_GT(got, 0);
		ASSERT_LE(got, 11);
		ptr += got;
		sz -= got;
	}
	while (sz);
	
	ASSERT_EQ(memcmp(buf, val.data(), val.size()), 0);
}

TEST(RwLoopBuffer, clear)
{
	RwLoopBuffer rw;
	ASSERT_EQ(rw.write(val.data(), val.size()), val.size());
	rw.clear();
	ASSERT_EQ(rw.read(buf, BUF_SZ), 0);
}

TEST(RwCounter, counter_and_loop_buffer)
{
	RwLoopBuffer rwb;
	RwCounter rw(rwb, rwb);		// now provides read() and write() for the buffer

	ASSERT_EQ(rw.read_count(), 0);
	ASSERT_EQ(rw.write_count(), 0);

	ASSERT_EQ(rw.write(val.data(), val.size()), val.size());
	ASSERT_EQ(rw.read_count(), 0);
	ASSERT_EQ(rw.write_count(), val.size());

	ASSERT_EQ(rw.read(buf, BUF_SZ), val.size());
	ASSERT_EQ(memcmp(buf, val.data(), val.size()), 0);
	ASSERT_EQ(rw.read_count(), val.size());
	ASSERT_EQ(rw.write_count(), val.size());

	rw.read_count_reset();
	ASSERT_EQ(rw.read_count(), 0);
	ASSERT_EQ(rw.write_count(), val.size());

	rw.write_count_reset();
	ASSERT_EQ(rw.read_count(), 0);
	ASSERT_EQ(rw.write_count(), 0);
}

TEST(IoHelpers, counter_timer)
{
	//! [Counter and timer]
	using namespace std::chrono;

	struct Slow : public Reader, public Writer
	{
		Slow() {}
		virtual ~Slow() {}
		size_t read(void*, size_t l)
		{
			std::this_thread::sleep_for(milliseconds(50));
			return l;
		}
		size_t write(const void*, size_t l)
		{
			std::this_thread::sleep_for(milliseconds(100));
			return l;
		}
	};

	Slow base;
	RwCounter c(base, base);
	RwTimer t(c, c);
	t.read(0, 50);
	t.write(0, 100);
	cout << "read  timer=" << t.read_dur().count() << endl;
	cout << "write timer=" << t.write_dur().count() << endl;
	cout << "read  count=" << c.read_count() << endl;
	cout << "write count=" << c.write_count() << endl;

	ASSERT_EQ(duration_cast<milliseconds>(t.read_dur()).count(), 50);
	ASSERT_EQ(duration_cast<milliseconds>(t.write_dur()).count(), 100);
	ASSERT_EQ(c.read_count(), 50);
	ASSERT_EQ(c.write_count(), 100);
	//! [Counter and timer]
}

TEST(IoHelpers, chained_test)
{
	//! [Chained test]
	shared_ptr<RwLoopBuffer> rbuf(new RwLoopBuffer);

	shared_ptr<ReadCounter> rc(new ReadCounter(rbuf));

	shared_ptr<RwLoopBuffer> wbuf(new RwLoopBuffer);

	shared_ptr<WriteCounter> wc(new WriteCounter(wbuf));

	InStream in(rc);
	OutStream out(wc);

	ASSERT_EQ(rc.use_count(), 2);
	ASSERT_EQ(wc.use_count(), 2);
	ASSERT_EQ(rbuf.use_count(), 2);
	ASSERT_EQ(wbuf.use_count(), 2);

	string test("this is a test!");

	for (char ch : test)		out.put(ch);			// write to wbuf
	out.flush();

	cout << "wc count=" << wc->write_count() << endl;
	cout << "wbuf size=" << wbuf->size() << " idx=" << wbuf->idx() << " str=" << wbuf->str() << endl;

	ASSERT_EQ(test, wbuf->str());
	ASSERT_EQ(wc->write_count(), test.size());

	rbuf->set(test);

	string got;
	for (char ch; in.get(ch);)	got.push_back(ch);		// read from rbuf

	cout << "rc count=" << rc->read_count() << endl;
	cout << "rbuf size=" << rbuf->size() << " idx=" << rbuf->idx() << " str=" << rbuf->str() << endl;

	ASSERT_EQ(test, got);
	ASSERT_EQ(rc->read_count(), test.size());

	shared_ptr<RwLoopBuffer> wbuf2(new RwLoopBuffer);
	wc->write_reset(wbuf2);								// reset the write counter to send to a new buffer
	ASSERT_EQ(wc->write_count(), 0);

	ASSERT_EQ(wbuf2.use_count(), 2);

	wbuf.reset();										// safe to get rid of this now
	ASSERT_EQ(wbuf.use_count(), 0);

	for (char ch : test)		out.put(ch);			// write to rbuf2
	out.flush();

	cout << "wc count=" << wc->write_count() << endl;
	cout << "wbuf2 size=" << wbuf2->size() << " idx=" << wbuf2->idx() << " str=" << wbuf2->str() << endl;

	ASSERT_EQ(test, wbuf2->str());
	ASSERT_EQ(wc->write_count(), test.size());
	//! [Chained test]
}

static string twain =
	"One can survive everything, nowadays, except death, and live down everything except a good reputation.\n"
	"One should always play fairly when one has the winning cards.\n"
	"Patriotism is the virtue of the vicious.\n"
	"Selfishness is not living as one wishes to live, it is asking others to live as one wishes to live.";

struct IoPipelineTester : public testing::Test
{
	shared_ptr<RwLoopBuffer> buf;
	shared_ptr<IoPipeline> io;
	std::string got;

	IoPipelineTester()
	{
		buf.reset(new RwLoopBuffer);
		io.reset(new IoPipeline(buf, buf));
	}
	virtual ~IoPipelineTester() {}

	void read(unsigned n = UINT32_MAX)
	{
		char ch;
		for (unsigned i = 0; i < n && io->get(ch); i++)		got.push_back(ch);
	}
	void write(unsigned n = UINT32_MAX, bool flush=true)
	{
		for (unsigned i = 0; i < n && i < twain.size() && io->put(twain[i]); i++)	{}
		
		if (flush)		io->flush();
	}
	void pr(const string& msg)
	{
		cout << msg << endl;
		cout << "base rd ptr=" << (long)(Reader*)io->rd_base.get() << endl;
		cout << "base wr ptr=" << (long)(Writer*)io->wr_base.get() << endl;
		cout << "strm rd inp=" << (long)(Reader*)io->read_shared().get() << endl;
		cout << "strm wr inp=" << (long)(Writer*)io->write_shared().get() << endl;
	}
	void pr_chain()
	{
		cout << "read chain:" << endl;
		int n = 0;
		for (auto& i : io->rd_chain)
		{
			cout << "    ["<<n<<"]  ptr=" << (long)(Reader*)i.get() << endl;
			cout << "    ["<<n<<"]  inp=" << (long)(Reader*)i->read_shared().get() << endl;
			n++;
		}
		cout << "write chain:" << endl;
		n = 0;
		for (auto& i : io->wr_chain)
		{
			cout << "    ["<<n<<"]  ptr=" << (long)(Writer*)i.get() << endl;
			cout << "    ["<<n<<"]  inp=" << (long)(Writer*)i->write_shared().get() << endl;
			n++;
		}
	}

	void ver(void* a, void* b)
	{
		ASSERT_EQ(a, b);
	}
};

TEST_F(IoPipelineTester, basic)
{
	pr("*init");
	write();
	pr("*after write");
	ASSERT_EQ(buf->str(), twain);
	read();
	pr("*after read");
	ASSERT_EQ(got, twain);
	ver(io->rd_base.get(), (Reader*)buf.get());
	ver(io->wr_base.get(), (Writer*)buf.get());
	ver(io->rd_base.get(), io->read_shared().get());
	ver(io->wr_base.get(), io->write_shared().get());
}

TEST_F(IoPipelineTester, replace_base)
{
	write();
	ASSERT_EQ(buf->str(), twain);
	read();
	pr("*after read");
	ASSERT_EQ(got, twain);

	shared_ptr<RwLoopBuffer> buf2(new RwLoopBuffer);
	io->rd_base = buf2;
	io->rd_fix_chain();
	io->wr_base = buf2;
	io->wr_fix_chain();

	io->clear();
	got.clear();
	write();
	read();
	pr("*after read to buf2");
	ASSERT_EQ(got, twain);

	ver(io->rd_base.get(), (Reader*)buf2.get());
	ver(io->wr_base.get(), (Writer*)buf2.get());
	ver(io->rd_base.get(), io->read_shared().get());
	ver(io->wr_base.get(), io->write_shared().get());
}

TEST_F(IoPipelineTester, add_and_delete)
{
	shared_ptr<RwCounter> x(new RwCounter(buf, buf));
	shared_ptr<RwTimer> y(new RwTimer(buf, buf));

	io->rw_add_back(x, x);
	io->rw_add_back(y, y);

	pr("*added two elements to chain");
	pr_chain();

	write();
	read();
	cout << "*after read" << endl;
	cout << "count calls r=" << x->read_calls() << " w=" << x->write_calls() << endl;
	cout << "time  calls r=" << y->read_calls() << " w=" << y->write_calls() << endl;

	ASSERT_EQ(x->read_calls(), 2);		// one for the eof
	ASSERT_EQ(x->write_calls(), 1);
	ASSERT_EQ(y->read_calls(), 2);
	ASSERT_EQ(y->write_calls(), 1);

	ASSERT_EQ(got, twain);

	ASSERT_EQ(io->rd_chain.size(), 2);
	ver(io->rd_base.get(), (Reader*)buf.get());
	ver(io->rd_chain.back()->read_shared().get(), (Reader*)io->rd_base.get());
	ver(io->rd_chain.front()->read_shared().get(), (Reader*)io->rd_chain.back().get());
	ver(io->read_shared().get(), (Reader*)io->rd_chain.front().get());

	ASSERT_EQ(io->wr_chain.size(), 2);
	ver(io->wr_base.get(), (Writer*)buf.get());
	ver(io->wr_chain.back()->write_shared().get(), (Writer*)io->wr_base.get());
	ver(io->wr_chain.front()->write_shared().get(), (Writer*)io->wr_chain.back().get());
	ver(io->write_shared().get(), (Writer*)io->wr_chain.front().get());

	io->rd_del(x);
	io->wr_del(y);
	pr("*remove a reader and writer");
	pr_chain();

	x->read_calls_reset();
	x->write_calls_reset();
	y->read_calls_reset();
	y->write_calls_reset();
	ASSERT_EQ(x->read_calls(), 0);
	ASSERT_EQ(x->write_calls(), 0);
	ASSERT_EQ(y->read_calls(), 0);
	ASSERT_EQ(y->write_calls(), 0);

	io->clear();
	buf->clear();
	got.clear();
	write();
	read();

	ASSERT_EQ(got, twain);

	cout << "*after remove" << endl;
	cout << "count calls r=" << x->read_calls() << " w=" << x->write_calls() << endl;
	cout << "time  calls r=" << y->read_calls() << " w=" << y->write_calls() << endl;

	ASSERT_EQ(x->read_calls(), 0);
	ASSERT_EQ(x->write_calls(), 1);
	ASSERT_EQ(y->read_calls(), 2);
	ASSERT_EQ(y->write_calls(), 0);
	ASSERT_EQ(io->rd_chain.size(), 1);
	ASSERT_EQ(io->wr_chain.size(), 1);

	io->wr_del(x);
	io->rd_del(y);
	pr("*remove all");
	pr_chain();

	x->read_calls_reset();
	x->write_calls_reset();
	y->read_calls_reset();
	y->write_calls_reset();

	io->clear();
	buf->clear();
	got.clear();
	write();
	read();

	ASSERT_EQ(got, twain);

	cout << "*after remove all" << endl;
	cout << "count calls r=" << x->read_calls() << " w=" << x->write_calls() << endl;
	cout << "time  calls r=" << y->read_calls() << " w=" << y->write_calls() << endl;

	ASSERT_EQ(x->read_calls(), 0);
	ASSERT_EQ(x->write_calls(), 0);
	ASSERT_EQ(y->read_calls(), 0);
	ASSERT_EQ(y->write_calls(), 0);
	ASSERT_EQ(io->rd_chain.size(), 0);
	ASSERT_EQ(io->wr_chain.size(), 0);

	x.reset();
	y.reset();

	ASSERT_EQ(x.use_count(), 0);
	ASSERT_EQ(y.use_count(), 0);
}
