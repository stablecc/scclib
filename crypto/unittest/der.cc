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
#include <chrono>
#include <ctime>
#include <system_error>
#include <encode/hex.h>
#include <util/fs.h>

/** \addtogroup crypto_der
	@{
*/
/** Test file for \ref crypto_der
	\file
*/
/** \example scclib/crypto/unittest/der.cc
	
	Test for distinguished encoding rules.
*/
/** @} */

using std::cout;
using std::endl;
using std::string;
using std::runtime_error;
using scc::encode::Hex;
using scc::crypto::DerDocument;
using scc::crypto::PemDocument;
using scc::crypto::DerBase;
using scc::crypto::DerSequence;
using scc::crypto::DerSet;
using scc::crypto::DerInteger;
using scc::crypto::DerBitString;
using scc::crypto::DerOctetString;
using scc::crypto::DerObjectIdentifier;
using scc::crypto::DerBoolean;
using scc::crypto::DerNull;
using scc::crypto::DerPrintableString;
using scc::crypto::DerIa5String;
using scc::crypto::DerBmpString;
using scc::crypto::DerUtf8String;
using scc::crypto::BasePtr;				// a shared pointer

using CharVec = std::vector<char>;

TEST(DerTest, sanity)
{
	DerBase b;
	cout << "** base dump ***" << endl;
	cout << b << endl;

	DerInteger i;
	cout << "** integer dump ***" << endl;
	cout << i << endl;

	DerSequence seq;
	cout << "** sequence dump ***" << endl;
	cout << seq << endl;

	DerSet set;
	cout << "** set dump ***" << endl;
	cout << set << endl;
}

TEST(DerTest, low_tag_len_3)
{
	// class-context, id is 10, length 3
	CharVec inv = {'\x8a','\x03'};
	for (int i = 0; i < 3; i++)		inv.push_back((char)i);

	DerDocument d;
	d.parse(inv);

	cout << "** class, id 10, length 3 dump:" << endl;
	cout << d << endl;

	auto r = d.root();

	ASSERT_TRUE(r.context_class());
	ASSERT_EQ(r.id(), 10);
	ASSERT_EQ(r.pre_len(), 2);
	ASSERT_EQ(r.len(), 3);
	for (int i = 0; i < 3; i++)
	{
		ASSERT_EQ((int)(r.data()[i]), i%256);
	}

	CharVec prev(r.pre_len());
	r.dump_pre(prev);

	for (size_t i = 0; i < prev.size(); i++)
	{
		ASSERT_EQ(inv[i], prev[i]);
	}

	CharVec alld;
	d.dump(alld);

	cout << "doc before: " << Hex::bin_to_hex(inv) << endl;
	cout << "doc after:  " << Hex::bin_to_hex(alld) << endl;

	ASSERT_EQ(inv, alld);
}

TEST(DerTest, high_tag_length_257)
{
	// class-context, 2 byte id (128 + 1 = 129), length 257 (two bytes 128 + 1)
	CharVec inv = {'\x9f','\x81','\x01','\x82','\x01','\x01'};
	for (int i = 0; i < 257; i++)	inv.push_back((char)(i%256));

	DerDocument d;
	d.parse(inv);

	cout << "** class, id 129, length 3 dump:" << endl;
	cout << d << endl;

	auto r = d.root();

	ASSERT_TRUE(r.context_class());
	ASSERT_EQ(r.id(), 129);
	ASSERT_EQ(r.len(), 257);
	ASSERT_EQ(r.pre_len(), 6);
	for (int i = 0; i < 257; i++)
	{
		ASSERT_EQ((int)(r.data()[i]), i%256);
	}

	CharVec prev(r.pre_len());
	r.dump_pre(prev);

	for (size_t i = 0; i < prev.size(); i++)
	{
		ASSERT_EQ(inv[i], prev[i]);
	}

	CharVec alld;
	d.dump(alld);

	cout << "doc before: " << Hex::bin_to_hex(inv) << endl;
	cout << "doc after:  " << Hex::bin_to_hex(alld) << endl;

	ASSERT_EQ(inv, alld);
}

