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
#include <gtest/gtest.h>
#include <crypto/ecc.h>
#include <iostream>

/** \addtogroup crypto_ecc
	@{
*/
/** Test file for \ref crypto_ecc
	\file
*/
/** \example scclib/crypto/unittest/ipp/ecc.cc
	
	Elliptic curve tests.
*/
/** @} */

using std::cout;
using std::endl;
using std::vector;
using std::string;
using scc::crypto::EccGfp;
using scc::crypto::EccGfpPoint;
using scc::crypto::Bignum;

struct EccTest : public testing::Test
{
	EccTest() {}
	virtual ~EccTest() {}

	EccGfp ecc;

	void print(const Bignum& v)
	{
		cout << "width=" << v.width() << " val=" << v << endl;
	}
	void print(const EccGfpPoint& point)
	{
		Bignum x, y;
		point.get(x, y);
		cout << "width=( " << x.width() << " , " << y.width() << " ) val=( " << x << " , " << y << " )" << endl;
	}
	/*void equal_test(const EccGfpPoint& point1, const EccGfpPoint& point2)
	{
		Bignum x1, y1;
		point1.get(x1, y1, ecc);
		Bignum x2, y2;
		point2.get(x2, y2, ecc);
		ASSERT_EQ(x1, x2);
		ASSERT_EQ(y1, y2);
	}*/
};

TEST_F(EccTest, reset)
{
	ASSERT_TRUE(ecc.valid());
	ASSERT_EQ(ecc.bit_width(), 256);
	ecc.reset(EccGfp::Type::std_p521r1);
	ASSERT_TRUE(ecc.valid());
	ASSERT_EQ(ecc.bit_width(), 521);
}

TEST_F(EccTest, point_reset)
{
	EccGfpPoint p;
	ASSERT_FALSE(p.valid());
	Bignum x, y;
	ASSERT_THROW(p.get(x, y), std::runtime_error);

	p.reset(ecc);
	print(p);
	ASSERT_FALSE(p.valid());
	ASSERT_TRUE(p.infinite());
}

TEST_F(EccTest, private_key)
{
	cout << "curve width=" << ecc.bit_width() << endl;

	Bignum key;
	ecc.private_key(key);
	cout << "private key ";
	print(key);
}

TEST_F(EccTest, key_pair)
{
	cout << "curve width=" << ecc.bit_width() << endl;

	Bignum key;
	EccGfpPoint pub;
	ecc.generate_key_pair(key, pub);
	cout << "private key ";
	print(key);
	cout << "public key ";
	print(pub);
	ASSERT_TRUE(pub.valid());
	ASSERT_FALSE(pub.infinite());

	EccGfpPoint inf(ecc), inf2(ecc);
	ASSERT_TRUE(inf.infinite());
	ASSERT_EQ(inf, inf2);
	ASSERT_NE(inf, pub);

	ASSERT_TRUE(EccGfp::validate_key_pair(key, pub));
	ASSERT_FALSE(EccGfp::validate_key_pair(key, inf));
}

TEST_F(EccTest, infinity)
{
	Bignum key;
	EccGfpPoint pub;
	ecc.generate_key_pair(key, pub);
	cout << "public key ";
	print(pub);
	pub.set();
	cout << "set to infinity ";
	print(pub);
}

TEST_F(EccTest, ecdsa_sign)
{
	Bignum key;
	EccGfpPoint pub;
	ecc.generate_key_pair(key, pub);		// this is the regular key pair
	cout << "sign test" << endl;
	cout << "regular private key ";
	print(key);
	cout << "public key ";
	print(pub);

	Bignum eph;
	ecc.private_key(eph);					// this is the ephemeral private key
	cout << "ephemeral private key ";
	print(eph);

	EccGfpPoint eph_pub;					// a public key derived from the ephemeral key should not validate the signature
	ecc.public_key(eph, eph_pub);
	cout << "ephemeral public key ";
	print(eph_pub);

	string sign("this is a test!");
	string sign2("another test!");

	Bignum a, b;
	EccGfp::sign_ecdsa(sign.data(), sign.size(), ecc, key, eph, a, b);
	cout << "sign a ";
	print(a);
	cout << "sign b ";
	print(b);

	ASSERT_TRUE(EccGfp::verify_ecdsa(sign.data(), sign.size(), pub, a, b));				// test with the correct data
	ASSERT_FALSE(EccGfp::verify_ecdsa(sign2.data(), sign2.size(), pub, a, b));			// test with the wrong data

	EccGfpPoint inf(ecc);		// infinity
	ASSERT_FALSE(EccGfp::verify_ecdsa(sign.data(), sign.size(), inf, a, b));			// test with an infinite public key
	
	Bignum zero;
	ASSERT_FALSE(EccGfp::verify_ecdsa(sign.data(), sign.size(), pub, a, zero));			// test with wrong signature

	ASSERT_FALSE(EccGfp::verify_ecdsa(sign.data(), sign.size(), eph_pub, a, b));		// test with the wrong public key
}

TEST_F(EccTest, ecdsa_get_and_set_binary)
{
	auto fn = [&](EccGfp::Type t)
	{
		ecc.reset(t);
		Bignum priv;
		EccGfpPoint pub;
		ecc.generate_key_pair(priv, pub);
		print(priv);
		print(pub);

		vector<char> v;
		pub.get(v);

		EccGfpPoint pub2;
		pub2.set(v, ecc);
		print(pub2);

		ASSERT_EQ(pub, pub2);
	};

	fn(EccGfp::Type::std_p192r1);
	fn(EccGfp::Type::std_p224r1);
	fn(EccGfp::Type::std_p256r1);
	fn(EccGfp::Type::std_p384r1);
	fn(EccGfp::Type::std_p521r1);
	fn(EccGfp::Type::std_p256sm2);
	//fn(EccGfp::Type::curve_25519);
	//fn(EccGfp::Type::curve_448);
}

TEST_F(EccTest, dh_shared_secret)
{
	Bignum key1, key2;
	EccGfpPoint pub1, pub2;

	ecc.generate_key_pair(key1, pub1);
	cout << "***** private key1" << endl;
	print(key1);
	cout << "***** public key1" << endl;
	print(pub1);

	ecc.generate_key_pair(key2, pub2);
	cout << "***** private key2" << endl;
	print(key2);
	cout << "***** public key2" << endl;
	print(pub2);

	Bignum share1, share2;

	EccGfp::dh_shared_secret(key1, pub2, share1);
	cout << "***** shared key1" << endl;
	print(share1);

	EccGfp::dh_shared_secret(key2, pub1, share2);
	cout << "***** shared key2" << endl;
	print(share2);

	ASSERT_EQ(share1, share2);
}
