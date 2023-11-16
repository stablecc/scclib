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
#include <crypto/hash.h>
#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <util/iostream.h>
#include <util/rwloopbuf.h>
#include <encode/hex.h>

/** \addtogroup crypto_hash
	@{
*/
/** Test file for \ref crypto_hash
	\file
*/
/** \example scclib/crypto/unittest/hash_digest.cc
	
	Test for One-way hashing and message digests.
*/
/** @} */

using namespace std;
using namespace scc::encode;
using namespace scc::util;
using scc::crypto::Hash;
using scc::crypto::Hmac;
using CharVec = std::vector<char>;

static string hash_test = "This is a test sentence to test both hashes and digests. It is a bit longer than the key!";
static string hash_test1 = "This is a test sentence to test both hashes and digests. ";
static string hash_test2 = "It is a bit longer than the key!";
static string hash_key = "This is a keyphrase";
static string hmac_sha256 = "dbe8548042b534bd99ddf26b5fc4c2cdfeaf07d8df5427f5794a4445a0c425b2";

struct Hash_digest : public testing::Test
{
	map<int, Hash> m_digest;
	map<int, string> m_name;
	map<int, string> m_init;
	map<int, string> m_hash;
	map<int, int> m_size;

	Hash_digest()
	{
		m_digest.insert(std::make_pair(Hash::md5_type, Hash(Hash::md5_type)));
		m_name[Hash::md5_type] = "md5";
		m_init[Hash::md5_type] = "d41d8cd98f00b204e9800998ecf8427e";
		m_hash[Hash::md5_type] = "6630d84e20c4f20a87fcf7e069a2d34e";
		m_size[Hash::md5_type] = Hash::md5_size;

		m_digest.insert(std::make_pair(Hash::sha1_type, Hash(Hash::sha1_type)));
		m_name[Hash::sha1_type] = "sha1";
		m_init[Hash::sha1_type] = "da39a3ee5e6b4b0d3255bfef95601890afd80709";
		m_hash[Hash::sha1_type] = "3acf7561f5bd97534e575ba2565c6400b9128412";
		m_size[Hash::sha1_type] = Hash::sha1_size;

		m_digest.insert(std::make_pair(Hash::sha224_type, Hash(Hash::sha224_type)));
		m_name[Hash::sha224_type] = "sha224";
		m_init[Hash::sha224_type] = "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f";
		m_hash[Hash::sha224_type] = "317746d2199f50c2bf450bf3412d9ff74645aab7c3b747f7779a7b28";
		m_size[Hash::sha224_type] = Hash::sha224_size;

		m_digest.insert(std::make_pair(Hash::sha256_type, Hash(Hash::sha256_type)));
		m_name[Hash::sha256_type] = "sha256";
		m_init[Hash::sha256_type] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
		m_hash[Hash::sha256_type] = "382dfbf0cd153aec516de602ee6609ee73d97259cc78d74ea0caa9d5b02afab9";
		m_size[Hash::sha256_type] = Hash::sha256_size;

		m_digest.insert(std::make_pair(Hash::sha384_type, Hash(Hash::sha384_type)));
		m_name[Hash::sha384_type] = "sha384";
		m_init[Hash::sha384_type] = "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b";
		m_hash[Hash::sha384_type] = "3e7d844e6b9be37e4e8011dd258682c651ea151bf63897503e2ecffbcdfed3492d513028489be69ac3c3f9fb1649fc19";
		m_size[Hash::sha384_type] = Hash::sha384_size;

		m_digest.insert(std::make_pair(Hash::sha512_type, Hash(Hash::sha512_type)));
		m_name[Hash::sha512_type] = "sha512";
		m_init[Hash::sha512_type] = "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e";
		m_hash[Hash::sha512_type] = "a99f27da164286aea7f2c2928966dda6ce270b851536a6b8c4242f7e20131aa8260dd2239082cc02cc0f9fd0415e3bbe096868bbba7a59afb8a84188b2ce9cf5";
		m_size[Hash::sha512_type] = Hash::sha512_size;

		m_digest.insert(std::make_pair(Hash::sha512_224_type, Hash(Hash::sha512_224_type)));
		m_name[Hash::sha512_224_type] = "sha512/224";
		m_init[Hash::sha512_224_type] = "6ed0dd02806fa89e25de060c19d3ac86cabb87d6a0ddd05c333b84f4";
		m_hash[Hash::sha512_224_type] = "2ecce2ef45e929a959b3dd1ea8dbcc19ca644742d74a6d34ec654ac3";
		m_size[Hash::sha512_224_type] = Hash::sha512_224_size;

		m_digest.insert(std::make_pair(Hash::sha512_256_type, Hash(Hash::sha512_256_type)));
		m_name[Hash::sha512_256_type] = "sha512/256";
		m_init[Hash::sha512_256_type] = "c672b8d1ef56ed28ab87c3622c5114069bdd3ad7b8f9737498d0c01ecef0967a";
		m_hash[Hash::sha512_256_type] = "2730a349582f660c6d0660bf7a09b4aa6a8b4a11bb1ab8306950ee93d7d9f258";
		m_size[Hash::sha512_256_type] = Hash::sha512_256_size;

		m_digest.insert(std::make_pair(Hash::sm3_type, Hash(Hash::sm3_type)));
		m_name[Hash::sm3_type] = "sm3";
		m_init[Hash::sm3_type] = "1ab21d8355cfa17f8e61194831e81a8f22bec8c728fefb747ed035eb5082aa2b";
		m_hash[Hash::sm3_type] = "a4d3f1d0bb8d34696688a434606b5eb3d78e8bcd98ae12621ab36dd0d6a8d9e7";
		m_size[Hash::sm3_type] = Hash::sm3_size;
	}
	virtual ~Hash_digest() {}

