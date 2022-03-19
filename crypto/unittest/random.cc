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
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <random>
#include <map>
#include <vector>

/** \addtogroup crypto_random
	@{ */
/** Tests for \ref crypto_random \file */
/** \example crypto/unittest/random.cc */
/** @} */

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::uniform_int_distribution;
using std::mt19937;
using scc::crypto::RandomEngine;

TEST(random_tests, uints)
{
	uint32_t r32 = 0;
	for (int i = 1; i < 5; i++)
	{
		uint32_t last = r32;
		r32 = RandomEngine::rand_uint32();
		cout << "Rand uint32_t: " << r32 << endl;
		EXPECT_NE(last, r32);
	}
	uint64_t r64 = 0;
	for (int i = 1; i < 5; i++)
	{
		uint64_t last = r64;
		RandomEngine::rand_bytes(&r64, sizeof(r64));
		cout << "Rand uint64_t: " << r64 << endl;
		EXPECT_NE(last, r64);
	}
	RandomEngine rd;
	r32 = 0;
	for (int i = 1; i < 5; i++)
	{
		uint32_t last = r32;
		r32 = rd();
		cout << "Rand operator(): " << r32 << endl;
		EXPECT_NE(last, r32);
	}
}

TEST(random_tests, seed)
{
	string seed1("this is a random seed for the generator");
	seed1.resize(32);
	string seed2("this is a different random seed for the generator");
	seed2.resize(32);

	RandomEngine::seed(seed1);
	auto r1 = RandomEngine::rand_uint64();
	cout << "rand with seed1: " << r1 << endl;

	RandomEngine::random_seed();
	auto r2 = RandomEngine::rand_uint64();
	cout << "rand with random_seed: " << r2 << endl;
	EXPECT_NE(r1, r2);

	RandomEngine::seed(seed2);
	auto r3 = RandomEngine::rand_uint64();
	cout << "rand with seed2: " << r3 << endl;
	EXPECT_NE(r1, r3);
	EXPECT_NE(r2, r3);

	/*
	The following test is not guaranteed to pass on all random number generators.
	The ippcp rng passes it, the openssl one does not

	RandomEngine::seed(seed1);
	auto r4 = RandomEngine::rand_uint64();
	cout << "rand with seed1: " << r4 << endl;
	ASSERT_EQ(r4, r1);
	*/
}

TEST(random_tests, UniformRandomBitGenerator)
{
	RandomEngine rd;
	uniform_int_distribution<int> dis(0, 9);
	map<int, int> hist;
	for (int n = 0; n < 100000; ++n)
	{
		++hist[dis(rd)];
	}
	cout << "random engine 100000 values 0 to 9 distribution" << endl;
	for (auto& p : hist)
	{
		cout << p.first << " : " << p.second << endl;
	}
	ASSERT_EQ(hist.size(), 10);
	for (int i = 0; i < 10; i++)
	{
		ASSERT_NE(hist.find(i), hist.end());
		ASSERT_GT(hist[i], 0);
	}
}

TEST(random_tests, mersenne_twister_engine)
{
	RandomEngine rd;
	mt19937 gen(rd()); // mersenne_twister_engine
	uniform_int_distribution<int> dis(0, 9);
	map<int, int> hist;
	for (int n = 0; n < 100000; ++n)
	{
		++hist[dis(gen)];
	}
	cout << "mersenne twister 100000 values 0 to 9 distribution:" << endl;
	for (auto p : hist)
	{
		cout << p.first << " : " << p.second << endl;;
	}
	ASSERT_EQ(hist.size(), 10);
	for (int i = 0; i < 10; i++)
	{
		ASSERT_NE(hist.find(i), hist.end());
		ASSERT_GT(hist[i], 0);
	}
}

TEST(random_tests, bytes_sanity)
{
	uint8_t by[5];
	RandomEngine::rand_bytes(by, 5);
	cout << "tbytes " << 5 << ": ";
	cout << std::hex << (uint16_t)by[0] << std::dec << " ";
	auto b = by[0];
	int same = 1;
	for (int i = 1; i < 5; i++)
	{
		if (by[i] == b)		same++;
		cout << std::hex << (uint16_t)by[i] << std::dec << " ";
	}
	cout << " same as byte[0]=" << same;
	cout << endl;
	EXPECT_NE(same, 5);
}
