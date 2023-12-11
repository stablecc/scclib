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
#include <stdexcept>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <functional>
#include <stack>
#include <ctime>
#include <cstring>
#include <fstream>
#include <encode/base64.h>
#include <encode/hex.h>

static void throw_err(const std::string& msg)
{
	throw std::runtime_error(msg);
}

using namespace scc::crypto;
using scc::encode::Hex;
using scc::encode::Base64;

std::ostream& operator<<(std::ostream& os, const DerBase& b)
{
	return os.write(b.str().c_str(), b.str().size());
}

BasePtr DerBase::create(int tag)
{
	auto id = tag & DerBase::id_mask;
	auto cls = tag & DerBase::class_mask;

	if (cls != 0)		// application, context or private
	{
		return BasePtr(new DerBase(tag));
	}

	// universal object
	switch (id)
	{
	case DerBase::type_sequence:
		return BasePtr(new DerSequence(tag));
	case DerBase::type_set:
		return BasePtr(new DerSet(tag));
	case DerBase::type_integer:
		return BasePtr(new DerInteger(tag));
	case DerBase::type_bit_string:
		return BasePtr(new DerBitString(tag));
	case DerBase::type_octet_string:
		return BasePtr(new DerOctetString(tag));
	case DerBase::type_utf8_string:
		return BasePtr(new DerUtf8String(tag));
	case DerBase::type_printable_string:
		return BasePtr(new DerPrintableString(tag));
	case DerBase::type_ia5_string:
		return BasePtr(new DerIa5String(tag));
	case DerBase::type_bmp_string:
		return BasePtr(new DerBmpString(tag));
	case DerBase::type_universal_string:
		return BasePtr(new DerUniversalString(tag));
	case DerBase::type_teletex_string:
		return BasePtr(new DerTeletexString(tag));
	case DerBase::type_null:
		return BasePtr(new DerNull(tag));
	case DerBase::type_boolean:
		return BasePtr(new DerBoolean(tag));
	case DerBase::type_utc_time:
		return BasePtr(new DerUtcTime(tag));
	case DerBase::type_generalized_time:
		return BasePtr(new DerGeneralizedTime(tag));
	case DerBase::type_object_identifier:
		return BasePtr(new DerObjectIdentifier(tag));
	}

	return BasePtr(new DerBase(tag));
}

size_t DerBase::pre_len() const
{
	size_t ret = 1;// tag
	if ((m_tag & DerBase::id_mask) == DerBase::id_mask)
	{
		uint32_t id = m_id;
		do
		{
			ret++;
			id >>= 7;		// base 128 per byte
		}
		while (id);
	}

	size_t l= len();
	if (l < 128)
	{
		ret++;
	}
	else
	{
		ret++; // the length byte

		do
		{
			ret++;
			l >>= 8;		// base 256 per byte
		}
		while (l);
	}

	return ret;
}

void DerBase::dump_pre(std::vector<char>& v) const
{
	if (v.size() < pre_len())		throw_err("dump_pre vector too small");

	int idx=0;
	v[idx] = m_tag;

	if ((m_tag & DerBase::id_mask) == DerBase::id_mask)
	{
		uint32_t i = m_id;
		std::stack<uint8_t> bs;
		do
		{
			bs.push(i & ~0x80);		// put the bottom 7 bits on the stack
			i >>= 7;				// base 128 per byte
		}
		while (i);

		while (!bs.empty())
		{
			auto b = bs.top();
			bs.pop();

			if (!bs.empty())		b |= 0x80;		// set the 8th bit on (meaning there is more to come)
			
			v[++idx] = b;
		}
	}

	size_t l = len();

	if (l < 128)
	{
		v[++idx] = l;
	}
	else
	{
		std::stack<uint8_t> bs;
		do
		{
			bs.push(l & 0xff);
			l >>= 8;						// base 256 per byte
		}
		while (l);

		v[++idx] = 0x80 | bs.size();		// number of bytes

		while (!bs.empty())
		{
			auto b = bs.top();
			bs.pop();

			v[++idx] = b;
		}
	}
}

void DerBase::dump_data(std::vector<char>& v) const
{
	if (v.size() < m_dat.size())			throw_err("dump_data vector too small");

	for (size_t i = 0; i < m_dat.size(); i++)
	{
		v[i] = static_cast<char>(m_dat[i]);
	}
}

bool DerBase::is_seq() const
{
	return typeid(*this) == typeid(DerSequence);
}

