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
#include <crypto/ecc.h>
#include <system_error>
#include <cstring>
#include <memory>
#include <crypto/random.h>
#include <crypto/secvec.h>
#include <crypto/bignum.h>
#include <ippcp.h>

//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

using namespace scc::crypto;

static void throw_ipperr(const std::string& name, IppStatus status)
{
	if (status != ippStsNoErr)
	{
		throw std::runtime_error(name+": "+ippcpGetStatusString(status));
	}
}

struct scc::crypto::EccGfpBase
{
	int bit_width;
	SecVecChar gfctx;
	IppsGFpState* gfstate;
	SecVecChar ecctx;
	IppsGFpECState* ecstate;
	SecVecChar scratchbuf;
	Ipp8u* scratch;

	EccGfpBase(EccGfp::Type type) : gfstate(nullptr)
	{
		const IppsGFpMethod* method = nullptr;
		//Bignum prime;

		switch (type)
		{
			case EccGfp::Type::std_p192r1:
				bit_width = 192;
				method = ippsGFpMethod_p192r1();
				break;
			case EccGfp::Type::std_p224r1:
				bit_width = 224;
				method = ippsGFpMethod_p224r1();
				break;
			case EccGfp::Type::std_p256r1:
				bit_width = 256;
				method = ippsGFpMethod_p256r1();
				break;
			case EccGfp::Type::std_p384r1:
				bit_width = 384;
				method = ippsGFpMethod_p384r1();
				break;
			case EccGfp::Type::std_p521r1:
				bit_width = 521;
				method = ippsGFpMethod_p521r1();
				break;
			case EccGfp::Type::std_p256sm2:
				bit_width = 256;
				method = ippsGFpMethod_p256sm2();
				break;
/*			case EccGfp::Type::curve_25519:
				// 2^255 - 19
				prime = 1;
				for (int i = 0; i < 255; i++)		prime *= 2;
				prime -= 19;

				bit_width = prime.width();
//std::cerr << "******** curve_25519 width=" << prime.width() << " p=" << prime << std::endl;
//bit_width = 521;
//method = ippsGFpMethod_p521r1();		// fake it
				break;
			case EccGfp::Type::curve_448:
				// 2^448 - 2^224 - 1
				{
					prime = 1;
					for (int i = 0; i < 448; i++)		prime *= 2;

					Bignum p2(1);
					for (int i = 0; i < 224; i++)		p2 *= 2;

					prime -= p2;
					prime -= 1;

					bit_width = prime.width();
//std::cerr << "******** curve_448 width=" << prime.width() << " p=" << prime << std::endl;
//bit_width = 521;
//method = ippsGFpMethod_p521r1();		// fake it
				}
				break;*/
		}

		// galois field init

		int r, sz;
		r = ippsGFpGetSize(bit_width, &sz);
		throw_ipperr("ippsGFpGetSize", r);

		gfctx.resize(sz);
		gfstate = (IppsGFpState*)gfctx.data();

		if (method)
		{
			r = ippsGFpInitFixed(bit_width, method, gfstate);		// standard field does not require a prime to initialize
			throw_ipperr("ippsGFpInitFixed", r);
		}
/*		else
		{
			// use an "arbitrary" modulus curve
			r = ippsGFpInit(static_cast<const IppsBigNumState*>(prime.const_bn()), bit_width, ippsGFpMethod_pArb(), gfstate);
			throw_ipperr("ippsGFpInitFixed", r);
		}
*/
		// elliptical curve over galois field init

		r = ippsGFpECGetSize(gfstate, &sz);
		throw_ipperr("ippsGFpECGetSize", r);

		ecctx.resize(sz);
		ecstate = (IppsGFpECState*)ecctx.data();

		switch (type)
		{
			case EccGfp::Type::std_p192r1:
				r = ippsGFpECInitStd192r1(gfstate, ecstate);
				throw_ipperr("ippsGFpECInitStd192r1", r);
				break;
			case EccGfp::Type::std_p224r1:
				r = ippsGFpECInitStd224r1(gfstate, ecstate);
				throw_ipperr("ippsGFpECInitStd224r1", r);
				break;
			case EccGfp::Type::std_p256r1:
				r = ippsGFpECInitStd256r1(gfstate, ecstate);
				throw_ipperr("ippsGFpECInitStd256r1", r);
				break;
			case EccGfp::Type::std_p384r1:
				r = ippsGFpECInitStd384r1(gfstate, ecstate);
				throw_ipperr("ippsGFpECInitStd384r1", r);
				break;
			case EccGfp::Type::std_p521r1:
				r = ippsGFpECInitStd521r1(gfstate, ecstate);
				throw_ipperr("ippsGFpECInitStd521r1", r);
				break;
			case EccGfp::Type::std_p256sm2:
				r = ippsGFpECInitStdSM2(gfstate, ecstate);
				throw_ipperr("ippsGFpECInitStdSM2", r);
				break;
/*			case EccGfp::Type::curve_25519:
				{
					r = ippsGFpECInit(gfstate, a, b, ecstate);
				}
				break;
			case EccGfp::Type::curve_448:
				break;*/
		}

		r = ippsGFpECScratchBufferSize(6, ecstate, &sz);		// maximal sized scratch buffer
		throw_ipperr("ippsGFpECScratchBufferSize", r);

		scratchbuf.resize(sz);
		scratch = (Ipp8u*)scratchbuf.data();
	}

