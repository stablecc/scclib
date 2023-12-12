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
#include <crypto/cert.h>
#include <gtest/gtest.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>

/** \addtogroup crypto_cert
	@{
*/
/** Test file for \ref crypto_cert
	\file
*/
/** \example scclib/crypto/unittest/ipp/cert.cc
	
	Tests for public and private key certificates.
*/
/** @} */

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::ifstream;
using std::vector;
using scc::crypto::DerDocument;
using scc::crypto::PemDocument;
using scc::crypto::RsaPrivateKey;
using scc::crypto::RsaPublicKey;
using scc::crypto::BasePtr;

struct CertTest : public testing::Test
{
	PemDocument doc;
	PemDocument param;
	PemDocument priv;

	string reldir;

	CertTest()
	{
		// ensure the system detects bazel and finds files in the correct location
		// see https://bazel.build/reference/test-encyclopedia for a description of bazel environment
		auto sd = getenv("TEST_SRCDIR");
		if (sd)
		{
			cout << "TEST_SRCDIR=" << sd << endl;
		}

		stringstream dir;

		if (sd)
		{
			dir << sd << "/com_stablecc_scclib/crypto/unittest/openssl/";
		}
		else
		{
			dir << "openssl/";
		}

		reldir = dir.str();
	}
	virtual ~CertTest() {}

	void load(const std::string& fn, PemDocument& doc)
	{
		cout << "loading pem from " << reldir+fn << endl;
		ifstream f(reldir+fn);
		ASSERT_NO_THROW(doc.parse(f));
	}
	void load(const std::string& fn)
	{
		load(fn, doc);
	}
	void load_ecc(const std::string& fn)		// ecc private key
	{
		cout << "loading ecc pem from " << reldir+fn << endl;
		ifstream f(reldir+fn);
		// the openssl ecc file is parameters followed by private key
		ASSERT_NO_THROW(param.parse(f));
		ASSERT_NO_THROW(priv.parse(f));
	}

	void compare(const BasePtr& a, const BasePtr& b)
	{
		vector<uint8_t> av, bv;
		ASSERT_NO_THROW(DerDocument::dump_element(a, av));
		ASSERT_NO_THROW(DerDocument::dump_element(b, bv));
		ASSERT_EQ(av, bv);
	}
};

TEST_F(CertTest, rsa_certs)
{
	using scc::crypto::PublicKeyCert;
	using scc::crypto::RsaPublicKeyCert;
	using scc::crypto::RsaPrivateKeyCert;
	using scc::crypto::KeyAlgoType;

	load("osslpub.pem");						// load the openssl public key info file
	ASSERT_EQ(doc.label(), "PUBLIC KEY");

	PublicKeyCert cert;
	cert.parse(doc);

	cout << cert.str() << endl;

	ASSERT_EQ(cert.type(), KeyAlgoType::rsa);

	RsaPublicKey key;
	cert.get(key);								// get the key from the cert

	cout << key.str() << endl;

	load("rsapub.pem");							// load the openssl public key
	ASSERT_EQ(doc.label(), "RSA PUBLIC KEY");

	RsaPublicKey key2;
	RsaPublicKeyCert::parse(doc, key2);			// get the key from the cert

	cout << key2.str() << endl;

	ASSERT_EQ(key, key2);						// stored and embedded keys must be the same

	PublicKeyCert cert2;
	cert2.set(key);

	auto d1 = cert.dump();
	auto d2 = cert2.dump();

	compare(d1, d2);

	auto k2 = RsaPublicKeyCert::dump(key);

	compare(k2, doc.root_ptr());

	load("rsapriv.pem");						// load the openssl private key file
	ASSERT_EQ(doc.label(), "RSA PRIVATE KEY");

	RsaPrivateKey priv;
	RsaPrivateKeyCert::parse(doc, priv);		// pull out the private key

	cout << priv.str() << endl;

	ASSERT_EQ(key, priv.pub_key());
	ASSERT_TRUE(priv.validate(key));			// validate the key pair

	auto p2 = RsaPrivateKeyCert::dump(priv);
	compare(p2, doc.root_ptr());
}

