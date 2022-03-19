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
#ifndef _SCC_CRYPTO_BIGNUM_H
#define _SCC_CRYPTO_BIGNUM_H

#include <string>
#include <ostream>
#include <memory>

class BignumCtx;		// forward declaration

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_bignum Big number arithmetic
	@{

	Defines an arbitrary length big number for use in cryptographic applications.
*/

/** Big number arithmetic.
	\file
*/

/** Big number.
*/
class Bignum
{
	std::unique_ptr<BignumCtx> m_bnctx;

public:
	/** Construct and set to 0. */
	Bignum();

	/** Move constructor */
	Bignum(Bignum&& other) : Bignum()
	{
		move(other);
	}
	/** Move assignment */
	Bignum& operator =(Bignum&& other)
	{
		move(other);
		return *this;
	}

	/** Copy constructor */
	Bignum(const Bignum& other) : Bignum()
	{
		copy(other);
	}
	/** Copy assignment */
	Bignum& operator =(const Bignum& other)
	{
		copy(other);
		return *this;
	}

	/** Word constructor */
	Bignum(uint32_t w) : Bignum()
	{
		set(w);
	}
	/** Word assignment */
	Bignum& operator =(uint32_t w)
	{
		set(w);
		return *this;
	}

	/** Set to data */
	Bignum(const void * loc, int len) : Bignum()
	{
		set(loc, len);
	}

	virtual ~Bignum();

	/** Bignum context for use by external services. */
	void* bn();
	/** Bignum context for use by external services. */
	const void* const_bn() const;

	/** Set the number to a specified bit width.
	*/
	void set_width(int);

	/** Set to a word.

		This setter will not necessarily zero out all data.
	*/
	void set(uint32_t);
	
	/** Set a positive integer from input bytes.
	*/
	void set(const void *, int);

	/** Set integer from twos complement input stream.
	*/
	void set_2sc(const void *, int);

	/** Required length of get() output in bytes.

		The number zero has length 1.
	*/
	int len() const;
	
	/** Octal (byte) output of a positive number.

		\param loc data location
		\param len length of data

		Throws an exception if the number is negative.
	*/
	void get(void *, int) const;
	
	/** Get the length of octal output in twos complement form.
	*/
	int len_2sc() const;

	/** Octal (byte) output, in two's complement form.

		Returns the minimum number of bytes required.

		Example:

		128  bin 0000 0000 1000 0000 hex 00 80
		127  bin 0111 1111           hex 7f
		-1   bin 1111 1111           hex ff
		-127 bin 1000 0001           hex 81
		-128 bin 1000 0000           hex 80
		-129 bin 1111 1111 0111 1111 hex ff 7f

		\param loc data location
		\param len length of data
	*/
	void get_2sc(void *, int) const;
	
	/** Copy into this bignum.
	*/
	void copy(const Bignum&);

	/** Move into this bignum.
		The other bignum will be initialized and set to zero.
	*/
	void move(Bignum&);

	/** Clear and set to 0.
	*/
	void clear();

	/** Number of significant bits. The number 0 has width 1.
	*/
	int width() const;
	
	/** Exponent a^b.
	*/
	void exp(const Bignum&);
	/** Exponent a^b. */
	void exp(uint32_t b)
	{
		Bignum bn(b);
		exp(bn);
	}

	/** Greatest common divisor for a and b.
	*/
	void gcd(const Bignum&);

	/** Generate a random number of the specified bit width.

		A cryptographically strong number has the upper two bits set, so that the product
		of two such numbers will always have 2*bit_width size.

		\param bit_width The size of the prime in bits, must be > 0. If strong, must be > 1.
		\param strong If this is true, generate a cryptographically strong number (top two bits are on).
		\param odd If this is true, generates an odd number.
	*/
	void gen_rand(int, bool = false, bool = false);

	/** Perform a Miller-Rabin prime test to test primality.

		The default calculates primes with expected 1/10^80 probability of false positive.

		\param trials Number of trials, <= 0 means calculate trials automatically.
	*/	
	bool is_prime(int = -1);

	/** Generate a random pseudo-prime number of the specified bit width.

		\param bit_width The size of the prime in bits.
	*/
	void gen_prime(int);
	
	/** Left bit shift. */
	void lshift(int);
	/** Right bit shift. */
	void rshift(int);
	/** Add a+b. */
	void add(const Bignum&);
	/** Word add a+b. */
	void add(uint32_t b)
	{
		Bignum bn(b);
		add(bn);
	}
	/** Subtract a-b. */
	void sub(const Bignum&);
	/** Subtract an integer. */
	void sub(uint32_t b)
	{
		Bignum bn(b);
		sub(bn);
	}
	/** Mulitply a*b. */
	void mul(const Bignum&);
	/** Word multiply. */
	void mul(uint32_t b)
	{
		Bignum bn(b);
		mul(bn);
	}
	/** Divide a/b. Optional remainder. */
	void div(const Bignum&, Bignum*);
	/** Divide a/b. Optional remainder. */
	void div(uint32_t b, Bignum* rem)
	{
		Bignum bn(b);
		div(bn, rem);
	}
	/** Modulus a%b. */
	void mod(const Bignum&);
	/** Word modulus. */
	void mod(uint32_t b)
	{
		Bignum bn(b);
		mod(bn);
	}
	/** Negate (change sign). */
	void negate();

	/** Bitwise not ~a. */
	void bit_not()
	{
		for (int i = 0; i < width(); i++)
		{
			if (is_bit_set(i))
			{
				clear_bit(i);
			}
			else
			{
				set_bit(i);
			}
		}
	}
	/** Bitwise not a&b. */
	void bit_and(const Bignum& b)
	{
		for (int i = 0; i < std::max(width(), b.width()); i++)
		{
			if (is_bit_set(i) && b.is_bit_set(i))
			{
				set_bit(i);
			}
			else
			{
				clear_bit(i);
			}
		}
	}
	/** Bitwise or a|b. */
	void bit_or(const Bignum& b)
	{
		for (int i = 0; i < std::max(width(), b.width()); i++)
		{
			if (is_bit_set(i) || b.is_bit_set(i))
			{
				set_bit(i);
			}
			else
			{
				clear_bit(i);
			}
		}
	}
	/** Bitwise xor a^b. */
	void bit_xor(const Bignum& b)
	{
		for (int i = 0; i < std::max(width(), b.width()); i++)
		{
			if ((is_bit_set(i) && !b.is_bit_set(i)) || (!is_bit_set(i) && b.is_bit_set(i)))
			{
				set_bit(i);
			}
			else
			{
				clear_bit(i);
			}
		}
	}

	/** Is the bit set?

		Bits are 0 indexed.

		\param bit_number bit to set, must be >= 0.
	*/
	bool is_bit_set(int) const;

	/** Set a bit.

		Bits are 0 indexed. Expands the number if necessary.

		\param bit_number bit to set, must be >= 0.
	*/
	void set_bit(int);

	/** Clear a bit.

		Bits are 0 indexed.

		\param bit_number bit to set, must be >= 0.
	*/
	void clear_bit(int);

	/** Greatest common divisor of a and b. */
	void gcd(uint32_t b)
	{
		Bignum bn(b);
		gcd(bn);
	}

	/** Is prime for an integer. */
	static bool is_prime(uint32_t w)
	{
		Bignum bn(w);
		return bn.is_prime();
	}

	/** Is negative? */
	bool is_negative() const;

	/** Arithmetic compare for an integer. */
	int cmp(uint32_t w) const;

	/** Arithmetic compare. */
	int cmp(const Bignum&) const;

	/** Equal operator. */
	bool operator ==(const Bignum& o) const { return cmp(o) == 0; }
	/** Not equal operator. */
	bool operator !=(const Bignum& o) const { return cmp(o) != 0; }
	/** Less than operator. */
	bool operator <(const Bignum& o) const { return cmp(o) < 0; }
	/** Greater than operator. */
	bool operator >(const Bignum& o) const { return cmp(o) > 0; }
	/** Less than equal operator. */
	bool operator <=(const Bignum& o) const { return cmp(o) <= 0; }
	/** Greater than equal operator. */
	bool operator >=(const Bignum& o) const { return cmp(o) >= 0; }
	/** Equal operator. */
	bool operator ==(uint32_t o) const { return cmp(o) == 0; }
	/** Not equal operator. */
	bool operator !=(uint32_t o) const { return cmp(o) != 0; }
	/** Less than operator. */
	bool operator <(uint32_t o) const { return cmp(o) < 0; }
	/** Greater than operator. */
	bool operator >(uint32_t o) const { return cmp(o) > 0; }
	/** Less than equal operator. */
	bool operator <=(uint32_t o) const { return cmp(o) <= 0; }
	/** Greater than equal operator. */
	bool operator >=(uint32_t o) const { return cmp(o) >= 0; }

	/** Right shift operator. */
	Bignum operator <<(int shift) const
	{
		Bignum b(*this);
		b.lshift(shift);
		return b;
	}
	/** Right shift assign operator. */
	Bignum& operator <<=(int shift)
	{
		lshift(shift);
		return *this;
	}
	/** Right shift operator. */
	Bignum operator >>(int shift) const
	{
		Bignum b(*this);
		b.rshift(shift);
		return b;
	}
	/** Right shift assign operator. */
	Bignum& operator >>=(int shift)
	{
		rshift(shift);
		return *this;
	}
	/** Add operator. */
	Bignum operator +(const Bignum& a) const
	{
		Bignum r(*this);
		r.add(a);
		return r;
	}
	/** Add assign operator. */
	Bignum operator +=(const Bignum& a)
	{
		add(a);
		return *this;
	}
	/** Add operator. */
	Bignum operator +(uint32_t a) const
	{
		Bignum r(*this);
		r.add(a);
		return r;
	}
	/** Add assign operator. */
	Bignum operator +=(uint32_t a)
	{
		add(a);
		return *this;
	}
	/** Prefix addition operator. */
	Bignum& operator ++()
	{
		add(1);	// prefix
		return *this;
	}
	/** Postfix addition operator. */
	Bignum operator ++(int d)
	{
		Bignum r(*this);
		add(1);	// postfix
		return r;
	}
	/** Negation operator  -a */
	Bignum operator -()
	{
		negate();
		return *this;
	}
	/** Subtract operator. */
	Bignum operator -(const Bignum& a) const
	{
		Bignum r(*this);
		r.sub(a);
		return r;
	}
	/** Subtract assign operator. */
	Bignum operator -=(const Bignum& a)
	{
		sub(a);
		return *this;
	}
	/** Subtract operator. */
	Bignum operator -(uint32_t a) const
	{
		Bignum r(*this);
		r.sub(a);
		return r;
	}
	/** Subtract assign operator. */
	Bignum operator -=(uint32_t a)
	{
		sub(a);
		return *this;
	}
	/** Prefix decrement. */
	Bignum& operator --()
	{
		sub(1);	// prefix
		return *this;
	}
	/** Postfix decrement. */
	Bignum operator --(int d)
	{
		Bignum r(*this);
		sub(1);	// postfix
		return r;
	}
	/** Multiply operator. */
	Bignum operator *(const Bignum& a) const
	{
		Bignum r(*this);
		r.mul(a);
		return r;
	}
	/** Multiply assign operator. */
	Bignum operator *=(const Bignum& a)
	{
		mul(a);
		return *this;
	}
	/** Multiply operator. */
	Bignum operator *(uint32_t a) const
	{
		Bignum r(*this);
		r.mul(a);
		return r;
	}
	/** Multiply assign operator. */
	Bignum operator *=(uint32_t a)
	{
		mul(a);
		return *this;
	}
	/** Divide operator. */
	Bignum operator /(const Bignum& a) const
	{
		Bignum r(*this);
		r.div(a, nullptr);
		return r;
	}
	/** Divide assign operator. */
	Bignum operator /=(const Bignum& a)
	{
		div(a, nullptr);
		return *this;
	}
	/** Divide operator. */
	Bignum operator /(uint32_t a) const
	{
		Bignum r(*this);
		r.div(a, nullptr);
		return r;
	}
	/** Divide assign operator. */
	Bignum operator /=(uint32_t a)
	{
		div(a, nullptr);
		return *this;
	}
	/** Modulus operator. */
	Bignum operator %(const Bignum& a) const
	{
		Bignum r(*this);
		r.mod(a);
		return r;
	}
	/** Modulus assign operator. */
	Bignum operator %=(const Bignum& a)
	{
		mod(a);
		return *this;
	}
	/** Modulus operator. */
	Bignum operator %(uint32_t a) const
	{
		Bignum r(*this);
		r.mod(a);
		return r;
	}
	/** Modulus assign operator. */
	Bignum operator %=(uint32_t a)
	{
		mod(a);
		return *this;
	}
	/** And operator. */
	Bignum operator &(const Bignum& a) const
	{
		Bignum r(*this);
		r.bit_and(a);
		return r;
	}
	/** And assign operator. */
	Bignum operator &=(const Bignum& a)
	{
		bit_and(a);
		return *this;
	}
	/** And operator. */
	Bignum operator &(uint32_t a) const
	{
		Bignum r(*this);
		Bignum b(a);
		r.bit_and(b);
		return r;
	}
	/** And assign operator. */
	Bignum operator &=(uint32_t a)
	{
		Bignum b(a);
		bit_and(b);
		return *this;
	}
	/** Or operator. */
	Bignum operator |(const Bignum& a) const
	{
		Bignum r(*this);
		r.bit_or(a);
		return r;
	}
	/** Or assign operator. */
	Bignum operator |=(const Bignum& a)
	{
		bit_or(a);
		return *this;
	}
	/** Or operator. */
	Bignum operator |(uint32_t a) const
	{
		Bignum r(*this);
		Bignum b(a);
		r.bit_or(b);
		return r;
	}
	/** Or assign operator. */
	Bignum operator |=(uint32_t a)
	{
		Bignum b(a);
		bit_or(b);
		return *this;
	}
	/** Xor operator. */
	Bignum operator ^(const Bignum& a) const
	{
		Bignum r(*this);
		r.bit_xor(a);
		return r;
	}
	/** Xor assign operator. */
	Bignum operator ^=(const Bignum& a)
	{
		bit_xor(a);
		return *this;
	}
	/** Xor operator. */
	Bignum operator ^(uint32_t a) const
	{
		Bignum r(*this);
		Bignum b(a);
		r.bit_xor(b);
		return r;
	}
	/** Xor assign operator. */
	Bignum operator ^=(uint32_t a)
	{
		Bignum b(a);
		bit_xor(b);
		return *this;
	}
	/** Not operator. */
	Bignum operator ~() const
	{
		Bignum r(*this);
		r.bit_not();
		return r;
	}

	/** Return a string representation in decimal or hex.

		\param hex emit hexadecimal
	*/
	std::string str(bool=false) const;
};

/** @} */
/** @} */

/** Googletest printer. */
void PrintTo(const Bignum&, std::ostream*);

}

/** Print the bignum to a stream. */
std::ostream& operator <<(std::ostream&, const scc::crypto::Bignum&);
/** Exponent helper. */
scc::crypto::Bignum exp(const scc::crypto::Bignum&, const scc::crypto::Bignum&);
/** Exponent helper. */
scc::crypto::Bignum exp(const scc::crypto::Bignum&, uint32_t);
/** Greatest common divisor helper. */
scc::crypto::Bignum gcd(const scc::crypto::Bignum&, const scc::crypto::Bignum&);
/** Greatest common divisor helper. */
scc::crypto::Bignum gcd(const scc::crypto::Bignum&, uint32_t);

#endif