	string final(Hash& h)
	{
		CharVec v;
		h.final(v);
		return Hex::bin_to_hex(v.data(), v.size());
	}
};

TEST_F(Hash_digest, Lengths)
{
	for (auto& d : m_digest)
	{
		cout << "Length " << m_name[d.first] << ": " << d.second.size() << endl;
		ASSERT_EQ(d.second.size(), m_size[d.first]);
	}
}

TEST_F(Hash_digest, Init)
{
	for (auto& d : m_digest)
	{
		string hex = final(d.second);
		cout << "Final " << m_name[d.first] << ": " << hex << endl;
		ASSERT_EQ(hex, m_init[d.first]);
	}
}

TEST_F(Hash_digest, Copy_final)
{
	for (auto& d : m_digest)
	{
		d.second.update(hash_test);
		char bin[m_size[d.first]];
		d.second.final(bin, d.second.size());
		string hex = Hex::bin_to_hex(CharVec(&bin[0], &bin[m_size[d.first]]));
		cout << "Init " << m_name[d.first] << ": " << hex << endl;
		ASSERT_EQ(hex, m_hash[d.first]);
	}
}

TEST_F(Hash_digest, Test_one_part)
{
	for (auto& d : m_digest)
	{
		d.second.update(hash_test);
		string hex = final(d.second);
		ASSERT_EQ(hex, m_hash[d.first]);
	}
}

TEST_F(Hash_digest, Get_tag)
{
	for (auto& d : m_digest)
	{
		d.second.update(hash_test);
		CharVec fin;
		d.second.final(fin);

		Hash h(d.first);
		h.update(hash_test);

		CharVec t;
		ASSERT_EQ(h.get_tag(t), m_size[d.first]);

		for (int i = 1; i <= m_size[d.first]; i++)
		{
			CharVec x(fin);
			x.resize(i);
			ASSERT_EQ(h.get_tag(t, i), i);
			ASSERT_EQ(x, t);
		}
	}
}

TEST_F(Hash_digest, Test_two_parts)
{
	for (auto& d : m_digest)
	{
		d.second.update(hash_test1);
		d.second.update(hash_test2);
		string hex = final(d.second);
		ASSERT_EQ(hex, m_hash[d.first]);
	}
}

TEST_F(Hash_digest, Reset_val)
{
	for (auto& d : m_digest)
	{
		d.second.update(hash_test);
		string hex = final(d.second);
		ASSERT_EQ(hex, m_hash[d.first]);

		d.second.update(hash_test);
		hex = final(d.second);
		ASSERT_EQ(hex, m_hash[d.first]);
	}
}


TEST_F(Hash_digest, Reset_cmd)
{
	for (auto& d : m_digest)
	{
		d.second.update("bad data");
		d.second.reset();
		d.second.update(hash_test);
		string hex = final(d.second);
		ASSERT_EQ(hex, m_hash[d.first]);
	}
}

TEST_F(Hash_digest, Streams)
{
	for (auto& d : m_digest)
	{
		RwLoopBuffer bufrw;
		scc::crypto::HashReader rd(bufrw, d.second);
		scc::crypto::HashWriter wr(bufrw, d.second);
		IoStream ios(rd, wr);

		ios << hash_test1;
		ios << hash_test2;
		ios.flush();

		string hex = final(d.second);
		ASSERT_EQ(hex, m_hash[d.first]);

		string s;
		ASSERT_TRUE(std::getline(ios, s));
		ASSERT_EQ(s, hash_test);

		hex = final(d.second);
		ASSERT_EQ(hex, m_hash[d.first]);
	}
}

TEST_F(Hash_digest, Hmac_lengths)
{
	for (auto& d : m_digest)
	{
		cout << "HMAC length " << m_name[d.first] << ": " << d.second.size() << endl;
		Hmac h(hash_key, d.first);
		ASSERT_EQ(h.size(), m_size[d.first]);
	}
}

TEST_F(Hash_digest, Hmac_compare) {	 // compare two hmacs using the same key
	for (auto& d : m_digest)
	{
		Hmac h1(hash_key, d.first);
		h1.update(hash_test);
		char h1buf[h1.size()];
		ASSERT_EQ(h1.final(h1buf, h1.size()), h1.size());
		Hmac h2(hash_key, d.first);
		h2.update(hash_test1);
		h2.update(hash_test2);
		char h2buf[h2.size()];
		ASSERT_EQ(h2.final(h2buf, h2.size()), h2.size());
		ASSERT_EQ(h1.size(), h2.size());
		ASSERT_EQ(memcmp(h1buf, h2buf, h1.size()), 0);
	}
}

