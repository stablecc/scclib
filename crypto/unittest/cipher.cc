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
#include <crypto/cipher.h>
#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <encode/hex.h>

/** \addtogroup crypto_cipher
	@{
*/
/** Test file for \ref crypto_cipher
	\file
*/
/** \example scclib/crypto/unittest/ipp/cipher.cc
	
	Test for symmetric block ciphers.
*/
/** @} */

using std::cout;
using std::endl;
using std::map;
using std::pair;
using std::string;
using scc::encode::Hex;
using scc::crypto::Cipher;

using CharVec = std::vector<char>;

string key16 = "use a 16 b key!!";
string key24 = "use a 24 b key!!!!!!!!!!";
string key32 = "use a 32 b key!!!!!!!!!!!!!!!!!!";
string nonce = "the nonce!!!";
string adddata = "this is the additional data";
string plain = "\
To be, or not to be, that is the question: \n\
Whether 'tis nobler in the mind to suffer \n\
The slings and arrows of outrageous fortune, \n\
Or to take Arms against a Sea of troubles, \n\
And by opposing end them: to die, to sleep; \n\
No more; and by a sleep, to say we end \n\
The heart-ache, and the thousand natural shocks \n\
That Flesh is heir to? 'Tis a consummation \n\
Devoutly to be wished. To die, to sleep, \n\
perchance to Dream; aye, there's the rub...";

using mappair = std::pair<const string,Cipher>;

struct Ciphertest : public testing::Test
{
	map<string, Cipher> ciphers;
	Ciphertest()
	{
		// construct these to avoid construction of default cipher (which is not allowed)
		ciphers.insert(std::make_pair(std::string("gcm16"), Cipher(Cipher::aes_gcm_type, key16)));
		ciphers.insert(std::make_pair(std::string("gcm24"), Cipher(Cipher::aes_gcm_type, key24)));
		ciphers.insert(std::make_pair(std::string("gcm32"), Cipher(Cipher::aes_gcm_type, key32)));
		ciphers.insert(std::make_pair(std::string("ccm16"), Cipher(Cipher::aes_ccm_type, key16)));
		ciphers.insert(std::make_pair(std::string("ccm24"), Cipher(Cipher::aes_ccm_type, key24)));
		ciphers.insert(std::make_pair(std::string("ccm32"), Cipher(Cipher::aes_ccm_type, key32)));
	}
	virtual ~Ciphertest() {}
	void reset(mappair& c, const string& nonce, const string& aad)
	{
		cout << "reset for: " << c.first << " nonce " << nonce.size() << " bytes, aad " << aad.size() << " bytes" << endl;
		c.second.reset(nonce.data(), nonce.size(), aad.data(), aad.size());
	}
	void reset(mappair& c, const string& nonce)
	{
		cout << "reset for: " << c.first << " nonce " << nonce.size() << " bytes" << endl;
		c.second.reset(nonce.data(), nonce.size());
	}
	string auth_tag(mappair& c)
	{
		cout << "auth tag for: " << c.first << endl;
		CharVec tag;
		tag.resize(16);
		c.second.auth_tag(tag.data(), tag.size());
		return Hex::bin_to_hex(tag, 16);
	}
	CharVec enc(mappair& c, const string& plain)
	{
		cout << "encrypt " << plain.size() << " bytes for: " << c.first << endl;
		CharVec cipher;
		cipher.resize(plain.size(), '\0');
		c.second.encrypt(plain.data(), plain.size(), cipher.data(), cipher.size());
		return cipher;
	}
	string dec(mappair& c, const CharVec& cipher)
	{
		cout << "decrypt " << cipher.size() << " bytes for: " << c.first << endl;
		string plain;
		plain.resize(cipher.size(), '\0');
		c.second.decrypt(cipher.data(), cipher.size(), plain.data(), plain.size());
		return plain;
	}
};

