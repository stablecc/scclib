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
#include <crypto/rsa.h>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <crypto/bignum.h>
#include <crypto/random.h>
#include <crypto/hash.h>
#include <crypto/secvec.h>
#include <encode/hex.h>
#include <ippcp.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

using namespace scc::crypto;

static void throw_err(const std::string& msg)
{
	throw std::runtime_error(msg);
}

static void throw_ipp_err(const std::string& msg, IppStatus status)
{
	if (status != ippStsNoErr)
	{
		std::stringstream s;
		s << msg << ": " << ippcpGetStatusString(status);
		throw_err(s.str());
	}
}

RsaPublicKey::RsaPublicKey() {}

RsaPublicKey::~RsaPublicKey()
{
	m_n.clear();
	m_e.clear();
}

void RsaPublicKey::clear()
{
	m_n.clear();
	m_e.clear();
}

std::string RsaPublicKey::dump() const
{
	std::stringstream s;
	
	s << "modulus (" << m_n.width() << " width): " << std::hex << m_n << std::dec << std::endl;
	s << "public exponent (" << m_e.width() << " width): " << std::hex << m_e << std::dec;

	return s.str();
}

static std::string emit_hex(const Bignum& bn, unsigned max)
{
	std::vector<char> v(bn.len(), '\0');

	bn.get((char*)v.data(), v.size());

	return scc::encode::Hex::bin_to_hexstr(v.data(), v.size(), ":", max);
}

std::string RsaPublicKey::str(unsigned max_bytes) const
{
	std::stringstream s;
	s << "modulus n width: " << m_n.width() << " val: " << emit_hex(m_n, max_bytes) << std::endl;
	s << "public exponent e width: " << m_e.width() << " val: " << emit_hex(m_e, max_bytes);
	return s.str();
}

int RsaPublicKey::width() const
{
	if (m_n == 0)		return 0;

	return m_n.width();
}

RsaPrivateKey::RsaPrivateKey() {}

RsaPrivateKey::~RsaPrivateKey()
{
	m_d.clear();
	m_p.clear();
	m_q.clear();
	m_ep.clear();
	m_eq.clear();
	m_qinv.clear();
}

void RsaPrivateKey::clear()
{
	RsaPublicKey::clear();
	m_d.clear();
	m_p.clear();
	m_q.clear();
	m_ep.clear();
	m_eq.clear();
	m_qinv.clear();
}

std::string RsaPrivateKey::dump() const
{
	std::stringstream s;
	s << "modulus (" << m_n.width() << " width): " << std::hex << m_n << std::dec << std::endl;
	s << "public exponent (" << m_e.width() << " width): " << std::hex << m_e << std::dec << std::endl;
	s << "private exponent (" << m_d.width() << " width): " << std::hex << m_d << std::dec << std::endl;
	s << "prime 1 (" << m_p.width() << " width): " << std::hex << m_p << std::dec << std::endl;
	s << "prime 2 (" << m_q.width() << " width): " << std::hex << m_q << std::dec << std::endl;
	s << "exponent 1 (" << m_ep.width() << " width): " << std::hex << m_ep << std::dec << std::endl;
	s << "exponent 2 (" << m_eq.width() << " width): " << std::hex << m_eq << std::dec << std::endl;
	s << "coefficient (" << m_qinv.width() << " width): " << std::hex << m_qinv << std::dec;
	return s.str();
}