bool DerBase::is_set() const
{
	return typeid(*this) == typeid(DerSet);
}

std::vector<BasePtr>& DerBase::contain()
{
	if (!is_contain())			throw_err("invalid cast attempt to container");

	return *dynamic_cast<std::vector<BasePtr>*>(this);
}

bool DerBase::is_integer() const
{
	return typeid(*this) == typeid(DerInteger);
}

Bignum& DerBase::integer()
{
	if (!is_integer())			throw_err("invalid cast attempt to integer");

	return dynamic_cast<DerInteger*>(this)->data();
}

bool DerBase::is_bit_string() const
{
	return typeid(*this) == typeid(DerBitString);
}

BitString& DerBase::bit_string()
{
	if (!is_bit_string())			throw_err("invalid cast attempt to bit string");

	return *dynamic_cast<BitString*>(this);
}

bool DerBase::is_octet_string() const
{
	return typeid(*this) == typeid(DerOctetString);
}

bool DerBase::is_printable_string() const
{
	return typeid(*this) == typeid(DerPrintableString);
}

bool DerBase::is_utf8_string() const
{
	return typeid(*this) == typeid(DerUtf8String);
}

bool DerBase::is_ia5_string() const
{
	return typeid(*this) == typeid(DerIa5String);
}

bool DerBase::is_bmp_string() const
{
	return typeid(*this) == typeid(DerBmpString);
}

bool DerBase::is_universal_string() const
{
	return typeid(*this) == typeid(DerUniversalString);
}

bool DerBase::is_teletex_string() const
{
	return typeid(*this) == typeid(DerTeletexString);
}

bool DerBase::is_visible_string() const
{
	return typeid(*this) == typeid(DerVisibleString);
}

std::string DerBase::string()
{
	if (!is_string())			throw_err("invalid cast attempt to string");

	return dynamic_cast<DerStringBase*>(this)->string();
}

void DerBase::string(const std::string& s)
{
	if (!is_string())			throw_err("invalid cast attempt to string");

	dynamic_cast<DerStringBase*>(this)->set(s);
}

void DerBase::string_set(const std::vector<uint8_t>& s)
{
	if (!is_string())			throw_err("invalid cast attempt to string");

	dynamic_cast<DerStringBase*>(this)->set(s);
}

void DerBase::string_set(const std::vector<char>& s)
{
	if (!is_string())			throw_err("invalid cast attempt to string");

	dynamic_cast<DerStringBase*>(this)->set(s);
}

void DerBase::string_get(std::vector<uint8_t>& s)
{
	if (!is_string())			throw_err("invalid cast attempt to string");

	dynamic_cast<DerStringBase*>(this)->get(s);
}

void DerBase::string_get(std::vector<char>& s)
{
	if (!is_string())			throw_err("invalid cast attempt to string");

	dynamic_cast<DerStringBase*>(this)->get(s);
}

bool DerBase::is_null() const
{
	return typeid(*this) == typeid(DerNull);
}

bool DerBase::is_boolean() const
{
	return typeid(*this) == typeid(DerBoolean);
}

bool DerBase::boolean()
{
	if (!is_boolean())			throw_err("invalid cast attempt to boolean");

	return dynamic_cast<DerBoolean*>(this)->get();
}

void DerBase::boolean(bool v)
{
	if (!is_boolean())			throw_err("invalid cast attempt to boolean");

	return dynamic_cast<DerBoolean*>(this)->set(v);
}

bool DerBase::is_utc_time() const
{
	return typeid(*this) == typeid(DerUtcTime);
}

bool DerBase::is_generalized_time() const
{
	return typeid(*this) == typeid(DerGeneralizedTime);
}

time_t DerBase::time_epoch()
{
	if (!is_time())			throw_err("invalid cast attempt to time");

	return dynamic_cast<DerTimeBase*>(this)->epoch();
}

bool DerBase::is_object_id() const
{
	return typeid(*this) == typeid(DerObjectIdentifier);
}

oid_value DerBase::object_id()
{
	if (!is_object_id())			throw_err("invalid cast attempt to object id");

	return dynamic_cast<DerObjectIdentifier*>(this)->data();
}

void DerBase::object_id(const oid_value& v)
{
	if (!is_object_id())			throw_err("invalid cast attempt to object id");

	return dynamic_cast<DerObjectIdentifier*>(this)->set(v);
}

