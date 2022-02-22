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
#include "util/fs.h"
#include <gtest/gtest.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <algorithm>
#include <fstream>
#include <fcntl.h>
#include "util/safe_clib.h"
#include "util/filedesc.h"

/** \addtogroup util_fs
	@{ */
/** Tests for \ref util_fs \file
*/
/** \example scclib/util/unittest/fs.cc
	Tests for \ref util_fs */
/** @} */

using namespace std;
using fs = scc::util::Filesystem;
using scc::util::FileType;

struct FsTest : public testing::Test
{
	string m_curdir;
	FsTest()
	{
		system_error err;
		m_curdir = fs::get_current_dir();
		fs::remove_all("sandbox", &err);
		fs::create_dir("sandbox");
	}
	virtual ~FsTest()
	{
		fs::change_dir(m_curdir);
		system_error err;
		fs::remove_all("sandbox", &err);
	}
	void create_files()
	{
		fs::create_reg("sandbox/reg");
		fs::create_symlink("reg", "sandbox/link");
		fs::create_link("sandbox/reg", "sandbox/reg2");
		fs::create_fifo("sandbox/fifo");
		struct sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		std::strcpy(addr.sun_path, "sandbox/sock");
		int fd = socket(PF_UNIX, SOCK_STREAM, 0);
		bind(fd, (struct sockaddr*)&addr, sizeof addr);
		scc::util::safe_close(fd);
	}
};

TEST_F(FsTest, list_current)
{
	cout << "listing current directory contents" << endl;
	auto files = fs::scan_dir(".");
	ASSERT_GT(files.size(), 0);
	for (auto& f : files)
	{
		cout << f.first << " type: " << f.second << endl;
	}
}

TEST_F(FsTest, list_sandbox)
{
	cout << "creating sandbox directory files" << endl;
	create_files();
	cout << "listing sandbox directory contents" << endl;
	auto files = fs::scan_dir("sandbox");
	ASSERT_GT(files.size(), 0);
	for (auto& f : files)
	{
		cout << f.first << " type: " << f.second << endl;
	}
}

//! [Scan directory]
static bool reg_filt(const string& s, FileType t)
{
	if (s == "reg" && t == FileType::reg)
		return true;
	return false;
}


TEST_F(FsTest, filter)
{
	create_files();
	auto files = fs::scan_dir("sandbox", reg_filt);
	ASSERT_EQ(files.size(), 1);
	ASSERT_EQ(files.begin()->first, "reg");
//	ASSERT_EQ(files.begin()->second, FileType::reg);  // some problem googletest has with ostream conversion...
	ASSERT_TRUE(files.begin()->second == FileType::reg);
}

//! [Scan directory]

TEST_F(FsTest, create_and_delete)
{
	ASSERT_THROW(fs::create_dir("sandbox"), system_error);
	fs::create_reg("sandbox/test");
	auto n = fs::create_tmp_reg("sandbox/test");
	cout << "tmp file: " << n << endl;
	auto d = fs::scan_dir("sandbox");
	for (auto& f : d)
	{
		cout << f.first << " type: " << f.second << endl;
	}
	ASSERT_EQ(d.size(), 2);
	ASSERT_NE(d.find("test"), d.end());

	fs::remove(n);
	d = fs::scan_dir("sandbox");
	ASSERT_EQ(d.size(), 1);
	ASSERT_THROW(fs::remove(n), system_error);

	fs::remove_all("sandbox");
	d = fs::scan_dir(".");
	ASSERT_EQ(d.find("sandbox"), d.end());

	ASSERT_THROW(fs::remove("sandbox"), system_error);
}