TEST(DerTest, failure)
{
	DerDocument d;

	CharVec inv = CharVec({'\x8a'});	// no length byte
	ASSERT_THROW(d.parse(inv), runtime_error);

	inv = CharVec({'\x8a','\x03','\xa1','\xa2'});	// len 3, short data
	ASSERT_THROW(d.parse(inv), runtime_error);

	inv = CharVec({'\x8a','\x82','\x01','\x01','\xab'});	// multbyte length 257, short data
	ASSERT_THROW(d.parse(inv), runtime_error);

	inv = CharVec({'\x8a','\x80'});	// multbyte length malformed
	ASSERT_THROW(d.parse(inv), runtime_error);

	inv = CharVec({'\x8a','\x82','\x01'});	// multibyte insufficient length bytes
	ASSERT_THROW(d.parse(inv), runtime_error);
}

TEST(DerTest, container_and_integers)
{
	CharVec inv;

	inv = CharVec({
	'\x30', '\x1d',					// container
	'\x8a','\x03','X','Y','Z',		// context id 10
	'\x02','\x01','\x00',			// 0
	'\x02','\x01','\x7f',			// 127
	'\x02','\x02','\x00','\x80',	// 128
	'\x02','\x02','\x01','\x00',	// 256
	'\x02','\x01','\x80',			// -128
	'\x02','\x02','\xff','\x7f',	// -129
	'\x02','\x01','\xff'			// -1
	});

	DerDocument d;
	d.parse(inv);

	cout << "** container dump:" << endl;
	cout << d << endl;

	ASSERT_TRUE(d.root().is_seq());
	ASSERT_TRUE(d.root().is_contain());
	ASSERT_FALSE(d.root().is_set());
	auto& c = d.root().contain();
	ASSERT_EQ(c.size(), 8);
	ASSERT_FALSE(c[0]->is_integer());
	ASSERT_FALSE(c[0]->is_contain());
	ASSERT_TRUE(c[1]->is_integer());
	ASSERT_TRUE(c[2]->is_integer());
	ASSERT_TRUE(c[3]->is_integer());
	ASSERT_TRUE(c[4]->is_integer());
	ASSERT_TRUE(c[5]->is_integer());
	ASSERT_TRUE(c[6]->is_integer());
	ASSERT_TRUE(c[7]->is_integer());

	scc::crypto::Bignum bn;
	ASSERT_EQ(c[1]->integer(), bn);
	bn = 127;
	ASSERT_EQ(c[2]->integer(), bn);
	bn = 128;
	ASSERT_EQ(c[3]->integer(), bn);
	bn = 256;
	ASSERT_EQ(c[4]->integer(), bn);
	bn = 0;
	bn -= 128;
	ASSERT_EQ(c[5]->integer(), bn);
	bn = 0;
	bn -= 129;
	ASSERT_EQ(c[6]->integer(), bn);
	bn = 0;
	bn -= 1;
	ASSERT_EQ(c[7]->integer(), bn);

	CharVec alld;
	d.dump(alld);

	cout << "doc before: " << Hex::bin_to_hex(inv) << endl;
	cout << "doc after:  " << Hex::bin_to_hex(alld) << endl;

	ASSERT_EQ(inv, alld);

	// construct test

	DerSequence* x = new DerSequence;
	BasePtr valptr(x);

	DerBase* b = new DerBase(DerBase::class_context|0x0a);
	b->data() = std::vector<uint8_t>({'X','Y','Z'});
	x->push_back(BasePtr(b));

	DerInteger* i = new DerInteger;
	x->push_back(BasePtr(i));

	i = new DerInteger;
	i->data() = 127;
	x->push_back(BasePtr(i));

	i = new DerInteger;
	i->data() = 128;
	x->push_back(BasePtr(i));

	i = new DerInteger;
	i->data() = 256;
	x->push_back(BasePtr(i));

	i = new DerInteger;
	i->data() -= 128;
	x->push_back(BasePtr(i));

	i = new DerInteger;
	i->data() -= 129;
	x->push_back(BasePtr(i));

	i = new DerInteger;
	i->data() -= 1;
	x->push_back(BasePtr(i));

	cout << endl << "contructed doc:\n" << DerDocument::print_element(valptr) << endl;

	CharVec valv;
	DerDocument::dump_element(valptr, valv);

	ASSERT_EQ(alld, valv);

	// bad data test
	inv = CharVec({'\x03','\x04','\x08','\x6e','\x5d','\xc0'});	// pad 8, must be < 8
	ASSERT_THROW(d.parse(inv), runtime_error);
}