	virtual ~EccGfpBase()
	{
	}
};

struct scc::crypto::EccGfpPointBase
{
	std::shared_ptr<EccGfpBase> shared;		// makes the point independent of the ec object in which it was created
	IppsGFpECState* ecstate;
	SecVecChar ctxbuf;
	IppsGFpECPoint* state;

	EccGfpPointBase(const std::shared_ptr<EccGfpBase>& curve) : shared(curve), ecstate(shared->ecstate), state(nullptr)
	{
		int r, sz;
		r = ippsGFpECPointGetSize(ecstate, &sz);
		throw_ipperr("ippsGFpECPointGetSize", r);

		ctxbuf.resize(sz);
		state = (IppsECCPPointState*)ctxbuf.data();

		r = ippsGFpECPointInit(nullptr, nullptr, state, ecstate);		// initialize the point to "infinity"
		throw_ipperr("ippsGFpECPointGetSize", r);
	}

	virtual ~EccGfpPointBase()
	{
	}
};

EccGfpPoint::~EccGfpPoint()
{
}

bool EccGfpPoint::valid(const EccGfpPoint& p, const EccGfp& curve)
{
	if (!p.m_ctx)		return false;
	
	IppECResult ver;
	int r = ippsGFpECTstPoint(p.m_ctx->state, &ver, curve.m_ctx->ecstate);
	throw_ipperr("ippsGFpECTstPoint", r);
	return ver == ippECValid ? true : false;
}

bool EccGfpPoint::valid(const EccGfpPoint& p)
{
	if (!p.m_ctx)		return false;
	
	IppECResult ver;
	int r = ippsGFpECTstPoint(p.m_ctx->state, &ver, p.m_ctx->ecstate);
	throw_ipperr("ippsGFpECTstPoint", r);
	return ver == ippECValid ? true : false;
}

bool EccGfpPoint::infinite(const EccGfpPoint& p, const EccGfp& curve)
{
	if (!p.m_ctx)		return false;
	
	IppECResult ver;
	int r = ippsGFpECTstPoint(p.m_ctx->state, &ver, curve.m_ctx->ecstate);
	throw_ipperr("ippsGFpECTstPoint", r);
	return ver == ippECPointIsAtInfinite ? true : false;
}

bool EccGfpPoint::infinite(const EccGfpPoint& p)
{
	if (!p.m_ctx)		return false;
	
	IppECResult ver;
	int r = ippsGFpECTstPoint(p.m_ctx->state, &ver, p.m_ctx->ecstate);
	throw_ipperr("ippsGFpECTstPoint", r);
	return ver == ippECPointIsAtInfinite ? true : false;
}