std::string DerBase::id_str() const
{
	if (! uni_class())
	{
		return class_str() + "_id";			// id is used by application
	}

	switch (id())
	{
	case type_boolean:
		return "type_boolean";
	case type_integer:
		return "type_integer";
	case type_bit_string:
		return "type_bit_string";
	case type_octet_string:
		return "type_octet_string";
	case type_null:
		return "type_null";
	case type_object_identifier:
		return "type_object_identifier";
	case type_utf8_string:
		return "type_utf8_string";
	case type_sequence:
		return "type_sequence";
	case type_set:
		return "type_set";
	case type_printable_string:
		return "type_printable_string";
	case type_ia5_string:
		return "type_ia5_string";
	case type_utc_time:
		return "type_utc_time";
	case type_generalized_time:
		return "type_generalized_time";
	case type_bmp_string:
		return "type_bmp_string";
	}
	return "unknown";
}

std::string DerBase::class_str() const
{
	switch (type_class())
	{
	case 0:
		return "class_universal";
	case class_application:
		return "class_application";
	case class_context:
		return "class_context";
	case class_private:
		return "class_private";
	}
	return "unknown";
}

std::string DerBase::construct_str() const
{
	if (constructed())
	{
		return "constructed";
	}
	return "primitive";
}

std::string DerBase::str(uint32_t max_line) const
{
	std::stringstream s;
	
	s << "id " << id_str() << "(" << id() << ") " << construct_str() << " len " << len() << data_str();
	
	return s.str();
}

DerContainerBase::~DerContainerBase()
{
}

size_t DerContainerBase::len() const
{
	size_t r = 0;
	for (auto& i : *this)
	{
		r += i->pre_len();
		r += i->len();
	}
	return r;
}

std::string DerContainerBase::data_str() const
{
	std::stringstream s;

	s << " items " << size();

	return s.str();
}

static std::string bin_print(const void* xloc, size_t len, size_t max=12)
{
	char* loc = (char*)xloc;
	std::stringstream s;

	while (len)
	{
		if (!max)
		{
			s << " +more";
			break;
		}
		if (isprint(*loc))
		{
			s << *loc;
		}
		else
		{
			s << ".";
		}
		loc++;
		len--;
		max--;
	}

	return s.str();
}

static std::string bn_print(const Bignum& bn, unsigned max=8)
{
	std::vector<char> v(bn.len_2sc(), '\0');

	bn.get_2sc((char*)v.data(), v.size());

	return Hex::bin_to_hexstr(v.data(), v.size(), ":", max);
}

std::string DerBase::data_str() const
{
	std::stringstream s;

	s << " str " << bin_print(m_dat.data(), m_dat.size());

	s << " hex " << Hex::bin_to_hexstr(m_dat.data(), m_dat.size(), ":", 8);

	return s.str();
}

std::string DerObjectIdentifier::oid_str() const
{
	std::stringstream s;

	if (!m_v.size())		return "(empty oid)";

	s << m_v;

	return s.str();
}

std::ostream& operator<<(std::ostream& s, const std::vector<uint32_t>& oid)
{
	std::stringstream outs;

	for (auto i = oid.begin(); i != oid.end(); ++i)
	{
		if (i != oid.begin())		s << ".";
		s << *i;
	}
	return s;
}

void DerObjectIdentifier::parse(const std::vector<uint8_t>& v)
{
	if (v.size() == 0)			throw_err("oid parse error no data");

	m_v.resize(2);
	m_v[0] = v[0]/40;
	m_v[1] = v[0]%40;

	size_t i = 1;
	while (i < v.size())
	{
		std::stack<uint8_t> bs;
		while (i < v.size())		// parse base 128 multilength byte
		{
			auto x = v[i++];
			bs.push(x & ~0x80);
			if ((x & 0x80) == 0)		break;		// finished when first bit is 0
		}
		uint32_t nxt = 0;
		uint32_t mult = 1;
		while (!bs.empty())
		{
			nxt += bs.top()*mult;
			bs.pop();
			mult <<= 7;
		}
		m_v.push_back(nxt);
	}
}

std::string DerObjectIdentifier::data_str() const
{
	std::stringstream s;

	s << " oid ";
	for (size_t i = 0; i < m_v.size(); i++)
	{
		s << m_v[i];
		if (i != m_v.size()-1)		s << ".";
	}

	return s.str();
}

