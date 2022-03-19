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
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <encode/hex.h>

/** \addtogroup crypto_bignum
	@{ */
/** Tests for \ref crypto_bignum \file */
/** \example crypto/unittest/bignum.cc */
/** @} */

using namespace std;
using namespace scc::encode;
using scc::crypto::Bignum;

TEST(bignum_test, init)
{
	Bignum n;
	cout << "Init: " << n << endl;
	ASSERT_EQ(n.str(), "0");
}

TEST(bignum_test, set_zero)
{
	Bignum n;
	n = 0;
	cout << "Zero: " << n << endl;
	ASSERT_EQ(n.str(), "0");
}

TEST(bignum_test, set_one)
{
	Bignum n;
	n = 1;
	cout << "One: " << n << endl;
	ASSERT_EQ(n.str(), "1");
}

TEST(bignum_test, init_ten)
{
	Bignum n(10);
	cout << "Init ten: " << n << endl;
	ASSERT_EQ(n.str(), "10");
}

TEST(bignum_test, copy_construct)
{
	Bignum a(10);
	Bignum b(a);
	cout << "copy_construct a=" << a << " b=" << b << endl;
	ASSERT_TRUE(a == 10);
	ASSERT_TRUE(b == 10);
}

TEST(bignum_test, copy_op)
{
	Bignum a(10);
	Bignum b;
	b = a;
	cout << "copy_op a=" << a << " b=" << b << endl;
	ASSERT_TRUE(a == 10);
	ASSERT_TRUE(b == 10);
}

TEST(bignum_test, move_construct)
{
	Bignum a(10);
	Bignum b = std::move(a);
	cout << "move_construct a=" << a << " b=" << b << endl;
	ASSERT_TRUE(b == 10);
	ASSERT_FALSE(a == 10);
}

TEST(bignum_test, move_opt)
{
	Bignum a(10);
	Bignum b;
	b = std::move(a);
	cout << "move_op a=" << a << " b=" << b << endl;
	ASSERT_TRUE(b == 10);
	ASSERT_FALSE(a == 10);
}

TEST(bignum_test, print_hex)
{
	Bignum a(1 << 20);
	a += 15*16;
	a = -a;
	cout << "print_hex " << std::hex << a << std::dec << endl;
	ASSERT_EQ(a.str(true), "-1000f0");
	
}

TEST(bignum_test, print_dec)
{
	Bignum a(1 << 20);
	a = -a;
	cout << "print_dec " << a << endl;
	ASSERT_EQ(a.str(), "-1048576");
}

TEST(bignum_test, compare)
{
	Bignum one(1), ten(10);
	ASSERT_TRUE(one == one);
	ASSERT_TRUE(one >= one);
	ASSERT_TRUE(one <= one);
	ASSERT_FALSE(one < one);
	ASSERT_FALSE(one > one);
	ASSERT_TRUE(one != ten);
	ASSERT_TRUE(one < ten);
	ASSERT_TRUE(one <= ten);
	ASSERT_TRUE(ten > one);
	ASSERT_TRUE(ten >= one);
	ASSERT_TRUE(one == 1);
	ASSERT_TRUE(one >= 1);
	ASSERT_TRUE(one <= 1);
	ASSERT_FALSE(one < 1);
	ASSERT_FALSE(one > 1);
	ASSERT_TRUE(one != 10);
	ASSERT_TRUE(one < 10);
	ASSERT_TRUE(one <= 10);
	ASSERT_TRUE(ten > 1);
	ASSERT_TRUE(ten >= 1);
}


TEST(bignum_test, shift)
{
	Bignum one(1);
	Bignum sft(1<<7);
	Bignum a(1);

	a <<= 7;
	ASSERT_EQ(a, sft);
	Bignum b = one << 7;
	ASSERT_EQ(b, sft);
	Bignum c(sft);
	c >>= 7;
	ASSERT_EQ(c, one);
	Bignum d = sft >> 7;
	ASSERT_EQ(d, one);
	Bignum shiftaway(123456);
	shiftaway >>= shiftaway.width();
	ASSERT_EQ(shiftaway, 0);
}

