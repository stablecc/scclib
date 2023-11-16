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
#ifndef _CRYPTO_HASH_H
#define _CRYPTO_HASH_H

#include <vector>
#include <string>
#include <memory>
#include <crypto/secvec.h>
#include <util/iobase.h>

// forward declarations
class HashBase;
class HmacBase;

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_hash One-way hashing and message digests
	@{

	Provides SHA, MD5, and message digest algorithms.

	Secure hash algorithms (SHA): https://tools.ietf.org/html/rfc4634
	Message digests using keyed hashing: https://tools.ietf.org/html/rfc2104
	The MD5 messsage digest algorithm: https://tools.ietf.org/html/rfc1321
*/

/** One-way hashing and message digests.
	\file
*/

/** General one-way hashing algorithms.

	On 64-bit processors, sha512 and sha384 outperforms sha256 for general data, due to the
	use of 64-bit word size for calculations.

	Secure hash standard is here:
	https://csrc.nist.gov/csrc/media/publications/fips/180/4/final/documents/fips180-4-draft-aug2014.pdf

	Security of hash algorithms is here:
	https://ws680.nist.gov/publication/get_pdf.cfm?pub_id=911479

	Collision resistance strength is, in general, 2^(n/2) where n is bit length of hash.

	So, for sha512/224, for example, this is 2^112 or S~=10^34

	Can show that the chance of collision for k hashes, is approx. k^2 / S.

	So for 10^9 hashes, probablility of collision would be approx. 10^-16

	For sha512/256, S~=10^38, so 10^9 hashes collision probablity would be approx. 10^-20

	MD5 and SHA1 hashes are not considered secure, but are included for applications where better performance is needed.
*/
class Hash
{
	std::unique_ptr<HashBase> m_ptr;
	int m_alg;
	int m_size;
public:

	/** Hash type. */
	enum Algorithm
	{
		md5_type		=1001,		// Note: the MD5 algorithm is not considered secure for cryptographic purposes, but has been included to support legacy services
		sha1_type		=1002,		// Note: SHA-1 is also considered insecure, SHA-2 (SHA-256) is suggested as an alternative.
		sha224_type		=1003,
		sha256_type		=1004,
		sha384_type		=1005,
		sha512_type		=1006,
		sha512_224_type	=1007,		// 224 bit hash based on 512 bit blocks (performance better on 64 bit machines)
		sha512_256_type	=1008,		// 256 bit hash based on 512 bit blocks (performance better on 64 bit machines)
		sm3_type		=1009,		// 256 bit hash based on 512 bit blocks (performance better on 64 bit machines)
	};
	/** Hash size in bytes. */
	enum Size
	{
		md5_size		=16,
		sha1_size		=20,
		sha224_size		=28,
		sha256_size		=32,
		sha384_size		=48,
		sha512_size		=64,
		sha512_224_size	=28,
		sha512_256_size	=32,
		sm3_size		=32,
	};

	static int get_size(Algorithm alg)
	{
		switch (alg)
		{
			case md5_type:			return md5_size;
			case sha1_type:			return sha1_size;
			case sha224_type:		return sha224_size;
			case sha256_type:		return sha256_size;
			case sha384_type:		return sha384_size;
			case sha512_type:		return sha512_size;
			case sha512_224_type:	return sha512_224_size;
			case sha512_256_type:	return sha512_256_size;
			case sm3_type:			return sm3_size;
		};
		return 0;
	}

	/** Initialize with hash type.

		Creating an algorithm for an unsupported type results in an exception.

		\param alg Hash algorithm, \see Hash::Algorithm
	*/
	Hash(int);
	virtual ~Hash();
	
	Hash(const Hash&) = delete;
	Hash& operator=(const Hash&) = delete;

	/** Move constructor. Original hash will be invalid, and throw an exception if operations are attempted. */
	Hash(Hash&& other);
	/** Move assign. Original hash will be invalid, and throw an exception if operations are attempted. */
	Hash& operator=(Hash&& other);

	/** Test if an algorithm type is supported. */
	static bool supported(int);

	/** Reset the hash to initial value. */
	void reset();

	/**	Update the hash by adding data.
		
		\param loc Update data buffer
		\param len Size of buffer
	*/
	void update(const void*, int);
	/** Update using a char vector. */
	void update(const std::vector<char>& v)
	{
		update(v.data(), v.size());
	}
	/** Update using a string. */
	void update(const std::string v)
	{
		update(v.data(), v.size());
	}

	/**	Get the current hash value without resetting the hash.

		\param loc Hash value buffer
		\param len Size of buffer, can be between 1 and full hash size.
		\return size written

		Can request a subset of hash.

		Throws exception if buffer size is out of range.
	*/
	int get_tag(void*, int);
	/** Get the full current hash value using a secure vector. */
	int get_tag(std::vector<char>& v)
	{
		v.resize(size());
		return get_tag(v.data(), v.size());
	}
	/** Get the current hash value using a vector, for length between 1 and size of hash. 
		Return size written to output vector.
	*/
	int get_tag(std::vector<char>& v, int sz)
	{
		if (sz >= 0)	v.resize(sz);
		return get_tag(v.data(), v.size());
	}

	/**	Write out a final value, and reset the hash.
		\param loc Hash value buffer
		\param len Size of buffer, must be the full hash size.
		\return size written

		Throws an exception if len is too small to store hash.
	*/
	int final(void*, int);
	/** Get the final value using a vector. Return size written to output vector. */
	int final(std::vector<char>& v)
	{
		v.resize(size());
		return final(v.data(), v.size());
	}

	/** Hash algorithm. \see Hash::Algorithm */
	int alg() const { return m_alg; }

	/** Return the current hash size. */
	int size() const { return m_size; }
};

/** Helper class to hash an incoming stream.
*/
class HashReader : public scc::util::Reader
{
	scc::util::Reader& m_rd;
	Hash& m_chk;
	SecVecChar m_buf;
public:
	HashReader(scc::util::Reader&, Hash&);
	virtual ~HashReader();
	virtual size_t read(void*, size_t);
};

/** Helper class to hash an outgoing stream.
*/
class HashWriter : public scc::util::Writer
{
	scc::util::Writer& m_wr;
	Hash& m_chk;
public:
	HashWriter(scc::util::Writer&, Hash&);
	virtual ~HashWriter();
	virtual size_t write(const void*, size_t);
	virtual void shutdown() {}
};

/** Hmac, aka hash-based message authentication code.

	https://tools.ietf.org/html/rfc2104

	Streaming class allows updates of the hash and calculation of a final result.
*/
class Hmac
{
	std::unique_ptr<HmacBase> m_ptr;
	int m_alg;
	int m_size;
public:
	/** Construct an hmac. 
		\param key key to set into the hmac
		\param len length of key
		\param hash_alg hash algorithm, \see Hash::Algorithm
	*/
	Hmac(const void*, int, int);
	/** Construct an hmac.
		\param key key to set into the hmac
		\param hash_alg hash algorithm, \see Hash::Algorithm
	*/
	Hmac(const std::vector<char>& key, int hash_alg) : Hmac(key.data(), key.size(), hash_alg) {}
	/** Construct an hmac.
		\param key key to set into the hmac
		\param hash_alg hash algorithm, \see Hash::Algorithm
	*/
	Hmac(const std::string& key, int hash_alg) : Hmac(key.data(), key.size(), hash_alg) {}
	virtual ~Hmac();
	Hmac(const Hmac&) = delete;					// no copy
	Hmac& operator=(const Hmac&) = delete;
	/** Move constructor. Original hmac will throw an exception if operations are attempted. */
	Hmac(Hmac&& other);
	/** Move assign. Original hmac will throw an exception if operations are attempted. */
	Hmac& operator=(Hmac&& other);

	/** Test if an algorithm type is supported. */
	static bool supported(int);

	/** Initialize the hmac with a new key.
		\param key key to set into the hmac
		\param len length of key
	*/
	void init(const void*, int);
	/** Initialize the hmac with a new key vector. */
	void init(std::vector<char>& v)
	{
		init(v.data(), v.size());
	}
	/** Reset the hmac, using the same key. */
	void reset();
	/** Update the hmac with new data. */
	void update(const void*, int);
	/** Update the hmac with new data. */
	void update(const std::vector<char>& v)
	{
		update(v.data(), v.size());
	}
	void update(const std::string& v)
	{
		update(v.data(), v.size());
	}
	
	/**  Calculate the final hmac value.
	
		Reset the hmac, using the same key.
		
		\param loc Location to write final hash value
		\param len Available buffer size, should be >= size required by hash.
		\return Size written
	*/
	int final(void*, int);
	/** Calculate the final hmac value. Return size written. */
	int final(std::vector<char>& v)
	{
		v.resize(size());
		return final(v.data(), v.size());
	}

	/** Hmac algorithm. \see Hash::Algorithm */
	int alg() const { return m_alg; }
	
	/** Hmac size. */
	int size() const { return m_size; }
};

/** @} */
/** @} */

}		// namespace

#endif
