#ifndef _SCC_ENCODE_ADLER32_H
#define _SCC_ENCODE_ADLER32_H

#include <cstdint>

namespace scc::encode {

/** \addtogroup encode
	@{
*/

/** \defgroup encode_checksums Checksums
	@{

	Common checksums.
*/

/** Adler32 rolling checksum.
	\file
*/

/** Adler-32 checksum allowing rolling calculation.

	Rolling calculation allows very fast calculations within a sliding window, removing the overhead of
	recalculating for the entire window size.

	Test example from \ref scclib/encode/unittest/adler32.cc

	\snippet scclib/encode/unittest/adler32.cc Test vars
	\snippet scclib/encode/unittest/adler32.cc Rolling update
*/
class Adler32
{
	uint32_t m_val;
	int m_sz;
public:
	/** Construct with initialized value. */
	Adler32() : m_val(1), m_sz(0) {}
	/** Construct with initial value. 
		\param loc Buffer location.
		\param len Buffer size.
	*/
	Adler32(const void* loc, int len) : m_val(1), m_sz(0)
	{
		update(loc, len);
	}
	virtual ~Adler32() {}

	operator uint32_t() const { return m_val; }
	uint32_t val() const { return m_val; }

	/** Size of the current window.
	*/
	int size()
	{
		return m_sz;
	}

	/** Reset the checksum.
	*/
	uint32_t reset()
	{
		m_val = 1;
		m_sz = 0;
		return m_val;
	}
	
	/** Reset the checksum, and update with initial value.
		\param loc Buffer location.
		\param len Buffer size. Window length is set to len.
	*/
	uint32_t reset(const void* loc, int len)
	{
		m_val = 1;
		m_sz = 0;
		return update(loc, len);
	}

	/** Update the checksum with buffer contents.
	
		\param loc Buffer location.
		\param len Buffer size. Window length is increased by len.
	*/
	uint32_t update(const void*, int);
	

	/*	Combine two adler checksums, using the length of the second.
	
		The checksum has size incremented by len.

		\param add Checksum to combine.
	*/
	uint32_t combine(const Adler32&);

	/** Update the checksum with next byte. Usable for a "rolling window" checksum.

		For example for bytes 01234 with checksum ch1,
		calling rotate('0', '5') will produce a checksum ad2 on bytes 12345.
		
		\param xrem Byte to rotate out (first in old window)
		\param xadd Byte to rotate in (last in new window)
		
		see https://openresearch-repository.anu.edu.au/bitstream/1885/40765/3/TR-CS-96-05.pdf
	*/
	uint32_t rotate(unsigned char, unsigned char);
	uint32_t rotate(char xrem, char xadd) {
		return rotate(static_cast<unsigned char>(xrem), static_cast<unsigned char>(xadd));
	}
};
/** @} */
/** @} */
}

#endif