TEST(DerTest, bit_string)
{
	// tag 03 len 4 pad 6 bytes 01101110 (6e) 01011101 (5d) 11 (c0) (bits 18, padding 6)
	auto inv = CharVec({'\x03','\x04','\x06','\x6e','\x5d','\xc0'});

	auto outv = CharVec({'\x6e','\x5d','\xc0'});

	DerDocument d;
	d.parse(inv);

	cout << "** bitstring dump:" << endl;
	cout << d << endl;

	auto& r = d.root();
	ASSERT_TRUE(r.is_bit_string());
	auto& bs = r.bit_string();
	
	ASSERT_EQ(bs.width(), 18);
	ASSERT_EQ(bs.pad_bits(), 6);

	ASSERT_FALSE(bs.is_bit_set(0));
	ASSERT_TRUE(bs.is_bit_set(1));
	ASSERT_TRUE(bs.is_bit_set(2));
	ASSERT_FALSE(bs.is_bit_set(3));
	ASSERT_TRUE(bs.is_bit_set(4));
	ASSERT_TRUE(bs.is_bit_set(5));
	ASSERT_TRUE(bs.is_bit_set(6));
	ASSERT_FALSE(bs.is_bit_set(7));
	ASSERT_FALSE(bs.is_bit_set(8));
	ASSERT_TRUE(bs.is_bit_set(9));
	ASSERT_FALSE(bs.is_bit_set(10));
	ASSERT_TRUE(bs.is_bit_set(11));
	ASSERT_TRUE(bs.is_bit_set(12));
	ASSERT_TRUE(bs.is_bit_set(13));
	ASSERT_FALSE(bs.is_bit_set(14));
	ASSERT_TRUE(bs.is_bit_set(15));
	ASSERT_TRUE(bs.is_bit_set(16));
	ASSERT_TRUE(bs.is_bit_set(17));
	ASSERT_FALSE(bs.is_bit_set(18));
	ASSERT_FALSE(bs.is_bit_set(19));
	ASSERT_FALSE(bs.is_bit_set(20));
	ASSERT_FALSE(bs.is_bit_set(21));
	ASSERT_FALSE(bs.is_bit_set(22));
	ASSERT_FALSE(bs.is_bit_set(23));
	ASSERT_FALSE(bs.is_bit_set(24));

	CharVec dumpvec;
	bs.get(dumpvec);

	cout << " dump: " << Hex::bin_to_hexstr(dumpvec.data(), dumpvec.size(), ":", 12) << endl;
	cout << " outv: " << Hex::bin_to_hexstr(outv.data(), outv.size(), ":", 12) << endl;

	ASSERT_EQ(dumpvec, outv);

	// ensure that bitstream set/get works
	auto newv = CharVec({'\x6e','\x5d','\xff'});		// all 1s

	scc::crypto::BitString b;
	b.set(newv, 18);				// last 6 bits should be set to 0

	CharVec t;
	b.get(t);

	ASSERT_EQ(outv, t);

	// bad data test
	inv = CharVec({'\x03','\x04','\x08','\x6e','\x5d','\xc0'});	// pad 8, must be < 8
	ASSERT_THROW(d.parse(inv), runtime_error);
}