size_t DerObjectIdentifier::len() const
{
	int len = 1;
	for (size_t i = 2; i < m_v.size(); i++)
	{
		auto nxt = m_v[i];
		do
		{
			nxt >>= 7;						// base 128
			len++;
		}
		while (nxt);
	}
	return len;
}

void DerObjectIdentifier::dump_data(std::vector<char>& v) const
{
	v[0] = m_v[0]*40 + m_v[1];

	int idx = 1;
	for (size_t i = 2; i < m_v.size(); i++)
	{
		auto nxt = m_v[i];
		std::stack<uint8_t> bs;
		do
		{
			bs.push(nxt & 0x7f);			// 7 bits
			nxt >>= 7;						// base 128
		}
		while (nxt);

		while (!bs.empty())
		{
			auto x = bs.top() | 0x80;		// set continuation bit
			bs.pop();
			if (bs.empty())		x &= ~0x80;
			v[idx++] = x;
		}
	}
}

void DerObjectIdentifier::set(const oid_value& v)
{
	if (v.size() < 2)		throw_err("oid missing v1 and v2 values");
	if (v[0] > 2)			throw_err("oid v1 out of range");
	if (v[1] > 39)			throw_err("oid v2 out of range");
	m_v = v;
}

#include <iostream>

// set time in the utc timezone (with an offset from utc time)
void DerTimeBase::set_time(int year, int month, int day, int hour, int minute, int second, int tzmins)
{
	std::tm tm;
	memset(&tm, 0, sizeof(tm));

	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = minute - tzmins;
	tm.tm_sec = second;
	tm.tm_isdst = -1;

	m_t = timegm(&tm);
}

// set time in the local timezone
void DerTimeBase::set_time(int year, int month, int day, int hour, int minute, int second)
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

	m_t = mktime(&tm);
}

std::string DerTimeBase::data_str() const
{
	std::stringstream s;

	std::tm tm;
	auto t = m_t;
	localtime_r(&t, &tm);
	s << " local " << std::put_time(&tm, "%F %T %Z");
	gmtime_r(&t, &tm);
	s << " utc " << std::put_time(&tm, "%F %T UTC");
	return s.str();
}

// return true if tz postfix found
// set tzval with value and tzsize with the size of the postfix
static bool parse_tz(std::string& v, int& tzval, int& tzsize)
{
	auto p = v.find_last_of("+-Z");
	
	if (p == std::string::npos)		return false;

	tzsize = v.size()-p;

	if (v[p] == 'Z')
	{	
		if (tzsize > 1)		throw std::runtime_error(std::string("timezone parse error on ")+v);
		
		v.resize(p);
		tzval = 0;
		return true;
	}

	if (v[p] != '+' && v[p] != '-')			throw std::runtime_error(std::string("timezone parse error on ")+v);

	int sign = 1, hh, mm = 0;
	if (v[p] == '-')	sign = -1;

	int n = sscanf(&v[p+1], "%02d%02d", &hh, &mm);

	if (n < 1)	throw std::runtime_error(std::string("timezone parse error on ")+v);

	v.resize(p);
	tzval = sign*(hh*60+mm);
	return true;
}

void DerUtcTime::parse(const std::vector<uint8_t>& vin)
{
	std::string v(&vin[0], &vin[0]+vin.size());
	
	int tzmins, tzsize;
	
	if (!parse_tz(v, tzmins, tzsize))		throw_err("utc time no timezone found");

	if (tzsize > 1 && tzsize < 5)			throw_err("utc time requires Z or +/-HHMM format");

	unsigned yy, mm, dd, h, m, s = 0;

	if (v.size() != 10 && v.size() != 12)	throw_err("utc time parse error YYMMDDhhmm[ss] wrong size");

	int n = sscanf(&v[0], "%02u%02u%02u%02u%02u%02u", &yy, &mm, &dd, &h, &m, &s);
	if (n < 5)		throw_err("utc time parse error YYMMDDhhmm[ss]");

	set_time(yy < 70 ? yy + 2000 : yy + 1900, mm, dd, h, m, s, tzmins);
}

void DerUtcTime::dump_data(std::vector<char>& v) const
{
	std::tm tm;
	time_t t = m_t;
	gmtime_r(&t, &tm);

	sprintf(&v[0], "%02d%02d%02d%02d%02d",
		tm.tm_year < 70 ? tm.tm_year : tm.tm_year-100,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min);

	int idx = 10;
	if (tm.tm_sec)
	{
		sprintf(&v[idx], "%02d", tm.tm_sec);
		idx += 2;
	}
	v[idx] = 'Z';
}