TEST_F(FsTest, create_types)
{
	create_files();
	
	fs::change_dir("sandbox");
	auto d = fs::scan_dir(".");
	for (auto& f : d)
	{
		cout << "sandbox/" << f.first << " type: " << f.second << ":" << endl;
		auto ty = fs::file_stat(f.first);
		cout << ty << endl;
		ASSERT_TRUE(f.second == ty.type);
	}

	ASSERT_EQ(fs::read_symlink("link"), "reg");

	auto ty = fs::file_stat("/dev/null");
	cout << "/dev/zero " << ty << endl;
	ASSERT_TRUE(ty.type == FileType::chr);

	fs::change_dir("/dev");
	d = fs::scan_dir(".");
	auto i = std::find_if(d.begin(), d.end(), [](const auto& x) -> bool
	{
		return x.second == FileType::block;
	});
	ASSERT_NE(i, d.end());
	ty = fs::file_stat(i->first);
	cout << "/dev/" << i->first << " " << ty << endl;
	ASSERT_TRUE(ty.type == FileType::block);
}

TEST_F(FsTest, attributes)
{
	fs::change_dir("sandbox");

	fs::create_reg("reg");
	fs::set_mode("reg", 0660);
	fs::create_symlink("reg", "link");
	fs::create_reg("reg2");

	const uint64_t NS = 1000000000;
	const uint64_t DAY = 24*60*60*NS;

	auto st = fs::file_stat("reg");
	fs::set_times("reg2", st.access_time-DAY, st.mod_time-2*DAY);
	//fs::set_times("link", st.access_time-3*DAY, st.mod_time-4*DAY);

	auto d = fs::scan_dir(".");
	for (auto& f : d)
	{
		cout << "sandbox/" << f.first << " type: " << f.second << ":" << endl;
		auto ty = fs::file_stat(f.first);
		cout << ty << endl;
	}

	auto streg = fs::file_stat("reg");
	ASSERT_EQ(streg.mode, 0660);
	
	auto streg2 = fs::file_stat("reg2");
	ASSERT_EQ(streg2.mode, 0600);
	ASSERT_EQ(streg2.access_time, streg.access_time-DAY);
	ASSERT_EQ(streg2.mod_time, streg.mod_time-2*DAY);

	auto stlink = fs::file_stat("link");
	ASSERT_EQ(stlink.mode, 0777);

	// NOTE: can't test the uid / gid settings easily...

}

//! [Sparse file]
TEST(FsExampleTest, sparse_and_trunc)
{
	using namespace scc::util;

	system_error err;
	fs::remove_all("sandbox", &err);
	fs::create_dir("sandbox");
	auto curdir = fs::get_current_dir();
	cout << "curdir: " << curdir << endl;

	fs::change_dir("sandbox");

	string s("this is a test of the emergency sizing system\n");
	cout << "sparse test with string len=" << s.size() << endl;

	string fn("sparse_file");

	auto write_sparse = [&fn, &s](off_t loc)
	{
		// use clib to write data into the middle of the file
		// if we use fstream, the system may truncate the file

		// use system wrapper FileDesc to ensure the file is always closed
		scc::util::FileDesc fd(safe_open_throw(fn.c_str(), O_WRONLY));
		
		lseek(fd, loc, SEEK_SET);
		safe_write_throw(fd, &s[0], s.size());
		
		cout << "wrote " << s.size() << " bytes at loc " << loc << endl;
	};

	fs::create_reg(fn);
	fs::set_size(fn, 16384);			// 16 K file, with a 0 at the end
	
	auto st = fs::file_stat(fn);
	cout << "12288 hole at start of file:\n" << st << endl;

	ASSERT_EQ(st.size, 16384);
	ASSERT_EQ(st.alloc_size, 4096);		// the last block has data

	auto sm = fs::sparse_map(fn);
	cout << setw(6) << "start" << setw(6) << "data" << endl;
	for (auto& x : sm)
	{
		cout << setw(6) << x.first << setw(6) << x.second << endl;
	}
	
	std::map<int64_t,int64_t> ver;
	
	ver[0] = 12287;						// 12288 sized hole at 0 (beginning)
	ASSERT_EQ(sm, ver);

	write_sparse(1024);					// write the string at 1024, causing first block to be written
	
	st = fs::file_stat(fn);
	cout << "8192 hole in middle of file\n" << st << endl;

	ASSERT_EQ(st.alloc_size, 8192);

	sm = fs::sparse_map(fn);
	cout << setw(6) << "start" << setw(6) << "data" << endl;
	for (auto& x : sm)
	{
		cout << setw(6) << x.first << setw(6) << x.second << endl;
	}

	ver.clear();
	ver[4096] = 12287;					// 8192 sized hole at 4096 (middle)
	ASSERT_EQ(sm, ver);

	fs::set_size(fn, 12288);

	st = fs::file_stat(fn);
	cout << "8192 hole at end of file\n" << st << endl;

	ASSERT_EQ(st.alloc_size, 4096);

	sm = fs::sparse_map(fn);
	cout << setw(6) << "start" << setw(6) << "data" << endl;
	for (auto& x : sm)
	{
		cout << setw(6) << x.first << setw(6) << x.second << endl;
	}

	ver.clear();
	ver[4096] = 12287;					// 8192 sized hole at 4096 (end)
	ASSERT_EQ(sm, ver);

	ifstream f(fn);
	f.seekg(1024);
	string val;
	val.resize(s.size(), '\x01');
	f.read(&val[0], val.size());
	ASSERT_EQ(val, s);

	fs::change_dir(curdir);
	fs::remove_all("sandbox");
}
//! [Sparse file]