TEST(bignum_test, big_shift)
{
	Bignum n(0xffff0000);
	cout << "bigshift in  : " << std::hex << n << std::dec << " width: " << n.width() << endl;
	n <<= 47;
	auto hx = n.str(true);
	cout << "bigshift <<47: " << std::hex << hx << std::dec << " width: " << n.width() << endl;
	ASSERT_EQ(n.width(), 32+47);
	ASSERT_EQ(hx, "7fff8000000000000000");
	n >>= 9;
	hx = n.str(true);
	cout << "bigshift >>9: " << std::hex << hx << std::dec << " width: " << n.width() << endl;
	ASSERT_EQ(hx, "3fffc0000000000000");
	ASSERT_EQ(n.width(), 32+47-9);
}

TEST(bignum_test, incdec)
{
	Bignum a;
	a = 1;
	ASSERT_EQ(++a, 2);
	ASSERT_EQ(a, 2);
	ASSERT_EQ(a++, 2);
	ASSERT_EQ(a, 3);
	ASSERT_EQ(--a, 2);
	ASSERT_EQ(a, 2);
	ASSERT_EQ(a--, 2);
	ASSERT_EQ(a, 1);
}

TEST(bignum_test, arith)
{
	Bignum zero, one(1), two(2), three(3), nine(9), ten(10), eleven(11), twelve(12), hund(100);

	Bignum a = one * ten;
	ASSERT_EQ(a, ten);
	a /= ten;
	ASSERT_EQ(a, one);

	a = nine / three;
	ASSERT_EQ(a, three);
	a *= three;
	ASSERT_EQ(a, nine);

	a = nine + one;
	ASSERT_EQ(a, ten);
	a -= one;
	ASSERT_EQ(a, nine);

	a = ten - one;
	ASSERT_EQ(a, nine);
	a += one;
	ASSERT_EQ(a, ten);

	a = hund % nine;
	ASSERT_EQ(a, one);

	a = hund;
	Bignum rem;
	a.div(nine, &rem);
	ASSERT_EQ(a, eleven);
	ASSERT_EQ(rem, one);

	a = exp(nine, zero);
	ASSERT_EQ(a, one);

	a = exp(ten, two);
	ASSERT_EQ(a, hund);
	a = ten;
	a.exp(two);
	ASSERT_EQ(a, hund);

	a = gcd(nine, twelve);
	ASSERT_EQ(a, three);
	a = nine;
	a.gcd(twelve);
	ASSERT_EQ(a, three);
}

TEST(bignum_test, arith_num)
{
	Bignum one(1), two(2), three(3), nine(9), ten(10), eleven(11), twelve(12), hund(100);

	Bignum a = one * 10;
	ASSERT_EQ(a, 10);
	a /= 10;
	ASSERT_EQ(a, 1);

	a = nine / 3;
	ASSERT_EQ(a, 3);
	a *= 3;
	ASSERT_EQ(a, 9);

	a = nine + 1;
	ASSERT_EQ(a, 10);
	a -= 1;
	ASSERT_EQ(a, 9);

	a = ten - 1;
	ASSERT_EQ(a, 9);
	a += 1;
	ASSERT_EQ(a, 10);

	a = hund % 9;
	ASSERT_EQ(a, 1);

	a = hund;
	Bignum rem;
	a.div(nine, &rem);
	ASSERT_EQ(a, 11);
	ASSERT_EQ(rem, 1);

	a = exp(nine, 0);
	ASSERT_EQ(a, 1);

	a = exp(ten, 2);
	ASSERT_EQ(a, 100);
	a = ten;
	a.exp(2);
	ASSERT_EQ(a, 100);

	a = gcd(nine, 12);
	ASSERT_EQ(a, 3);
	a = nine;
	a.gcd(12);
	ASSERT_EQ(a, 3);
}

TEST(bignum_test, prime)
{
	Bignum p1(217645177);
	Bignum p2(236887691);
	ASSERT_TRUE(p1.is_prime());
	ASSERT_TRUE(p2.is_prime());
	Bignum c = p1*p2;
	ASSERT_FALSE(c.is_prime());
	cout << "Prime 1:   " << p1 << endl;
	cout << "Prime 2:   " << p2 << endl;
	cout << "Composite: " << c << endl;

	// find the first hundred primes (see primes.utm.edu)
	int count = 0;
	Bignum b;
	for (int t = 2; t <= 541; t++)
	{
		b = t;
		if (b.is_prime())
			count++;
	}
	cout << "First hundred primes found: " << count << endl;
	EXPECT_EQ(count, 100);
}

