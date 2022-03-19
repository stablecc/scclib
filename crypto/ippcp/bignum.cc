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
#include <crypto/bignum.h>
#include <iostream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <cstring>
#include <crypto/random.h>
#include <ippcp.h>

// Uses table 4.4 from he Handbook of Applied Cryptography [Menezes, van Oorschot, Vanstone; CRC Press 1996]
// This will have an error rate of less than  2^-80 for the given bit width
#define TRIALS(w) ( \
	(w) <= 100 ? 27 : \
	(w) <= 150 ? 18 : \
	(w) <= 200 ? 15 : \
	(w) <= 250 ? 12 : \
	(w) <= 300 ? 9 : \
	(w) <= 350 ? 8 : \
	(w) <= 400 ? 7 : \
	(w) <= 500 ? 6 : \
	(w) <= 600 ? 5 : \
	(w) <= 800 ? 4 : \
	(w) <= 1250 ? 3 : \
	2)

#define BITS_TO_BYTES(bits)		((((bits)+7)&~7)/8)			// bytes containing specified bits
#define BITS_TO_WORDS(bits) 	((((bits)+31)&~31)/32)		// words containing specified bits
#define BYTES_TO_WORDS(bytes) 	((((bytes)+3)&~3)/4)		// words containing specified bytes

static void throwErr(const std::string& msg, int err)
{
	std::stringstream s;
	s << msg << ": " << ippcpGetStatusString(err);
	throw std::runtime_error(s.str());
}

struct BignumCtx
{
	std::vector<uint8_t> m_buf;
	
	int m_bnwords;					 	// size in words of the bn (according to ipp)

	BignumCtx() : m_bnwords(0)
	{
		init(1);
	}
	virtual ~BignumCtx()
	{
		explicit_bzero(&m_buf[0], m_buf.size());
	}

	IppsBigNumState* bn()
	{
		return (IppsBigNumState*)m_buf.data();
	}

	void init(int words)
	{
		assert(words > 0);

		/*
			Zero out any data before initializing, so it does not stay in memory.
		*/

		clear();
		
		int r, size;
		if ((r = ippsBigNumGetSize(words, &size)) != ippStsNoErr)
		{
			throwErr("ippsBigNumGetSize", r);
		}
		
		explicit_bzero(&m_buf[0], m_buf.size());		// zero original before resizing
		
		m_buf.resize(size);

		if ((r = ippsBigNumInit(words, bn())) != ippStsNoErr)
		{
			throwErr("ippsBigNumInit", r);
		}
		if ((r = ippsGetSize_BN(bn(), &m_bnwords)) != ippStsNoErr)
		{
			throwErr("ippsGetSize_BN", r);
		}
	}

	void clear()
	{
		if (!m_bnwords)
		{
			return;
		}

		std::vector<uint32_t> z(m_bnwords, 0);
		int r;
		if ((r = ippsSet_BN(IppsBigNumPOS, m_bnwords, &z[0], bn())) != ippStsNoErr)
		{
			throwErr("ippsSet_BN", r);
		}
	}
};

using namespace scc::crypto;

Bignum exp(const Bignum& a, const Bignum& b)
{
	scc::crypto::Bignum r(a);
	r.exp(b);
	return r;
}
Bignum exp(const Bignum& a, uint32_t b)
{
	scc::crypto::Bignum r(a);
	r.exp(b);
	return r;
}
Bignum gcd(const Bignum& a, const Bignum& b)
{
	Bignum r(a);
	r.gcd(b);
	return r;
}
Bignum gcd(const Bignum& a, uint32_t b)
{
	Bignum r(a), bn(b);
	r.gcd(bn);
	return r;
}

std::ostream& operator<<(std::ostream& os, const scc::crypto::Bignum& sa)
{
	std::string s;

	if (os.flags() & std::ios_base::hex)
	{
		s = sa.str(true);
	}
	else
	{
		s = sa.str();
	}
	return os.write(s.c_str(), s.size());
}

void scc::crypto::PrintTo(const scc::crypto::Bignum& bn, std::ostream* os)
{
	*os << bn;
}

Bignum::Bignum() : m_bnctx(new BignumCtx())
{
}
Bignum::~Bignum()
{
}

void* Bignum::bn()
{
	return reinterpret_cast<void*>(m_bnctx->bn());
}

const void* Bignum::const_bn() const
{
	return reinterpret_cast<const void*>(m_bnctx->bn());
}

