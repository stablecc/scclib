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
#include <cstring>
#include <iostream>
#include <cstdio>
#include "util/filedesc.h"

/** \addtogroup util_filedesc
	@{ */
/** Tests for \ref util_filedesc \file */
/** \example scclib/util/unittest/filedesc.cc
	Tests for \ref util_filedesc */
/** @} */

using std::cout;
using std::endl;
using scc::util::FileDesc;

void pr(int fd)
{
	cout << "fd: " << static_cast<int>(fd) << endl;
}

TEST(FileDesc_test, nil)
{
	FileDesc fd;
	ASSERT_EQ(static_cast<int>(fd), -1);
	pr(fd);
}

TEST(FileDesc_test, dup_stdout)
{
	cout << "dup stdout:" << endl;
	FileDesc fd;
	cout << "fd: " << (int)fd << endl;
	dprintf(1, "print to stdout\n");
	fd.dup(1);	// stdout
	cout << "fd: " << (int)fd << endl;
	dprintf(fd, "print to dup\n");
	ASSERT_NE(static_cast<int>(fd), -1);
	ASSERT_NE(static_cast<int>(fd), 1);
	dprintf(1, "print to stdout again\n");
}

TEST(FileDesc_test, copy_construct)
{
	cout << "copy construct:" << endl;
	FileDesc fd;
	cout << "fd: " << (int)fd << endl;
	dprintf(1, "print to stdout\n");
	fd.dup(1);	// stdout
	cout << "fd: " << (int)fd << endl;
	dprintf(fd, "print to dup\n");
	FileDesc fd2(fd);
	cout << "fd: " << (int)fd << endl;
	cout << "fd2: " << (int)fd2 << endl;
	dprintf(fd2, "print to dup2\n");
	ASSERT_NE(static_cast<int>(fd), -1);
	ASSERT_NE(static_cast<int>(fd2), -1);
	ASSERT_NE(static_cast<int>(fd), static_cast<int>(fd2));
}

TEST(FileDesc_test, copy)
{
	cout << "copy:" << endl;
	FileDesc fd, fd2;
	cout << "fd: " << (int)fd << endl;
	dprintf(1, "print to stdout\n");
	fd.dup(1);	// stdout
	cout << "fd: " << (int)fd << endl;
	dprintf(fd, "print to dup\n");
	fd2 = fd;
	cout << "fd: " << (int)fd << endl;
	cout << "fd2: " << (int)fd2 << endl;
	dprintf(fd2, "print to dup2\n");
	ASSERT_NE(static_cast<int>(fd), -1);
	ASSERT_NE(static_cast<int>(fd2), -1);
	ASSERT_NE(static_cast<int>(fd), static_cast<int>(fd2));
}

TEST(FileDesc_test, move_construct)
{
	cout << "move construct:" << endl;
	FileDesc fd;
	cout << "fd: " << (int)fd << endl;
	dprintf(1, "print to stdout\n");
	fd.dup(1);	// stdout
	cout << "fd: " << (int)fd << endl;
	dprintf(fd, "print to dup\n");
	FileDesc fd2 = std::move(fd);
	cout << "fd: " << (int)fd << endl;
	cout << "fd2: " << (int)fd2 << endl;
	dprintf(fd2, "print to dup2\n");
	ASSERT_EQ(static_cast<int>(fd), -1);
	ASSERT_NE(static_cast<int>(fd2), -1);
}

TEST(FileDesc_test, move)
{
	cout << "move:" << endl;
	FileDesc fd, fd2;
	cout << "fd: " << (int)fd << endl;
	dprintf(1, "print to stdout\n");
	fd.dup(1);	// stdout
	cout << "fd: " << (int)fd << endl;
	dprintf(fd, "print to dup\n");
	fd2 = std::move(fd);
	cout << "fd: " << (int)fd << endl;
	cout << "fd2: " << (int)fd2 << endl;
	dprintf(fd2, "print to dup2\n");
	ASSERT_EQ(static_cast<int>(fd), -1);
	ASSERT_NE(static_cast<int>(fd2), -1);
}
