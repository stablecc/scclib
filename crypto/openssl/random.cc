
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
#include <crypto/random.h>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <openssl/rand.h>
#include <openssl/err.h>

struct OsslRandInit
{
	void sys_seed(uint32_t* loc, int len)
	{
		assert(len == 16);		// 512 bit value

		std::ifstream udev("/dev/urandom");
		if (udev.is_open())
		{
			if (!udev.read((char*)loc, len))		throw("read from /dev/urandom");
		}
		else
		{
			uint32_t locr[4] = {4228180601,2893615799,3051165367,1313327497};
			memcpy(loc, locr, len);
		}

		using namespace std::chrono;
		auto x = high_resolution_clock::now().time_since_epoch().count();
		assert(sizeof(x) == 8);
		for (int i = 0; i < 8; i++)
		{
			loc[i] ^= *((char*)&x + i);
		}
	}

	OsslRandInit()
	{
		random_seed();
	}
	virtual ~OsslRandInit()
	{
	}

	void random_seed()
	{
		uint32_t bigr[16];
		sys_seed(bigr, 16);
		explicit_bzero(bigr, 16);
	}

	void seed(const void * buf, int len)
	{
		RAND_seed(buf, len);
	}

	void bytes(void * buf, int len)
	{
		if (!RAND_bytes(static_cast<unsigned char*>(buf), len))
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << "RAND_bytes: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
	}
};

static OsslRandInit* _osslrandinitptr;

class OsslRandShut
{
public:
	OsslRandShut() {}
	~OsslRandShut()
	{
		delete _osslrandinitptr;
	}
};

OsslRandInit& sslrand()
{
	static OsslRandInit* ri = new OsslRandInit();		// this is only initialized the first time it is scanned
	_osslrandinitptr = ri;
	return *ri;
}

using namespace scc::crypto;

/**	OpenSSL does not have context. */
void* RandomEngine::ctx()
{
	return nullptr;
}

/** OpenSSL 1.1 is thread-safe, and has its own internal locking. */
void RandomEngine::lock()
{
}

void RandomEngine::unlock()
{
}

void RandomEngine::seed(const void * buf, int len)
{
	sslrand().seed(buf, len);
}

void RandomEngine::random_seed()
{
	sslrand().random_seed();
}

void RandomEngine::rand_bytes(void * buf, int len)
{
	sslrand().bytes(buf, len);
}