bool EccGfpPoint::equal(const EccGfpPoint& other) const
{
	if (!m_ctx || !other.m_ctx)		return false;

	IppECResult ver;
	int r = ippsGFpECCmpPoint(m_ctx->state, other.m_ctx->state, &ver, m_ctx->ecstate);
	throw_ipperr("ippsGFpECCmpPoint", r);
	return ver == ippECPointIsEqual ? true : false;
}

void EccGfpPoint::reset(const EccGfp& curve)
{
	m_ctx.reset(new EccGfpPointBase(curve.m_ctx));
}

void EccGfpPoint::set()
{
	if (!m_ctx)		throw std::runtime_error("set() called on invalid point");

	int r = ippsGFpECSetPointAtInfinity(
		m_ctx->state, m_ctx->ecstate);
	throw_ipperr("ippsGFpECSetPointAtInfinity", r);
}

void EccGfpPoint::set(const scc::crypto::Bignum& x, const scc::crypto::Bignum& y)
{
	if (!m_ctx)		throw std::runtime_error("set() called on invalid point");

	int r = ippsGFpECSetPointRegular(
		static_cast<const IppsBigNumState*>(x.const_bn()),
		static_cast<const IppsBigNumState*>(y.const_bn()),
		m_ctx->state, m_ctx->ecstate);
	throw_ipperr("ippsGFpECSetPointRegular", r);
}

void EccGfpPoint::set(const void* loc, int len)
{
	if (!m_ctx)							throw std::runtime_error("set() binary called on invalid point");
	if (loc == nullptr || len < 0)		throw std::runtime_error("set() binary data error");

	int r = ippsGFpECSetPointOctString(
		static_cast<const Ipp8u*>(loc), len, m_ctx->state, m_ctx->ecstate);
	throw_ipperr("ippsGFpECSetPointOctString", r);
}

void EccGfpPoint::get(scc::crypto::Bignum& x, scc::crypto::Bignum& y) const
{
	if (!m_ctx)		throw std::runtime_error("get() called on invalid point");

	x.set_width(m_ctx->shared->bit_width);
	y.set_width(m_ctx->shared->bit_width);

	int r = ippsGFpECGetPointRegular(
		m_ctx->state,
		static_cast<IppsBigNumState*>(x.bn()),
		static_cast<IppsBigNumState*>(y.bn()),
		m_ctx->ecstate);
	throw_ipperr("ippsGFpECGetPointRegular", r);
}

void EccGfpPoint::get(std::vector<char>& v) const
{
	if (!m_ctx)		throw std::runtime_error("get() binary called on invalid point");

	// Docs say 2 * size of field, round up to next byte size (as the bignum does)
	v.resize(2* ((m_ctx->shared->bit_width+7)&~7) / 8);

	int r = ippsGFpECGetPointOctString(
		m_ctx->state,
		reinterpret_cast<Ipp8u*>(v.data()),
		v.size(), m_ctx->ecstate);
	throw_ipperr("ippsGFpECGetPointOctString", r);
}

EccGfp::EccGfp(Type type)
{
	reset(type);
}

EccGfp::~EccGfp()
{
}

bool EccGfp::valid(const EccGfp& curve)
{
	IppECResult ver;
	int r = ippsGFpECVerify(&ver, curve.m_ctx->ecstate, curve.m_ctx->scratch);
	throw_ipperr("ippsGFpECPrivateKey", r);
	return r == ippECValid ? true : false;
}

void EccGfp::reset(Type type)
{
	m_ctx.reset(new EccGfpBase(type));
}

int EccGfp::bit_width() const
{
	return m_ctx->bit_width;
}

void EccGfp::private_key(Bignum& priv_key)
{
	priv_key.set_width(bit_width());

	scc::crypto::RandomEngineLocker rel;
	int r = ippsGFpECPrivateKey(
		static_cast<IppsBigNumState*>(priv_key.bn()),
		m_ctx->ecstate, ippsPRNGen, scc::crypto::RandomEngine::ctx());
	throw_ipperr("ippsGFpECPrivateKey", r);
}