size_t DerUtcTime::len() const
{
	int ret = 11; // yymmddhhmmZ
	
	if (m_t%60)
	{
		ret += 2;			// ss
	}
	return ret;
}

// parse fracional part , or . as decimal followed by digits
// if found, trims string to remove
static double parse_frac(std::string& v)
{
	auto p = v.find_last_of(",.");
	if (p == std::string::npos)		return 0.0;

	v[p] = '.';			// change comma to a "fullstop"

	double ret;
	int n = sscanf(&v[p], "%lf", &ret);

	if (n < 1)			return -1.0;

	v.resize(p);
	return ret;
}

void DerGeneralizedTime::parse(const std::vector<uint8_t>& vin)
{
	std::string v(&vin[0], &vin[0]+vin.size());
	
	int tzmins, tzsize;

	bool tzpost = parse_tz(v, tzmins, tzsize); // parse tz, expecting nothing or Z|+-hh[mm]

	double frac = parse_frac(v);
	if (frac < 0.0)			throw_err("generalized time parse error fractional part");

	if (v.size() < 10)		throw_err("generalized time parse error YYYYMMDDhh wrong size");

	unsigned yy, mm, dd, h, m=0, s=0;

	int n = sscanf(&v[0], "%4u%02u%02u%02u", &yy, &mm, &dd, &h);
	if (n < 4)		throw_err("generalized time parse error YYYYMMDDhh");

	if (v.size() == 10)
	{
		if (frac > 0.0)			// fractional hours
		{
			m += frac*60.0;
		}
	}
	else if (v.size() == 12)
	{
		n = sscanf(&v[10], "%02u", &m);
		if (n < 1)		throw_err("generalized time parse error mm");
		
		if (frac > 0.0)			// fractional minutes
		{
			s += frac*60.0;
		}
	}
	else if (v.size() == 14)
	{
		n = sscanf(&v[10], "%02u%02u", &m, &s);
		if (n < 2)		throw_err("generalized time parse error mmss");

		// ignore fractional seconds
	}
	else
	{
		throw_err("generalized time parse error YYYYMMDDhh wrong size");
	}

	if (tzpost)
	{
		set_time(yy, mm, dd, h, m, s, tzmins);
	}
	else
	{
		set_time(yy, mm, dd, h, m, s);
	}
}

void DerGeneralizedTime::dump_data(std::vector<char>& v) const
{
	std::tm tm;
	time_t t = m_t;
	gmtime_r(&t, &tm);

	sprintf(&v[0], "%4d%02d%02d%02d",
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour);

	int idx = 10;
	if (tm.tm_min)
	{
		sprintf(&v[idx], "%02d", tm.tm_min);
		idx += 2;
	}
	if (tm.tm_sec)
	{
		sprintf(&v[idx], "%02d", tm.tm_sec);
		idx += 2;
	}
	v[idx] = 'Z';
}

size_t DerGeneralizedTime::len() const
{
	int ret = 11; //yyyymmddhhZ
	
	if (m_t%60)
	{
		ret += 4;		// mmss
	}
	else if (m_t%3600)
	{
		ret += 2;		// mm
	}
	return ret;
}

void DerStringBase::parse(const std::vector<uint8_t>& v)
{
	m_val.resize(v.size());

	for (size_t i = 0; i < v.size(); i++)
	{
		m_val[i] = v[i];
	}
}

std::string DerStringBase::data_str() const
{
	std::stringstream s;

	s << " str " << bin_print(m_val.data(), m_val.size(), 80);

	return s.str();
}

void DerStringBase::dump_data(std::vector<char>& v) const
{

	for (size_t i = 0; i < m_val.size(); i++)
	{
		v[i] = m_val[i];
	}
}

void DerBoolean::parse(const std::vector<uint8_t>& v)
{
	m_bool = v[0] == 0 ? false : true;
}

void DerBoolean::dump_data(std::vector<char>& v) const
{
	v[0] = m_bool ? 1 : 0;
}

void DerInteger::parse(const std::vector<uint8_t>& v)
{
	m_bn.set_2sc(&v[0], v.size());		// twos complement input
}

std::string DerInteger::data_str() const
{
	std::stringstream s;

	s << " width " << m_bn.width();
	
	if (m_bn <= 0xffffffff)
	{
		s << " dec " << m_bn;
	}

	s << " hex " << bn_print(m_bn);

	return s.str();
}

