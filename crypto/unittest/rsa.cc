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
#include <crypto/rsa.h>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <encode/hex.h>
#include <crypto/hash.h>

/** \addtogroup crypto_rsa
	@{
*/
/** Test file for \ref crypto_rsa
	\file
*/
/** \example scclib/crypto/unittest/ipp/rsa.cc
	
	Test for RSA public key cryptography.
*/
/** @} */

using std::cout;
using std::endl;
using std::string;
using scc::encode::Hex;
using scc::crypto::RsaPrivateKey;
using scc::crypto::RsaPublicKey;
using scc::crypto::PkcsSignature;
using scc::crypto::PssSignature;
using scc::crypto::Hash;
using scc::crypto::RsaOaepEncrypt;
using scc::crypto::RsaOaepDecrypt;

using charv = std::vector<char>;

TEST(rsa_key, private_zero)
{
	RsaPrivateKey k;
	cout << "zero key:" << k.str();
	cout << k.str();
	ASSERT_EQ(k.width(), 0);
}

TEST(rsa_key, private_generate)
{
	RsaPrivateKey k;
	k.generate(256);
	cout << "256 bit key:" << k.str();
	ASSERT_EQ(k.width(), 256);
}

TEST(rsa_key, private_validate)
{
	RsaPrivateKey k;
	k.generate(256);
	cout << "validate key:" << k.str();
	ASSERT_EQ(k.width(), 256);
	ASSERT_TRUE(k.validate());
}

TEST(rsa_key, zero_construct)
{
	RsaPrivateKey a;
	ASSERT_EQ(a.width(), 0);
	ASSERT_FALSE(a.validate());
}

TEST(rsa_key, clear)
{
	RsaPrivateKey a;
	a.generate(512);
	ASSERT_EQ(a.width(), 512);
	ASSERT_TRUE(a.validate());
	a.clear();
	ASSERT_EQ(a.width(), 0);
	ASSERT_FALSE(a.validate());
}

TEST(rsa_key, copy_construct)
{
	RsaPrivateKey a;
	a.generate(512);
	ASSERT_EQ(a.width(), 512);
	ASSERT_TRUE(a.validate());

	RsaPrivateKey b(a);
	ASSERT_EQ(a.width(), 512);
	ASSERT_TRUE(a.validate());
	ASSERT_EQ(b.width(), 512);
	ASSERT_TRUE(b.validate());
}

TEST(rsa_key, copy_op)
{
	RsaPrivateKey a;
	a.generate(512);
	ASSERT_EQ(a.width(), 512);
	ASSERT_TRUE(a.validate());

	RsaPrivateKey b;
	b = a;
	ASSERT_EQ(a.width(), 512);
	ASSERT_TRUE(a.validate());
	ASSERT_EQ(b.width(), 512);
	ASSERT_TRUE(b.validate());
}

TEST(rsa_key, move_construct)
{
	RsaPrivateKey a;
	a.generate(512);
	ASSERT_EQ(a.width(), 512);
	ASSERT_TRUE(a.validate());

	RsaPrivateKey b = std::move(a);
	ASSERT_EQ(a.width(), 0);
	ASSERT_FALSE(a.validate());
	ASSERT_EQ(b.width(), 512);
	ASSERT_TRUE(b.validate());
}

TEST(rsa_key, move_op)
{
	RsaPrivateKey a;
	a.generate(512);
	ASSERT_EQ(a.width(), 512);
	ASSERT_TRUE(a.validate());

	RsaPrivateKey b;
	b = std::move(a);
	ASSERT_EQ(a.width(), 0);
	ASSERT_FALSE(a.validate());
	ASSERT_EQ(b.width(), 512);
	ASSERT_TRUE(b.validate());
}

static string plaintext =
"To be, or not to be, that is the question:\n"
"Whether 'tis nobler in the mind to suffer\n"
"The slings and arrows of outrageous fortune,\n"
"Or to take Arms against a Sea of troubles,\n"
"And by opposing end them: to die, to sleep;\n" 
"No more; and by a sleep, to say we end\n"
"The heart-ache, and the thousand natural shocks\n"
"That Flesh is heir to? 'Tis a consummation\n"
"Devoutly to be wished. To die, to sleep,\n"
"perchance to Dream; aye, there's the rub...\n"
;