void EccGfp::public_key(const Bignum& priv_key, EccGfpPoint& pub_key)
{
	pub_key.reset(*this);

	int r = ippsGFpECPublicKey(
		static_cast<const IppsBigNumState*>(priv_key.const_bn()),
		pub_key.m_ctx->state, m_ctx->ecstate, m_ctx->scratch);
	throw_ipperr("ippsGFpECPublicKey", r);
}

bool EccGfp::validate_key_pair(const Bignum& priv_key, const EccGfpPoint& pub_key)
{
	if (!pub_key.valid())			return false;

	IppECResult ver;
	int r = ippsGFpECTstKeyPair(
		static_cast<const IppsBigNumState*>(priv_key.const_bn()),
		pub_key.m_ctx->state, &ver, pub_key.m_ctx->ecstate, pub_key.m_ctx->shared->scratch);		// verify with the embedded curve
	throw_ipperr("ippsGFpECTstKeyPair", r);
	return ver == ippECValid ? true : false;
}

void EccGfp::sign_ecdsa(const void* loc, int len, const EccGfp& curve,
	const Bignum& reg_private, Bignum& temp_private, Bignum& sig_x, Bignum& sig_y)
{
	if (!loc || len <= 0)
	{
		throw std::runtime_error("sign_ecdsa private invalid data");
	}

	if (reg_private == temp_private)
	{
		throw std::runtime_error("sign_ecdsa private keys are equal");
	}

	// set up a bignum message digest
	Bignum digest;
	//digest.set(loc, std::max(len, bit_width()/8));
	digest.set(loc, len);

	// set output keys to correct bit width
	sig_x.set_width(curve.m_ctx->bit_width);
	sig_y.set_width(curve.m_ctx->bit_width);

	int r = ippsGFpECSignDSA(
		static_cast<const IppsBigNumState*>(digest.const_bn()),
		static_cast<const IppsBigNumState*>(reg_private.const_bn()),
		static_cast<IppsBigNumState*>(temp_private.bn()),				// NOTE: the published docs show this is const, but the API does not...
		static_cast<IppsBigNumState*>(sig_x.bn()),
		static_cast<IppsBigNumState*>(sig_y.bn()),
		curve.m_ctx->ecstate, curve.m_ctx->scratch);
	throw_ipperr("ippsGFpECSignDSA", r);
}

bool EccGfp::verify_ecdsa(const void* loc, int len,
	const EccGfpPoint& reg_public,
	const scc::crypto::Bignum& sig_x, const scc::crypto::Bignum& sig_y)
{
	if (!loc || len <= 0)	throw std::runtime_error("verify_ecdsa private invalid data");

	if (!reg_public.m_ctx)	throw std::runtime_error("verify_ecdsa invalid point");

	// set up a bignum message digest
	Bignum digest;
	digest.set(loc, len);

	IppECResult v;
	int r = ippsGFpECVerifyDSA(
		static_cast<const IppsBigNumState*>(digest.const_bn()),
		reg_public.m_ctx->state,
		static_cast<const IppsBigNumState*>(sig_x.const_bn()),
		static_cast<const IppsBigNumState*>(sig_y.const_bn()),
		&v, reg_public.m_ctx->shared->ecstate, reg_public.m_ctx->shared->scratch);
	throw_ipperr("ippsGFpECVerifyDSA", r);
	
	return v == ippECValid ? true : false;
}

void EccGfp::dh_shared_secret(const scc::crypto::Bignum& my_private, const EccGfpPoint& other_public, scc::crypto::Bignum& shared_secret)
{
	shared_secret.set_width(other_public.m_ctx->shared->bit_width);

	int r = ippsGFpECSharedSecretDH(
		static_cast<const IppsBigNumState*>(my_private.const_bn()),
		other_public.m_ctx->state,
		static_cast<IppsBigNumState*>(shared_secret.bn()),
		other_public.m_ctx->ecstate, other_public.m_ctx->shared->scratch);
	throw_ipperr("ippsGFpECSharedSecretDH", r);
}