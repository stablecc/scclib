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
#ifndef _SCC_CRYPTO_DER_H
#define _SCC_CRYPTO_DER_H

#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <cstring>
#include <memory>
#include <crypto/bignum.h>
#include <crypto/secvec.h>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_der Distinguished encoding rules (DER)
	@{

	DER (Distinguished Encoding Rules), is a subset of the binary packaging format ASN.1 commonly applied to cryptography objects.

	ASN.1: https://tools.ietf.org/html/rfc6025
	X.680 ASN.1 basic rules: https://www.itu.int/rec/T-REC-X.680/en (defines all universal types)
	DER specification detail X.690: https://www.itu.int/rec/T-REC-X.690-201508-I/en
	
	Technical note for DER format: http://luca.ntop.org/Teaching/Appunti/asn1.html

	This contains enough to parse X.509 certificates.
	X.509 certificates: https://tools.ietf.org/html/rfc5280
	Information on algorithms used in certificates: https://tools.ietf.org/html/rfc3279
	PKCS exchange syntax: https://tools.ietf.org/html/rfc7292

	PEM format is base64 encoded DER format.
	Certificate encoding using DER and PEM format: https://tools.ietf.org/html/rfc7468

	https://tools.ietf.org/html/rfc3447 contains info on rsa private key asn.1 syntax.

*/

/** Distinguished encoding rules (DER).
	\file der.h
*/

/** Bit string.

	A bit string is a sequence of bits, with a bit width.
*/
class BitString
{
	friend class DerBitString;
	std::vector<uint8_t> m_data;			// bit string bits 0-width at the beginning. The rest will be padded with zeroes.
	uint32_t m_width;						// bit width will always be <= m_data width
public:
	BitString() : m_width(0) {}
	virtual ~BitString() {}
	
	/** Bit width. */
	uint32_t width() const { return m_width; }
	
	/** Number of padding bits (0 bits at the end of the last byte). */
	uint32_t pad_bits() const
	{
		return m_width%8 == 0 ? 0 : 8 - m_width%8;
	}
	
	/** Set width in bits. Resizes the storage, and pads with 0 bits if necessary. */
	void width(uint32_t v)
	{
		m_width = v;
		m_data.resize(((v+7)&~7)/8, '\x00');
		for (uint32_t i = 0; i < pad_bits(); i++)
		{
			m_data.at(m_data.size()-1) &= ~(1 << i);			// set the bits in the last byte to 0
		}
	}
	
	/** Set the bit string from a bit string input.
		
		Result will be a bit string of width w, beginning with v.
	*/
	void set(const std::vector<uint8_t>& v, size_t w)
	{
		m_data = v;
		width(w);
	}
	void set(const std::vector<char>& v, size_t w)
	{
		m_data.clear();
		m_data.insert(m_data.begin(), v.begin(), v.end());
		width(w);
	}

	/** Is the bit set? First bit is bit 0. */
	bool is_bit_set(uint32_t bit) const
	{
		if (bit >= m_width)					return false;

		uint8_t mask = 1 << (7-bit%8);		// msb is bit 0
		
		return (m_data.at(bit/8) & mask) == mask ? true : false;
	}
	
	/** Set a bit. First bit is bit 0. */
	void set_bit(uint32_t bit, bool set=true)
	{
		if (bit >= m_width)		return;

		uint8_t mask = 1 << (7-bit%8);
		
		if (set)	m_data.at(bit/8) |= mask;
		else		m_data.at(bit/8) &= ~mask;
	}
	
	/** Clear a bit. */
	void clear_bit(uint32_t bit)
	{
		set_bit(bit, false);
	}
	
	/** Resize and get the vector. Ensures that all padded bits are 0. */
	void get(std::vector<uint8_t>& v) const
	{
		v = m_data;		// the last bits are always set to 0
	}
	void get(std::vector<char>& v) const
	{
		v.clear();
		v.insert(v.begin(), m_data.begin(), m_data.end());
	}
};

class DerBase;
using BasePtr = std::shared_ptr<DerBase>;

using oid_value = std::vector<uint32_t>;

/** ASN.1 base.

	All objects can be manipulated using unique_ptr to Base object.
*/
class DerBase
{
	friend class DerDocument;
	friend class X509Cert;

	std::vector<uint8_t> m_dat;
	size_t m_eloff;
	size_t m_elsz;
	size_t m_hdrsz;
protected:
	uint8_t m_tag;
	uint32_t m_id;
public:
	enum Tag
	{
		id_mask = 					0x1f,	// bits 1-5 are the tag id, if they are all 1s, this is a multi-byte id
		construct_mask =			0x20,	// bit 6 is the constructed mask (otherwise primitive)
		class_mask =				0xc0,	// bits 7-8 are the class, 00 means universal
		class_application =			0x40,	// 01
		class_context = 			0x80,	// 10
		class_private =				0xc0,	// 11
		length_multi_mask =			0x80,	// bit 8 is on if this is a multi-byte length
		length_bytes_mask =			0x7f,	// bits 1-7 give length for multi-byte length
		type_boolean	=			1,
		type_integer =				2,
		type_bit_string =			3,
		type_octet_string =			4,
		type_null =					5,
		type_object_identifier =	6,
		type_utf8_string =			12,	// 0x0c
		type_sequence =				16,	// 0x10
		type_set =					17,	// 0x11
		type_printable_string = 	19,	// 0x13
		type_teletex_string = 		20,	// 0x14
		type_ia5_string =			22,	// 0x16
		type_utc_time =				23,	// 0x17
		type_generalized_time =		24,	// 0x18
		type_visible_string =		26,	// 0x1a
		type_universal_string =		28,	// 0x1c
		type_bmp_string =			30,	// 0x1e
	};

	/** Construct a base object.

		NOTE: An Id less than 0x1f can be embedded in the tag. If an id > 30 is required, must call the id() function to set.
	*/
	DerBase(uint8_t tag = 0) : m_eloff(0), m_elsz(0), m_hdrsz(0), m_tag(tag), m_id(0) {}
	virtual ~DerBase()
	{
		explicit_bzero(m_dat.data(), m_dat.size());
	}

	/** Create a base pointer, using only the tag byte. */
	static BasePtr create(int);
	/** Create a base pointer, with extended header bytes. */
	static BasePtr create(const std::vector<uint8_t>&, size_t);
	
	/** Change a context element to explicit.

		\param ctx Original context element. Data from this element is parsed to create a new element.

		The ASN.1 spec says that explicit elements must be context-class and constructed. This api throws an exception otherwise.

		The data is used to construct a new element, which is returned.
	 */
	static BasePtr context_to_explicit(const BasePtr&);
	
	/** Convert explicit to context.

		\param orig The element to be converted to context.
		\param id Id to be applied to the returned context element.

		Output will be a constructed context-class element with the input id, and data DER-encoded input element.
	*/
	static BasePtr explicit_to_context(const BasePtr& BasePtr, uint32_t);

	/** Change a context element to implicit.

		\param ctx Original element.
		\param id The type id (type_* from DerBase::Tag)

		The ASN.1 spec says that implicit elements must be context-class. This api throws an exception otherwise.

		A universal element is created with the same construct flag as the original element.

		A new element is constructed by changing the tag to the input tag provided. If the input tag is constructed, the input element must be as well.
	 */
	static BasePtr context_to_implicit(const BasePtr&, uint32_t);
	
	/** Convert implicit to context. 

		\param orig The element to be converted to context.
		\param id Id to be applied to the returned context element.

		Output will be a context-class element with the input id, and data the same as the input element. If the input element is constructed, the output
		element will also be set constructed.
	*/
	static BasePtr implicit_to_context(const BasePtr& BasePtr, uint32_t);

	/** Offset of the element into the binary vector. */
	size_t eloff() const { return m_eloff; }
	void eloff(size_t v) { m_eloff = v; }
	/** Binary vector element length, not including the header. */
	size_t elsz() const { return m_elsz; }
	void elsz(size_t v) { m_elsz = v; }
	/** Header size of the element (tag, id, and length bytes). */
	size_t hdrsz() const { return m_hdrsz; }
	void hdrsz(size_t v) { m_hdrsz = v; }

	/** The length of prefix bytes (tag/id, and length). */
	size_t pre_len() const;
	/** Length of the data. */
	virtual size_t len() const { return m_dat.size(); }
	
	/** Parse raw data into the object. */
	virtual void parse(const std::vector<uint8_t>& v) { m_dat = v; }

	/** Dump prefix data. Throws exception if buffer is too small. */
	virtual void dump_pre(std::vector<char>&) const;
	/** Dump data. Throws exception if buffer is too small. */
	virtual void dump_data(std::vector<char>&) const;

	/** Print summary line to maximum width. */
	virtual std::string str(uint32_t=100) const;
	/** Print vizualized data */
	virtual std::string data_str() const;

	virtual std::string name() const { return "DerBase"; }

	/** Underlying data. */
	std::vector<uint8_t>& data() { return m_dat; }

	/** Get raw data. */
	void get_base(std::vector<uint8_t>& v) const
	{
		v = m_dat;
	}
	/** Set raw data. */
	void set_base(const std::vector<uint8_t>& v)
	{
		m_dat = v;
	}
	void set_base(const std::vector<char>& v)
	{
		m_dat.assign(v.begin(), v.end());
	}

	/** Tag id of the type.
		
		The id can be encoded in the tag (low-tag), or separately using a multibyte method (high-tag).
	*/
	uint32_t id() const
	{
		return (m_tag & id_mask) == id_mask ? m_id : m_tag & id_mask;
	}
	void id(uint32_t v)
	{
		if (v < id_mask)
		{
			m_tag = (m_tag & ~id_mask) | v;		// set the low bits to v
			m_id = 0;
		}
		else
		{
			m_tag |= id_mask;		// set the low bits to 1
			m_id = v;
		}
	}
	
	/** String version of the id */
	std::string id_str() const;

	/** Classification of the type (bits 7-8) */
	uint8_t type_class() const { return m_tag & class_mask; }
	/** Set type class */
	void type_class(uint8_t f) { m_tag = (m_tag & ~class_mask) | (f & class_mask); }

	/** Universal class. */
	bool uni_class() const { return (m_tag & class_mask) == 0; }
	/** Application class. */
	bool app_class() const { return (m_tag & class_mask) == class_application; }
	/** Context class. */
	bool context_class() const { return (m_tag & class_mask) == class_context; }
	/** Private class. */
	bool priv_class() const { return (m_tag & class_mask) == class_private; }

	/** String version of the classification */
	std::string class_str() const;

	/** Constructed flag (bit 6). */
	bool constructed() const { return m_tag & construct_mask ? true : false; }
	/** Set constructed flag */
	void constructed(bool cons) { m_tag = cons ? m_tag | construct_mask : m_tag & ~construct_mask; }

	/** String version of the construct flag */
	std::string construct_str() const;

	/** Is this a sequence type? */
	bool is_seq() const;
	/** Is this a set type? */
	bool is_set() const;
	/** Is this a container type? */
	bool is_contain() const
	{
		return is_seq() || is_set();
	}

	/** Return container, or throw an error if this is not a container. */
	std::vector<BasePtr>& contain();

	/** Is this a DerInteger? */
	bool is_integer() const;
	/** Return reference to a scc::crypto::Bignum. Throw error if this is not an integer. */
	scc::crypto::Bignum& integer();

	/** Is this a DerBitString? */
	bool is_bit_string() const;
	/** Return reference to bit string. */
	BitString& bit_string();

	/** Is this an octet string (8 bit chars)? */
	bool is_octet_string() const;
	/** Is this a printable string? */
	bool is_printable_string() const;
	/** Is this a utf8 (ascii) string? */
	bool is_utf8_string() const;
	/** Is this an ia5 (ascii) string? */
	bool is_ia5_string() const;
	/** Is this a bmp (ascii) string? */
	bool is_bmp_string() const;
	/** Is this a univeral (ascii) string? */
	bool is_universal_string() const;
	/** Is this a teletex (ascii) string? */
	bool is_teletex_string() const;
	/** Is this a visible (ascii) string? */
	bool is_visible_string() const;
	/** Is this a generic ascii string? */
	bool is_string() const
	{
		return is_octet_string() || is_printable_string() || is_utf8_string() || is_ia5_string()
			|| is_bmp_string() || is_universal_string() || is_teletex_string() || is_visible_string();
	}

	/** Return string. */
	std::string string();
	/** Set the string. */
	void string(const std::string&);

	/** Get string vector. */
	void string_get(std::vector<char>&);
	void string_get(std::vector<uint8_t>&);

	/** Set string vector. */
	void string_set(const std::vector<char>&);
	void string_set(const std::vector<uint8_t>&);

	/** Is this a null type? */
	bool is_null() const;

	/** Is this a boolean type? */
	bool is_boolean() const;
	/** Return boolean value. */
	bool boolean();
	/** Set boolean value. */
	void boolean(bool);

	/** Is this a utc time type? */
	bool is_utc_time() const;
	/** Is this as generalized time type? */
	bool is_generalized_time() const;
	/** Is this a time type? */
	bool is_time() const
	{
		return is_utc_time() || is_generalized_time();
	}
	/** Epoch time. Seconds since Jan 1, 1970 in the local time zone. */
	time_t time_epoch();
	/** Time point. */
	std::chrono::system_clock::time_point time_point()
	{
		return std::chrono::system_clock::from_time_t(time_epoch());
	}

	/** Is this an object identifier? */
	bool is_object_id() const;
	/** Return the object identifier value. */
	oid_value object_id();
	/** Set the object identifier. */
	void object_id(const oid_value&);
};

/** Container base class.
*/
class DerContainerBase : public DerBase, public std::vector<BasePtr>
{
public:
	DerContainerBase(uint8_t tag) : DerBase(tag) {}
	virtual ~DerContainerBase();
	virtual std::string data_str() const;
	virtual void parse(const std::vector<uint8_t>& v) { }					// containers do not parse their data
	virtual void dump_data(std::vector<char>& v) const { }					// containers do not dump their data
	virtual size_t len() const;
	virtual std::string name() const = 0;
};

/** An ASN.1 SEQUENCE or SEQUENCE OF type.

	SEQUENCE means 1 or more ordered elements of any type.
	SEQUENCE OF means 0 or more ordered elements of the same type.

	Does not enforce the semantic rules.
*/
class DerSequence : public DerContainerBase
{
public:
	DerSequence(uint8_t tag = DerBase::construct_mask | DerBase::type_sequence) : DerContainerBase(tag) {}
	virtual ~DerSequence() {}
	virtual std::string name() const { return "DerSequence"; }
};

/** An ASN.1 SET or SET OF type.

	SET means 1 or more unordered elements of any type.
	SET OF means 0 or more unordered elements of the same type.

	For convenience uses a vector to represent the container, and does not enforce the semantic rules.
*/
class DerSet : public DerContainerBase
{
public:
	DerSet(uint8_t tag = DerBase::construct_mask | DerBase::type_set) : DerContainerBase(tag) {}
	virtual ~DerSet() {}
	virtual std::string name() const { return "DerSet"; }
};

class DerNull : public DerBase
{
public:
	DerNull(uint8_t tag = DerBase::type_null) : DerBase(tag) {}
	virtual ~DerNull() {}

	virtual void parse(const std::vector<uint8_t>&) {}

	virtual std::string data_str() const { return ""; }
	virtual void dump_data(std::vector<char>& v) const { v.resize(0); }
	virtual size_t len() const { return 0; }
	virtual std::string name() const { return "DerNull"; }
};

class DerBoolean : public DerBase
{
	bool m_bool;
public:
	DerBoolean(uint8_t tag  = DerBase::type_boolean) : DerBase(tag) {}
	virtual ~DerBoolean()
	{
		explicit_bzero(&m_bool, sizeof(bool));
	}

	virtual void parse(const std::vector<uint8_t>&);

	virtual std::string data_str() const { return m_bool ? " true" : " false"; }
	virtual void dump_data(std::vector<char>& v) const;
	virtual size_t len() const { return 1; }
	virtual std::string name() const { return "DerBoolean"; }

	bool get() const { return m_bool; }
	void set(bool b) { m_bool = b; }
};

/** Object identifier class.

	OIDs are defined in X.660: https://www.itu.int/rec/T-REC-X.660-201107-I/en

	Must have at least 2 digits in the form: v1.v2.[v3].[v4]....

	v1 is root arc, v1 < 3
		0 ITU-T, 1 ISO, or 2 Joint-ISO-ITU-T
	v2 < 40
*/
class DerObjectIdentifier : public DerBase
{
	oid_value m_v = {0,0};

public:
	DerObjectIdentifier(uint8_t tag = DerBase::type_object_identifier) : DerBase(tag) {}
	virtual ~DerObjectIdentifier()
	{
		explicit_bzero(m_v.data(), m_v.size());
	}

	virtual void parse(const std::vector<uint8_t>&);

	virtual std::string data_str() const;
	virtual size_t len() const;
	virtual void dump_data(std::vector<char>& v) const;
	virtual std::string name() const { return "DerObjectIdentifier"; }

	oid_value& data()
	{
		return m_v;
	}

	/** Set oid values. Throws exception if oid values are invalid.
	*/
	void set(const oid_value& v);
	
	void get(oid_value& v) const
	{
		v = m_v;
	}

	std::string oid_str() const;
};

/** Time base class.

	Time values can be read in any format, and are stored in epoch time in the local time zone.

	Epoch time is seconds since January 1, 1970.
*/
class DerTimeBase : public DerBase
{
protected:
	time_t m_t;
public:
	DerTimeBase(uint8_t tag) : DerBase(tag), m_t(0) {}
	virtual ~DerTimeBase()
	{
		explicit_bzero(&m_t, sizeof(time_t));
	}

	/** Epoch time (seconds since Jan 1, 1970), in the local time zone. */
	time_t epoch() const { return m_t; }
	/** Set epoch time. */
	void epoch(time_t v) { m_t = v; }

	/** Set time in the local time zone. */
	void set_time(int year, int month, int day, int hour, int minute, int second);
	/** Set time in UTC time zone with tzmins offset. */
	void set_time(int year, int month, int day, int hour, int minute, int second, int tzmins);

	virtual std::string name() const = 0;
	virtual std::string data_str() const;
};

/** UTC time.

	Basic format is yymmddhhmm[ss]<Z|+hhmm|-hhmm>
	The output of this implementation will always use Z for UTC time zone.
*/
class DerUtcTime : public DerTimeBase
{
public:
	DerUtcTime(uint8_t tag = DerBase::type_utc_time) : DerTimeBase(tag) {}
	virtual ~DerUtcTime() {}

	virtual void parse(const std::vector<uint8_t>&);

	virtual void dump_data(std::vector<char>& v) const;
	virtual size_t len() const;
	virtual std::string name() const { return "DerUtcTime"; }
};

/** Generalized time.

	Basic format is yyyymmddhh[mm][ss][.frac]<Z|+hh[mm]|-hh[mm]>
	If no timezone, then we are in local timezone.
	The output of this implementation will always use Z for UTC timezone, and will not use fractions.
*/
class DerGeneralizedTime : public DerTimeBase
{
public:
	DerGeneralizedTime(uint8_t tag = DerBase::type_generalized_time) : DerTimeBase(tag) {}
	virtual ~DerGeneralizedTime() {}

	virtual void parse(const std::vector<uint8_t>&);

	virtual void dump_data(std::vector<char>& v) const;
	virtual size_t len() const;
	virtual std::string name() const { return "DerGeneralizedTime"; }
};

/**	All strings derive from simple string base class. This implementation ignores the difference between
	7 and 8 bit string types, which is a limitation on allowed characters.
*/
class DerStringBase : public DerBase
{
	std::vector<uint8_t> m_val;

public:
	DerStringBase(uint8_t tag) : DerBase(tag) {}
	virtual ~DerStringBase()
	{
		explicit_bzero(m_val.data(), m_val.size());
	}
	
	virtual void parse(const std::vector<uint8_t>&);
	
	virtual std::string data_str() const;
	virtual void dump_data(std::vector<char>& v) const;
	virtual size_t len() const { return m_val.size(); }
	virtual std::string name() const = 0;

	std::string string()
	{
		return std::string(m_val.begin(), m_val.end());
	}

	void get(std::vector<uint8_t>& v)
	{
		v = m_val;
	}

	void get(std::vector<char>& v)
	{
		v.clear();
		v.insert(v.begin(), m_val.begin(), m_val.end());
	}
	
	void get(std::string& v)
	{
		v.clear();
		v.insert(v.begin(), m_val.begin(), m_val.end());
	}

	void set(const std::vector<uint8_t>& v)
	{
		m_val = v;
	}

	void set(const std::vector<char>& v)
	{
		m_val.clear();
		m_val.insert(m_val.begin(), v.begin(), v.end());
	}
	
	void set(const std::string& v)
	{
		m_val.clear();
		m_val.insert(m_val.begin(), v.begin(), v.end());
	}
};

class DerTeletexString : public DerStringBase
{
public:
	DerTeletexString(uint8_t tag = DerBase::type_teletex_string) : DerStringBase(tag) {}
	virtual ~DerTeletexString() {}
	virtual std::string name() const { return "DerTeletexString"; }
};

class DerVisibleString : public DerStringBase
{
public:
	DerVisibleString(uint8_t tag = DerBase::type_visible_string) : DerStringBase(tag) {}
	virtual ~DerVisibleString() {}
	virtual std::string name() const { return "DerVisibleString"; }
};

class DerUniversalString : public DerStringBase
{
public:
	DerUniversalString(uint8_t tag = DerBase::type_universal_string) : DerStringBase(tag) {}
	virtual ~DerUniversalString() {}
	virtual std::string name() const { return "DerUniversalString"; }
};

class DerOctetString : public DerStringBase
{
public:
	DerOctetString(uint8_t tag = DerBase::type_octet_string) : DerStringBase(tag) {}
	virtual ~DerOctetString() {}
	virtual std::string name() const { return "DerOctetString"; }
};

class DerUtf8String : public DerStringBase
{
public:
	DerUtf8String(uint8_t tag = DerBase::type_utf8_string) : DerStringBase(tag) {}
	virtual ~DerUtf8String() {}
	virtual std::string name() const { return "DerUtf8String"; }
};

class DerPrintableString : public DerStringBase
{
public:
	DerPrintableString(uint8_t tag = DerBase::type_printable_string) : DerStringBase(tag) {}
	virtual ~DerPrintableString() {}
	virtual std::string name() const { return "DerPrintableString"; }
};

class DerIa5String : public DerStringBase
{
public:
	DerIa5String(uint8_t tag = DerBase::type_ia5_string) : DerStringBase(tag) {}
	virtual ~DerIa5String() {}
	virtual std::string name() const { return "DerIa5String"; }
};

class DerBmpString : public DerStringBase
{
public:
	DerBmpString(uint8_t tag = DerBase::type_bmp_string) : DerStringBase(tag) {}
	virtual ~DerBmpString() {}
	virtual std::string name() const { return "DerBmpString"; }
};

class DerInteger : public DerBase
{
	scc::crypto::Bignum m_bn;
public:
	DerInteger(uint8_t tag = DerBase::type_integer) : DerBase(tag) {}
	virtual ~DerInteger() {}		// Bignum memory is cleared on delete

	virtual void parse(const std::vector<uint8_t>&);

	virtual std::string data_str() const;
	virtual size_t len() const;
	virtual void dump_data(std::vector<char>&) const;
	virtual std::string name() const { return "DerInteger"; }

	/** Return the integer (an scc::crypto::Bignum)
	*/
	scc::crypto::Bignum& data() { return m_bn; }

	void set(const scc::crypto::Bignum& bn)	{ m_bn = bn; }
};

class DerBitString : public DerBase, public BitString
{
public:
	DerBitString(uint8_t tag = DerBase::type_bit_string) : DerBase(tag) {}
	virtual ~DerBitString() {}

	virtual void parse(const std::vector<uint8_t>&);

	virtual std::string data_str() const;
	virtual size_t len() const;
	virtual void dump_data(std::vector<char>& v) const;
	virtual std::string name() const { return "DerBitString"; }
};

/** DER document. Binary document using the ASN.1 DER subset.

	This assumes ownership of any DerBase object set to the root element.

	Specification: https://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
*/
class DerDocument
{
	friend class X509Cert;
	BasePtr m_root;
protected:
	friend class X509Cert;
	SecVecUchar m_bin;
	void parse_bin();
	void dump_bin();
public:
	DerDocument() {}
	virtual ~DerDocument() {}

	DerDocument(const DerDocument&) = delete;				// copy not allowed
	DerDocument& operator=(const DerDocument&) = delete;	// copy not allowed
	DerDocument(DerDocument&& b)
	{
		m_root.reset(b.m_root.get());
		b.m_root.reset();
	}
	DerDocument& operator=(DerDocument&& b)
	{
		m_root.reset(b.m_root.get());
		b.m_root.reset();
		return *this;
	}
	
	/** Compare binary data. */
	bool equal(const DerDocument&) const;

	/** Dump the binary vector.
	*/
	void dump_bin(std::vector<char>& v) const
	{
		v.clear();
		v.insert(v.end(), m_bin.begin(), m_bin.end());
	}

	/** Parse an element from a data vector.
	
		\param binv Binary vector to parse.
		\param off Offset into vector.

		Parse error, empty or invalid input data throws an exception.

		Parses elements and fills containers. Returns the root element parsed.
	*/
	static BasePtr parse_element(const std::vector<uint8_t>&, size_t = 0);
	
	static BasePtr parse_element(const std::vector<char>& v, size_t idx = 0)
	{
		std::vector<uint8_t> b(v.begin(), v.end());
		return parse_element(b, idx);
	}

	/** Dump an element to a data vector. Null element produces no data.
	
		Data is appended to the vector.
	*/
	static void dump_element(const BasePtr&, std::vector<uint8_t>&);
	static void dump_element(const BasePtr& b, std::vector<char>& v)
	{
		SecVecUchar d;
		dump_element(b, d);
		v.insert(v.end(), d.begin(), d.end());
	}

	/** Print an element and any sub-elements to a string. Empty base pointer produces "<null>" output.

		\param base Base element.
		\param debug Print debugging offset information.
		\param indent Indentation string, printed once per sublevel.
		
		Container elements print an indentation string, then a delimiter string, before all sub-elements
	*/
	static std::string print_element(const BasePtr&, bool = false, const std::string& = " |");

	/** Parse a buffer from a binary DER-formatted vector. Binary data is reset.

		Parsing an empty vector results in an empty document.
	*/
	virtual void parse(const std::vector<char>&);
	
	/** Parse document from an input stream. Throws exception on read error or parse error.
	*/
	virtual void parse(std::istream&);

	/** Dump and append to a buffer. Binary data is reset.

		Throws exception on error.
	*/
	virtual void dump(std::vector<char>&);
	
	/** Dump to output stream. Throws exception on write error.
	*/
	virtual void dump(std::ostream&);

	/** Debug string dump.

		\param debug Print document and element offset and size information.

		Returns <null> string if document is empty.
	*/
	std::string str(bool = false) const;

	/** Root DerBase.
		
		Throws exception if document is empty.
	*/
	DerBase& root();
	
	/** Return the root pointer. May be null if document is empty.
	*/
	BasePtr root_ptr() const { return m_root; }
	
	/** Clear the document.
	*/
	void clear()
	{
		m_root.reset();
	}
};

/** PEM formatted DER document.

	Base64 encoded format defined in https://tools.ietf.org/html/rfc1421
*/
class PemDocument : public DerDocument
{
	unsigned m_chars_per_line;
	std::string m_label;
public:
	PemDocument(unsigned chars_per_line = 64) : DerDocument(), m_chars_per_line(chars_per_line) {}
	PemDocument(const std::string& label, unsigned chars_per_line = 64) : DerDocument(), m_chars_per_line(chars_per_line), m_label(label) {}
	virtual ~PemDocument() {}

	unsigned chars_per_line() const { return m_chars_per_line; }
	void chars_per_line(unsigned v) { m_chars_per_line = v; }
	std::string label() const { return m_label; }
	void label(std::string v) { m_label = v; }

	/** Parse document from an input stream.
	
		Scans the input stream until "-----BEGIN <label>-----" line is found.
		Continues until "-----END <label>-----" line is found.
		Decodes base64-encoded DER data.

		Throws exception on parse error, or if BEGIN/END block not found.
	*/
	virtual void parse(std::istream&);

	/** Parse a PEM-formatted buffer.
	*/
	virtual void parse(const std::vector<char>&);

	/** PEM-formatted dump to a buffer.

		Data is appended to the buffer, starting with -----BEGIN <label>----- line, and ending with -----END <label>----- line.

		Writes chars per line for the output base64 data.

		Exception on error, or if label is empty.
	*/
	virtual void dump(std::vector<char>&);
	
	/** Dump to output stream.
	
		Throws exception on dump or write error.
	*/
	virtual void dump(std::ostream&);
};

/** @} */
/** @} */

}	// namespace

/** Print out base and all sub-elements. */
std::ostream& operator<<(std::ostream&, const scc::crypto::DerBase&);
/** Print out document. */
std::ostream& operator<<(std::ostream&, const scc::crypto::DerDocument&);
/** Print out object identifier. */
std::ostream& operator<<(std::ostream&, const std::vector<uint32_t>&);

#endif
