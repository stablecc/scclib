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
#ifndef _SCC_CRYPTO_RSA_H
#define _SCC_CRYPTO_RSA_H

#include <string>
#include <vector>
#include <memory>
#include <crypto/bignum.h>
#include <crypto/hash.h>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_rsa RSA public key cryptography
	@{

	Provides RSA public key cryptography, suitable for use in TLS encryption.

	RSA cryptography defined in: https://tools.ietf.org/html/rfc8017

	OAEP optimal asymmetric encryption scheme, defined in public key cryptography standards v2.1: https://tools.ietf.org/html/rfc3447
*/

/** RSA public key cryptography.
	\file rsa.h
*/

class RsaPrivateKey;

/**	RSA Public Key

	A public key consists of:
		modulus n = p*q
		public exponent e (usually set to 65537 0x10001)
*/
class RsaPublicKey
{
	friend class PkcsSignature;
	friend class PssSignature;
	friend class RsaOaepEncrypt;
	friend class RsaPrivateKey;

protected:
	scc::crypto::Bignum m_n;		// modulus
	scc::crypto::Bignum m_e;		// public exponent

private:
	
	void move(RsaPublicKey& other)
	{
		m_n = std::move(other.m_n);
		m_e = std::move(other.m_e);
	}

	void copy(const RsaPublicKey& other)
	{
		m_n = other.m_n;
		m_e = other.m_e;
	}

public:
	RsaPublicKey();
	virtual ~RsaPublicKey();

	RsaPublicKey(RsaPublicKey&& other) : RsaPublicKey()
	{
		move(other);
	}
	RsaPublicKey& operator=(RsaPublicKey&& other)
	{
		move(other);
		return *this;
	}

	RsaPublicKey(const RsaPublicKey& other) : RsaPublicKey()
	{
		copy(other);
	}
	RsaPublicKey& operator=(const RsaPublicKey& other)
	{
		copy(other);
		return *this;
	}

	bool operator==(const RsaPublicKey& o) const
	{
		return m_n == o.m_n && m_e == o.m_e;
	}
	
	bool operator!=(const RsaPublicKey& o) const
	{
		return m_n != o.m_n || m_e != o.m_e;
	}

	void get(scc::crypto::Bignum& n, scc::crypto::Bignum& e) const
	{
		n = m_n;
		e = m_e;
	}

	void set(const scc::crypto::Bignum& n, const scc::crypto::Bignum& e)
	{
		m_n = n;
		m_e = e;
	}
	
	/** Clear and erase all data. */
	void clear();

	/** Output with full values. */
	std::string dump() const;

	/** Output with formatted values. Emits a maximum number of bytes for each value. */
	std::string str(unsigned = 8) const;

	/** Bit width of the key. An initialized or cleared key will have width 0. */
	int width() const;

	/** Width in bytes of this key. */
	int width_bytes() const
	{
		return ((width()+7)&~7)/8;
	}
};

/**	RSA Private Key

	A private key consists of public key and the following:
		private exponent d
		prime1 p
		prime2 q
		exponent1 ep = d % (p-1)
		exponent2 eq = d % (q-1)
		inverse coefficient c = (~q) % p

	For security considerations, see:
	https://www.websecurity.digicert.com/content/dam/websitesecurity/digitalassets/desktop/pdfs/whitepaper/Elliptic_Curve_Cryptography_ECC_WP_en_us.pdf

	The key size of a private key is the width(), which is the bit size of the modulus.

	Security level means the equivalent security strength to a symmetric key with the stated bit size. Use of algorithms with security below 128 bits is not
	recommended.

	Examples:

	- width 1024 has security level 80
	- width 2048 has security level 112
	- width 3072 has security level 128
	- width 7680 has security level 192
	- width 15360 has security level 256
*/
class RsaPrivateKey : public RsaPublicKey
{
	friend class PssSignature;
	friend class PkcsSignature;
	friend class RsaOaepDecrypt;
	
	scc::crypto::Bignum m_d;		// private exponent
	scc::crypto::Bignum m_p;		// prime 1
	scc::crypto::Bignum m_q;		// prime 2
	scc::crypto::Bignum m_ep;		// exponent 1
	scc::crypto::Bignum m_eq;		// exponent 2
	scc::crypto::Bignum m_qinv;		// inverse coefficient

	void move(RsaPrivateKey& other)
	{
		m_n = std::move(other.m_n);
		m_e = std::move(other.m_e);
		m_d = std::move(other.m_d);
		m_p = std::move(other.m_p);
		m_q = std::move(other.m_q);
		m_ep = std::move(other.m_ep);
		m_eq = std::move(other.m_eq);
		m_qinv = std::move(other.m_qinv);
	}

	void copy(const RsaPrivateKey& other)
	{
		m_n = other.m_n;
		m_e = other.m_e;
		m_d = other.m_d;
		m_p = other.m_p;
		m_q = other.m_q;
		m_ep = other.m_ep;
		m_eq = other.m_eq;
		m_qinv = other.m_qinv;
	}

public:
	RsaPrivateKey();
	virtual ~RsaPrivateKey();

	RsaPrivateKey(RsaPrivateKey&& other) : RsaPrivateKey()
	{
		move(other);
	}
	RsaPrivateKey& operator=(RsaPrivateKey&& other)
	{
		move(other);
		return *this;
	}

	RsaPrivateKey(const RsaPrivateKey& other) : RsaPrivateKey()
	{
		copy(other);
	}
	RsaPrivateKey& operator=(const RsaPrivateKey& other)
	{
		copy(other);
		return *this;
	}

	bool operator==(const RsaPrivateKey& o) const
	{
		return m_n == o.m_n && m_e == o.m_e && m_d == o.m_d && m_p == o.m_p && m_q == o.m_q && m_ep == o.m_ep && m_eq == o.m_eq && m_qinv == o.m_qinv;
	}
	
	bool operator!=(const RsaPrivateKey& o) const
	{
		return m_n != o.m_n || m_e != o.m_e || m_d != o.m_d || m_p != o.m_p || m_q != o.m_q || m_ep != o.m_ep || m_eq != o.m_eq || m_qinv != o.m_qinv;
	}

	/** Clear and erase all data. */
	void clear();

	/** Dump a string with full values. Not recommended to log this output. */
	std::string dump() const;

	/** Output with formatted values. Emits a maximum number of bytes for each value. */
	std::string str(unsigned = 8) const;

	/** Generate a private key.

		Creates a private key with specified width.

		\param width Width in bits. Must be even and positive. Note that widths below 1024 are not considered secure.

		Throws exception on error.
	*/
	void generate(int);

	/**	Validate a public key with the private key.
	*/
	bool validate(const RsaPublicKey&) const;

	/**	Validate with my public key.
	*/
	bool validate() const
	{
		return validate(pub_key());
	}

	void get(scc::crypto::Bignum& n, scc::crypto::Bignum& e,
		scc::crypto::Bignum& d, scc::crypto::Bignum& p, scc::crypto::Bignum& q,
		scc::crypto::Bignum& ep, scc::crypto::Bignum& eq, scc::crypto::Bignum& qinv) const
	{
		n = m_n;
		e = m_e;
		d = m_d;
		p = m_p;
		q = m_q;
		ep = m_ep;
		eq = m_eq;
		qinv = m_qinv;
	}

	void set(const scc::crypto::Bignum& n, const scc::crypto::Bignum& e,
		scc::crypto::Bignum& d, scc::crypto::Bignum& p, scc::crypto::Bignum& q,
		scc::crypto::Bignum& ep, scc::crypto::Bignum& eq, scc::crypto::Bignum& qinv)
	{
		m_n = n;
		m_e = e;
		m_d = d;
		m_p = p;
		m_q = q;
		m_ep = ep;
		m_eq = eq;
		m_qinv = qinv;
	}

	/** Export the public key. */
	RsaPublicKey pub_key() const
	{
		RsaPublicKey pub;
		pub.set(m_n, m_e);
		return pub;
	}
};

struct RsaOaepEncryptCtx;

/** RSA OAEP encryption.

	Encrypts using OAEP scheme. Requires a public key, and choice of hash algorithm.
*/
class RsaOaepEncrypt
{
	std::unique_ptr<RsaOaepEncryptCtx> m_ctx;

public:
	/** Construct an RSA encryptor.
		\param key Valid public key.
		\param hash_type Hash algorithm from scc::crypto::Hash.
	*/
	RsaOaepEncrypt(RsaPublicKey&, Hash::Algorithm = Hash::sha1_type);
	virtual ~RsaOaepEncrypt();
	/** The maximum allowable plain text size in bytes.
		
		len(public key width) - 2*size(hash output) - 2
	*/
	int max_msg_size() const;
	/** Cipher text size in bytes.
		
		len(public key width)
	*/
	int cipher_size() const;
	
	/** Encrypt a message.

		\param msg_loc Plaintext message buffer
		\param msg_len Plaintext size, must be <= max_msg_len()
		\param cipher_loc Encrypted ciphertext buffer
		\param cipher_len Ciphertext size, must be exactly cipher_len()
		\param label_loc Optional label, which will be verified but not encrypted
		\param label_len Label length

		Throws exception on parameter error.
	*/
	void encrypt(const void*, int, void*, int, const void* = nullptr, int = 0);
};

struct RsaOaepDecryptCtx;

/** RSA OAEP decryption.

	Decrypts using OAEP scheme. Requires a private key, and choice of hash algorithm.

	Hash algoritm comes from scc::crypto::Hash.
*/
class RsaOaepDecrypt
{
	std::unique_ptr<RsaOaepDecryptCtx> m_ctx;

public:
	/** Construct an RSA decryptor

		\param key Valid private key.
		\param hash_type Hash algorithm from scc::crypto::Hash.
	*/
	RsaOaepDecrypt(RsaPrivateKey&, Hash::Algorithm = Hash::sha1_type);
	virtual ~RsaOaepDecrypt();
	/** The maximum allowable plain text message length in bytes.
		
		len(public key width) - 2*size(hash output) - 2
	*/
	int max_msg_size() const;
	/** Cipher text length in bytes.
		
		len(public key width)
	*/
	int cipher_size() const;

	/** Decrypt a message.

		\param msg_loc Plaintext message buffer, will be zeroed before decryption.
		\param msg_len Plaintext size, must be exactly max_msg_size()
		\param cipher_loc Encrypted ciphertext buffer
		\param cipher_len Ciphertext size, must be exactly cipher_size()
		\param label_loc Optional label, which will be verified but not encrypted
		\param label_len Label length
		\return The size of the decrypted message, or -1 if the decryption failed.
	*/
	int decrypt(void*, int, const void*, int, const void* = nullptr, int = 0);
};

/**
	RSASSA-PSS https://tools.ietf.org/html/rfc8017#section-8.1
	Notes on use in x.509: https://tools.ietf.org/html/rfc4055#section-3

	rsa_pss_rsae_sha256 is a required signature type for tls1_3.
*/

class PssSignature
{
public:
	enum class HashType
	{
		md5,
		sha1,
		sha224,
		sha256,
		sha384,
		sha512,
	};
	/** Size of signature in bytes.
	*/
	static int size(const RsaPublicKey& k)
	{
		return k.width_bytes();
	}

	/**
		Sign the signature using a private key.

		\param loc Message buffer
		\param len Message buffer size
		\param sig_loc Signature buffer
		\param sig_len Signature buffer size, must be the size returned by size().
		\param key Private RSA key
		\param hash Hash type
		\param salt_len Length of randomly generated salt

		Throws exception on parameter error.

		According to https://tools.ietf.org/html/rfc3447, "Typical salt lengths in octets are hLen (the length of the output
		of the hash function Hash) and 0".
	*/
	static void sign(const void*, int, void*, int, const RsaPrivateKey&, PssSignature::HashType, int = 0);
	static void sign(const void* loc, int len, std::vector<char>& sig, const RsaPrivateKey& key, PssSignature::HashType hash, int salt_len = 0)
	{
		sig.resize(size(key));
		sign(loc, len, sig.data(), sig.size(), key, hash, salt_len);
	}
	
	/**
		Verify the signature using a public key.

		\param loc Message buffer
		\param len Message buffer size
		\param sig_loc Signature buffer
		\param sig_len Signature buffer size, must be the size returned by size().
		\param key Public RSA key
		\param hash Hash type
		\return True if signature matches, false if signature size or value do not match.
	*/
	static bool verify(const void*, int, const void*, int, const RsaPublicKey&, PssSignature::HashType);
	static bool verify(const void* loc, int len, const std::vector<char>& sig, const RsaPublicKey& key, PssSignature::HashType hash)
	{
		return verify(loc, len, sig.data(), sig.size(), key, hash);
	}
};

/** PKCS #1 version 1.5 digital signature.

	RSASSA-PKCS1-v1_5 https://tools.ietf.org/html/rfc8017#section-8.2
	Notes on use in x.509: https://tools.ietf.org/html/rfc4055#page-13

	rsa_pks1_sha256 is a required signature type for tls1_3, see https://tools.ietf.org/html/rfc5116
*/
class PkcsSignature
{
public:
	enum class HashType
	{
		md5,
		sha1,
		sha224,
		sha256,
		sha384,
		sha512,
	};

	/** Size of signature in bytes.
	*/
	static int size(const RsaPublicKey& k)
	{
		return k.width_bytes();
	}

	/**
		Sign the signature using a private key.

		\param loc Message buffer
		\param len Message buffer size
		\param sig_loc Signature buffer
		\param sig_len Signature buffer size, must be the size returned by size().
		\param key Private RSA key
		\param hash Hash type

		Throws exception on parameter error.
	*/
	static void sign(const void*, int, void*, int, const RsaPrivateKey&, PkcsSignature::HashType);
	static void sign(const void* loc, int len, std::vector<char>& sig, const RsaPrivateKey& key, PkcsSignature::HashType hash)
	{
		sig.resize(size(key));
		sign(loc, len, sig.data(), sig.size(), key, hash);
	}
	static void sign(const void* loc, int len, std::vector<uint8_t>& sig, const RsaPrivateKey& key, PkcsSignature::HashType hash)
	{
		sig.resize(size(key));
		sign(loc, len, sig.data(), sig.size(), key, hash);
	}
	
	/**
		Verify the signature using a public key.

		\param loc Message buffer
		\param len Message buffer size
		\param sig_loc Signature buffer
		\param sig_len Signature buffer size, must be the size returned by size().
		\param key Public RSA key
		\param hash Hash type
		\return True if signature matches, false if signature size or value do not match.
	*/
	static bool verify(const void*, int, const void*, int, const RsaPublicKey&, PkcsSignature::HashType);
	static bool verify(const void* loc, int len, const std::vector<char>& sig, const RsaPublicKey& key, PkcsSignature::HashType hash)
	{
		return verify(loc, len, sig.data(), sig.size(), key, hash);
	}
};

/** @} */
/** @} */

}		// namespace

#endif
