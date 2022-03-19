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
#ifndef _SCC_CRYPTO_RANDOM_H
#define _SCC_CRYPTO_RANDOM_H

#include <climits>
#include <string>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_random Random number generator
	@{

	Cryptographically secure random numbers using pseudorandom number generator.
*/

/** Random number generator.
	\file
*/

/** The random number generator is initialized with random entropy on first use.

	The same seed will generate different bit sequences between runs.
*/
class RandomEngine
{
public:
	RandomEngine() {}

	/** Opaque reference to the number generator. */
	static void* ctx();

	/**	Lock the number generator while the opaque reference is in use. */
	static void lock();
	
	/**	Unlock the number generator. */
	static void unlock();

	/** Generate a random seed and seed the generator with it. */
	static void random_seed();

	/** Seed the generator. */
	static void seed(const void *, int len);
	/** Seed the generator. */
	static void seed(const std::string& buf)
	{
		seed((void*)buf.data(), buf.size());
	}

	/** Generate random bytes. */
	static void rand_bytes(void *, int len);
	/** Generate random word. */
	static uint32_t rand_uint32()
	{
		uint32_t r;
		rand_bytes(&r, sizeof(uint32_t));
		return r;
	}
	/** Generate random word. */
	static int32_t rand_int32()
	{
		int32_t r;
		rand_bytes(&r, sizeof(int32_t));
		return r;
	}
	/** Generate random long word. */
	static uint64_t rand_uint64()
	{
		uint64_t r;
		rand_bytes(&r, sizeof(uint64_t));
		return r;
	}
	/** Generate random long word. */
	static int64_t rand_int64()
	{
		int64_t r;
		rand_bytes(&r, sizeof(int64_t));
		return r;
	}

	/** Required for UniformRandomBitGenerator interface. */
	typedef uint32_t result_type;
	/** Required for UniformRandomBitGenerator interface. */
	uint32_t operator()()
	{
		return rand_uint32();
	}
	/** Required for UniformRandomBitGenerator interface. */
	uint32_t min() { return 0; }
	/** Required for UniformRandomBitGenerator interface. */
	uint32_t max() { return UINT_MAX; }
};

/** Helper to safely lock the engine.

	When created, the engine is locked. When the locker goes out of scope or unlock() is called, the engine is unlocked.
*/
class RandomEngineLocker
{
	bool m_lk;
public:
	RandomEngineLocker() : m_lk(false)
	{
		lock();
	}
	~RandomEngineLocker()
	{
		unlock();
	}
	/** Lock the engine. */
	void lock()
	{
		if (!m_lk)
		{
			RandomEngine::lock();
			m_lk = true;
		}
	}
	/** Unlock the engine. */
	void unlock()
	{
		if (m_lk)
		{
			RandomEngine::unlock();
			m_lk = false;
		}
	}
};
/** @} */
/** @} */

}	// namespace

#endif
