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
#include "util/logger.h"
#include <gtest/gtest.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <list>
#include "util/fs.h"

/** \addtogroup util_logger
	@{ */
/** Tests for \ref util_logger \file
*/
/** \example scclib/util/unittest/logger.cc
	Tests for \ref util_logger */
/** @} */

using namespace std;
using fs = scc::util::Filesystem;
using scc::util::Logger;

struct LoggerTest : public testing::Test
{
	shared_ptr<stringstream> strings;
	string fname;
	shared_ptr<ofstream> files;
	Logger log;

	string m_curdir;
	LoggerTest()
	{
		system_error err;
		m_curdir = fs::get_current_dir();
		fs::remove_all("sandbox", &err);
		fs::create_dir("sandbox");
		fs::change_dir("sandbox");

		strings.reset(new stringstream());
		fname = "ofstream_test";
		files.reset(new ofstream(fname));

		log.add_cout();
		log.add(strings);
		log.add(files);
	}
	virtual ~LoggerTest()
	{
		fs::change_dir(m_curdir);
		system_error err;
		fs::remove_all("sandbox", &err);
	}

	std::string read_file()
	{
		ifstream f(fname);
		// load the file contents using an iterator to the ifstream's streambuf
		// the default-constructed iterator is the stream end
		// see: https://en.cppreference.com/w/cpp/iterator/istreambuf_iterator
		istreambuf_iterator<char> it{f}, end;
		string ss{it, end};
		return ss;
	}

	void validate(const string& logs)
	{
		ASSERT_EQ(strings->str(), logs);
		ASSERT_EQ(read_file(), logs);
	}
};

TEST(LoggerTestBasic, move_and_construct)		// basic test not attached to the test class
{
	stringstream ver;

	auto s = make_shared<stringstream>();
	Logger log(s);
	log.add_cout();
	
	log.add(s);

	log << "Original 1" << endl;
	ver << "Original 1" << endl;

	Logger cc(log);
	cc << "Copy constructed" << endl;
	ver << "Copy constructed" << endl;
	log << "Original 2" << endl;
	ver << "Original 2" << endl;

	Logger ca = log;
	ca << "Copy assigned" << endl;
	ver << "Copy assigned" << endl;
	log << "Original 3" << endl;
	ver << "Original 3" << endl;

	Logger mc(std::move(cc));
	mc << "Move constructed" << endl;
	ver << "Move constructed" << endl;
	cc << "Original 4 (won't see this)" << endl;

	Logger ma = std::move(ca);
	ma << "Move assigned" << endl;
	ver << "Move assigned" << endl;
	ca << "Original 5 (won't see this)" << endl;

	cout << "** start readback from log stringstream" << endl;
	cout << s->str();
	cout << "** end readback from log stringstream" << endl;

	cout << "** start readback from verify stringstream" << endl;
	cout << ver.str();
	cout << "** end readback from verify stringstream" << endl;
	
	ASSERT_EQ(s->str(), ver.str());
}

TEST_F(LoggerTest, id)
{
	log.id("test");
	log << "line" << endl;
	validate("[test] line\n");
}

TEST_F(LoggerTest, timestamp)
{
	log.timestamp_iso();
	log << "line" << endl;

	ASSERT_EQ(strings->str().size(), 26);
	ASSERT_EQ(strings->str().substr(20), " line\n");
}

TEST_F(LoggerTest, multiline)
{
	stringstream s;
	log.multiline(5);
	log.id(1);

	log << "m 1" << endl;
	s << "[1] m 1" << endl;
	log << " m 2" << endl;
	s << " m 2" << endl;
	log << " m 3" << endl;
	s << " m 3" << endl;
	log << " m 4" << endl;
	s << " m 4" << endl;
	log << " m 5" << endl;
	s << " m 5" << endl;
	log << " m 6" << endl;
	s << "[1]  m 6" << endl;

	validate(s.str());
}

TEST_F(LoggerTest, multithread_multi_loggers)
{
	//! [Multithreaded log test]

	/*
		The test class has the following global objects
		
		shared_ptr<stringstream> strings;
		shared_ptr<ofstream> files;
		Logger log;						// logger with strings, files, and cout attached

		This test will create 5 threads, each thread create a logger and attach to the three streams.

		Each thread will write 5 log strings (with id set to the thread number)

		The main thread will write 5 log strings (with id set to 0).
	*/

	auto f = [&](int n)
	{
		Logger tlog(strings);
		tlog.add(files);
		tlog.add_cout();
		tlog.id(n);
		for (int i = 1; i <= 5; i++)
		{
			tlog << "this is log number " << i << " from thread number " << n << endl;
		}
	};

	vector<thread> tv;

	for (int n = 1; n <= 5; n++)
	{
		tv.emplace_back(f, n);
	}

	log.id(0);
	for (int i = 1; i <= 5; i++)
	{
		log << "this is log number " << i << " from thread number " << 0 << endl;
	}

	for (auto& t : tv)
	{
		t.join();
	}

	/*
		Read the lines back and put them in order
	*/

	string line;
	set<string> ordered_strings;
	while (getline(*strings, line))
	{
		ordered_strings.insert(line);
	}

	stringstream fs(read_file());
	set<string> ordered_files;
	while (getline(fs, line))
	{
		ordered_files.insert(line);
	}

	cout << "* ordered stringstream" << endl;
	for (auto& s : ordered_strings)
	{
		cout << s << endl;
	}
	cout << "* ordered filestream" << endl;
	for (auto& s : ordered_strings)
	{
		cout << s << endl;
	}

	ASSERT_EQ(ordered_strings, ordered_files);

	int cur_thread = 0;
	int cur_line = 1;

	cout << "validate the ordered logs:" << endl;
	for (auto& s : ordered_strings)
	{
		cout << s << endl;

		stringstream val;

		val << "[" << cur_thread << "] this is log number " << cur_line << " from thread number " << cur_thread;

		ASSERT_EQ(val.str(), s);

		cur_line++;
		if (cur_line > 5)
		{
			cur_thread++;
			cur_line = 1;
		}
	}

	/*
		Make sure we found all the lines
	*/

	ASSERT_EQ(cur_thread, 6);
	ASSERT_EQ(cur_line, 1);
	//! [Multithreaded log test]
}
