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
#include <cassert>
#include <fstream>
#include <mutex>
#include <cstring>
#include <chrono>
#include <cassert>
#include <crypto/bignum.h>
#include <ippcp.h>

struct IppRandInit
{
	IppsPRNGState* m_ctx;
	int m_size;
	std::mutex m_mx;

	void sys_seed(uint32_t* loc, int len)
	{
		assert(len == 16);		// 512 bit value

		std::lock_guard<std::mutex> mx(m_mx);

		IppStatus st = ippsTRNGenRDSEED(loc, 32*len, m_ctx);		// try using RDSEED instruction

		if (st != ippStsNoErr)
		{
			st = ippsPRNGenRDRAND(loc, 32*len, m_ctx);				// try using RDRAND
		}

		if (st != ippStsNoErr)
		{
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
		}

		using namespace std::chrono;
		auto x = high_resolution_clock::now().time_since_epoch().count();
		assert(sizeof(x) == 8);
		for (int i = 0; i < 8; i++)
		{
			loc[i] ^= *((char*)&x + i);
		}
	}

	IppRandInit()
	{
		IppStatus st;
		if ((st = ippsPRNGGetSize(&m_size)) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsPRNGGetSize: ")+ippcpGetStatusString(st));
		}
		m_ctx = (IppsPRNGState*)malloc(m_size);
		if ((st = ippsPRNGInit(512, m_ctx)) != ippStsNoErr)		// maximum bit size
		{
			throw std::runtime_error(std::string("ippsPRNGInit: ")+ippcpGetStatusString(st));
		}

		uint32_t bigr[16];		// 512 bits
		sys_seed(bigr, 16);

		// Set the entropy. Ensures that the user cannot recreate the same bit sequence between runs using the same seed.
		// https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-90Ar1.pdf requires an entropy input for a true random
		// bit generator.

		scc::crypto::Bignum b;
		b.set(bigr, 64);
		if ((st = ippsPRNGSetAugment((const IppsBigNumState*)b.bn(), m_ctx)) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsPRNGSetAugment: ")+ippcpGetStatusString(st));
		}

		explicit_bzero(bigr, 16);
	}
	virtual ~IppRandInit()
	{
		explicit_bzero(m_ctx, m_size);
		free(m_ctx);
	}

	void random_seed()
	{
		uint32_t bigr[16];		// 512 bytes
		sys_seed(bigr, 16);
		seed(bigr, 16);
		explicit_bzero(bigr, 16);
	}

	void seed(const void * loc, int len)
	{
		std::lock_guard<std::mutex> l(m_mx);
		if (len <= 0)
		{
		if (len <= 0)
		{
			throw std::runtime_error("random seed called with bad length");
		}
		}
		scc::crypto::Bignum b;
		b.set(loc, len);
		int r;
		if ((r = ippsPRNGSetSeed((const IppsBigNumState*)b.bn(), m_ctx)) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsPRNGSetSeed: ")+ippcpGetStatusString(r));
		}
	}
	
	void bytes(void* loc, int len)
	{
		std::lock_guard<std::mutex> l(m_mx);
		if (len <= 0)
		{
			throw std::runtime_error("random bytes called with bad length");
		}

		if (len/4)	  // handle up to word aligned part
		{
			uint32_t* ptr = (uint32_t*)loc;
			int words = len/4;
			int r;
			if ((r = ippsPRNGen(ptr, words*32, m_ctx)) != ippStsNoErr)
			{
				throw std::runtime_error(std::string("ippsPRNGen: ")+ippcpGetStatusString(r));
			}
			loc = (void*)(ptr+words);
			len -= words*4;
		}

		if (len)		// handle the last few bytes
		{
			uint32_t w;
			int r;
			if ((r = ippsPRNGen(&w, 32, m_ctx)) != ippStsNoErr)
			{
				throw std::runtime_error(std::string("ippsPRNGen: ")+ippcpGetStatusString(r));
			}
			memcpy(loc, &w, len);
		}
	}

	void* ctx()
	{
		return m_ctx;
	}
	
	void lock()
	{
		m_mx.lock();
	}
	
	void unlock()
	{
		m_mx.unlock();
	}
};

static IppRandInit* _ipprandinitptr;

struct IppRandShut
{
	IppRandShut() {}
	virtual ~IppRandShut()
	{
		delete _ipprandinitptr;
	}
};

static IppRandShut _ipprandshut;

// NOTE: declaring the following way creates a static initializer race
//
//static IppRandInit _ipprand;
//
// It is better to use a local static initializer here. This creates a "create on first use" initializer.

static IppRandInit& ipprand()
{
	static IppRandInit* iri = new IppRandInit();		// this is only initialized the first time it is scanned
	_ipprandinitptr = iri;
	return *iri;
}

using namespace scc::crypto;

void* RandomEngine::ctx()
{
	return ipprand().ctx();
}

void RandomEngine::lock()
{
	ipprand().unlock();
}

void RandomEngine::unlock()
{
	ipprand().unlock();
}

void RandomEngine::seed(const void * loc, int len)
{
	ipprand().seed(loc, len);
}

void RandomEngine::random_seed()
{
	ipprand().random_seed();
}

void RandomEngine::rand_bytes(void * loc, int len)
{
	ipprand().bytes(loc, len);
}