TEST(DerTest, strings)
{
	CharVec inv;

	inv = CharVec({
		'\x31',	'\x2c', // set length 34 + 10 = 44
		'\x04', '\x08', 'o','c','t','e','t','[','\x00',']', // len 8
		'\x0c', '\x04', 'u','t','f','8', // len 4
		'\x13', '\x10', 'p','r','i','n','t','a','b','l','e',' ','s','t','r','i','n','g', // len 16
		'\x16', '\x03', 'i','a','5', // len 3
		'\x1e', '\x03', 'b','m','p' // len 3
	});

	DerDocument d;
	d.parse(inv);

	cout << "** strings dump:" << endl;
	cout << d << endl;

	ASSERT_TRUE(d.root().is_set());
	ASSERT_TRUE(d.root().is_contain());
	ASSERT_FALSE(d.root().is_seq());
	auto& c = d.root().contain();
	ASSERT_EQ(c.size(), 5);

	for (int i = 0; i < 5; i++)
	{
		ASSERT_TRUE(c[i]->is_string());
	}
	ASSERT_TRUE(c[0]->is_octet_string());
	ASSERT_TRUE(c[1]->is_utf8_string());
	ASSERT_TRUE(c[2]->is_printable_string());
	ASSERT_TRUE(c[3]->is_ia5_string());
	ASSERT_TRUE(c[4]->is_bmp_string());

	ASSERT_EQ(c[2]->string(), string("printable string"));

	CharVec alld;
	d.dump(alld);

	cout << "doc before: " << Hex::bin_to_hex(inv) << endl;
	cout << "doc after:  " << Hex::bin_to_hex(alld) << endl;

	ASSERT_EQ(inv, alld);
	
	// construct test

	DerSet* x = new DerSet;
	BasePtr valptr(x);

	auto s1 = new DerOctetString;
	s1->set(CharVec({'o','c','t','e','t','[','\x00',']'}));
	x->push_back(BasePtr(s1));
	auto s2 = new DerUtf8String;
	s2->set("utf8");
	x->push_back(BasePtr(s2));
	auto s3 = new DerPrintableString;
	s3->set("printable string");
	x->push_back(BasePtr(s3));
	auto s4 = new DerIa5String;
	s4->set("ia5");
	x->push_back(BasePtr(s4));
	auto s5 = new DerBmpString;
	s5->set("bmp");
	x->push_back(BasePtr(s5));

	cout << endl << "contructed doc:\n" << DerDocument::print_element(valptr) << endl;

	CharVec valv;
	DerDocument::dump_element(valptr, valv);

	cout << "val after:   ";
	for (auto c : valv)
	{
		if (isprint(c)) cout << c;
		else cout << "<" << std::hex << (int)c << std::dec << ">";
	}
	cout << endl;

	ASSERT_EQ(alld, valv);
}

TEST(DerTest, null_and_bool)
{
	CharVec inv;

	inv = CharVec({
		'\x30',	'\x08', // sequence
		'\x05', '\x00', // null
		'\x01', '\x01', '\x00',	// false
		'\x01', '\x01', '\x01'	// true
	});

	DerDocument d;
	d.parse(inv);

	cout << "** null and bool dump:" << endl;
	cout << d << endl;

	ASSERT_TRUE(d.root().is_seq());
	ASSERT_TRUE(d.root().is_contain());
	ASSERT_FALSE(d.root().is_set());
	auto& c = d.root().contain();
	ASSERT_EQ(c.size(), 3);

	ASSERT_TRUE(c[0]->is_null());
	ASSERT_TRUE(c[1]->is_boolean());
	ASSERT_TRUE(c[2]->is_boolean());
	ASSERT_FALSE(c[1]->boolean());
	ASSERT_TRUE(c[2]->boolean());

	CharVec alld;
	d.dump(alld);

	cout << "doc before: " << Hex::bin_to_hex(inv) << endl;
	cout << "doc after:  " << Hex::bin_to_hex(alld) << endl;

	ASSERT_EQ(inv, alld);

	// construct test

	DerSequence* x = new DerSequence;
	BasePtr valptr(x);

	x->push_back(BasePtr(new DerNull));

	DerBoolean* b = new DerBoolean;
	b->set(false);
	x->push_back(BasePtr(b));
	
	b = new DerBoolean;
	b->set(true);
	x->push_back(BasePtr(b));

	cout << endl << "contructed doc:\n" << DerDocument::print_element(valptr) << endl;

	CharVec valv;
	DerDocument::dump_element(valptr, valv);

	cout << "val after:   ";
	for (auto c : valv)
	{
		if (isprint(c)) cout << c;
		else cout << "<" << std::hex << (int)c << std::dec << ">";
	}
	cout << endl;

	ASSERT_EQ(alld, valv);
}

