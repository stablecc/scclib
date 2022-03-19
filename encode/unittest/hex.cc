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
#include <encode/hex.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>

/** \addtogroup encode_hex
	@{ */
/** Tests for \ref encode_hex \file */
/** \example encode/unittest/hex.cc */
/** @} */

using CharVec = std::vector<char>;

using std::string;
using std::cout;
using std::endl;
using scc::encode::Hex;

TEST(hex, hex_to_bin_all)
{
	CharVec allbin(256, 0);
	for (int i = 0; i < 256; i++)
	{
		allbin[i] = (char)i;
	}
	string hex;
	Hex::bin_to_hex(allbin, hex);
	cout << "Hex: " << hex << endl;
	ASSERT_EQ(hex.size(), 512);

	CharVec newbin;
	Hex::hex_to_bin(hex, newbin);
	ASSERT_EQ(newbin.size(), 256);
	ASSERT_EQ(allbin, newbin);

	cout << "bin to hex string:" << endl;
	cout << Hex::bin_to_hexstr(allbin, ":", 20) << endl;
}

TEST(hex, zero)
{
	string z, s;
	Hex::bin_to_hex(CharVec(z.begin(), z.end()), s);
	ASSERT_EQ(s.size(), 0);
}

TEST(hex, bin_to_hex)
{
	string bin = "this is a test";
	string hex1 = "7468697320697320612074657374";
	string hex2;
	Hex::bin_to_hex(CharVec(bin.begin(), bin.end()), hex2);
	ASSERT_EQ(hex1, hex2);
}

TEST(hex, hex_to_bin_part)
{
	string corrupthex = "7468697......!";
	string validbin = "thi";
	CharVec bin, val(validbin.begin(), validbin.end());
	Hex::hex_to_bin(corrupthex, bin);
	ASSERT_EQ(val, bin);
}