std::string RsaPrivateKey::str(unsigned max_bytes) const
{
	std::stringstream s;
	s << "modulus n width: " << m_n.width() << " val: " << emit_hex(m_n, max_bytes) << std::endl;
	s << "public exponent e width: " << m_e.width() << " val: " << emit_hex(m_e, max_bytes) << std::endl;
	s << "private exponent d width: " << m_d.width() << " val: " << emit_hex(m_d, max_bytes) << std::endl;
	s << "prime 1 p width: " << m_p.width() << " val: " << emit_hex(m_p, max_bytes) << std::endl;
	s << "prime 2 q width: " << m_q.width()<< " val: " << emit_hex(m_q, max_bytes) << std::endl;
	s << "exponent 1 ep width: " << m_ep.width() << " val: " << emit_hex(m_ep, max_bytes) << std::endl;
	s << "exponent 2 eq width: " << m_eq.width() << " val: " << emit_hex(m_eq, max_bytes) << std::endl;
	s << "coefficient qinv width: " << m_qinv.width() << " val: " << emit_hex(m_qinv, max_bytes);
	return s.str();
}

void RsaPrivateKey::generate(int width)
{
	if (width <= 0 || width%2 == 1)
	{
		throw_err("generate(): invalid width");
	}

	clear();
	
	int sz;

	IppStatus s = ippsRSA_GetSizePrivateKeyType2(width/2, width-width/2, &sz);	// the two prime sizes add up to the width
	throw_ipp_err("ippsRSA_GetSizePrivateKeyType2", s);
	SecVecChar statebuf(sz);
	IppsRSAPrivateKeyState* state = (IppsRSAPrivateKeyState*)statebuf.data();

	s = ippsRSA_InitPrivateKeyType2(width/2, width-width/2, state, sz);
	throw_ipp_err("ippsRSA_InitPrivateKeyType2", s);

	// initialize the millar-rabin prime engine
	s = ippsPrimeGetSize(width, &sz);
	throw_ipp_err("ippsPrimeGetSize", s);
	SecVecChar psbuf(sz);
	IppsPrimeState* primestate = (IppsPrimeState*)psbuf.data();

	s = ippsPrimeInit(width, primestate);
	throw_ipp_err("ippsPrimeInit", s);

	s = ippsRSA_GetBufferSizePrivateKey(&sz, state);
	throw_ipp_err("RSA_GetBufferSizePrivateKey", s);
	SecVecChar scratch(sz);

	m_e = 65537;	// canonical initial value
	m_n.set_width(width);
	m_d.set_width(width);

	do
	{
		scc::crypto::RandomEngineLocker rel;		// lock up the random engine
		// choose 5 trials, which should be sufficient for 512 bit size (1/2 of the recommended minimum)
		s = ippsRSA_GenerateKeys(
			(const IppsBigNumState*)m_e.bn(),
			(IppsBigNumState*)m_n.bn(),
			(IppsBigNumState*)m_e.bn(),
			(IppsBigNumState*)m_d.bn(),
			state, (Ipp8u*)scratch.data(), 5, primestate, ippsPRNGen, scc::crypto::RandomEngine::ctx());
	}
	while (s == ippStsInsufficientEntropy);
	throw_ipp_err("ippsRSA_GenerateKeys", s);

	m_p.set_width(width/2);
	m_q.set_width(width/2);
	m_ep.set_width(width/2);
	m_eq.set_width(width/2);
	m_qinv.set_width(width/2);

	s = ippsRSA_GetPrivateKeyType2(
		(IppsBigNumState*)m_p.bn(),
		(IppsBigNumState*)m_q.bn(),
		(IppsBigNumState*)m_ep.bn(),
		(IppsBigNumState*)m_eq.bn(),
		(IppsBigNumState*)m_qinv.bn(),
		state);
	throw_ipp_err("ippsRSA_GetPrivateKeyType2", s);
}