size_t DerInteger::len() const
{
	return m_bn.len_2sc();
}

void DerInteger::dump_data(std::vector<char>& v) const
{
	m_bn.get_2sc(&v[0], v.size());
}

void DerBitString::parse(const std::vector<uint8_t>& v)
{
	if (v.size() == 0)
	{
		width(0);
		return;
	}

	uint8_t pad = v[0];				// number of 0 bits added to the end (unused bits)

	if (pad > 7)				throw_err("bit string parse error pad bits too high");

	m_data.clear();
	width((v.size()-1)*8 - pad);	// set the bit width (remaining array size - pad)

	// the BitString is now v.size()-1 in length

	for (size_t i = 1; i < v.size(); i++)
	{
		m_data.at(i-1)= v[i];
	}
}

std::string DerBitString::data_str() const
{
	if (m_data.size() < len()-1)			throw_err("bitstring data_str invalid size");

	std::stringstream s;

	s << " width " << width() << " pad bits " << pad_bits();
	
	s << " hex " << Hex::bin_to_hexstr(m_data.data(), m_data.size(), ":", 12);

	s << " bits";
	
	int max = 3;
	uint32_t w = width();
	for (uint32_t i = 0; i < w; i++)
	{
		if (i%8 == 0)
		{
			if (max == 0)
			{
				s << " +more";
				break;
			}
			s << " ";
			max--;
		}
		
		s << (is_bit_set(w-1-i) ? "1" : "0");		// print out most significant first
	}

	return s.str();
}

size_t DerBitString::len() const
{
	return m_data.size() + 1;		// bit data + pad byte
}

void DerBitString::dump_data(std::vector<char>& v) const
{
	v[0] = pad_bits();

	for (size_t i = 1; i < v.size(); i++)
	{
		v[i] = m_data.at(i-1);
	}
}

BasePtr DerBase::context_to_explicit(const BasePtr& ctx)
{
	if (!ctx->context_class() || !ctx->constructed())		throw_err("context_to_explicit() element must be constructed and context-class");

	BasePtr base = DerDocument::parse_element(ctx->m_dat);		// return a new element using the DER-encoded data

	return base;
}

BasePtr DerBase::explicit_to_context(const BasePtr& orig, uint32_t id)
{
	// NOTE: dump rework is next

	std::vector<uint8_t> v;
	DerDocument::dump_element(orig, v);

	BasePtr base = DerBase::create(DerBase::construct_mask|DerBase::class_context);
	base->id(id);
	base->parse(v);		// set the dumped data into the element

	return base;
}


BasePtr DerBase::context_to_implicit(const BasePtr& ctx, uint32_t id)
{
	if (id >= DerBase::id_mask)				throw_err("context_to_implicit() invalid id");
	if (ctx->id() >= DerBase::id_mask)		throw_err("context_to_implicit() context multi-byte id not supported");
	if (!ctx->context_class())				throw_err("context_to_implicit() context element must be context-class");

	std::vector<uint8_t> v;
	DerDocument::dump_element(ctx, v);

	v[0] = (v[0] & ~DerBase::id_mask) | id;				// set the id
	// constructed flag stays the same
	v[0] &= ~class_mask;								// clear the class to set the universal class 0

	BasePtr base = DerDocument::parse_element(v);		// return a new element from the modified element binary

	return base;
}

BasePtr DerBase::implicit_to_context(const BasePtr& orig, uint32_t id)
{
	if (id >= DerBase::id_mask)				throw_err("implicit_to_context() invalid id");
	if (orig->id() >= DerBase::id_mask)		throw_err("implicit_to_context() original multi-byte id not supported");
	if (!orig->uni_class())					throw_err("implicit_to_context() original element must be universal class");
	
	std::vector<uint8_t> v;
	DerDocument::dump_element(orig, v);
	
	v[0] = (v[0] & ~DerBase::id_mask) | id;		// set the id
	v[0] |= class_context;						// set class context

	BasePtr base = DerDocument::parse_element(v);	// return a new element from the modified element binary

	return base;
}

