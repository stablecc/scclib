#include <encode/adler32.h>
#include <system_error>
#include <zlib.h>

using namespace scc::encode;

uint32_t Adler32::update(const void* loc, int len)
{
	if (loc == nullptr)			throw std::runtime_error("adler32 update called with null");
	if (len <= 0)				throw std::runtime_error("adler32 update called with invalid len");

	m_val = adler32(m_val, static_cast<const Bytef*>(loc), len);
	m_sz += len;
	return m_val;
}

/*
		Using the recurrence relation where a(x, y) is low order and b(x, y)
		is high order 16 bits of the checksum:

		xrem = value of data at x
		xadd = value of data at y

		a(x+1,y+1) = (a(x,y) - xrem + xadd) MOD 65521
		b(x+1,y+1) = (b(x,y) - N*xrem + a(x+1,y+1) - 1) MOD 65521
		
		where N=x-y+1 is the window size N

		To avoid calculating the modulus of a negative number,
		can force the expression positive by adding a multiple of MOD
		that ensures the value will be positive.
*/

#define MOD 65521

uint32_t Adler32::rotate(unsigned char xrem, unsigned char xadd)
{
	if (m_sz == 0) throw std::runtime_error("Cannot rotate with window size 0");

	uint32_t a = m_val & 0xffff;
	a = (a - xrem + xadd + MOD) % MOD;
	uint32_t b = (m_val >> 16) & 0xffff;
	b = (b - m_sz*xrem + a - 1 + (m_sz*xrem/MOD+1)*MOD) % MOD;
	m_val = (b << 16) | a;
	return m_val;
}

uint32_t Adler32::combine(const Adler32& add)
{
	m_val = adler32_combine(m_val, add.m_val, add.m_sz);
	return m_val;
}