bool RsaPrivateKey::validate(const RsaPublicKey& pubk) const
{
	int sz, width = m_n.width(), ewidth = m_e.width(), pwidth = m_p.width(), qwidth = m_q.width();

	if (m_n == 0)		// initialized or cleared object
	{
		return false;
	}
	if (width != pubk.m_n.width() || ewidth != pubk.m_e.width())
	{
		return false;
	}

	// set up the private key
	IppStatus s = ippsRSA_GetSizePrivateKeyType2(pwidth, qwidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePrivateKeyType2", s);
	SecVecChar privbuf(sz);
	IppsRSAPrivateKeyState* privstate = (IppsRSAPrivateKeyState*)privbuf.data();

	s = ippsRSA_InitPrivateKeyType2(pwidth, qwidth, privstate, sz);
	throw_ipp_err("ippsRSA_InitPrivateKeyType2", s);

	s = ippsRSA_SetPrivateKeyType2(
		(const IppsBigNumState*)m_p.const_bn(),
		(const IppsBigNumState*)m_q.const_bn(),
		(const IppsBigNumState*)m_ep.const_bn(),
		(const IppsBigNumState*)m_eq.const_bn(),
		(const IppsBigNumState*)m_qinv.const_bn(),
		privstate);
	throw_ipp_err("ippsRSA_SetPrivateKeyType2", s);

	// set up the public key
	s = ippsRSA_GetSizePublicKey(width, ewidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePublicKey", s);
	SecVecChar pubbuf(sz);
	IppsRSAPublicKeyState* pubstate = (IppsRSAPublicKeyState*)pubbuf.data();

	s = ippsRSA_InitPublicKey(width, ewidth, pubstate, sz);
	throw_ipp_err("ippsRSA_InitPublicKey", s);

	s = ippsRSA_SetPublicKey(
		(const IppsBigNumState*)pubk.m_n.const_bn(),
		(const IppsBigNumState*)pubk.m_e.const_bn(),
		pubstate);
	throw_ipp_err("ippsRSA_SetPublicKey", s);

	// set up the prime buffer
	s = ippsPrimeGetSize(width, &sz);
	throw_ipp_err("ippsPrimeGetSize", s);
	SecVecChar psbuf(sz);
	IppsPrimeState* primestate = (IppsPrimeState*)psbuf.data();

	s = ippsPrimeInit(width, primestate);
	throw_ipp_err("ippsPrimeInit", s);

	s = ippsRSA_GetBufferSizePrivateKey(&sz, privstate);
	throw_ipp_err("RSA_GetBufferSizePrivateKey", s);
	SecVecChar scratch(sz);

	int r;
	s = ippsRSA_ValidateKeys(&r, pubstate, privstate, nullptr, (Ipp8u*)scratch.data(),
		5, primestate, ippsPRNGen, scc::crypto::RandomEngine::ctx());
	throw_ipp_err("ippsRSA_ValidateKeys", s);

	return r == IS_VALID_KEY;
}

struct scc::crypto::RsaOaepEncryptCtx
{
	SecVecChar pubbuf;
	IppsRSAPublicKeyState* pubstate;
	SecVecChar scratchbuf;
	Ipp8u* scratch;
	const IppsHashMethod* hashalg;
	int hashlen;
	int maxmsglen;
	int cipherlen;
	SecVecChar seedbuf;
	Ipp8u* seed;
};

RsaOaepEncrypt::RsaOaepEncrypt(RsaPublicKey& key, Hash::Algorithm hashid) : m_ctx(new RsaOaepEncryptCtx)
{
	int sz, width = key.m_n.width(), ewidth = key.m_e.width();
	
	IppStatus s = ippsRSA_GetSizePublicKey(width, ewidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePublicKey", s);
	m_ctx->pubbuf.resize(sz);
	m_ctx->pubstate = (IppsRSAPublicKeyState*)m_ctx->pubbuf.data();

	s = ippsRSA_InitPublicKey(width, ewidth, m_ctx->pubstate, sz);
	throw_ipp_err("ippsRSA_InitPublicKey", s);

	s = ippsRSA_SetPublicKey(
		(const IppsBigNumState*)key.m_n.bn(),
		(const IppsBigNumState*)key.m_e.bn(),
		m_ctx->pubstate);
	throw_ipp_err("ippsRSA_SetPublicKey", s);

	s = ippsRSA_GetBufferSizePublicKey(&sz, m_ctx->pubstate);
	throw_ipp_err("ippsRSA_GetBufferSizePublicKey", s);
	m_ctx->scratchbuf.resize(sz);
	m_ctx->scratch = (Ipp8u*)m_ctx->scratchbuf.data();

	using scc::crypto::Hash;

	switch (hashid)
	{
		//case Hash::md5_type: m_ctx->hashalg = ippsHashMethod_MD5(); break; // deprecated by IPP: insecure
		case Hash::sha1_type: m_ctx->hashalg = ippsHashMethod_SHA1(); break;
		case Hash::sha224_type: m_ctx->hashalg = ippsHashMethod_SHA224(); break;
		case Hash::sha256_type: m_ctx->hashalg = ippsHashMethod_SHA256(); break;
		case Hash::sha384_type: m_ctx->hashalg = ippsHashMethod_SHA384(); break;
		case Hash::sha512_type: m_ctx->hashalg = ippsHashMethod_SHA512(); break;
		case Hash::sha512_224_type: m_ctx->hashalg = ippsHashMethod_SHA512_224(); break;
		case Hash::sha512_256_type: m_ctx->hashalg = ippsHashMethod_SHA512_256(); break;
		case Hash::sm3_type: m_ctx->hashalg = ippsHashMethod_SM3(); break;
		default: throw std::runtime_error("unknown hash type"); break;
	}

	m_ctx->hashlen = Hash::alg_size(hashid);
	m_ctx->maxmsglen = key.width_bytes() - 2 - 2*m_ctx->hashlen;
	m_ctx->cipherlen = key.width_bytes();

	// create a scratch buffer for random seed
	m_ctx->seedbuf.resize(m_ctx->hashlen);
	m_ctx->seed = (Ipp8u*)m_ctx->seedbuf.data();
}

RsaOaepEncrypt::~RsaOaepEncrypt()
{
}

int RsaOaepEncrypt::max_msg_size() const
{
	return m_ctx->maxmsglen;
}

int RsaOaepEncrypt::cipher_size() const
{
	return m_ctx->cipherlen;
}

void RsaOaepEncrypt::encrypt(const void* msg_loc, int msg_len, void* cipher_loc, int cipher_len, const void* label_loc, int label_len)
{
	if (msg_len < 0 || msg_len > max_msg_size())
	{
		throw_err("oaep encrypt plaintext parameter error");
	}
	if (cipher_len != cipher_size())
	{
		throw_err("oaep encrypt ciphertext parameter error");
	}
	if (label_len < 0 || (label_loc == nullptr && label_len > 0))
	{
		throw_err("oaep encrypt label parameter error");
	}

	// set up the random seed
	RandomEngine::rand_bytes(m_ctx->seed, m_ctx->hashlen);

	explicit_bzero(cipher_loc, cipher_len);

	IppStatus s = ippsRSAEncrypt_OAEP_rmf(
		(const Ipp8u*)msg_loc, msg_len,
		(const Ipp8u*)label_loc, label_len,
		(const Ipp8u*)m_ctx->seed,
		(Ipp8u*)cipher_loc,
		m_ctx->pubstate,
		m_ctx->hashalg,
		m_ctx->scratch);
	throw_ipp_err("ippsRSAEncrypt_OAEP_rmf", s);
}

struct scc::crypto::RsaOaepDecryptCtx
{
	SecVecChar privbuf;
	IppsRSAPrivateKeyState* privstate;
	SecVecChar scratchbuf;
	Ipp8u* scratch;
	const IppsHashMethod* hashalg;
	int hashlen;
	int maxmsglen;
	int cipherlen;
};

RsaOaepDecrypt::RsaOaepDecrypt(RsaPrivateKey& key, Hash::Algorithm hashid) : m_ctx(new RsaOaepDecryptCtx)
{
	int sz, pwidth = key.m_p.width(), qwidth = key.m_q.width();

	IppStatus s = ippsRSA_GetSizePrivateKeyType2(pwidth, qwidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePrivateKeyType2", s);
	m_ctx->privbuf.resize(sz);
	m_ctx->privstate = (IppsRSAPrivateKeyState*)m_ctx->privbuf.data();

	s = ippsRSA_InitPrivateKeyType2(pwidth, qwidth, m_ctx->privstate, sz);
	throw_ipp_err("ippsRSA_InitPrivateKeyType2", s);

	s = ippsRSA_SetPrivateKeyType2(
		(const IppsBigNumState*)key.m_p.bn(),
		(const IppsBigNumState*)key.m_q.bn(),
		(const IppsBigNumState*)key.m_ep.bn(),
		(const IppsBigNumState*)key.m_eq.bn(),
		(const IppsBigNumState*)key.m_qinv.bn(),
		m_ctx->privstate);
	throw_ipp_err("ippsRSA_SetPrivateKeyType2", s);

	s = ippsRSA_GetBufferSizePrivateKey(&sz, m_ctx->privstate);
	throw_ipp_err("ippsRSA_GetBufferSizePrivateKey", s);
	m_ctx->scratchbuf.resize(sz);
	m_ctx->scratch = (Ipp8u*)m_ctx->scratchbuf.data();

	using scc::crypto::Hash;

	switch (hashid)
	{
		//case Hash::md5_type: m_ctx->hashalg = ippsHashMethod_MD5(); break; // deprecated by IPP: insecure
		case Hash::sha1_type: m_ctx->hashalg = ippsHashMethod_SHA1(); break;
		case Hash::sha224_type: m_ctx->hashalg = ippsHashMethod_SHA224(); break;
		case Hash::sha256_type: m_ctx->hashalg = ippsHashMethod_SHA256(); break;
		case Hash::sha384_type: m_ctx->hashalg = ippsHashMethod_SHA384(); break;
		case Hash::sha512_type: m_ctx->hashalg = ippsHashMethod_SHA512(); break;
		case Hash::sha512_224_type: m_ctx->hashalg = ippsHashMethod_SHA512_224(); break;
		case Hash::sha512_256_type: m_ctx->hashalg = ippsHashMethod_SHA512_256(); break;
		case Hash::sm3_type: m_ctx->hashalg = ippsHashMethod_SM3(); break;
		default: throw std::runtime_error("unknown hash type"); break;
	}

	m_ctx->hashlen = Hash::alg_size(hashid);
	m_ctx->maxmsglen = key.width_bytes() - 2 - 2*m_ctx->hashlen;
	m_ctx->cipherlen = key.width_bytes();
}

RsaOaepDecrypt::~RsaOaepDecrypt()
{
}

int RsaOaepDecrypt::max_msg_size() const
{
	return m_ctx->maxmsglen;
}

int RsaOaepDecrypt::cipher_size() const
{
	return m_ctx->cipherlen;
}

int RsaOaepDecrypt::decrypt(void* msg_loc, int msg_len, const void* cipher_loc, int cipher_len, const void* label_loc, int label_len)
{
	if (msg_len != max_msg_size())
	{
		throw_err("oaep decrypt plaintext parameter error");
	}
	if (cipher_len != cipher_size())
	{
		throw_err("oaep decrypt ciphertext parameter error");
	}
	if (label_len < 0 || (label_loc == nullptr && label_len > 0))
	{
		throw_err("oaep decrypt label parameter error");
	}

	explicit_bzero(msg_loc, msg_len);

	int decsz;

	IppStatus s = ippsRSADecrypt_OAEP_rmf(
		(const Ipp8u*)cipher_loc,
		(const Ipp8u*)label_loc, label_len,
		(Ipp8u*)msg_loc, &decsz,
		m_ctx->privstate,
		m_ctx->hashalg,
		m_ctx->scratch);

	return s == ippStsNoErr ? decsz : -1;
}

void PkcsSignature::sign(const void* loc, int len, void* sig_loc, int sig_len, const RsaPrivateKey& key, PkcsSignature::HashType hash)
{
	if (!loc || len <= 0)			throw std::runtime_error("PkcsSignature::sign() parameter error");

	const IppsHashMethod* alg;

	switch (hash)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	case HashType::md5:			alg = ippsHashMethod_MD5(); break;
#pragma GCC diagnostic pop
	case HashType::sha1:		alg = ippsHashMethod_SHA1(); break;
	case HashType::sha224:		alg = ippsHashMethod_SHA224(); break;
	case HashType::sha256:		alg = ippsHashMethod_SHA256(); break;
	case HashType::sha384:		alg = ippsHashMethod_SHA384(); break;
	case HashType::sha512:		alg = ippsHashMethod_SHA512(); break;
	}

	int sz, pubsz, pwidth = key.m_p.width(), qwidth = key.m_q.width(), nwidth = key.m_n.width(), ewidth = key.m_e.width();

	if (sig_len != key.width_bytes())
	{
		throw std::runtime_error("pkcs signature sign sig_len error");
	}

	explicit_bzero(sig_loc, sig_len);

	IppStatus s = ippsRSA_GetSizePrivateKeyType2(pwidth, qwidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePrivateKeyType2", s);
	
	SecVecChar privbuf(sz);
	IppsRSAPrivateKeyState* privstate = (IppsRSAPrivateKeyState*)privbuf.data();
	
	s = ippsRSA_InitPrivateKeyType2(pwidth, qwidth, privstate, sz);
	throw_ipp_err("ippsRSA_InitPrivateKeyType2", s);

	s = ippsRSA_SetPrivateKeyType2(
		(const IppsBigNumState*)key.m_p.const_bn(),
		(const IppsBigNumState*)key.m_q.const_bn(),
		(const IppsBigNumState*)key.m_ep.const_bn(),
		(const IppsBigNumState*)key.m_eq.const_bn(),
		(const IppsBigNumState*)key.m_qinv.const_bn(),
		privstate);
	throw_ipp_err("ippsRSA_SetPrivateKeyType2", s);

	s = ippsRSA_GetSizePublicKey(nwidth, ewidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePublicKey", s);

	SecVecChar pubbuf(sz);
	IppsRSAPublicKeyState* pubstate = (IppsRSAPublicKeyState*)pubbuf.data();

	s = ippsRSA_InitPublicKey(nwidth, ewidth, pubstate, sz);
	throw_ipp_err("ippsRSA_InitPublicKey", s);

	s = ippsRSA_SetPublicKey(
		(const IppsBigNumState*)key.m_n.const_bn(),
		(const IppsBigNumState*)key.m_e.const_bn(),
		pubstate);
	throw_ipp_err("ippsRSA_SetPublicKey", s);

	s = ippsRSA_GetBufferSizePublicKey(&pubsz, pubstate);
	throw_ipp_err("ippsRSA_GetBufferSizePublicKey", s);

	s = ippsRSA_GetBufferSizePrivateKey(&sz, privstate);
	throw_ipp_err("ippsRSA_GetBufferSizePrivateKey", s);
	
	SecVecChar scratchbuf(sz + pubsz);

	s = ippsRSASign_PKCS1v15_rmf((const Ipp8u*)loc, len, (Ipp8u*)sig_loc, privstate, pubstate, alg, (Ipp8u*)scratchbuf.data());
	throw_ipp_err("ippsRSASign_PKCS1v15_rmf", s);
}

bool PkcsSignature::verify(const void* loc, int len, const void* sig_loc, int sig_len, const RsaPublicKey& key, PkcsSignature::HashType hash)
{
	if (!loc || len <= 0)			throw std::runtime_error("PkcsSignature::sign() parameter error");

	const IppsHashMethod* alg;

	switch (hash)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	case HashType::md5:			alg = ippsHashMethod_MD5(); break;
#pragma GCC diagnostic pop
	case HashType::sha1:		alg = ippsHashMethod_SHA1(); break;
	case HashType::sha224:		alg = ippsHashMethod_SHA224(); break;
	case HashType::sha256:		alg = ippsHashMethod_SHA256(); break;
	case HashType::sha384:		alg = ippsHashMethod_SHA384(); break;
	case HashType::sha512:		alg = ippsHashMethod_SHA512(); break;
	}

	int sz, nwidth = key.m_n.width(), ewidth = key.m_e.width();

	if (sig_len != key.width_bytes())
	{
		return false;
	}

	IppStatus s = ippsRSA_GetSizePublicKey(nwidth, ewidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePublicKey", s);

	SecVecChar pubbuf(sz);
	IppsRSAPublicKeyState* pubstate = (IppsRSAPublicKeyState*)pubbuf.data();

	s = ippsRSA_InitPublicKey(nwidth, ewidth, pubstate, sz);
	throw_ipp_err("ippsRSA_InitPublicKey", s);

	s = ippsRSA_SetPublicKey(
		(const IppsBigNumState*)key.m_n.const_bn(),
		(const IppsBigNumState*)key.m_e.const_bn(),
		pubstate);
	throw_ipp_err("ippsRSA_SetPublicKey", s);

	s = ippsRSA_GetBufferSizePublicKey(&sz, pubstate);
	throw_ipp_err("ippsRSA_GetBufferSizePublicKey", s);
	
	SecVecChar scratchbuf(sz);

	int valid;

	s = ippsRSAVerify_PKCS1v15_rmf((const Ipp8u*)loc, len, (const Ipp8u*)sig_loc, &valid, pubstate, alg, (Ipp8u*)scratchbuf.data());

	return s == ippStsNoErr && valid;
}

void PssSignature::sign(const void* loc, int len, void* sig_loc, int sig_len, const RsaPrivateKey& key, PssSignature::HashType hash, int salt_len)
{
	if (!loc || len <= 0)			throw std::runtime_error("PssSignature::sign() parameter error");

	const IppsHashMethod* alg;

	switch (hash)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	case HashType::md5:			alg = ippsHashMethod_MD5(); break;
#pragma GCC diagnostic pop
	case HashType::sha1:		alg = ippsHashMethod_SHA1(); break;
	case HashType::sha224:		alg = ippsHashMethod_SHA224(); break;
	case HashType::sha256:		alg = ippsHashMethod_SHA256(); break;
	case HashType::sha384:		alg = ippsHashMethod_SHA384(); break;
	case HashType::sha512:		alg = ippsHashMethod_SHA512(); break;
	}

	int sz, pubsz, pwidth = key.m_p.width(), qwidth = key.m_q.width(), nwidth = key.m_n.width(), ewidth = key.m_e.width();

	if (salt_len < 0)
	{
		throw std::runtime_error("pkcs signature invalid salt parameter");
	}

	if (sig_len != key.width_bytes())
	{
		throw std::runtime_error("pkcs signature sign sig_len error");
	}

	explicit_bzero(sig_loc, sig_len);

	IppStatus s = ippsRSA_GetSizePrivateKeyType2(pwidth, qwidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePrivateKeyType2", s);
	
	SecVecChar privbuf(sz);
	IppsRSAPrivateKeyState* privstate = (IppsRSAPrivateKeyState*)privbuf.data();
	
	s = ippsRSA_InitPrivateKeyType2(pwidth, qwidth, privstate, sz);
	throw_ipp_err("ippsRSA_InitPrivateKeyType2", s);

	s = ippsRSA_SetPrivateKeyType2(
		(const IppsBigNumState*)key.m_p.const_bn(),
		(const IppsBigNumState*)key.m_q.const_bn(),
		(const IppsBigNumState*)key.m_ep.const_bn(),
		(const IppsBigNumState*)key.m_eq.const_bn(),
		(const IppsBigNumState*)key.m_qinv.const_bn(),
		privstate);
	throw_ipp_err("ippsRSA_SetPrivateKeyType2", s);

	s = ippsRSA_GetSizePublicKey(nwidth, ewidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePublicKey", s);

	SecVecChar pubbuf(sz);
	IppsRSAPublicKeyState* pubstate = (IppsRSAPublicKeyState*)pubbuf.data();

	s = ippsRSA_InitPublicKey(nwidth, ewidth, pubstate, sz);
	throw_ipp_err("ippsRSA_InitPublicKey", s);

	s = ippsRSA_SetPublicKey(
		(const IppsBigNumState*)key.m_n.const_bn(),
		(const IppsBigNumState*)key.m_e.const_bn(),
		pubstate);
	throw_ipp_err("ippsRSA_SetPublicKey", s);

	s = ippsRSA_GetBufferSizePublicKey(&pubsz, pubstate);
	throw_ipp_err("ippsRSA_GetBufferSizePublicKey", s);

	s = ippsRSA_GetBufferSizePrivateKey(&sz, privstate);
	throw_ipp_err("ippsRSA_GetBufferSizePrivateKey", s);
	
	Ipp8u* salt_data = nullptr;
	SecVecChar salt(salt_len);
	if (salt_len)
	{
		RandomEngine::rand_bytes(salt.data(), salt_len);
		salt_data = (Ipp8u*)salt.data();
	}

	SecVecChar scratchbuf(sz + pubsz);

	// docs say that providing pubstate helps mitigate "fault attack" but results in higher computation times
	s =  ippsRSASign_PSS_rmf((const Ipp8u*)loc, len, salt_data, salt_len, (Ipp8u*)sig_loc, privstate, pubstate, alg, (Ipp8u*)scratchbuf.data());
	throw_ipp_err("ippsRSASign_PSS_rmf", s);
}

bool PssSignature::verify(const void* loc, int len, const void* sig_loc, int sig_len, const RsaPublicKey& key, PssSignature::HashType hash)
{
	if (!loc || len <= 0)			throw std::runtime_error("PssSignature::sign() parameter error");

	const IppsHashMethod* alg;

	switch (hash)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	case HashType::md5:			alg = ippsHashMethod_MD5(); break;
#pragma GCC diagnostic pop
	case HashType::sha1:		alg = ippsHashMethod_SHA1(); break;
	case HashType::sha224:		alg = ippsHashMethod_SHA224(); break;
	case HashType::sha256:		alg = ippsHashMethod_SHA256(); break;
	case HashType::sha384:		alg = ippsHashMethod_SHA384(); break;
	case HashType::sha512:		alg = ippsHashMethod_SHA512(); break;
	}

	int sz, nwidth = key.m_n.width(), ewidth = key.m_e.width();

	if (sig_len != key.width_bytes())
	{
		return false;
	}

	IppStatus s = ippsRSA_GetSizePublicKey(nwidth, ewidth, &sz);
	throw_ipp_err("ippsRSA_GetSizePublicKey", s);

	SecVecChar pubbuf(sz);
	IppsRSAPublicKeyState* pubstate = (IppsRSAPublicKeyState*)pubbuf.data();

	s = ippsRSA_InitPublicKey(nwidth, ewidth, pubstate, sz);
	throw_ipp_err("ippsRSA_InitPublicKey", s);

	s = ippsRSA_SetPublicKey(
		(const IppsBigNumState*)key.m_n.const_bn(),
		(const IppsBigNumState*)key.m_e.const_bn(),
		pubstate);
	throw_ipp_err("ippsRSA_SetPublicKey", s);

	s = ippsRSA_GetBufferSizePublicKey(&sz, pubstate);
	throw_ipp_err("ippsRSA_GetBufferSizePublicKey", s);
	
	SecVecChar scratchbuf(sz);

	int valid;

	s = ippsRSAVerify_PSS_rmf((const Ipp8u*)loc, len, (const Ipp8u*)sig_loc, &valid, pubstate, alg, (Ipp8u*)scratchbuf.data());

	return s == ippStsNoErr && valid;
}
