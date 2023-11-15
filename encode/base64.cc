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
#include <encode/base64.h>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
using std::cout;

using namespace scc::encode;

//
// The following array has the character indexed by the 6 bits that it represents
//

static const char encVal[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//
// This array is indexed by character value and provides the 6 bits that such character
// contributes.  Values above 63 have the following meaning:
//
//	64 - ignore this character (pad)
//	65 - ignore this character (whitespace)
//  66 - invalid character, ignore and return false
//

static unsigned char decVal[128] =
{
	66, 66, 66, 66, 66, 66, 66, 66, 66, 65, 65, 66, 66, 65, 66, 66,
	66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
	65, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 62, 66, 66, 66, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 66, 66, 66, 64, 66, 66,
	66,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 66, 66, 66, 66, 66,
	66, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 66, 66, 66, 66, 66,
};

namespace scc {
namespace encode {

template <class T>
void base64_encode(const std::vector<T>& vin, std::string& s) noexcept
{
	/** Extra stuff */
	unsigned char* v = (unsigned char*)&vin[0];
	unsigned int b = 0;		// bits
	int bc = 0;				// count of bits

	s.clear();
	s.reserve((vin.size() + 2)/3 * 4);

	for (size_t i = 0; i < vin.size(); ++i)
	{
		b = (b << 8) | v[i];
		bc += 8;

		while (bc >= 6)
		{
			int val = (b >> (bc-6)) & 63;
			s.push_back(encVal[val]);
			bc -= 6;
		}
	}

	if (bc)
	{
		int val = (b << (6-bc)) & 63;
		s.push_back(encVal[val]);
	}

	switch (bc)
	{
		case 2:	s.push_back('=');		// plus fall through...
		case 4: s.push_back('=');
	}
}

template void base64_encode<char>(const std::vector<char>&, std::string&);
template void base64_encode<unsigned char>(const std::vector<unsigned char>&, std::string&);

std::string str_to_base64(const std::string& s) noexcept
{
	std::vector<char> v(s.begin(), s.end());
	std::string ret;
	base64_encode(v, ret);
	return ret;
}

template <class T>
bool base64_decode(const std::string& s, std::vector<T>& v) noexcept
{
	bool ok = true;
	unsigned int b = 0;
	int bc = 0;
	int pc = 0;

	v.clear();

	for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		if (*i < 0 || *i > 127)
		{
			ok = false;
			continue;
		}

		int d = decVal[(int)*i];

		switch (d)
		{
		case 66:	ok = false;			// ignored character (error)
		case 65:	pc = 0;				// ignored character (no error)
					break;
		case 64:	++pc;				// count the pad
					break;

		default:
			pc = 0;						// reset the pad count
			b = (b << 6) | (d & 63);
			bc += 6;
			break;
		}

		if (bc >= 8)
		{
			int val = b >> (bc-8);
			//v.push_back(static_cast<unsigned char>(val & 255));
			v.push_back(val & 255);
			bc -= 8;
		}
	}

	if (bc - 2*pc) ok = false;			// check on spare bits hanging around

	return ok;
}

template bool base64_decode<char>(const std::string& s, std::vector<char>& v) noexcept;
template bool base64_decode<unsigned char>(const std::string& s, std::vector<unsigned char>& v) noexcept;

std::string base64_to_str(const std::string& s) noexcept
{
	std::vector<char> v;
	if (!base64_decode(s, v))
	{
		return std::string();
	}
	return std::string(v.begin(), v.end());
}

std::string base64_to_base64url(const std::string& b) noexcept
{
	std::string ret;
	for (char ch : b)
	{
		if (ch == '+')			ret.push_back('-');
		else if (ch == '/')		ret.push_back('_');
		else if (ch != '=')		ret.push_back(ch);
	}
	return ret;
}

std::string base64url_to_base64(const std::string& u) noexcept
{
	std::string ret;
	for (char ch : u)
	{
		if (ch == '-')			ret.push_back('+');
		else if (ch == '_')		ret.push_back('/');
		else 					ret.push_back(ch);
	}
	switch (u.size() % 4)
	{
		case 2:			ret += "=="; break;
		case 3:			ret += "="; break;
		// 0 is allowed, 1 is technically illegal, but we will just produce bad base64 since we don't throw exceptions
	}

	return ret;
}

}
} //namespace
