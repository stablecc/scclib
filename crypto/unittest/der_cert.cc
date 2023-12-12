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
#include <crypto/der.h>
#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <memory>
#include <sstream>
#include <cstdlib>

/** \addtogroup crypto_der
	@{
*/
/** Test file for \ref crypto_der
	\file
*/
/** \example scclib/crypto/unittest/der_cert.cc
	
	Test for distinguished encoding rules. Validates some pre-generated certificates.
*/
/** @} */

using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::shared_ptr;
using std::stringstream;
using scc::crypto::DerDocument;
using scc::crypto::PemDocument;

struct DerCertTest : public testing::Test
{
	string reldir;

	DerCertTest()
	{
		// detect the bazel workspace environment, and create a relative path to the data files
		auto sd = getenv("TEST_SRCDIR");
		auto wd = getenv("TEST_WORKSPACE");

		stringstream dir;

		if (sd)
		{
			dir << sd << "/" << wd << "/crypto/unittest/openssl/";
		}
		else
		{
			dir << "openssl/";
		}

		reldir = dir.str();
	}

	static shared_ptr<PemDocument> pem_doc()
	{
		shared_ptr<PemDocument> ret(new PemDocument);
		return ret;
	}

	static shared_ptr<DerDocument> der_doc()
	{
		shared_ptr<DerDocument> ret(new DerDocument);
		return ret;
	}

	void parse(shared_ptr<PemDocument> doc, const string& name)
	{
		string infile = reldir+name;
		cout << "opening " << infile << endl;
		ifstream f(infile);
		ASSERT_TRUE(f.is_open());
		ASSERT_NO_THROW(doc->parse(f));
		cout << "***** PEM " << infile << " label=" << doc->label() << " chars=" << doc->chars_per_line() << ":" << endl;
		cout << *doc << endl;
	}

	void parse(shared_ptr<DerDocument> doc, const string& name)
	{
		string infile = reldir+name;
		cout << "opening " << infile << endl;
		ifstream f(infile);
		ASSERT_TRUE(f.is_open());
		ASSERT_NO_THROW(doc->parse(f));
		cout << "***** DER " << infile << ":" << endl;
		cout << *doc << endl;
	}
};

TEST_F(DerCertTest, rsapriv)
{
	auto pem = pem_doc();
	parse(pem, "rsapriv.pem");	
	auto der = der_doc();
	parse(der, "rsapriv.crt");
	ASSERT_TRUE(pem->equal(*der));
}

TEST_F(DerCertTest, rsapub)
{
	auto pem = pem_doc();
	parse(pem, "rsapub.pem");	
	auto der = der_doc();
	parse(der, "rsapub.crt");
	ASSERT_TRUE(pem->equal(*der));
}

TEST_F(DerCertTest, rsacert)
{
	auto pem = pem_doc();
	parse(pem, "rsacert.pem");	
	auto der = der_doc();
	parse(der, "rsacert.crt");
	ASSERT_TRUE(pem->equal(*der));
}

TEST_F(DerCertTest, ecpriv)
{
	auto pem = pem_doc();
	parse(pem, "ecpriv.pem");	
}

TEST_F(DerCertTest, ecpub)
{
	auto pem = pem_doc();
	parse(pem, "ecpub.pem");	
}

TEST_F(DerCertTest, eccert)
{
	auto pem = pem_doc();
	parse(pem, "eccert.pem");	
}

TEST_F(DerCertTest, rsacert_dumptest)
{
	auto pem = pem_doc();
	parse(pem, "rsacert.pem");	
	stringstream outs;
	pem->dump(outs);
	cout << "**** NEW CERT:" << endl;
	cout << outs.str() << endl;
	// now read a new doc
	auto newpem = pem_doc();
	ASSERT_NO_THROW(newpem->parse(outs));
	ASSERT_EQ(pem->chars_per_line(), newpem->chars_per_line());
	ASSERT_EQ(pem->label(), newpem->label());
	ASSERT_TRUE(pem->equal(*newpem));
}