BasePtr DerBase::create(const std::vector<uint8_t>& v, size_t off)
{
	if (off == v.size())			throw_err("create() no tag byte");

	BasePtr base = DerBase::create(v[off]);

	base->m_eloff = off;
	base->m_hdrsz = 1;

	if ((v[off] & DerBase::id_mask) == DerBase::id_mask)		// this is a high-tag id
	{
		std::stack<uint8_t> idb;
		do
		{
			if (++off == v.size())				throw_err("create() no id byte");
			base->m_hdrsz++;
			idb.push(v[off] & 0x7f);			// bits 1-7 are the id value (base 128, high byte first)
		}
		while ((v[off] & 0x80) == 0x80);		// while 8th bit is 1, there is another id byte coming

		uint32_t id = 0;
		uint32_t mult = 1;

		while (!idb.empty())					// build up the base 128 id
		{
			auto b = idb.top();
			idb.pop();
			id += mult*b;
			mult *= 128;
		}

		base->id(id);
	}

	if (++off == v.size())			throw_err("create() no length byte");
	base->m_hdrsz++;

	if (v[off] & DerBase::length_multi_mask)
	{
		size_t lensz = v[off] & DerBase::length_bytes_mask;

		if (lensz == 0)
		{
			// len = 0x80 means indefinite-sized
			// i.e. ended with octets 00 00
			// DER requires that elements be definite-sized
			throw_err("create() indefinite-sized elements not supported by DER");
		}

		for (size_t i = 0; i < lensz; i++)
		{
			if (++off == v.size())		throw_err("create() insufficient extended length bytes");
			base->m_hdrsz++;
			base->m_elsz += ((size_t)v[off] << (lensz-i-1)*8);		// base 256, high byte first
		}
	}
	else
	{
		base->m_elsz = v[off];
	}

	return base;
}

BasePtr DerDocument::parse_element(const std::vector<uint8_t>& binv, size_t off)
{
	BasePtr base;

	base = DerBase::create(binv, off);

	if (!base->is_contain())
	{
		if (off+base->hdrsz()+base->elsz() > binv.size())			throw_err(base->name()+" parse_element binary size mismatch");

		// NOTE: at some point, should rework to avoid object creation
		base->parse(std::vector<uint8_t>(&binv[off+base->hdrsz()], &binv[off+base->hdrsz()+base->elsz()]));

		return base;
	}

	// if this is a container, parse and store the sub-elements as constructed data

	off += base->hdrsz();					// move offset to the beginning of data
	size_t elsz = off+base->elsz();			// keep track of the container size
	
	while (off < elsz)
	{
		BasePtr nxt = parse_element(binv, off);
		base->contain().emplace_back(nxt);
		off += nxt->hdrsz() + nxt->elsz();
	}

	// sanity check, size of container element must match sizes of contained elements

	size_t allsz = 0;
	for (auto& e : base->contain())
	{
		allsz += e->hdrsz()+e->elsz();
	}
	if (allsz != base->elsz())			throw_err("parse_element() container/element size mismatch");

	return base;
}

void DerDocument::parse_bin()
{
	m_root.reset();
	if (!m_bin.empty())		m_root = parse_element(m_bin);			// the offsets are now correct
}

void DerDocument::parse(const std::vector<char>& v)
{
	m_bin.clear();
	m_bin.insert(m_bin.begin(), v.begin(), v.end());
	parse_bin();
}

void DerDocument::parse(std::istream& s)
{
	m_bin.clear();
	s >> m_bin;
	parse_bin();
}

void DerDocument::dump_element(const BasePtr& base, std::vector<uint8_t>& vout)
{
	if (base == nullptr)				return;

	std::vector<char> v(base->pre_len());
	base->dump_pre(v);
	vout.insert(vout.end(), v.begin(), v.end());		// append prefix bytes

	if (base->is_contain())
	{
		for (auto& x : base->contain())
		{
			if (base == nullptr)		throw_err("dump_element() container element with null object");

			dump_element(x, vout);
		}
	}
	else	// dump data if this is not a container
	{
		v.resize(base->len());
		base->dump_data(v);
		vout.insert(vout.end(), v.begin(), v.end());		// append data bytes
	}
}

bool DerDocument::equal(const DerDocument& other) const
{
	if (m_root == nullptr && other.m_root == nullptr)
	{
		return true;
	}

	std::vector<char> d1, d2;

	dump_element(m_root, d1);
	dump_element(other.m_root, d2);

	bool ret = (d1 == d2);

	explicit_bzero(d1.data(), d1.size());
	explicit_bzero(d2.data(), d2.size());

	return ret;
}

void DerDocument::dump(std::vector<char>& vout)
{
	if (m_root == nullptr)			throw_err("dump() called on empty document");

	dump_bin();
	vout.insert(vout.end(), m_bin.begin(), m_bin.end());
}

