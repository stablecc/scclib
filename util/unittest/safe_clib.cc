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
#include "util/safe_clib.h"

/** \addtogroup util_safeclib
	@{
*/

/** Test file for \ref util_safeclib
	\file scclib/util/unittest/safe_clib.cc
*/

/** @} */

using std::cout;
using std::endl;
using namespace scc::util;

TEST(safe_unistd, invalid)
{
	int invalid = 87743;
	int r;

	ASSERT_NO_THROW(r = safe_close(invalid));
	ASSERT_EQ(r, -1);
	ASSERT_THROW(safe_close_throw(invalid), std::system_error);
	ASSERT_NO_THROW(r = safe_dup(invalid));
	ASSERT_EQ(r, -1);
	ASSERT_THROW(safe_dup_throw(invalid), std::system_error);
	ASSERT_NO_THROW(r = safe_dup2(invalid, invalid));
	ASSERT_EQ(r, -1);
	ASSERT_THROW(safe_dup2_throw(invalid, invalid), std::system_error);

	ssize_t s;
	ASSERT_NO_THROW(s = safe_read(invalid, nullptr, 123));
	ASSERT_EQ(s, -1);
	ASSERT_THROW(safe_read_throw(invalid, nullptr, 123), std::system_error);
	ASSERT_NO_THROW(s = safe_write(invalid, nullptr, 123));
	ASSERT_EQ(s, -1);
	ASSERT_THROW(safe_write_throw(invalid, nullptr, 123), std::system_error);
}
