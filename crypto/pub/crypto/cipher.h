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
#ifndef _SCC_CRYPTO_CIPHER_H
#define _SCC_CRYPTO_CIPHER_H

#include <stdint.h>
#include <string>
#include <vector>
//#include <util/iobase.h>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_cipher Symmetric block ciphers
	@{

	Symmetric block ciphers suitable for use in TLS encryption.

	AES (Advanced Encryption Standard):
	* AES Spec: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
	* Block Ciphers: ISO/IEC 18033-3: https://www.sis.se/api/document/preview/912979/

	Algorithms for Authenticated Encryption with Associated Data (AEAD):
	* https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf

	AES-GCM (AES - Galois/Counter Mode):
	* IPP should make use of the carryless multiplication instruction PCLMULQDQ if available.
	
	AES-CCM (AES - Counter with Cipher Block Chaining-Message Authentication Code)

	These are considered cryptographically safe for TLS 1.3:
	* https://tools.ietf.org/html/rfc8446

	Used in TLS algorithms:

	https://tools.ietf.org/html/rfc5116 defines the cipher part of the following TLS 1.3 cipher suites:
	* TLS_AES_128_GCM_SHA256 (MUST be implemented)
		* key length is 16 octets (128 bits)
		* initialization vector (nonce) length is 12 octets (96 bits)
		* ciphertext is 16 octets longer than plaintext, with 16 octet (128 bit) authentication key appended
	* TLS_AES_256_GCM_SHA384 (SHOULD be implemented)
		* same as above, with key length 32 octets (256 bits)
	* TLS_AES_128_CCM_SHA256
		* same as above, with key length 16 octets (128 bits)
*/

/** Symmetric block ciphers.
	\file cipher.h
*/

class CipherBase
{
public:
	virtual void reset(const void*, int, const void*, int/*, uint64_t*/) = 0;
	virtual void aad(const void*, int) = 0;
	virtual void encrypt(const void*, int, void*, int) = 0;
	virtual void decrypt(const void*, int, void*, int) = 0;
	virtual void auth_tag(void*, int) = 0;
};

/** Symmetric block cipher.

	A symmetric block cipher with fix bit width.
*/
class Cipher
{
	CipherBase* m_ctx;
	int m_type;
public:

	enum Type
	{
		aes_gcm_type			=1000,
		aes_ccm_type			=2000,
	};

	/** Create a cipher.

		\param type Cipher type
		\param key_loc Key buffer
		\param key_len Key size, must be 16, 24, or 32 bytes
		\param tag_len Auth tag length, must be specified during creation only for ccm (between 4 and 16 and even).
		
		TLS_AES_128_GCM_SHA256 has key size 16
		TLS_AES_256_GCM_SHA384 has key size 32
		TLS_AES_128_CCM_SHA256 has key size 16, and tag_len 16

		https://tools.ietf.org/html/rfc5116

		Note for 

		AEAD_AES_128_GCM (used in TLS_AES_128_GCM_SHA256) has key 16, tag 16, nonce 12
		Ciphertext is Encrypted Plaintext + tag (16 bytes longer than plaintext)

		AEAD_AES_256_GCM (used in TLS_AES_256_GCM_SHA384) has key 32, tag 16, nonce 12
		Ciphertext is Encrypted Plaintext + tag (16 bytes longer than plaintext)

		AEAD_AES_256_GCM (used in TLS_AES_128_CCM_SHA256) has key 16, tag 16, nonce 12
		Ciphertext is Encrypted Plaintext + tag (16 bytes longer than plaintext)

		Symmetric block cipher, of either aes_gcm_type, or aes_ccm_type.
	*/
	Cipher(Type type, const void* key_loc, int key_len, int = 16);
	Cipher(Type type, const std::vector<char>& key, int tag_len = 16) : Cipher(type, key.data(), key.size(), tag_len) {}
	Cipher(Type type, const std::string& key, int tag_len = 16) : Cipher(type, key.data(), key.size(), tag_len) {}
	virtual ~Cipher();
	Cipher(const Cipher&) = delete;			// no copy
	Cipher& operator=(const Cipher&) = delete;
	Cipher(Cipher&& other)
	{
		m_ctx = other.m_ctx;
		m_type = other.m_type;
		other.m_ctx = nullptr;
		other.m_type = 0;
	}
	Cipher& operator=(Cipher&& other)
	{
		m_ctx = other.m_ctx;
		m_type = other.m_type;
		other.m_ctx = nullptr;
		other.m_type = 0;
		return *this;
	}

	size_t nonce_min() const
	{
		switch (m_type)
		{
			case aes_gcm_type: return 1;
			case aes_ccm_type: return 8;
		}
	}
	size_t nonce_max() const
	{
		switch (m_type)
		{
			case aes_gcm_type: return 128;		// not specified
			case aes_ccm_type: return 12;
		}
	}

	/** Reset the processor to prepare to encrypt or decrypt a new message.

		\param nonce_loc Initialization buffer
		\param nonce_len Initialization buffer size
		\param data_len	Data length

		Nonce is an initialization vector, generally a random byte sequence. The nonce must be between 8 and 12 for CCM,
		and greater than 1 for GCM.

		TLS_AES_128_GCM_SHA256, TLS_AES_256_GCM_SHA384, TLS_AES_128_CCM_SHA256 specify nonce size 12.

		Additional authenticated data can be specified, which is authenticated but not encrypted or decrypted.

		The total message length must be specified. Encrypt and decrypt will be called until the entire message is processed.

		TLS_AES_128_GCM_SHA256 and TLS_AES_256_GCM_SHA384 specifies max msg_len as 2^36 - 31
		TLS_AES_128_CCM_SHA256 specifies max msg_len as 2^24 - 1
	*/
	void reset(const void* nonce_loc, int nonce_len, const void* aad_loc = nullptr, int aad_len = 0/*, size_t msg_len*/)
	{
		m_ctx->reset(nonce_loc, nonce_len, aad_loc, aad_len);
	}

	/** Set additional authenticated data (for GCM type only).
	
		This must be called after reset(), but before and data is encrypted or decrypted.
		Can be called multiple times.

		TLS_AES_128_GCM_SHA256 and TLS_AES_256_GCM_SHA384 specifies max aad as 2^61 - 1
		TLS_AES_128_CCM_SHA256 specifies max aad as 2^64 - 1
	*/
	void aad(const void* aad_loc, int aad_len)
	{
		m_ctx->aad(aad_loc, aad_len);
	}

	/**	Encrypt a message.

		\param msg_loc Plaintext message buffer
		\param msg_len Plaintext message size
		\param cipher_loc Ciphertext message buffer
		\param cipher_len Ciphertext buffer size (must be >= msg_len)
		
		Resulting ciphertext must be the same length as plaintext.

		Can call multiple times to continue encrypting a single message.
	*/
	void encrypt(const void* msg_loc, int msg_len, void* cipher_loc, int cipher_len)
	{
		m_ctx->encrypt(msg_loc, msg_len, cipher_loc, cipher_len);
	}

	/**	Decrypt a message.

		\param cipher_loc Ciphertext message buffer
		\param cipher_len Ciphertext message size
		\param msg_loc Plaintext message buffer
		\param msg_len Plaintext message size (must be >= cipher_len)

		Resulting plaintext must be the same length as ciphertext.

		Can call multiple times to continue decrypting a single message.
	*/
	void decrypt(const void* cipher_loc, int cipher_len, void* msg_loc, int msg_len)
	{
		m_ctx->decrypt(cipher_loc, cipher_len, msg_loc, msg_len);
	}

	/** Gets the authentication tag. This validates the message and additional data.
		\param tag_loc Tag buffer
		\param tag_len Tag buffer size

		Size can be to be 1 <= tag_len <= 16 for GCM
		Size must be be the constructed tag_len for CCM
	*/
	void auth_tag(char* tag_loc, int tag_len)
	{
		m_ctx->auth_tag(tag_loc, tag_len);
	}
};
/** @} */
/** @} */

}	// namespace

#endif
