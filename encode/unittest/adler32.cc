#include <encode/adler32.h>
#include <crypto/random.h>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <fstream>

/** \addtogroup encode_checksums
	@{
*/

/** Test file for \ref encode_checksums
	\file
*/

/** \example scclib/encode/unittest/adler32.cc
	
	Tests for adler32 checksum.
*/

/** @} */

using namespace std;
using scc::encode::Adler32;

//! [Test vars]
static string test =
"ljpoaweu9uwat7a9g0ujaW219U0U;DSJGEOPUJGAfPVAPUAS:FGJALGJ7804-85,"		// 64 bytes
"G;AKGPTG[ASIGSFDAS[DFSAPDFJASPFJSPADFJPAJPGAJSGSAGJAPJGAPJGPOIOO";		// 64 bytes
static uint32_t all_ad = 0xf9932612;		// all bytes
static uint32_t last64_ad = 0x583e1280;		// last 64 bytes
//! [Test vars]

TEST(adler32, construct)
{
	Adler32 ad;
	ASSERT_EQ(ad.val(), 1);
	ASSERT_EQ(ad.size(), 0);

	Adler32 ad2(test.data(), test.size());
	ASSERT_EQ(ad2.val(), all_ad);
	ASSERT_EQ(ad2.size(), test.size());
}

TEST(adler32, single_update)
{
	Adler32 ad;
	ad.update(test.data(), test.size());
	ASSERT_EQ(ad.val(), all_ad);
}

TEST(adler32, two_updates)
{
	Adler32 ad;
	ad.update(test.data(), test.size()/2);
	ad.update(test.data()+test.size()/2, test.size()-test.size()/2);
	ASSERT_EQ(ad.val(), all_ad);
}

TEST(adler32, second_half_update)
{
	Adler32 ad;
	ad.update(test.data()+test.size()/2, test.size()-test.size()/2);
	ASSERT_EQ(ad.val(), last64_ad);
}

TEST(adler32, update_and_reset)
{
	Adler32 ad;
	ASSERT_EQ(ad.update(test.data(), test.size()), all_ad);
	ASSERT_EQ(ad.reset(test.data()+test.size()/2, test.size()-test.size()/2), last64_ad);
	ASSERT_EQ(ad.reset(), 1);
}

TEST(adler32, combine)
{
	Adler32	first(test.data(), test.size()/2),
			second(test.data()+test.size()/2, test.size()/2);

	ASSERT_EQ(first.combine(second), all_ad);
}

TEST(adler32, rolling_update)
{
	//! [Rolling update]
	Adler32 ad;
	size_t sz = test.size();				// 128 bytes
	
	ad.reset(test.data(), sz/2);			// first 64 bytes, sets window size to 64
	ASSERT_EQ(ad.size(), 64);
	
	// rolling 64 byte window, will calculate the checksum at each 64 byte window
	for (size_t i = 0; i < 64; i++)
	{
		ad.rotate(test[i], test[64+i]);		// first byte is 0-63, last is 64-127
	}
	ASSERT_EQ(ad, last64_ad);	  			// end up with the last 64 bytes checksum
	//! [Rolling update]
}

TEST(adler32, verify_large_blocks)
{
	Adler32 ad;
	Adler32 vad;

	int datasz = 1<<14;	// 512, block will be up to 5096
	char dat[datasz];

	scc::crypto::RandomEngine::rand_bytes(dat, datasz);

	for (int blksz = 1; blksz < datasz>>1; blksz <<= 1)
	{
		cout << "testing adler32 rolling checksum for block size: " << blksz << endl;
		
		ad.reset(&dat[0], blksz);		// reset the checksum we will use for rolling

		for (int i = 0; i < datasz-blksz; i++)
		{
			vad.reset(&dat[i], blksz);			// calculate the current block directly
			ASSERT_EQ(ad, vad);
			ad.rotate(dat[i], dat[i+blksz]);	// calculate the next window via rolling
		}
	}
}