TEST(hmac_test, Hmac_sanity)
{
	Hmac h(hash_key, Hash::sha256_type);
	h.update(hash_test1);
	h.update(hash_test2);
	char hbuf[h.size()];
	ASSERT_EQ(h.final(hbuf, h.size()), h.size());
	string hex1 = Hex::bin_to_hex(CharVec(&hbuf[0], &hbuf[h.size()]));
	ASSERT_EQ(hex1, hmac_sha256);
	h.reset();
	h.update(hash_test);
	ASSERT_EQ(h.final(hbuf, h.size()), h.size());
	string hex2 = Hex::bin_to_hex(CharVec(&hbuf[0], &hbuf[h.size()]));
	ASSERT_EQ(hex2, hmac_sha256);
}

TEST(hmac_test, Hmac_sanity_move)
{
	Hmac h(hash_key, Hash::sha256_type);
	h.update(hash_test);
	Hmac h2 = std::move(h);
	char hbuf[h2.size()];
	ASSERT_EQ(h2.final(hbuf, h2.size()), h2.size());
	string hex1 = Hex::bin_to_hex(CharVec(&hbuf[0], &hbuf[h2.size()]));
	ASSERT_EQ(hex1, hmac_sha256);
	ASSERT_THROW(h.final(hbuf, h.size()), std::runtime_error); 
}

TEST_F(Hash_digest, Hmac_reset)
{
	for (auto& d : m_digest)
	{
		Hmac h(hash_key, d.first);
		h.update(hash_test);
		char hbuf[h.size()];
		ASSERT_EQ(h.final(hbuf, h.size()), h.size());
		string hex1 = Hex::bin_to_hex(CharVec(&hbuf[0], &hbuf[h.size()]));
		h.reset();
		h.update(hash_test1);
		h.update(hash_test2);
		ASSERT_EQ(h.final(hbuf, h.size()), h.size());
		string hex2 = Hex::bin_to_hex(CharVec(&hbuf[0], &hbuf[h.size()]));
		ASSERT_EQ(hex1, hex2);
	}
}

TEST_F(Hash_digest, hash_vector_validate)
{
	CharVec test(hash_test.begin(), hash_test.end());
	CharVec test1(hash_test1.begin(), hash_test1.end());
	CharVec test2(hash_test2.begin(), hash_test2.end());
	CharVec fin;

	for (auto& d : m_digest)
	{
		d.second.update(test);
		d.second.final(fin);
		string hex = Hex::bin_to_hex(fin);
		ASSERT_EQ(hex, m_hash[d.first]);

		d.second.reset();
		d.second.update(test1);
		d.second.update(test2);
		d.second.final(fin);
		hex = Hex::bin_to_hex(fin);
		ASSERT_EQ(hex, m_hash[d.first]);
	}
}

TEST_F(Hash_digest, hmac_vector_validate)
{
	CharVec key(hash_key.begin(), hash_key.end());
	CharVec test(hash_test.begin(), hash_test.end());
	CharVec test1(hash_test1.begin(), hash_test1.end());
	CharVec test2(hash_test2.begin(), hash_test2.end());
	CharVec fin;

	for (auto& d : m_digest)
	{
		Hmac h(key, d.first);
		h.update(test);
		ASSERT_EQ(h.final(fin), h.size());
		string hex1 = Hex::bin_to_hex(fin);
		h.reset();
		h.update(test1);
		h.update(test2);
		ASSERT_EQ(h.final(fin), h.size());
		string hex2 = Hex::bin_to_hex(fin);
		ASSERT_EQ(hex1, hex2);
	}
}

TEST_F(Hash_digest, hash_tag)
{
	using charv = vector<char>;
	charv d1, d2, d1d2;

	ifstream f("/dev/urandom");
	d1.resize(1024);
	f.read(d1.data(), 1024);
	d2.resize(1024);
	f.read(d2.data(), 1024);
	d1d2 = d1;
	d1d2.insert(d1d2.end(), d2.begin(), d2.end());

	for (auto& d : m_hash)
	{
		Hash h(d.first);

		charv inith;
		h.final(inith);

		h.reset();
		h.update(d1);
		charv d1h;
		h.final(d1h);
		
		h.reset();
		h.update(d1d2);
		charv d1d2h;
		h.final(d1d2h);

		h.update(d1);
		CharVec d1tag;
		h.get_tag(d1tag);
		ASSERT_EQ(d1h, d1tag);
		
		h.update(d2);
		CharVec d1d2tag;
		h.get_tag(d1d2tag);
		ASSERT_EQ(d1d2h, d1d2tag);

		charv ver;
		h.final(ver);
		ASSERT_EQ(d1d2h, ver);
		
		CharVec inittag;
		h.get_tag(inittag);
		ASSERT_EQ(inittag, inith);
	}
}