struct Timetest : public testing::Test
{
	Timetest() {}
	virtual ~Timetest() {}

	DerDocument doc;
	void verify(int id, const string& v, const string& verify="")
	{
		CharVec inv;
		inv.push_back(id);
		inv.push_back(v.size());
		inv.insert(inv.end(), v.begin(), v.end());
		doc.parse(inv);

		cout << "*** value raw: " << v << endl;
		cout << doc << endl;

		DerBase& r = doc.root();

		ASSERT_TRUE(r.is_time());
		ASSERT_EQ(r.id(), id);
		ASSERT_TRUE(r.uni_class());
		ASSERT_FALSE(r.constructed());

		CharVec outv;
		doc.dump(outv);
		ASSERT_GT(outv.size(), 2);
		ASSERT_EQ(outv[0], (char)id);
		int sz = (int)outv[1];
		ASSERT_EQ(outv.size(), sz+2);
		string ts;
		auto i = outv.begin();
		++i;
		++i;
		ts.insert(ts.begin(), i, outv.end());
		cout << "output ts: " << ts << endl << endl;

		if (verify.size())
		{
			ASSERT_EQ(ts, verify);
		}
		else
		{
			ASSERT_EQ(ts, v);
		}
	}
	time_t make_time(int year, int month, int day, int hour, int minute, int second)
	{
		std::tm tm;
		memset(&tm, 0, sizeof(tm));

		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;
		tm.tm_isdst = -1;

		return mktime(&tm);
	}
};

