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
#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include "encode/base64.h"

/** \addtogroup encode_base64
	@{ */
/** Tests for \ref encode_base64. \file */
/** \example scclib/encode/unittest/base64.cc
	Tests for \ref encode_base64. */
/** @} */

using namespace std;
using namespace scc::encode;

static string b64_allchar_enc =
"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4"
"OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3Bx"
"cnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmq"
"q6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj"
"5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==";

static string b64_test1 = "This is a test     with some space";
static string b64_test1_enc = "VGhpcyBpcyBhIHRlc3QgICAgIHdpdGggc29tZSBzcGFjZQ==";
static string b64_test2 = "     ";
static string b64_test2_enc = "ICAgICA=";
static string b64_test3 = "hit";
static string b64_test3_enc = "aGl0";

TEST(base64, zero)
{
	string z;
	ASSERT_EQ(str_to_base64(z).size(), 0);
	ASSERT_EQ(base64_to_str(z).size(), 0);
}

TEST(base64, teststrings)
{
	ASSERT_EQ(str_to_base64(b64_test1), b64_test1_enc);
	ASSERT_EQ(base64_to_str(b64_test1_enc), b64_test1);
	ASSERT_EQ(str_to_base64(b64_test2), b64_test2_enc);
	ASSERT_EQ(base64_to_str(b64_test2_enc), b64_test2);
	ASSERT_EQ(str_to_base64(b64_test3), b64_test3_enc);
	ASSERT_EQ(base64_to_str(b64_test3_enc), b64_test3);
}

TEST(base64, allchars)
{
	std::vector<char> allvect;
	std::string allstr;

	for (int i = 0; i < 256; i++)
	{
		allvect.push_back(i);
		allstr.push_back(i);
	}

	std::string s;
	base64_encode(allvect, s);
	cout << "allvect encoded: " << s << " size: " << s.size() << endl;
	ASSERT_EQ(s, b64_allchar_enc);

	s = str_to_base64(allstr);
	cout << "allstr encoded : " << s << " size: " << s.size() << endl;
	ASSERT_EQ(s, b64_allchar_enc);

	s = base64_to_str(b64_allchar_enc);
	ASSERT_EQ(s, allstr);

	std::vector<char> v;
	base64_decode(b64_allchar_enc, v);
	ASSERT_EQ(v, allvect);
}

TEST(base64, teststrings_url)
{

	auto urltest = [] (const string& encs)
	{
		return encs.find_first_of("+/=") == string::npos;
	};

	auto urlall= base64_to_base64url(b64_allchar_enc);
	cout << "all char base64:" << endl << b64_allchar_enc << endl;
	cout << "all char base64url:" << endl << urlall << endl;
	ASSERT_TRUE(urltest(urlall));
	auto baseall = base64url_to_base64(urlall);
	ASSERT_EQ(baseall, b64_allchar_enc);

	auto url1 = base64_to_base64url(b64_test1_enc);
	ASSERT_TRUE(urltest(url1));
	auto base1 = base64url_to_base64(url1);
	ASSERT_EQ(base1, b64_test1_enc);

	auto url2 = base64_to_base64url(b64_test2_enc);
	ASSERT_TRUE(urltest(url2));
	auto base2 = base64url_to_base64(url2);
	ASSERT_EQ(base2, b64_test2_enc);

	auto url3 = base64_to_base64url(b64_test3_enc);
	ASSERT_TRUE(urltest(url3));
	auto base3 = base64url_to_base64(url3);
	ASSERT_EQ(base3, b64_test3_enc);
}
