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
#include <crypto/uuid.h>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

/** \addtogroup crypto_uuid
	@{ */
/** Tests for \ref crypto_uuid \file */
/** \example crypto/unittest/uuid.cc */
/** @} */

using std::cout;
using std::endl;
using std::string;
using scc::crypto::Uuid;

static string valid = "e70ef568-0c08-461f-5082-ae0f702508ea";
static string valid_upper = "E70ef568-0c08-461f-5082-ae0f702508eA";
static string invalid_form = "e70ef568-0c08-461f5082-ae0f702508ea";
static string invalid_data = "e70ef568-0c08-461f-5082-INVALIDDATA";
static string invalid_len = "e70ef568-0c08-461f-5082-ae0f702508ea-abcdefg";

TEST(uuid_test, zero)
{
	ASSERT_EQ(Uuid::zero.size(), 36);
}

TEST(uuid_test, generate_uuid)
{
	Uuid u;
	cout << "Uuid: " << u << endl;

	ASSERT_EQ(u.val().size(), 36);
	ASSERT_NE(u, Uuid::zero);
}

TEST(uuid_test, assign)
{
	Uuid u(valid);
	ASSERT_EQ(u, valid);
	ASSERT_NE(u, Uuid::zero);
	Uuid v(u);
	ASSERT_EQ(u, v);
	v = valid_upper;
	ASSERT_EQ(u, v);
	cout << "orig   : " << valid_upper << endl;
	cout << "stored : " << v << endl;

	u = invalid_form;
	ASSERT_EQ(u, Uuid::zero);
	u = invalid_data;
	ASSERT_EQ(u, Uuid::zero);
	u = invalid_len;
	ASSERT_EQ(u, Uuid::zero);
}