TEST_F(FsTest, paths)
{
	auto t = [](const std::string& b, const std::string& p) -> std::string
	{
		auto x = fs::norm_path(b, p);
		cout << "base:" << ">" << b << "< path: >" << p << "< norm: >" << x << "<" << endl;
		return x;
	};

	ASSERT_EQ(t("", ""), ".");
	ASSERT_EQ(t("", "."), ".");
	ASSERT_EQ(t(".", ""), ".");
	ASSERT_EQ(t(".", "."), ".");

	ASSERT_EQ(t("", "test/.."), ".");
	ASSERT_EQ(t("", "test/../.."), "./..");

	ASSERT_EQ(t("", "/"), "/");
	ASSERT_EQ(t("", "/test"), "/test");
	ASSERT_EQ(t("", "/test/"), "/test/");
	ASSERT_EQ(t("/", ""), "/");
	ASSERT_EQ(t("/", "/"), "/");

	ASSERT_EQ(t("base", "../../sandbox/../../path"), "./../../path");

	ASSERT_EQ(t("/base/", ""), "/base/");
	ASSERT_EQ(t("/base/", "."), "/base");
	ASSERT_EQ(t("/base/", "root"), "/base/root");
	ASSERT_EQ(t("/base", "root"), "/base/root");
	ASSERT_EQ(t("/base/", "sandbox/rel/path"), "/base/sandbox/rel/path");
	ASSERT_EQ(t("/base/sandbox", "../sandbox/path"), "/base/sandbox/path");
	ASSERT_EQ(t("/base/next/../again", "sandbox/next/../path"), "/base/again/sandbox/path");

	ASSERT_EQ(t("", "sandbox/rel/path"), "./sandbox/rel/path");
	ASSERT_EQ(t("", "sandbox/rel/../path"), "./sandbox/path");
	ASSERT_EQ(t("", "./sandbox/rel/../path"), "./sandbox/path");
	ASSERT_EQ(t("", "/sandbox/path"), "/sandbox/path");
	ASSERT_EQ(t("", "/sandbox/path/"), "/sandbox/path/");
	ASSERT_EQ(t("", "/sandbox/path"), "/sandbox/path");
	ASSERT_EQ(t("", "/base/../sandbox/next/../../path"), "/path");
	ASSERT_EQ(t("", "/this/../is/a/big/long/path/../../../../../"), "/");

	ASSERT_EQ(t("", "/this/../is/a/path/../../../../."), "/..");
	ASSERT_EQ(t("../", "../../sandbox/../../"), "./../../../../");
}