void Bignum::set_width(int width)
{
	m_bnctx->init(BITS_TO_WORDS(width));
}

void Bignum::clear()
{
	m_bnctx->clear();
}

void Bignum::copy(const Bignum& from)
{
	uint32_t* ptr;
	IppsBigNumSGN sign;
	int r, width;
	if ((r = ippsRef_BN(&sign, &width, &ptr, from.m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}
	int words = BITS_TO_WORDS(width);
	m_bnctx->init(words);
	if ((r = ippsSet_BN(sign, words, ptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsSet_BN", r);
	}
}

void Bignum::move(Bignum& other)
{
	m_bnctx.reset(other.m_bnctx.release());
	other.m_bnctx.reset(new BignumCtx());
}

int Bignum::width() const
{
	int r, width;
	if ((r = ippsExtGet_BN(nullptr, &width, nullptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsExtGet_BN", r);
	}

	return width;
}

void Bignum::exp(const Bignum& b)
{
	IppsBigNumSGN sign;
	int r;
	if ((r = ippsRef_BN(&sign, 0, 0, b.m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}
	if (sign == IppsBigNumNEG)
	{
		throw std::runtime_error("a^b with negative b");
	}
	
	Bignum a(*this), count(b);

	set(1);	 // initialize for b==0

	while (count > 0)
	{
		mul(a);		// multiply result by a
		count--;
	}
}

void Bignum::gcd(const Bignum& b)
{
	Bignum orig(*this);
	m_bnctx->init(BITS_TO_WORDS(std::max(orig.width(), b.width())));		// max of a and b
	int r;
	if ((r = ippsGcd_BN(orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsGcd_BN", r);
	}
}

int Bignum::cmp(const Bignum& b) const
{
	uint32_t v;
	int r;
	if ((r = ippsCmp_BN(m_bnctx->bn(), b.m_bnctx->bn(), &v)) != ippStsNoErr)
	{
		throwErr("ippsCmp_BN", r);
	}
	if (v == IS_ZERO)					{ return 0; }
	else if (v == GREATER_THAN_ZERO)	{ return 1; }
	else								{ return -1; }
}

int Bignum::cmp(uint32_t v) const
{
	if (v == 0)
	{
		uint32_t v;
		int r;
		if ((r = ippsCmpZero_BN(m_bnctx->bn(), &v)) != ippStsNoErr)
		{
			throwErr("ippsCmpZero_BN", r);
		}
		if (v == IS_ZERO)					{ return 0; }
		else if (v == GREATER_THAN_ZERO)	{ return 1; }
		else								{ return -1; }
	}

	Bignum n(v);
	return cmp(n);
}

bool Bignum::is_negative() const
{
	IppsBigNumSGN sign;
	int r;
	if ((r = ippsRef_BN(&sign, nullptr, nullptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}
	return sign == IppsBigNumNEG ? true : false;
}

void Bignum::lshift(int shift)
{
	if (shift == 0)
	{
		return;
	}
	if (shift < 0)
	{
		rshift(-shift);
		return;
	}

	uint32_t* inp;
	IppsBigNumSGN sign;
	int r, width;
	if ((r = ippsRef_BN(&sign, &width, &inp, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}

	int outw = BITS_TO_WORDS(width+shift);

	uint32_t output[outw];
	uint32_t* outp = &output[0];
	uint32_t* outend = outp + outw;
	while (shift >= 32)
	{
		*outp = 0;			// high order word of output is 0
		outp++;
		shift -= 32;
	}
	uint32_t carry = 0;
	while (outp != outend)
	{
		*outp = (*inp << shift) | (carry >> (32 - shift));
		carry = *inp;
		inp++;
		outp++;
	}

	m_bnctx->init(outw);
	if ((r = ippsSet_BN(sign, outw, output, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsSet_BN", r);
	}
}

void Bignum::rshift(int shift)
{
	if (shift == 0)
	{
		return;
	}
	if (shift < 0)
	{
		lshift(-shift);
		return;
	}

	uint32_t* inp;
	IppsBigNumSGN sign;
	int r, width;
	if ((r = ippsRef_BN(&sign, &width, &inp, m_bnctx->bn())) != ippStsNoErr)		// save the sign
	{
		throwErr("ippsRef_BN", r);
	}

	if (shift >= width)
	{
		set(0);
		return;
	}

	int inw = BITS_TO_WORDS(width);
	int outw = BITS_TO_WORDS(width-shift);

	uint32_t* inend = inp+inw;
	while (shift >= 32)
	{
		inp++;			  // discard low order word
		shift -= 32;
	}

	uint32_t output[outw];
	uint32_t* outp = &output[0];
	uint32_t* outend = outp + outw;
	while (outp != outend)
	{
		int carry = inp != inend ? *(inp+1) : 0;
		*outp = (*inp >> shift) | (carry << (32 - shift));
		inp++;
		outp++;
	}

	m_bnctx->init(outw);
	if ((r = ippsSet_BN(sign, outw, output, m_bnctx->bn())) != ippStsNoErr)	 // write it back out
	{
		throwErr("ippsSet_BN", r);
	}
}

void Bignum::add(const Bignum& b)
{
	Bignum orig(*this);
	m_bnctx->init(BITS_TO_WORDS(std::max(orig.width(), b.width())));		// max size of a and b
	int r;
	if ((r = ippsAdd_BN(orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsAdd_BN", r);
	}
}

void Bignum::sub(const Bignum& b)
{
	Bignum orig(*this);
	m_bnctx->init(BITS_TO_WORDS(std::max(orig.width(), b.width())));		// max size of a and b
	int r;
	if ((r = ippsSub_BN(orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsSub_BN", r);
	}
}

void Bignum::mul(const Bignum& b)
{
	Bignum orig(*this);
	m_bnctx->init(BITS_TO_WORDS(orig.width()+b.width()));		// max size of a * b
	int r;
	if ((r = ippsMul_BN(orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsMul_BN", r);
	}
}

void Bignum::div(const Bignum& b, Bignum* rem)
{
	Bignum orig(*this), dum;
	
	dum.m_bnctx->init(BITS_TO_WORDS(b.width()));	   // set remainder to size of b (quotient does not need to be resized)
	
	int r;
	if ((r = ippsDiv_BN(orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->bn(), dum.m_bnctx->bn())) != ippStsNoErr)	   // this number gets the quotient
	{
		throwErr("ippsDiv_BN", r);
	}

	if (rem)
	{
		*rem = dum;
	}
}

void Bignum::mod(const Bignum& b)
{
	Bignum orig(*this);
	int r;
	if ((r = ippsMod_BN(orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsMod_BN", r);
	}
}

void Bignum::negate()
{
	uint32_t* ptr;
	IppsBigNumSGN sign;
	int r, width;
	if ((r = ippsRef_BN(&sign, &width, &ptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}
	sign = sign == IppsBigNumPOS ? IppsBigNumNEG : IppsBigNumPOS;
	int words = BITS_TO_WORDS(width);
	if ((r = ippsSet_BN(sign, words, ptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsSet_BN", r);
	}
}

bool Bignum::is_bit_set(int bit_number) const
{
	if (bit_number < 0)
	{
		throw std::runtime_error("is_bit_set() bit number invalid");
	}
	uint32_t* ptr;
	int r, width;
	if ((r = ippsRef_BN(0, &width, &ptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}

	if (bit_number >= width)
	{
		return false;
	}

	uint32_t bitw = ptr[bit_number/32];
	return ((bitw>>(bit_number%32))&1) == 1;
}

void Bignum::set_bit(int bit_number)
{
	if (bit_number < 0)
	{
		throw std::runtime_error("set_bit() bit number invalid");
	}
	uint32_t* ptr;
	int r, width;
	if ((r = ippsRef_BN(0, &width, &ptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}

	if (bit_number < width)
	{
		uint32_t bitw = ptr[bit_number/32];
		if (((bitw>>(bit_number%32))&1) == 1)
		{
			return;
		}
	}

	Bignum fix(1);
	fix.lshift(bit_number);
	add(fix);
}

void Bignum::clear_bit(int bit_number)
{
	if (bit_number < 0)
	{
		throw std::runtime_error("clear_bit() bit number invalid");
	}
	uint32_t* ptr;
	int r, width;
	if ((r = ippsRef_BN(0, &width, &ptr, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsRef_BN", r);
	}

	if (bit_number >= width)
	{
		return;
	}

	uint32_t bitw = ptr[bit_number/32];
	if (((bitw>>(bit_number%32))&1) == 0)
	{
		return;
	}

	Bignum fix(1);
	fix.lshift(bit_number);
	sub(fix);
}

void Bignum::gen_rand(int bit_width, bool strong, bool odd)
{
	if ((bit_width <= 0) || (strong && bit_width == 1))
	{
		throw std::runtime_error("gen_rand bit width invalid");
	}
	m_bnctx->init(BITS_TO_WORDS(bit_width));
	{
		scc::crypto::RandomEngineLocker rel;
		int r;
		if ((r = ippsPRNGen_BN(m_bnctx->bn(), bit_width, scc::crypto::RandomEngine::ctx())) != ippStsNoErr)
		{
			throwErr("ippsPRNGen_BN", r);
		}
	}
	
	// not ideal for smaller random numbers, but will work for most cases
	
	if (odd && !is_bit_set(0))
	{
		set_bit(0);
	}
	if (strong)
	{
		if (!is_bit_set(bit_width-1))
		{
			set_bit(bit_width-1);
		}
		if (!is_bit_set(bit_width-2))
		{
			set_bit(bit_width-2);
		}
	}
}

bool Bignum::is_prime(int trials)
{
	if (trials <= 0)
	{
		trials = TRIALS(width());
	}

	int size;
	int r;
	if ((r = ippsPrimeGetSize(width(), &size)) != ippStsNoErr)
	{
		throwErr("ippsPrimeGetSize", r);
	}

	std::vector<uint8_t> buf;
	buf.resize(size);

	if ((r = ippsPrimeInit(width(), (IppsPrimeState*)buf.data())) != ippStsNoErr)
	{
		throwErr("ippsPrimeInit", r);
	}

	scc::crypto::RandomEngineLocker rel;
	uint32_t result;
	if ((r = ippsPrimeTest_BN(m_bnctx->bn(), trials, &result, (IppsPrimeState*)buf.data(),
		ippsPRNGen, scc::crypto::RandomEngine::ctx())) != ippStsNoErr)
	{
		throwErr("ippsPrimeTest_BN", r);
	}

	explicit_bzero(&buf[0], buf.size());

	return result == IS_PRIME ? true : false;
}

void Bignum::gen_prime(int bit_width)
{
	if (bit_width < 2)
	{
		throw std::runtime_error("gen_prime bit size too low");
	}

	int size;
	int r;
	if ((r = ippsPrimeGetSize(bit_width, &size)) != ippStsNoErr)
	{
		throwErr("ippsPrimeGetSize", r);
	}

	std::vector<uint8_t> buf;
	buf.resize(size);

	if ((r = ippsPrimeInit(bit_width, (IppsPrimeState*)buf.data())) != ippStsNoErr)
	{
		throwErr("ippsPrimeInit", r);
	}

	m_bnctx->init(BITS_TO_WORDS(bit_width));

	IppStatus s;
	do
	{
		scc::crypto::RandomEngineLocker rel;
		s = ippsPrimeGen_BN(m_bnctx->bn(), bit_width, TRIALS(bit_width),
			(IppsPrimeState*)buf.data(), ippsPRNGen, scc::crypto::RandomEngine::ctx());
	}
	while (s == ippStsInsufficientEntropy);
		
	if (s != ippStsNoErr)
	{
		throwErr("ippsPRNGen_BN", r);
	}

	explicit_bzero(&buf[0], buf.size());
}

int Bignum::len() const
{
	return ((width()+7)&~7)/8;		// round up to byte
}

void Bignum::get(void * inloc, int inlen) const
{
	char* loc = (char*)inloc;

	if (inlen != len())
	{
		std::stringstream s;
		s << "get() called with len " << inlen << ", expected " << len();
		throw std::runtime_error(s.str());
	}

	// IPP can only emit word sizes

	if (cmp(0) == 0)
	{
		*loc = '\x00';
		return;
	}

	int r, size;
	if ((r = ippsGetSize_BN(m_bnctx->bn(), &size)) != ippStsNoErr)
	{
		throwErr("ippsGetSize_BN", r);
	}
	size *= 4;			// words
	Ipp8u buf[size];
	if ((r = ippsGetOctString_BN(&buf[0], size, m_bnctx->bn())) != ippStsNoErr)
	{
		if (r == ippStsRangeErr)
		{
			throw std::runtime_error("get called on negative number; use get_2sc()");
		}
		throwErr("ippsGetOctString_BN", r);
	}
	
	// forward to the first non-zero byte
	int i = 0;
	while (i < size && buf[i] == '\x00')		i++;

	if (inlen < size-i)		throw std::runtime_error("get invalid length");

	memcpy(loc, &buf[i], size-i);
}

void Bignum::set(uint32_t w)
{
	m_bnctx->init(1);
	int r;
	if ((r = ippsSet_BN(IppsBigNumPOS, 1, &w, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsSet_BN", r);
	}
}

void Bignum::set(const void * loc, int len)
{
	m_bnctx->init(BYTES_TO_WORDS(len));
	int r;
	if ((r = ippsSetOctString_BN(reinterpret_cast<const Ipp8u*>(loc), len, m_bnctx->bn())) != ippStsNoErr)
	{
		throwErr("ippsSetOctString_BN", r);
	}
}

int Bignum::len_2sc() const
{
	int w = width();
	int bitw = (w+7)&~7;

	if (*this >= 0)
	{
		return w == bitw ? bitw/8+1 : bitw/8;		// will pad with 0 byte if the msb is set
	}

	if (w < bitw)			return bitw/8;

	// msb is set, if any other bits are set, will add a byte when converted to 2s complement
	for (int i = 0; i < w-1; i++)
	{
		if (is_bit_set(i))		return bitw/8+1;
	}
	return bitw/8;
}

void Bignum::get_2sc(void * inloc, int len) const
{
	int w = width();
	int bitw = (w+7)&~7;
	uint8_t* loc = static_cast<uint8_t*>(inloc);

	if (len <= 0)		return;

	if (*this >= 0)
	{
		if (w == bitw)		// msb is set, pad with 0
		{
			*loc = '\x00';
			loc++;
			len--;
		}
		get(loc, len);
		return;
	}

	/*
		128 0000 0000 1000 0000 - 00 80
		127 0111 1111 - 7f
		-1 1111 1111 - ff
		-127 1000 0001 - 81
		-128 1000 0000 - 80
		-129 1111 1111 0111 1111 - ff 7f
	*/

	Bignum bn(1);
	bn <<= bitw;

	if (w == bitw)		// if any other bits are on, add an additional byte, otherwise, this is the boundary value
	{
		for (int i = 0; i < w-1; i++)
		{
			if (is_bit_set(i))
			{
				bn <<= 8;
				break;
			}
		}
	}

	bn += *this;		// subtract the absolute value

	bn.get(loc, len);
}

void Bignum::set_2sc(const void * loc, int len)
{
	if (len <= 0)		return;

	bool neg = (*(uint8_t*)loc & 0x80) == 0x80 ? true : false;		// check if msb is on

	set(loc, len);			// sets as a positive number

	if (!neg)			return;

	int w = width();
	clear_bit(w-1);				// clear the msb

	Bignum bn(1);				// form the number with msb set
	bn <<= w-1;

	*this -= bn;		// i.e. 10000001 = 00000001 - 10000000
}

std::string Bignum::str(bool hex) const
{
	const char digs[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

	std::string ret;

	if (hex)
	{
		uint32_t* ptr;
		IppsBigNumSGN sign;
		int r, width;
		if ((r = ippsRef_BN(&sign, &width, &ptr, m_bnctx->bn())) != ippStsNoErr)
		{
			throwErr("ippsRef_BN", r);
		}

		int bytes = BITS_TO_BYTES(width);
		
		for (int i = 0; i < bytes; i++)
		{
			uint32_t word = ptr[i/4];
			int shift = (i%4)*8;

			int d = (word >> shift)&0xf;

			ret.push_back(digs[d]);

			shift += 4;
			d = (word >> shift)&0xf;

			ret.push_back(digs[d]);
		}

		if (sign == IppsBigNumNEG)
		{
			ret.push_back('-');
		}

		std::reverse(ret.begin(), ret.end());
	}
	else
	{
		if (*this == 0)
		{
			return "0";
		}

		Bignum calc(*this), ten(10), dig;

		if (is_negative())
		{
			calc = -calc;
		}

		while (calc > 0)
		{
			calc.div(ten, &dig);		// divide by 10, with digit remainder

			uint32_t* ptr;
			int r;
			if ((r = ippsRef_BN(nullptr, nullptr, &ptr, dig.m_bnctx->bn())) != ippStsNoErr)
			{
				throwErr("ippsRef_BN", r);
			}

			ret.push_back(digs[*ptr]);
		}

		if (is_negative())
		{
			ret.push_back('-');
		}

		std::reverse(ret.begin(), ret.end());
	}

	return ret;
}