TEST(rsa_encryption, pkcs_signature)
{
	cout << "*** Pkcs signature test" << endl;

	RsaPrivateKey key;
	key.generate(1024);

	cout << "*** Key:\n" << key.str() << endl;

	cout << "*** Orig:\n" << plaintext << endl;

	charv sig;
	PkcsSignature::sign(plaintext.data(), plaintext.size(), sig, key, PkcsSignature::HashType::sha256);

	cout << "*** Signature (" << sig.size() << " bytes):\n" << Hex::bin_to_hexstr(sig, ":", 16) << endl;
	
	ASSERT_EQ(sig.size(), PkcsSignature::size(key));

	RsaPublicKey pubkey = key.pub_key();

	bool ver = PkcsSignature::verify(plaintext.data(), plaintext.size(), sig, pubkey, PkcsSignature::HashType::sha256);
	cout << "*** Verify: " << (ver ? "OK" : "FAIL") << endl;
	
	ASSERT_TRUE(ver);

	ver = PkcsSignature::verify(plaintext.data(), 1, sig, pubkey, PkcsSignature::HashType::sha256);
	ASSERT_FALSE(ver);
	ver = PkcsSignature::verify(plaintext.data(), plaintext.size(), sig.data(), 1, pubkey, PkcsSignature::HashType::sha256);
	ASSERT_FALSE(ver);

	charv t(sig);
	explicit_bzero(t.data(), t.size());
	ver = PkcsSignature::verify(plaintext.data(), plaintext.size(), t.data(), t.size(), pubkey, PkcsSignature::HashType::sha256);
	ASSERT_FALSE(ver);
}

TEST(rsa_encryption, oaep_encrypt)
{
	cout << "*** Oaep encryption test" << endl;

	RsaPrivateKey key;
	key.generate(3072);
	RsaPublicKey pub = key.pub_key();

	RsaOaepEncrypt enc(pub, Hash::sha1_type);

	cout << "plaintext size=" << plaintext.size() << endl;
	cout << "enc max_msg_len=" << enc.max_msg_size() << endl;
	cout << "enc cipher_len=" << enc.cipher_size() << endl;

	charv plain(plaintext.begin(), plaintext.end());
	plain.resize(enc.max_msg_size());

	charv cipher;
	cipher.resize(enc.cipher_size());

	string label("this is a label, which will be signed but not encrypted");

	enc.encrypt(plain.data(), plain.size(), cipher.data(), cipher.size(), label.data(), label.size());

	cout << "encrypted plain size=" << plain.size() << endl;

	RsaOaepDecrypt dec(key, Hash::sha1_type);

	charv decplain;
	decplain.resize(dec.max_msg_size());

	int r = dec.decrypt(decplain.data(), decplain.size(), cipher.data(), cipher.size(), label.data(), label.size());

	cout << "decrypt returned " << r << endl;

	ASSERT_EQ(r, plain.size());
	ASSERT_EQ(plain, decplain);

	r = dec.decrypt(decplain.data(), decplain.size(), cipher.data(), cipher.size());

	cout << "decrypt without label returned " << r << endl;

	ASSERT_EQ(r, -1);

	// same without labels
	enc.encrypt(plain.data(), plain.size(), cipher.data(), cipher.size());
	r = dec.decrypt(decplain.data(), decplain.size(), cipher.data(), cipher.size());
	ASSERT_EQ(plain, decplain);

	// short data
	plain.resize(dec.max_msg_size()/2);
	enc.encrypt(plain.data(), plain.size(), cipher.data(), cipher.size());
	r = dec.decrypt(decplain.data(), decplain.size(), cipher.data(), cipher.size());
	ASSERT_EQ(r, dec.max_msg_size()/2);
	decplain.resize(r);
	ASSERT_EQ(plain, decplain);
}

TEST(rsa_encryption, pss_signature)
{
	cout << "*** Pss signature test" << endl;

	RsaPrivateKey key;
	key.generate(1024);

	cout << "*** Key:\n" << key.str() << endl;

	cout << "*** Orig:\n" << plaintext << endl;

	charv sig;
	PssSignature::sign(plaintext.data(), plaintext.size(), sig, key, PssSignature::HashType::sha256);

	cout << "*** Signature (" << sig.size() << " bytes):\n" << Hex::bin_to_hexstr(sig, ":", 16) << endl;
	
	ASSERT_EQ(sig.size(), PssSignature::size(key));

	RsaPublicKey pubkey = key.pub_key();

	bool ver = PssSignature::verify(plaintext.data(), plaintext.size(), sig, pubkey, PssSignature::HashType::sha256);
	cout << "*** Verify: " << (ver ? "OK" : "FAIL") << endl;
	
	ASSERT_TRUE(ver);

	ver = PssSignature::verify(plaintext.data(), 1, sig, pubkey, PssSignature::HashType::sha256);
	ASSERT_FALSE(ver);
	ver = PssSignature::verify(plaintext.data(), plaintext.size(), sig.data(), 1, pubkey, PssSignature::HashType::sha256);
	ASSERT_FALSE(ver);

	charv t(sig);
	explicit_bzero(t.data(), t.size());
	ver = PssSignature::verify(plaintext.data(), plaintext.size(), t.data(), t.size(), pubkey, PssSignature::HashType::sha256);
	ASSERT_FALSE(ver);
}