#if 0
// mgw Not sure about auth tag without data...
TEST_F(Ciphertest, nonce_only)
{
	for (auto& c: ciphers)
	{
		string tag0 = auth_tag(c);
		cout << "tag0 for " << c.first << ": " << tag0 << endl;

		reset(c, nonce);
		string tag1 = auth_tag(c);
		cout << "tag1 for " << c.first << ": " << tag1 << endl;
		ASSERT_EQ(tag0, tag1);

		reset(c, nonce);
		string tag2 = auth_tag(c);
		cout << "tag2 for " << c.first << ": " << tag2 << endl;
		ASSERT_EQ(tag1, tag2);
	}
}
#endif

TEST_F(Ciphertest, authdata_only)
{
	for (auto& c: ciphers)
	{
		string tag0 = auth_tag(c);
		cout << "tag0 for " << c.first << ": " << tag0 << endl;

		reset(c, nonce, adddata);
		string tag1 = auth_tag(c);
		cout << "tag1 for " << c.first << ": " << tag1 << endl;
		ASSERT_NE(tag0, tag1);

		reset(c, nonce, adddata);
		string tag2 = auth_tag(c);
		cout << "tag2 for " << c.first << ": " << tag1 << endl;
		ASSERT_EQ(tag1, tag2);
	}
}

TEST_F(Ciphertest, enc_reset)
{
	for (auto& c: ciphers)
	{
		reset(c, nonce);
		CharVec cip0 = enc(c, plain);
		string tag0 = auth_tag(c);

		reset(c, nonce);
		CharVec cip1 = enc(c, plain);
		string tag1 = auth_tag(c);

		ASSERT_EQ(cip0, cip1);
		ASSERT_EQ(tag0, tag1);
	}
}

TEST_F(Ciphertest, enc_reset_adddata)
{
	for (auto& c: ciphers)
	{
		reset(c, nonce, adddata);
		CharVec cip0 = enc(c, plain);
		string tag0 = auth_tag(c);

		reset(c, nonce, adddata);
		CharVec cip1 = enc(c, plain);
		string tag1 = auth_tag(c);

		ASSERT_EQ(cip0, cip1);
		ASSERT_EQ(tag0, tag1);
	}
}

TEST_F(Ciphertest, enc_dec)
{
	for (auto& c: ciphers)
	{
		reset(c, nonce);
		CharVec cip = enc(c, plain);
		string tag = auth_tag(c);

		reset(c, nonce);
		string plain1 = dec(c, cip);
		string tag1 = auth_tag(c);

		ASSERT_EQ(plain, plain1);
		ASSERT_EQ(tag, tag1);
	}
}

TEST_F(Ciphertest, enc_dec_adddata)
{
	for (auto& c: ciphers)
	{
		reset(c, nonce, adddata);
		CharVec cip = enc(c, plain);
		string tag = auth_tag(c);

		reset(c, nonce, adddata);
		string plain1 = dec(c, cip);
		string tag1 = auth_tag(c);

		ASSERT_EQ(plain, plain1);
		ASSERT_EQ(tag, tag1);
	}
}

TEST_F(Ciphertest, enc_dec_samebuf)
{
	for (auto& c: ciphers)
	{
		reset(c, nonce, adddata);

		CharVec buf(plain.begin(), plain.end());
		buf.resize(plain.size()+1024, '\0');		// both encrypt and decrypt should be able to handle larger target buffer

		CharVec verify(plain.begin(), plain.end());
		verify.resize(plain.size()+1024, '\0');	

		auto size = plain.size();

		cout << "encrypt in place " << size << "bytes for: " << c.first << endl;

		c.second.encrypt(buf.data(), size, buf.data(), buf.size());

		CharVec enctag(16, '\0');
		c.second.auth_tag(enctag.data(), enctag.size());

		reset(c, nonce, adddata);

		cout << "decrypt in place " << size << "bytes for: " << c.first << endl;

		c.second.decrypt(buf.data(), size, buf.data(), buf.size());

		CharVec dectag(16, '\0');
		c.second.auth_tag(dectag.data(), dectag.size());

		ASSERT_EQ(buf, verify);
		ASSERT_EQ(enctag, dectag);
	}
}
