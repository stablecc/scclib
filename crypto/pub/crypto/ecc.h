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
#ifndef _SCC_CRYPTO_ECC_H
#define _SCC_CRYPTO_ECC_H

#include <memory>
#include <vector>
#include <crypto/bignum.h>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_ecc Elliptic curve cryptography
	@{

	Elliptic curve ciphers for use in PKI cryptography & TLS encryption.

	see: https://www.secg.org/

	For use in TLS key exchange and X.509 certificate signing, e.g. https://tools.ietf.org/html/rfc4492
*/

/** Elliptic curve cryptography.
	\file ecc.h
*/

struct EccGfpBase;
struct EccGfpPointBase;
class EccGfpPoint;

/** Elliptic curve cryptography over Galois prime field GF(p) curve.

	Uses standard fields only, does not allow custom parameters.

	See https://www.secg.org/ for details on ec cryptography.

	For X25519 and X448 (montgomery curves), see: https://tools.ietf.org/html/rfc7748

	For security considerations, see:
	https://www.websecurity.digicert.com/content/dam/websitesecurity/digitalassets/desktop/pdfs/whitepaper/Elliptic_Curve_Cryptography_ECC_WP_en_us.pdf

	Security level means the equivalent security strength to a symmetric key with the stated bit size. Use of algorithms with security below 128 bits is not
	recommended.
*/
class EccGfp
{
	friend class EccGfpPoint;

	std::shared_ptr<EccGfpBase> m_ctx;

public:

	/** Standard field type. */
	enum class Type
	{
		std_p192r1,			///< standard curve secp192r1 (96 bit security level)
		std_p224r1,			///< standard curve secp224r1 (112 bit security level)
		std_p256r1,			///< standard curve secp256r1 (128 bit security level)
		std_p384r1,			///< standard curve secp384r1 (192 bit security level)
		std_p521r1,			///< standard curve secp521r1 (256 bit security level)
		std_p256sm2,		///< standard curve secp256sm2
		//curve_25519,		///< X25519 algorithm (128 bit security level)
		//curve_448,		///< X448 algorithm (224 bit security level)
	};

	EccGfp(Type type = Type::std_p256r1);
	virtual ~EccGfp();

	/** Reset the curve to a new standard type. */
	void reset(Type type);

	/** Verify the curve. Failure means the curve parameters are invalid or insecure. */
	static bool valid(const EccGfp&);
	bool valid() const
	{
		return valid(*this);
	}
	
	/** Elliptic curve ordinal bit width. */
	int bit_width() const;

	/** Generate a private and public key pair for this curve.
		\param priv_key Private key on the elliptic curve
		\param pub_key Public key point corresponding to the private key
	*/
	void generate_key_pair(Bignum& priv_key, EccGfpPoint& pub_key)
	{
		private_key(priv_key);
		public_key(priv_key, pub_key);
	}

	/** Validate a key pair. Both must be a key pair on the public key's elliptical curve.

		\param priv_key Private key
		\param pub_key Public key

		Public key must be valid on the elliptic curve.
	*/
	static bool validate_key_pair(const Bignum&, const EccGfpPoint&);

	/** Generate a private key.
		\param priv_key Private key on the elliptic curve
	*/
	void private_key(Bignum&);

	/** Generate a public key corresponding to a private key.
		\param priv_key Private key on the elliptic curve.
		\param pub_key Public key point corresponding to the private key
	*/
	void public_key(const Bignum&, EccGfpPoint&);

	/** Generate a public key from the private key.
	*/
	void generate_public_key(const Bignum&, EccGfpPoint&);

	/** Sign a message using ECDSA.

		A curve, temporary private and public key, and a regular private key should be provided.

		The signature output is two coordinate points on the elliptic curve.

		Generating a signature using ecdsa_secp256r1_sha256 (for example), is a two step process. First
		generate a hash of the data using sha256, then sign_ecdsa with the hash value.

		\param loc Signing data buffer.
		\param len Signing data size. Must be > 0.
		\param curve Elliptical curve.
		\param reg_private Regular private key derived from the curve.
		\param temp_private Temporary (ephemeral) private key derived from the curve. Must be different than the regular key.
		\param sig_x Signature X coordinate output.
		\param sig_y Signature Y coordinate output.
	*/
	static void sign_ecdsa(const void*, int, const EccGfp&, const scc::crypto::Bignum&, scc::crypto::Bignum&, scc::crypto::Bignum&, scc::crypto::Bignum&);

	static void sign_ecdsa(const void* loc, int len, const EccGfp::Type& t, const scc::crypto::Bignum& rk, scc::crypto::Bignum& tk, scc::crypto::Bignum& x, scc::crypto::Bignum& y)
	{
		EccGfp curve(t);
		sign_ecdsa(loc, len, curve, rk, tk, x, y);
	}

	/** Verify a message using the ECDSA.

		\param loc Signing data buffer.
		\param len Signing data size. Must be > 0.
		\param reg_public Regular public key corresponding to the regular private key used to sign.
		\param sig_x Signature X coordinate.
		\param sig_y Signature Y coordinate.

		Uses the curve associated with the point.
	*/
	static bool verify_ecdsa(const void*, int, const EccGfpPoint&, const scc::crypto::Bignum&, const scc::crypto::Bignum&);

	/** Calculate a shared secret using the Diffie-Hellman scheme.

		\param my_private My private key.
		\param other_public Other public key.
		\param shared_secret Shared private key.

		Given a private key and other parties public key on a curve, generate a shared secreet key (x coordinate on the curve), which will be the same for
		both parties.
	*/
	static void dh_shared_secret(const scc::crypto::Bignum&, const EccGfpPoint&, scc::crypto::Bignum&);
};

class EccGfpPoint
{
	friend class EccGfp;
	friend class EcdsaSignature;

	std::shared_ptr<EccGfpPointBase> m_ctx;

public:
	/** Construct an invalid point. */
	EccGfpPoint() {}
	/** Construct a point on the elliptic curve. Point is set to infinity. */
	EccGfpPoint(const EccGfp& curve)
	{
		reset(curve);
	}
	EccGfpPoint(const EccGfp::Type& curve_type)
	{
		EccGfp curve(curve_type);
		reset(curve);
	}

	virtual ~EccGfpPoint();

	/** Reset the point to invalid. */
	void reset()
	{
		m_ctx.reset();
	}
	/** Reset the point to fall on the elliptic curve. Point will be initially set to infinity. */
	void reset(const EccGfp&);

	/** Set point.
		\param x X coordinate
		\param y Y coordinate

		Throws an exception if the point is invalid or coordinates are not on the curve.
	*/
	void set(const scc::crypto::Bignum&, const scc::crypto::Bignum&);

	/** Set point to a new curve.
	
		Throws an exception if the coordinates are not on the curve.
	*/
	void set(const scc::crypto::Bignum& x, const scc::crypto::Bignum& y, EccGfp& curve)
	{
		reset(curve);
		set(x, y);
	}
	/** Set the point from a data string.
		
		Throws an exception if the point or input data is invalid.
	*/
	void set(const void*, int);
	void set(const std::vector<char>& v)
	{
		set(v.data(), v.size());
	}
	
	/** Set the point from a data string on a new curve.
		
		Throws an exception if the point or input data is invalid.
	*/
	void set(const void* loc, int len, EccGfp& curve)
	{
		reset(curve);
		set(loc, len);
	}
	void set(const std::vector<char>& v, EccGfp& curve)
	{
		set(v.data(), v.size(), curve);
	}

	/** Set point to infinity.
	
		Throws exception if point is invalid.
	*/
	void set();

	/** Get the coordinates.
		\param x X coordinate
		\param y Y coordinate

		Throws an exception if the point is invalid.
	*/
	void get(scc::crypto::Bignum&, scc::crypto::Bignum&) const;
	/** Get the coordinates to a data string. String will be reset to the coordinate data length.
		
		Throws an exception if the point is invalid.
	*/
	void get(std::vector<char>&) const;

	/** Test the point for infinity on a curve. Failure means the point is invalid, or is not the infinity point. */
	static bool infinite(const EccGfpPoint&, const EccGfp&);
	bool infinite(const EccGfp& curve) const
	{
		return infinite(*this, curve);
	}

	/** Test the point for infinity. Failure means the point is invalid, or is not the infinity point. */
	static bool infinite(const EccGfpPoint&);
	bool infinite() const
	{
		return infinite(*this);
	}

	/** Verify the point on a curve. Failure means the point is invalid, infinite or does not fall on the curve.
	*/
	static bool valid(const EccGfpPoint&, const EccGfp&);
	bool valid(const EccGfp& curve) const
	{
		return valid(*this, curve);
	}
	
	/** Verify the point. Failure means the point is invalid, infinite or does not fall on the curve.
	*/
	static bool valid(const EccGfpPoint&);
	bool valid() const
	{
		return valid(*this);
	}

	/** Test for equality with a second point. */
	bool equal(const EccGfpPoint&) const;

	bool operator==(const EccGfpPoint& b) const
	{
		return equal(b);
	}
	bool operator!=(const EccGfpPoint& b) const
	{
		return !equal(b);
	}
};

/** @} */
/** @} */

}		// namespace

#endif