TEST(bignum_test, gcd)
{
	Bignum p1(2829604451);
	cout << "p1: " << std::hex << p1 << endl;
	ASSERT_TRUE(p1.is_prime());
	Bignum p2(1787494861);
	cout << "p2: " << std::hex << p2 << endl;
	ASSERT_TRUE(p2.is_prime());
	Bignum p3(3954380029);
	cout << "p3: " << std::hex << p3 << endl;
	ASSERT_TRUE(p3.is_prime());
	Bignum p12 = p1 * p2;
	cout << "p1 * p2: " << std::hex << p12 << endl;
	Bignum p13 = p1 * p3;
	cout << "p1 * p3: " << std::hex << p13 << endl;
	Bignum gcd(p12);
	gcd.gcd(p13);
	cout << "gcd: " << std::hex << gcd << endl;
	ASSERT_EQ(p1, gcd);
}

TEST(bignum_test, bintest)
{
	uint8_t octval[] = "\x12\x34\x56\x78\x9a\xbc\xde\xf0"
		"\xcc";

	Bignum b(octval, sizeof(octval));
	cout << "bin: " << std::hex << b << " " << std::dec << b << endl;
	Bignum t;
	t = 0x12345678;
	t <<= 32;
	t += 0x9abcdef0;
	t <<= 16;
	t += 0xcc00;
	cout << "tst: " << std::hex << t << " " << std::dec << t << endl;
	ASSERT_EQ(b, t);
}

TEST(bignum_test, generate_rand)
{
	auto gen = [](int w, bool s=false, bool o=false)
	{
		Bignum p;
		p.gen_rand(w, s, o);
		cout << "genrand request width="<<w<<" strong="<<s<<" odd="<<o<<": " << std::hex << p << std::dec << " width: " << p.width() << endl;
		ASSERT_LE(p.width(), w);
		if (s)
		{
			ASSERT_TRUE(p.is_bit_set(w-1));
			ASSERT_TRUE(p.is_bit_set(w-2));
		}
		if (o)
		{
			// must be odd
			ASSERT_TRUE(p.is_bit_set(0));
		}

	};
	for (int i = 1; i <= 128; i++)		gen(i);
	for (int i = 1; i <= 128; i++)		gen(i, false, true);
	for (int i = 2; i <= 128; i++)		gen(i, true, false);
	for (int i = 2; i <= 128; i++)		gen(i, true, true);
}

// https://prime-numbers.info/list/first-1000-primes
// https://prime-numbers.info/list/safe-primes (this library does not generate them, but will check here)
TEST(bignum_test, generate_primes)
{
	auto gen = [](int w)
	{
		Bignum p;
		p.gen_prime(w);
		cout << "genprime width=" << p.width() << ": " << p;
		ASSERT_TRUE(p.is_prime());
		ASSERT_EQ(p.width(), w);

		p -= 1;
		p /= 2;
		if (p.is_prime())
		{
			cout << " SAFE";
		}
		cout << endl;
	};
	for (int i = 2; i <= 128; i++)		gen(i);
}

