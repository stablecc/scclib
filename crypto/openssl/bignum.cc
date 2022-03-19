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
#include <sstream>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <cctype>
#include <crypto/random.h>

struct BignumCtx
{
	BIGNUM* m_bn;
	BN_CTX* m_ctx;

	BignumCtx() : m_ctx{nullptr}
	{
		m_bn = BN_new();
		if (!m_bn)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << "BN_new: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
	}
	~BignumCtx()
	{
		BN_free(m_bn);
		if (m_ctx)
		{
			BN_CTX_free(m_ctx);
		}
	}
	BIGNUM* bn() const
	{
		return m_bn;
	}
	BN_CTX* ctx()
	{
		if (!m_ctx)
		{
			m_ctx = BN_CTX_new();
			if (!m_ctx)
			{
				auto e = ERR_get_error();
				std::stringstream s;
				s << "BN_CTX_new: " << ERR_error_string(e, nullptr);
				throw std::runtime_error(s.str());
			}
		}
		return m_ctx;
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

void Bignum::copy(const Bignum& other)
{
	if (!BN_copy(m_bnctx->bn(), other.m_bnctx->bn()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_copy: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::move(Bignum& other)
{
	m_bnctx.reset(other.m_bnctx.release());
	other.m_bnctx.reset(new BignumCtx());
}

void Bignum::clear()
{
	BN_clear(m_bnctx->bn());
}

int Bignum::width() const
{
	if (BN_is_zero(m_bnctx->bn()))
	{
		return 1;
	}
	return BN_num_bits(m_bnctx->bn());
}

void Bignum::exp(const Bignum& b)
{
	Bignum orig(*this);
	if (!BN_exp(m_bnctx->bn(), orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->ctx()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_exp modulus: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::gcd(const Bignum& b)
{
	Bignum orig(*this);
	if (!BN_gcd(m_bnctx->bn(), orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->ctx()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_gcd modulus: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

bool Bignum::is_prime(int trials)
{
	int r = BN_is_prime_ex(m_bnctx->bn(), trials <= 0 ? BN_prime_checks : trials, m_bnctx->ctx(), nullptr);
	if (r == -1)
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_is_prime_ex: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
	return r == 1;
}

void Bignum::gen_rand(int bit_width, bool strong, bool odd)
{
	if ((bit_width <= 0) || (strong && bit_width == 1))
	{
		throw std::runtime_error("gen_rand bit width invalid");
	}

	if (!BN_rand(m_bnctx->bn(), bit_width,
		strong ? BN_RAND_TOP_TWO : 0,
		odd ? BN_RAND_BOTTOM_ODD : 0))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_rand: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::gen_prime(int bit_width)
{
	if (bit_width < 2)
	{
		throw std::runtime_error("gen_prime bit size too low");
	}

	int r = BN_generate_prime_ex(m_bnctx->bn(), bit_width, 0, 0, 0, 0);
	if (r == 0)
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_generate_prime_ex: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::negate()
{
	BN_set_negative(m_bnctx->bn(), 1);
}

bool Bignum::is_negative() const
{
	return BN_is_negative(m_bnctx->bn()) == 1 ? true : false;
}

int Bignum::cmp(const Bignum& other) const
{
	return BN_cmp(m_bnctx->bn(), other.m_bnctx->bn());
}

int Bignum::cmp(uint32_t w) const
{
	if (is_negative())
	{
		return -1;
	}
	if (w == 0)
	{
		if (BN_is_zero(m_bnctx->bn()))
		{
			return 0;
		}
		return 1;
	}
	if (w == 1)
	{
		if (BN_is_zero(m_bnctx->bn()))
		{
			return -1;
		}
		if (BN_is_one(m_bnctx->bn()))
		{
			return 0;
		}
		return 1;
	}
	Bignum b(w);
	return cmp(b);
}

void Bignum::lshift(int shift)
{
	Bignum orig(*this);
	if (!BN_lshift(m_bnctx->bn(), orig.m_bnctx->bn(), shift))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_lshift: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::rshift(int shift)
{
	Bignum orig(*this);
	if (!BN_rshift(m_bnctx->bn(), orig.m_bnctx->bn(), shift))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_rshift: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::add(const Bignum& b)
{
	Bignum orig(*this);
	if (!BN_add(m_bnctx->bn(), orig.m_bnctx->bn(), b.m_bnctx->bn()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_add: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::sub(const Bignum& b)
{
	Bignum orig(*this);
	if (!BN_sub(m_bnctx->bn(), orig.m_bnctx->bn(), b.m_bnctx->bn()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_sub: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::mul(const Bignum& b)
{
	Bignum orig(*this);
	if (!BN_mul(m_bnctx->bn(), orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->ctx()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_mul: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::div(const Bignum& b, Bignum* rem)
{
	Bignum orig(*this);
	if (!BN_div(m_bnctx->bn(), rem?rem->m_bnctx->bn():nullptr, orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->ctx()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_div divide: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::mod(const Bignum& b)
{
	Bignum orig(*this);
	if (!BN_div(nullptr, m_bnctx->bn(), orig.m_bnctx->bn(), b.m_bnctx->bn(), m_bnctx->ctx()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_div modulus: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

bool Bignum::is_bit_set(int bit_number) const
{
	if (bit_number < 0)
	{
		throw std::runtime_error("is_bit_set() bit number invalid");
	}
	if (bit_number >= width())
	{
		return false;
	}
	return BN_is_bit_set(m_bnctx->bn(), bit_number) == 1;
}

void Bignum::set_bit(int bit_number)
{
	if (bit_number < 0)
	{
		throw std::runtime_error("set_bit() bit number invalid");
	}
	if (!BN_set_bit(m_bnctx->bn(), bit_number))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_set_bit: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::clear_bit(int bit_number)
{
	if (bit_number < 0)
	{
		throw std::runtime_error("clear_bit() bit number invalid");
	}
	if (bit_number >= width())
	{
		return;
	}
	if (!BN_clear_bit(m_bnctx->bn(), bit_number))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_clear_bit: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

std::string Bignum::str(bool hex) const
{
	char * bns;
	if (!hex)
	{
		bns = BN_bn2dec(m_bnctx->bn());
	}
	else
	{
		bns = BN_bn2hex(m_bnctx->bn());	 // library returns in upper case
		char* c = bns;
		for (; *c != 0; c++)
		{
			*c = static_cast<char>(std::tolower(static_cast<unsigned char>(*c)));
		}
	}
	if (!bns)
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << (hex ? "BN_bn2hex" : "BN_bn2dec") << ": " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
	std::string s(bns);
	OPENSSL_free(bns);
	return s;
}

int Bignum::len() const
{
	if (BN_is_zero(m_bnctx->bn()))
	{
		return 1;
	}
	return BN_num_bytes(m_bnctx->bn());
}

void Bignum::get(void * inloc, int inlen) const
{
	if (inlen != len())
	{
		std::stringstream s;
		s << "get() called with buffer len " << inlen << ", expected " << len();
		throw std::runtime_error(s.str());
	}

	if (BN_is_negative(m_bnctx->bn()))
	{
		throw std::runtime_error("get() called with negative number");
	}

	if (BN_is_zero(m_bnctx->bn()))
	{
		*((char*)inloc) = 0;
		return;
	}

	if (!BN_bn2bin(m_bnctx->bn(), static_cast<unsigned char*>(inloc)))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_bn2bin: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::set(uint32_t w)
{
	if (w == 0)
	{
		BN_zero(m_bnctx->bn());
	}
	else if (w == 1)
	{
		if (!BN_one(m_bnctx->bn()))
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << "BN_one: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
	}
	else if (!BN_set_word(m_bnctx->bn(), w))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_set_word: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

void Bignum::set(const void * loc, int len)
{
	if (!BN_bin2bn(reinterpret_cast<const unsigned char *>(loc), len, m_bnctx->bn()))
	{
		auto e = ERR_get_error();
		std::stringstream s;
		s << "BN_bin2bn: " << ERR_error_string(e, nullptr);
		throw std::runtime_error(s.str());
	}
}

int Bignum::len_2sc() const
{
	int w = width();
	int bitw = (w+7)&~7;

	if (!is_negative())
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

	if (!is_negative())
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
