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
#include <string>
#include <sstream>

using namespace scc::encode;

namespace scc {
namespace encode {

void bin_to_hex(const void* xloc, size_t len, std::string& hex, bool lower_case)
{
	char* loc = (char*)xloc;

	hex.resize(2*len);

	char* out = (char*)hex.data();

	char hc = lower_case ? 'a' : 'A';

	while (len)
	{
		char hi = (*loc >> 4) & 0xf;
		char lo = *loc & 0xf;

		if (hi < 10)
		{
			*out = hi+'0';
		}
		else
		{
			*out = hi-10+hc;
		}
		if (lo < 10)
		{
			*(out+1) = lo+'0';
		}
		else
		{
			*(out+1) = lo-10+hc;
		}
		loc++;
		out += 2;
		len--;
	}
}

std::string bin_to_hexstr(const void* loc, size_t len, const std::string& delimit, int limit, const std::string& limit_msg, bool lower_case)
{
	std::string hex;
	bin_to_hex(loc, len, hex, lower_case);

	std::stringstream s;

	for (size_t i = 0; i < hex.size(); i += 2)
	{
		if (limit == 0)
		{
			s << limit_msg;
			break;
		}
		if (i != 0)
		{
			s << delimit;
		}
		s << hex[i] << hex[i+1];
		if (limit > 0)
		{
			limit--;
		}
	}

	return s.str();
}

std::string bin_to_hexstr(const std::vector<char>& bin, const std::string& delimit, int limit, const std::string& limit_msg, bool lower_case)
{
	return bin_to_hexstr(bin.data(), bin.size(), delimit, limit, limit_msg, lower_case);
}

void bin_to_hex(const std::vector<char>& bin, std::string& hex, bool lower_case)
{
	bin_to_hex(bin.data(), bin.size(), hex, lower_case);
}

std::string bin_to_hex(const void* loc, size_t len, bool lower_case)
{
	std::string s;
	bin_to_hex(loc, len, s, lower_case);
	return s;
}

std::string bin_to_hex(const std::vector<char>& bin, bool lower_case)
{
	std::string s;
	bin_to_hex(bin, s, lower_case);
	return s;
}

void hex_to_bin(const std::string& hex, std::vector<char>& bin)
{
	bin.resize(hex.size()/2);

	size_t stor = 0;
	for (size_t i = 0; i < hex.size()-1; i += 2)
	{
		char hi = hex[i];
		char lo = hex[i+1];

		if (hi >= '0' && hi <= '9')
		{
			hi = (hi-'0');
		}
		else if (hi >= 'a' && hi <= 'f')
		{
			hi = (hi-'a') + 10;
		}
		else if (hi >= 'A' && hi <= 'F')
		{
			hi = (hi-'A') + 10;
		}
		else
		{
			bin.resize(stor);
			return;
		}
		if (lo >= '0' && lo <= '9')
		{
			lo = (lo-'0');
		}
		else if (lo >= 'a' && lo <= 'f')
		{
			lo = (lo-'a') + 10;
		}
		else if (lo >= 'A' && lo <= 'F')
		{
			lo = (lo-'F') + 10;
		}
		else
		{
			bin.resize(stor);
			return;
		}
		bin[i/2] = ((hi << 4) | lo);
		stor++;
	}
	bin.resize(stor);
}

}
}