TEST(bignum_test, bit_test_set_clear)
{
	Bignum w32(3);
	cout << "init w32	  n="<<std::hex<<w32<<std::dec<<" width: "<< w32.width()<<std::endl;
	ASSERT_EQ(w32.width(), 2);
	ASSERT_TRUE(w32.is_bit_set(0));
	ASSERT_TRUE(w32.is_bit_set(1));
	ASSERT_FALSE(w32.is_bit_set(2));
	w32.set_bit(30);
	w32.set_bit(31);
	cout << "set bit 30-31 n="<<std::hex<<w32<<std::dec<<" width: "<< w32.width()<<std::endl;
	ASSERT_EQ(w32, 0xc0000003);
	ASSERT_EQ(w32.width(), 32);
	ASSERT_TRUE(w32.is_bit_set(0));
	ASSERT_TRUE(w32.is_bit_set(1));
	ASSERT_FALSE(w32.is_bit_set(2));
	ASSERT_TRUE(w32.is_bit_set(30));
	ASSERT_TRUE(w32.is_bit_set(31));
	ASSERT_FALSE(w32.is_bit_set(32));
	w32.clear_bit(30);
	w32.clear_bit(31);
	cout << "clr w32 30-31 n="<<std::hex<<w32<<std::dec<<" width: "<< w32.width()<<std::endl;
	ASSERT_EQ(w32, 0x3);
	ASSERT_EQ(w32.width(), 2);
	ASSERT_TRUE(w32.is_bit_set(0));
	ASSERT_TRUE(w32.is_bit_set(1));
	ASSERT_FALSE(w32.is_bit_set(2));
	ASSERT_FALSE(w32.is_bit_set(30));
	ASSERT_FALSE(w32.is_bit_set(31));
	ASSERT_FALSE(w32.is_bit_set(32));

	Bignum w64(3);
	w64 <<= 32;
	cout << "init w64	  n="<<std::hex<<w64<<std::dec<<" width: "<< w64.width()<<std::endl;
	ASSERT_EQ(w64.width(), 34);
	ASSERT_TRUE(w64.is_bit_set(32));
	ASSERT_TRUE(w64.is_bit_set(33));
	ASSERT_FALSE(w64.is_bit_set(34));
	w64.set_bit(62);
	w64.set_bit(63);
	cout << "set bit 62-63 n="<<std::hex<<w64<<std::dec<<" width: "<< w64.width()<<std::endl;
	ASSERT_EQ(w64.width(), 64);
	ASSERT_TRUE(w64.is_bit_set(32));
	ASSERT_TRUE(w64.is_bit_set(33));
	ASSERT_FALSE(w64.is_bit_set(34));
	ASSERT_TRUE(w64.is_bit_set(62));
	ASSERT_TRUE(w64.is_bit_set(63));
	ASSERT_FALSE(w64.is_bit_set(64));
	w64.clear_bit(62);
	w64.clear_bit(63);
	cout << "clr bit 62-63 n="<<std::hex<<w64<<std::dec<<" width: "<< w64.width()<<std::endl;
	ASSERT_EQ(w64.width(), 34);
	ASSERT_TRUE(w64.is_bit_set(32));
	ASSERT_TRUE(w64.is_bit_set(33));
	ASSERT_FALSE(w64.is_bit_set(34));
	ASSERT_FALSE(w64.is_bit_set(62));
	ASSERT_FALSE(w64.is_bit_set(63));
	ASSERT_FALSE(w64.is_bit_set(64));
}

TEST(bignum_test, bit_ops_and)
{
	auto doit = [](int wa, int wb){
		Bignum a(1);
		a <<= wa-1;
		cout << "bit ops   a="<<a.str(true)<<" width: "<< a.width()<<std::endl;
		Bignum b(1);
		b <<= wb-1;
		cout << "bit ops   b="<<b.str(true)<<" width: "<< b.width()<<std::endl;
		Bignum c;
		c = a + b;
		cout << "bit ops   c="<<c.str(true)<<" width: "<< c.width()<<std::endl;
		Bignum x;
		x = a & b;
		cout << "bit ops a&b="<<x.str(true)<<" width: "<< x.width()<<std::endl;
		ASSERT_EQ(x, 0);
		x = a & c;
		cout << "bit ops a&c="<<x.str(true)<<" width: "<< x.width()<<std::endl;
		ASSERT_EQ(x, a);
		x = b & c;
		cout << "bit ops b&c="<<x.str(true)<<" width: "<< x.width()<<std::endl;
		ASSERT_EQ(x, b);
		x = c;
		x &= a;
		ASSERT_EQ(x, a);
		x = c;
		x &= b;
		ASSERT_EQ(x, b);
	};
	doit(1, 2);
	doit(30, 31);
	doit(31, 32);
	doit(32, 33);

	Bignum a(1), b(2);
	ASSERT_EQ(a&3, 1);
	ASSERT_EQ(b&3, 2);
	Bignum x = a&3;
	ASSERT_EQ(x, 1);
	x = b&3;
	ASSERT_EQ(x, 2);
}

TEST(bignum_test, bit_ops_or)
{
	auto doit = [](int wa, int wb){
		Bignum a(1);
		a <<= wa-1;
		Bignum b(1);
		b <<= wb-1;
		Bignum c;
		c = a + b;
		Bignum x;
		x = a | b;
		ASSERT_EQ(x, c);
		x = a | c;
		ASSERT_EQ(x, c);
		x = b | c;
		ASSERT_EQ(x, c);
		x = c;
		x |= a;
		ASSERT_EQ(x, c);
		x = c;
		x |= b;
		ASSERT_EQ(x, c);
	};
	doit(1, 2);
	doit(30, 31);
	doit(31, 32);
	doit(32, 33);

	Bignum a(1), b(2);
	ASSERT_EQ(a|3, 3);
	ASSERT_EQ(b|3, 3);
	Bignum x = a|3;
	ASSERT_EQ(x, 3);
	x = b|3;
	ASSERT_EQ(x, 3);
}

