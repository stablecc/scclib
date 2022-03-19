
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
#include <ctype.h>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <crypto/random.h>
#include <encode/hex.h>

using namespace scc::crypto;
using scc::encode::Hex;

std::ostream& operator <<(std::ostream& os, const Uuid& u)
{
	return os.write(u.val().c_str(), u.val().size());
}
void scc::crypto::PrintTo(const Uuid& u, std::ostream* os)
{
	*os << u;
}

const std::string Uuid::zero = "00000000-0000-0000-0000-000000000000";

void Uuid::assign(const std::string& s)
{
	std::stringstream str(s);
	std::string tok;
	std::vector<std::string> v;

	while (std::getline(str, tok, '-'))
	{
		v.push_back(tok);
	}

	if (v.size() != 5)			{ m_uuid = zero; return; }
	
	if (v[0].size() != 8)		{ m_uuid = zero; return; }
	if (v[1].size() != 4)		{ m_uuid = zero; return; }
	if (v[2].size() != 4)		{ m_uuid = zero; return; }
	if (v[3].size() != 4)		{ m_uuid = zero; return; }
	if (v[4].size() != 12)		{ m_uuid = zero; return; }

	for (auto& x : v)
	{
		for (auto b : x)
		{
			if ( ((b<'0')||(b>'9')) && ((b<'a')||(b>'f')) && ((b<'A')||(b>'F')) )
			{
				m_uuid = zero; return;
			}
		}
	}

	// NOTE: does not check for binary requirements

	m_uuid = s;

	std::for_each(m_uuid.begin(), m_uuid.end(), [](char& c)
	{
		c = tolower(c);
	});
}

std::string Uuid::generate()
{
	std::vector<char> u(16);
	RandomEngine::rand_bytes(&u[0], 16);

	// set most significant bits of time_hi_and_version (octet 6) to 0100 (version)
	u[6] &= 0x4f;
	u[6] |= 0x40;

	// set most significant bits of clock_seq_hi_and_reserved (octet 8) to 01
	u[8] &= 0x7f;
	u[8] |= 0x40;

	// emit hex HxHxHxHx-HxHx-HxHx-HxHx-HxHxHxHxHxHx
	std::stringstream uuid;

	typedef std::vector<char> CharVec;

	uuid << Hex::bin_to_hex(CharVec(&u[0], &u[4]));
	uuid << "-";
	uuid << Hex::bin_to_hex(CharVec(&u[4], &u[6]));
	uuid << "-";
	uuid << Hex::bin_to_hex(CharVec(&u[6], &u[8]));
	uuid << "-";
	uuid << Hex::bin_to_hex(CharVec(&u[8], &u[10]));
	uuid << "-";
	uuid << Hex::bin_to_hex(CharVec(&u[10], &u[16]));

	m_uuid = uuid.str();
	
	return m_uuid;
}