TEST_F(CertTest, ecc_certs)
{
	using scc::crypto::PublicKeyCert;
	using scc::crypto::EcParametersCert;
	using scc::crypto::KeyAlgoType;
	using scc::crypto::EcPrivateKeyCert;
	using scc::crypto::EccGfp;
	using scc::crypto::EccGfpPoint;
	using scc::crypto::Bignum;

	load("ecpub.pem");									// load the openssl public key info file
	ASSERT_EQ(doc.label(), "PUBLIC KEY");

	PublicKeyCert cert;
	cert.parse(doc);

	cout << cert.str() << endl;

	ASSERT_EQ(cert.type(), KeyAlgoType::ec_p521r1);

	load_ecc("ecpriv.pem");
	ASSERT_EQ(param.label(), "EC PARAMETERS");			// load the parameters from the first part of openssl private key file
	ASSERT_EQ(priv.label(), "EC PRIVATE KEY");			// load the private key from second part
	cout << "** params" << endl;
	cout << param << endl;
	cout << "** priv" << endl;
	cout << priv << endl;

	KeyAlgoType algo;
	EcParametersCert::parse(param.root_ptr(), algo);	// get the algo from the parameters cert
	ASSERT_EQ(algo, KeyAlgoType::ec_p521r1);

	Bignum key;
	EccGfpPoint pub;
	EcPrivateKeyCert::parse(priv.root_ptr(), key, algo, pub);		// get private, public keys and algo from private cert
	ASSERT_EQ(algo, KeyAlgoType::ec_p521r1);

	cout << "*** private cert" << endl;
	cout << "key: " << key << endl;
	cout << "algo: " << algo << endl;
	Bignum x, y;
	pub.get(x, y);
	cout << "pub: width=( " << x.width() << " , " << y.width() << " ) val=( " << x << " , " << y << " )" << endl;

	ASSERT_TRUE(EccGfp::validate_key_pair(key, pub));		// validate the private and public key pair

	EccGfpPoint pub2;
	cert.get(pub2);				// pull out the public key from the public key certificate
	ASSERT_EQ(pub, pub2);

	// dump and compare

	BasePtr d = EcParametersCert::dump(algo);
	compare(d, param.root_ptr());

	BasePtr d2 = EcPrivateKeyCert::dump(key, algo, pub);
	compare(d2, priv.root_ptr());
}

struct CaBundleTest : public testing::Test
{
	std::vector<BasePtr> cacerts;

	CaBundleTest()
	{
		ifstream f("/etc/ssl/certs/ca-certificates.crt");
		while (1)
		{
			try
			{
				// ca bundle is a bunch of root x.509 certificates added together
				PemDocument doc;
				doc.parse(f);
				cacerts.push_back(doc.root_ptr());
			}
			catch (std::exception& ex)
			{
				cout << "loaded ca certs bundle, exception: " << ex.what() << endl;
				break;
			}
		}
	}

	virtual ~CaBundleTest() {}
};

TEST_F(CaBundleTest, sanity_test)
{
	using scc::crypto::PublicKeyCert;
	using scc::crypto::KeyAlgoType;
	using scc::crypto::EccGfpPoint;

	cout << "Loaded ca certs, size=" << cacerts.size() << endl;

	int n = 0;
	for (auto& c : cacerts)
	{
		cout << "*******CA CERT " << ++n << endl;

		// find the subjectpublickeyinfo element in the x.509 certificate
		ASSERT_TRUE(c->is_seq());
		ASSERT_GT(c->contain().size(), 0);
		ASSERT_TRUE(c->contain()[0]->is_seq());
		ASSERT_GE(c->contain()[0]->contain().size(), 7);

		PublicKeyCert cert;
		cert.parse(c->contain()[0]->contain()[6]);
		cout << cert.str() << endl;

		ASSERT_NE(cert.type(), KeyAlgoType::unknown);		// we must be able to understand the root keys

		if (cert.type() == KeyAlgoType::rsa)
		{
			RsaPublicKey key;
			cert.get(key);
			ASSERT_GT(key.width(), 0);
			cout << "rsa key width: " << key.width() << endl;
		}
		else					// this is ecdsa type
		{
			EccGfpPoint key;
			cert.get(key);
			ASSERT_TRUE(key.valid());
			cout << "ecdsa point is valid" << endl;
		}
	}
}
