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
#include <crypto/secvec.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <util/fs.h>

/** \addtogroup crypto_secvec
	@{ */
/** Tests for \ref crypto_secvec \file */
/** \example crypto/unittest/secvec.cc */
/** @} */

using std::cout;
using std::endl;
using std::system_error;
using std::runtime_error;
using std::fstream;
using std::ofstream;
using std::ifstream;
using std::vector;
using std::string;
using std::ios;
using scc::crypto::SecVecChar;
using scc::crypto::SecVecUchar;
using fs = scc::util::Filesystem;

TEST(secure_vector_test, char_sanity)
{
	SecVecChar v;
	ASSERT_EQ(v.size(), 0);
	SecVecChar v2(10);
	ASSERT_EQ(v2.size(), 10);
	SecVecChar v3(10, '\1');
	ASSERT_EQ(v3.size(), 10);
	vector<char> t;
	for (unsigned i = 0; i < v3.size(); i++)		t.push_back('\1');
	for (unsigned i = 0; i < v3.size(); i++)		ASSERT_EQ(v3[i], t[i]);

	v3.resize(5);
	ASSERT_EQ(v3.size(), 5);
	t.resize(5);
	for (unsigned i = 0; i < v3.size(); i++)		ASSERT_EQ(v3[i], t[i]);

	v3.resize(10, '\2');
	ASSERT_EQ(v3.size(), 10);
	t.resize(10, '\2');
	for (unsigned i = 0; i < v3.size(); i++)		ASSERT_EQ(v3[i], t[i]);

	v3.clear();
	ASSERT_EQ(v3.size(), 0);
}

TEST(secure_vector_test, uchar_sanity)
{
	SecVecUchar v;
	ASSERT_EQ(v.size(), 0);
	SecVecUchar v2(10);
	ASSERT_EQ(v2.size(), 10);
	SecVecUchar v3(10, '\1');
	ASSERT_EQ(v3.size(), 10);
	vector<unsigned char> t;
	for (unsigned i = 0; i < v3.size(); i++)		t.push_back('\1');
	for (unsigned i = 0; i < v3.size(); i++)		ASSERT_EQ(v3[i], t[i]);

	v3.resize(5);
	ASSERT_EQ(v3.size(), 5);
	t.resize(5);
	for (unsigned i = 0; i < v3.size(); i++)		ASSERT_EQ(v3[i], t[i]);

	v3.resize(10, '\2');
	ASSERT_EQ(v3.size(), 10);
	t.resize(10, '\2');
	for (unsigned i = 0; i < v3.size(); i++)		ASSERT_EQ(v3[i], t[i]);

	v3.clear();
	ASSERT_EQ(v3.size(), 0);
}

static string plaintext = "\
To be, or not to be, that is the question: \n\
Whether 'tis nobler in the mind to suffer \n\
The slings and arrows of outrageous fortune, \n\
Or to take Arms against a Sea of troubles, \n\
And by opposing end them: to die, to sleep; \n\
No more; and by a sleep, to say we end \n\
The heart-ache, and the thousand natural shocks \n\
That Flesh is heir to? 'Tis a consummation \n\
Devoutly to be wished. To die, to sleep, \n\
perchance to Dream; aye, there's the rub...\n";

struct SecVecCharTest : public testing::Test
{
	string fname;
	string tname;
	SecVecChar plainsv;

	SecVecCharTest() : fname("testfile.txt"), tname("testfile2.txt")
	{
		ofstream file(fname, ios::out|ios::trunc);
		
		if (!file.is_open())
		{
			throw runtime_error("could not open for writing");
		}

		file << plaintext;
		file.flush();
		file.close();

		for (auto ch : plaintext)
		{
			plainsv.push_back(ch);
		}
	}
	virtual ~SecVecCharTest()
	{
		system_error err;
		fs::remove(fname, &err);
		fs::remove(tname, &err);
	}
};

TEST_F(SecVecCharTest, read_from_file)
{
	SecVecChar sv;
	ifstream f(fname);
	f >> sv;

	string ver(sv.begin(), sv.end());

	cout << "<start plaintext>";
	cout << ver;
	cout << "<end plaintext>" << endl;

	ASSERT_EQ(plaintext, ver);
	ASSERT_EQ(plainsv, sv);
}

TEST_F(SecVecCharTest, write_to_file)
{
	ofstream file(tname);

	file << plainsv;
	file.close();

	ifstream t(tname);
	string s;
	char ch;
	while (t.get(ch))
	{
		s.push_back(ch);
	}
	ASSERT_EQ(s, plaintext);
}

TEST_F(SecVecCharTest, bad_read)
{
	ifstream f("nothere");
	SecVecChar sv;
	ASSERT_THROW(f >> sv, runtime_error);
}

TEST_F(SecVecCharTest, bad_write)
{
	fstream file(fname, ios::in);		// open read only
	if (!file.is_open())
	{
		throw runtime_error("could not open test file for reading");
	}

	ASSERT_THROW(file << plainsv, runtime_error);
}