void DerDocument::dump_bin()
{
	m_bin.clear();
	dump_element(m_root, m_bin);
	parse_bin();					// the offsets are now correct
}

void DerDocument::dump(std::ostream& s)
{
	dump_bin();
	s << m_bin;
}

DerBase& DerDocument::root()
{
	if (m_root == nullptr)			throw_err("root() called on empty document");

	return *m_root;
}

struct PHelp
{
	bool debug;
	std::string indent;
	std::stringstream s;
};

static void print_helper(PHelp& ph, BasePtr base, int level)
{
	if (base == nullptr)
	{
		ph.s << "<empty>";
		return;
	}

	for (int i = 0; i < level; i++)		ph.s << ph.indent;

	if (ph.debug) ph.s << "(" << base->eloff() << "," << base->elsz() << "," << base->hdrsz() << ") ";
		
	ph.s << *base;

	if (base->is_contain())
	{
		level++;
		for (auto& x : base->contain())
		{
			ph.s << std::endl;
			print_helper(ph, x, level);
		}
	}
}

std::string DerDocument::print_element(const BasePtr& base, bool debug, const std::string& indent)
{
	PHelp ph;
	ph.debug = debug;
	ph.indent = indent;

	print_helper(ph, base, 0);

	return ph.s.str();
}

std::string DerDocument::str(bool debug) const
{
	std::stringstream s;

	if (debug) s << "bin_sz(" << m_bin.size() << ") " << std::endl;

	return s.str()+print_element(m_root, debug);
}

std::ostream& operator<<(std::ostream& os, const DerDocument& b)
{
	return os.write(b.str().c_str(), b.str().size());
}

void PemDocument::parse(const std::vector<char>& v)
{
	std::stringstream sst;
	sst.write(v.data(), v.size());
	parse(sst);
	explicit_bzero(sst.str().data(), sst.str().size());
}

void PemDocument::parse(std::istream& sst)
{
	std::string l;
	const auto np = std::string::npos;

	// read forward until begin line is found
	while (1)
	{
		if (!std::getline(sst, l))			throw_err("PEM input end of stream before BEGIN");
		auto p = l.find("-----BEGIN ");
		
		if (p != np)		break;			// found begin line
	}

	auto ep = l.rfind("-----");
	if (ep == np)						throw_err("PEM input BEGIN line does not end with -----");

	m_label = l.substr(11, l.size()-16);

	if (m_label.empty())				throw_err("PEM input zero length label");

	std::string b64;
	m_chars_per_line = 0;
	while (std::getline(sst, l))
	{
		auto p = l.find("-----END ");
		if (p != np)
		{
			ep = l.rfind("-----");
			if (ep == np)						throw_err("PEM input END does not end with -----");

			auto elabel = l.substr(9, l.size()-14);

			if (m_label != elabel)				throw_err("PEM input BEGIN and END labels do not match");

			if (!Base64::base64_decode(b64, m_bin))		throw_err("PEM input invalid base64 data");
			
			explicit_bzero(b64.data(), b64.size());
			
			parse_bin();
			return;
		}
		b64.insert(b64.end(), l.begin(), l.end());
		if (m_chars_per_line < l.size())
		{
			m_chars_per_line = l.size();
		}
	}

	// if we got here, there was no end found
	throw_err("PEM input no ----END found");
}

void PemDocument::dump(std::vector<char>& v)
{
	if (m_chars_per_line == 0)		throw_err("PEM output cannot have 0 chars per line");
	if (m_label.empty())			throw_err("PEM output zero length label");

	DerDocument::dump_bin();		// dump the binary document

	std::string b64;
	Base64::base64_encode(m_bin, b64);

	std::string l;
	l = "-----BEGIN "+m_label+"-----\n";
	v.insert(v.end(), l.begin(), l.end());

	size_t sz = b64.size(), towrite = sz;
	while (towrite)
	{
		size_t lw = towrite > m_chars_per_line ? m_chars_per_line : towrite;
		v.insert(v.end(), &b64[sz-towrite], &b64[sz-towrite]+lw);
		towrite -= lw;
		v.insert(v.end(), '\n');
	}

	l = "-----END "+m_label+"-----\n";
	v.insert(v.end(), l.begin(), l.end());

	explicit_bzero(b64.data(), b64.size());
}

void PemDocument::dump(std::ostream& s)
{
	SecVecChar v;
	dump(v);
	s << v;
}