TEST(bignum_test, bit_ops_xor)
{
	auto doit = [](int wa, int wb){
		Bignum a(1);
		a <<= wa-1;
		Bignum b(1);
		b <<= wb-1;
		Bignum c;
		c = a + b;
		Bignum x;
		x = a ^ b;
		ASSERT_EQ(x, c);
		x = a ^ c;
		ASSERT_EQ(x, b);
		x = b ^ c;
		ASSERT_EQ(x, a);
		x = c;
		x ^= a;
		ASSERT_EQ(x, b);
		x = c;
		x ^= b;
		ASSERT_EQ(x, a);
	};
	doit(1, 2);
	doit(30, 31);
	doit(31, 32);
	doit(32, 33);

	Bignum a(1), b(2), c(3);
	ASSERT_EQ(c^1, 2);
	ASSERT_EQ(c^2, 1);
	Bignum x = c^1;
	ASSERT_EQ(x, 2);
	x = c^2;
	ASSERT_EQ(x, 1);
}

TEST(bignum_test, bit_ops_not)
{
	Bignum a=1, b=0;
	ASSERT_EQ(~a, b);
	ASSERT_EQ(~b, a);
	a=0xffff0000;
	b=0xffff;
	ASSERT_EQ(~a, b);
	a <<= 16;
	b=0xffffffff;
	ASSERT_EQ(~a, b);
}

TEST(bignum_test, width)
{
	Bignum a(0xf);
	ASSERT_EQ(a.width(), 4);
	auto p = [](int shift)
	{
		Bignum b = 1;
		b <<= shift;
		cout << "width of " << b.str(true) << ": " << b.width() << endl;
		ASSERT_EQ(b.width(), shift+1);
	};
	Bignum b = 0;
	cout << "width of " << b.str(true) << ": " << b.width() << endl;
	ASSERT_EQ(b.width(), 1);
	p(0);
	p(1);
	p(30);
	p(31);
	p(32);
	p(33);
	p(147);
}

TEST(bignum_test, negative)
{
	Bignum a;
	
	ASSERT_FALSE(a.is_negative());
	a = 128;
	ASSERT_FALSE(a.is_negative());
	a = -a;
	ASSERT_TRUE(a.is_negative());
}

TEST(bignum_test, get)
{
	Bignum a;
	vector<char> v;

	auto get_test = [&]
	{
		v.resize(a.len());
		a.get((void*)v.data(), v.size());
		cout << "a = " << a << " get hex " << bin_to_hexstr(v, " ") << endl;
	};

	get_test();
	a = 127;
	get_test();
	ASSERT_EQ(v, vector<char>{'\x7f'});
	a = -a;
	ASSERT_THROW(get_test(), runtime_error);
	a = 128;
	get_test();
	ASSERT_EQ(v, vector<char>{'\x80'});
}

TEST(bignum_test, get_2sc)
{
	Bignum a;
	vector<char> v;

	auto get_test = [&](vector<char> chk)
	{
		cout << "a = " << a << " len_2sc() = " << a.len_2sc() << endl;
		v.resize(a.len_2sc());
		a.get_2sc((void*)v.data(), v.size());
		cout << "a = " << a << " get hex " << bin_to_hexstr(v, " ") << endl;
		ASSERT_EQ(v, chk);
	};
/*
		128 0000 0000 1000 0000 - 00 80
		127 0111 1111 - 7f
		-1 1111 1111 - ff
		-127 1000 0001 - 81
		-128 1000 0000 - 80
		-129 1111 1111 0111 1111 - ff 7f
*/

	get_test({'\x00'});
	a = 128;
	get_test({'\x00','\x80'});
	a = 127;
	get_test({'\x7f'});
	a = 1;
	a = -a;
	get_test({'\xff'});
	a = 127;
	a = -a;
	get_test({'\x81'});
	a = 128;
	a = -a;
	get_test({'\x80'});
	a = 129;
	a = -a;
	get_test({'\xff','\x7f'});
}