// NOTE: using a parsed value such as yymmddhhmmss+0000 will not dump to same value
TEST_F(Timetest, time)
{
	ASSERT_THROW(verify(DerBase::type_utc_time, "030201120102"), runtime_error);
	ASSERT_THROW(verify(DerBase::type_utc_time, "030201120102+04"), runtime_error);
	ASSERT_THROW(verify(DerBase::type_utc_time, "03020112+04"), runtime_error);

	verify(DerBase::type_utc_time, "030201120102Z");
	verify(DerBase::type_utc_time, "0302011201Z");

	verify(DerBase::type_utc_time, "030201120102+0000", "030201120102Z");
	verify(DerBase::type_utc_time, "0302011201+0000", "0302011201Z");

	verify(DerBase::type_utc_time, "030201120102-0430", "030201163102Z");
	verify(DerBase::type_utc_time, "0302011201-0430", "0302011631Z");

	verify(DerBase::type_utc_time, "030201120102+0430", "030201073102Z");
	verify(DerBase::type_utc_time, "0302011201+0430", "0302010731Z");
	
	ASSERT_THROW(verify(DerBase::type_generalized_time, "20030201Z"), runtime_error);

	verify(DerBase::type_generalized_time, "20030201120102Z");
	verify(DerBase::type_generalized_time, "200302011201Z");
	verify(DerBase::type_generalized_time, "2003020112Z");

	verify(DerBase::type_generalized_time, "20030201120102-0430", "20030201163102Z");
	verify(DerBase::type_generalized_time, "200302011201-0430", "200302011631Z");
	verify(DerBase::type_generalized_time, "2003020112-0430", "200302011630Z");

	verify(DerBase::type_generalized_time, "20030201120102+0430", "20030201073102Z");
	verify(DerBase::type_generalized_time, "200302011201+0430", "200302010731Z");
	verify(DerBase::type_generalized_time, "2003020112+0430", "200302010730Z");

	verify(DerBase::type_generalized_time, "20030201120102-04", "20030201160102Z");
	verify(DerBase::type_generalized_time, "200302011201-04", "200302011601Z");
	verify(DerBase::type_generalized_time, "2003020112-04", "2003020116Z");

	verify(DerBase::type_generalized_time, "20030201120102+04", "20030201080102Z");
	verify(DerBase::type_generalized_time, "200302011201+04", "200302010801Z");
	verify(DerBase::type_generalized_time, "2003020112+04", "2003020108Z");

	verify(DerBase::type_generalized_time, "20030201120102.5Z", "20030201120102Z");		// no fractional seconds
	verify(DerBase::type_generalized_time, "200302011201.5Z", "20030201120130Z");
	verify(DerBase::type_generalized_time, "2003020112.5Z", "200302011230Z");

	// set the current timezone to pacific time, so we can test local time
	setenv("TZ", "/usr/share/zoneinfo/America/Los_Angeles", 1);
	auto t = make_time(2020, 3, 8, 1, 59, 59);		// standard time UTC-8
	verify(DerBase::type_generalized_time, "20200308015959", "20200308095959Z");
	ASSERT_EQ(t, doc.root().time_epoch());

	ASSERT_NE(getenv("TZ"), nullptr);
	ASSERT_EQ(string(getenv("TZ")), "/usr/share/zoneinfo/America/Los_Angeles");		// the APIs should not have modified the time zone

	unsetenv("TZ");
	tzset();
}

struct Oidtest : public testing::Test
{
	Oidtest() {}
	virtual ~Oidtest() {}

	CharVec inv;
	DerDocument doc;

	void parse(CharVec v)
	{
		inv.clear();
		inv.push_back(DerBase::type_object_identifier);
		inv.push_back(v.size());
		inv.insert(inv.end(), v.begin(), v.end());
		doc.parse(inv);

		cout << endl << "***\n" << doc << endl;

		DerBase& r = doc.root();

		ASSERT_TRUE(r.is_object_id());
		ASSERT_TRUE(r.uni_class());
		ASSERT_FALSE(r.constructed());

		CharVec outv;
		doc.dump(outv);
		ASSERT_EQ(inv, outv);
	}
	void validate(std::vector<uint32_t> v)
	{
		ASSERT_EQ(v, doc.root().object_id());

		DerObjectIdentifier* x = new DerObjectIdentifier;
		x->set(v);

		// construct test

		BasePtr valptr(x);

		cout << endl << "contructed doc:\n" << DerDocument::print_element(valptr) << endl;

		CharVec valv;
		DerDocument::dump_element(valptr, valv);
		ASSERT_EQ(inv, valv);
	}
};

TEST_F(Oidtest, oid)
{
	parse({'\x2a','\x00','\x01','\x02'});	// 1.2.0.1.2 RSA Data Security, Inc. PKCS
	validate({1,2,0,1,2});

	parse({'\x2a','\x86','\x48','\x86','\xf7','\x0d','\x01'});	// 1.2.840.113549.1 RSA Data Security, Inc. PKCS
	validate({1,2,840,113549,1});

	// failure test

	using OidVec = std::vector<uint32_t>;

	DerObjectIdentifier x;
	ASSERT_THROW(x.set(OidVec({})), runtime_error);
	ASSERT_THROW(x.set(OidVec({1})), runtime_error);
	ASSERT_THROW(x.set(OidVec({0,50})), runtime_error);
	ASSERT_THROW(x.set(OidVec({3,1})), runtime_error);
}
