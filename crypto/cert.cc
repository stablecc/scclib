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
#include <crypto/cert.h>
#include <map>
#include <sstream>
#include <iomanip>
#include <memory>
#include <system_error>
#include <iostream>
#include <algorithm>

using namespace scc::crypto;

static void throw_err(const std::string& msg)
{
	throw std::runtime_error(msg);
}

static oid_value s_rsa_key = {1, 2, 840, 113549, 1, 1, 1};
static oid_value s_ec_key = {1, 2, 840, 10045, 2, 1};
static std::map<KeyAlgoType, oid_value> s_ec_param =
{
	{KeyAlgoType::ec_p192r1, 		{1, 2, 840, 10045, 3, 1, 1}},
	{KeyAlgoType::ec_p224r1,		{1, 3, 132, 0, 33}},
	{KeyAlgoType::ec_p256r1,		{1, 2, 840, 10045, 3, 1, 7}},
	{KeyAlgoType::ec_p384r1,		{1, 3, 132, 0, 34}},
	{KeyAlgoType::ec_p521r1,		{1, 3, 132, 0, 35}},
};

std::ostream& operator<<(std::ostream& os, const scc::crypto::KeyAlgoType& algo)
{
	std::stringstream s;
	switch (algo)
	{
	case KeyAlgoType::rsa:
		s << "rsa"; break;
	case KeyAlgoType::ec_p192r1:
		s << "ecdsa 192"; break;
	case KeyAlgoType::ec_p224r1:
		s << "ecdsa 224"; break;
	case KeyAlgoType::ec_p256r1:
		s << "ecdsa 256"; break;
	case KeyAlgoType::ec_p384r1:
		s << "ecdsa 384"; break;
	case KeyAlgoType::ec_p521r1:
		s << "ecdsa 521"; break;
	case KeyAlgoType::unknown:
		s << "unknown"; break;
	}
	os.write(s.str().c_str(), s.str().size());
	return os;
}

KeyAlgoType PublicKeyCert::type() const
{
	if (algorithm_id == s_rsa_key && parameters != nullptr && parameters->is_null())
	{
		return KeyAlgoType::rsa;
	}
	else if (algorithm_id == s_ec_key && parameters != nullptr && parameters->is_object_id())
	{
		auto it = std::find_if(s_ec_param.begin(), s_ec_param.end(), [&](auto& i)
		{
			return parameters->object_id() == i.second;
		});

		if (it != s_ec_param.end())		return it->first;
	}

	return KeyAlgoType::unknown;
}

void PublicKeyCert::parse(const BasePtr base)
{
	algorithm_id.clear();
	parameters.reset();
	public_key.clear();

	if (!base || !base->is_seq())				throw_err("PublicKeyInfoCert parse: base not a sequence");
	if (base->contain().size() != 2	)			throw_err("PublicKeyInfoCert parse: wrong base sequence size");

	if (!base->contain()[0]->is_seq())			throw_err("PublicKeyInfoCert parse: missing algorithm id element");
	if (!base->contain()[1]->is_bit_string())	throw_err("PublicKeyInfoCert parse: missing public key element");

	auto& seq = base->contain()[0]->contain();

	if (!seq.size() || seq.size() > 2)			throw_err("PublicKeyInfoCert parse: algorithm id empty");
	
	if (!seq[0]->is_object_id())				throw_err("PublicKeyInfoCert parse: algorithm element not oid");
	algorithm_id = seq[0]->object_id();

	if (seq.size() == 2)
	{
		// store the parameters as a new element
		SecVecUchar v;
		DerDocument::dump_element(seq[1], v);
		parameters = DerDocument::parse_element(v);
	}

	// the public key is stored as DER-encoded bytes
	auto bits = base->contain()[1]->bit_string();
	bits.get(public_key);
}

BasePtr PublicKeyCert::dump() const
{
	auto seq = new DerSequence;
	BasePtr base(seq);
	
	auto algseq = new DerSequence;
	seq->emplace_back(algseq);

	auto oid = new DerObjectIdentifier;
	algseq->emplace_back(oid);
	oid->set(algorithm_id);

	if (parameters != nullptr)
	{
		SecVecUchar v;
		DerDocument::dump_element(parameters, v);
		algseq->push_back(DerDocument::parse_element(v));
	}
	
	auto bits = new DerBitString;
	seq->emplace_back(bits);
	bits->set(public_key, public_key.size()*8);

	return base;
}

std::string PublicKeyCert::str() const
{
	using std::endl;

	std::stringstream s;

	s << "pub key info: " << type() << " id: " << algorithm_id;
	if (parameters != nullptr && parameters->is_object_id()) 	s << " param: " << parameters->object_id();
	if (parameters != nullptr && parameters->is_null()) 		s << " param: null";
	s << " size: " << public_key.size();
	return s.str();
}

void PublicKeyCert::get(RsaPublicKey& key) const
{
	if (type() != KeyAlgoType::rsa)									throw_err("PublicKeyInfoCert get rsa: wrong algorithm type");

	BasePtr certseq = DerDocument::parse_element(public_key, 0);

	RsaPublicKeyCert::parse(certseq, key);
}

void PublicKeyCert::set(const RsaPublicKey& key)
{
	algorithm_id = s_rsa_key;
	parameters.reset(new DerNull);

	auto base = RsaPublicKeyCert::dump(key);

	public_key.clear();
	DerDocument::dump_element(base, public_key);
}

#include "encode/hex.h"

void PublicKeyCert::get(EccGfpPoint& key) const
{
	auto t = type();

	if (t == KeyAlgoType::unknown || t == KeyAlgoType::rsa)			throw_err("PublicKeyCert get ec: wrong algorithm type");

	EcPublicKeyCert::parse(public_key, t, key);

//std::cout << "**** EC GET algo-" << algorithm_id << std::endl;
//std::cout << "**** EC GET params=" << DerDocument::print_element(parameters) << std::endl;
//std::cout << "**** EC GET public_key size=" << public_key.size() << " hex: " << scc::encode::bin_to_hexstr(public_key.data(), public_key.size(), ":", 16) << std::endl;
}

void PublicKeyCert::set(const KeyAlgoType& algo, const EccGfpPoint& key)
{
	if (algo == KeyAlgoType::unknown || algo == KeyAlgoType::rsa)	throw_err("PublicKeyCert set ec: wrong algorithm type");

	algorithm_id = s_ec_key;
	
	auto oid = new DerObjectIdentifier;
	parameters.reset(oid);
	auto it = s_ec_param.find(algo);
	if (it == s_ec_param.end())				throw_err("PublicKeyCert set ec: algorithm error");
	oid->data() = it->second;

	public_key.clear();
	EcPublicKeyCert::dump(key, public_key);		// set the binary data directly to the key

//std::cout << "**** EC GET algo=" << algorithm_id << std::endl;
//std::cout << "**** EC SET params=" << DerDocument::print_element(parameters) << std::endl;
//std::cout << "**** EC SET public_key size=" << public_key.size() << " hex: " << scc::encode::bin_to_hexstr(public_key.data(), public_key.size(), ":", 16) << std::endl;

}

void RsaPublicKeyCert::parse(const BasePtr& base, RsaPublicKey& key)
{
	if (!base || !base->is_seq())		throw_err("rsa pub cert parse: base element not sequence");

	auto& seq = base->contain();

	if (seq.size() != 2)				throw_err("rsa pub cert parse: size error");

	if (!seq[0]->is_integer() || !seq[1]->is_integer())			throw_err("rsa pub cert parse: element error");

	key.set(seq[0]->integer(), seq[1]->integer());
}

BasePtr RsaPublicKeyCert::dump(const RsaPublicKey& key)
{
	auto seq = new DerSequence;
	BasePtr base(seq);

	Bignum n, e;
	key.get(n, e);

	BasePtr np(new DerInteger), ep(new DerInteger);
	np->integer() = n;
	ep->integer() = e;

	seq->push_back(np);
	seq->push_back(ep);

	return base;
}

void RsaPrivateKeyCert::parse(const BasePtr& base, RsaPrivateKey& key)
{
	if (!base || !base->is_seq())		throw_err("rsa priv cert parse: base element not sequence");

	auto seq = base->contain();

	if (seq.size() != 9)				throw_err("rsa priv cert parse: size error");

	if (	!seq[0]->is_integer() || !seq[1]->is_integer() || !seq[2]->is_integer()
		||	!seq[3]->is_integer() || !seq[4]->is_integer() || !seq[5]->is_integer()
		||	!seq[6]->is_integer() || !seq[7]->is_integer() || !seq[8]->is_integer())
	{
		throw_err("private key parse: data type error");
	}

	if (seq[0]->integer() != 0)		throw_err("private key parse: version error");

	key.set(seq[1]->integer(), seq[2]->integer(), seq[3]->integer(), seq[4]->integer(),
		seq[5]->integer(), seq[6]->integer(), seq[7]->integer(), seq[8]->integer());
}

BasePtr RsaPrivateKeyCert::dump(const RsaPrivateKey& key)
{
	Bignum n, e, d, p, q, ep, eq, qinv;
	key.get(n, e, d, p, q, ep, eq, qinv);

	auto *seq = new DerSequence;
	BasePtr ret(seq);

	DerInteger *ver_p = new DerInteger, *n_p = new DerInteger, *e_p = new DerInteger, *d_p = new DerInteger,
		*p_p = new DerInteger, *q_p = new DerInteger, *ep_p = new DerInteger, *eq_p = new DerInteger, *qinv_p = new DerInteger;

	seq->emplace_back(ver_p);
	seq->emplace_back(n_p);
	seq->emplace_back(e_p);
	seq->emplace_back(d_p);
	seq->emplace_back(p_p);
	seq->emplace_back(q_p);
	seq->emplace_back(ep_p);
	seq->emplace_back(eq_p);
	seq->emplace_back(qinv_p);

	n_p->data() = n;
	e_p->data() = e;
	d_p->data() = d;
	p_p->data() = p;
	q_p->data() = q;
	ep_p->data() = ep;
	eq_p->data() = eq;
	qinv_p->data() = qinv;

	return ret;
}

void EcParametersCert::parse(const BasePtr& b, KeyAlgoType& algo)
{
	if (!b || !b->is_object_id())				throw_err("ec params parse: base element not oid");
	
	auto it = std::find_if(s_ec_param.begin(), s_ec_param.end(), [&b](auto& i)
	{
		return b->object_id() == i.second;
	});

	if (it != s_ec_param.end())				algo = it->first;
	else									algo = KeyAlgoType::unknown;
}

BasePtr EcParametersCert::dump(const KeyAlgoType& algo)
{
	auto it = s_ec_param.find(algo);

	if (it == s_ec_param.end())				throw_err("ec params dump: invalid algorithm");

	auto oid = new DerObjectIdentifier;
	BasePtr ret(oid);
	oid->data() = it->second;

	return ret;
}

void EcPublicKeyCert::parse(const void* loc, int len, const KeyAlgoType& algo, EccGfpPoint& pub)
{
	char* b = (char*)loc;

	if (len < 2 || b == nullptr || *b != '\x04')			throw_err("ec public key parse: data error");

	EccGfp curve;
	switch (algo)
	{
	case KeyAlgoType::ec_p192r1:
		curve.reset(EccGfp::Type::std_p192r1); break;
	case KeyAlgoType::ec_p224r1:
		curve.reset(EccGfp::Type::std_p224r1); break;
	case KeyAlgoType::ec_p256r1:
		curve.reset(EccGfp::Type::std_p256r1); break;
	case KeyAlgoType::ec_p384r1:
		curve.reset(EccGfp::Type::std_p384r1); break;
	case KeyAlgoType::ec_p521r1:
		curve.reset(EccGfp::Type::std_p521r1); break;
	case KeyAlgoType::rsa:
	case KeyAlgoType::unknown:
		throw_err("ec public key parse: invalid algorithm");
	}

	pub.set(b+1, len-1, curve);
}

void EcPublicKeyCert::parse(const BasePtr& b, const KeyAlgoType& algo, EccGfpPoint& pub)
{
	if (!b || !b->is_bit_string())			throw_err("ec public key parse: not bit string");

	SecVecChar v;
	b->bit_string().get(v);
	parse(v, algo, pub);
}

BasePtr EcPublicKeyCert::dump(const EccGfpPoint& pub)
{
	SecVecChar v;
	pub.get(v);
	
	v.insert(v.begin(), '\x04');		// uncompressed byte

	auto bst = new DerBitString;
	BasePtr ret(bst);
	bst->set(v, v.size()*8);
	
	return ret;
}

void EcPublicKeyCert::dump(const EccGfpPoint& pub, std::vector<uint8_t>& uv)
{
	SecVecChar v;
	pub.get(v);	
	v.insert(v.begin(), '\x04');

	uv.clear();
	uv.insert(uv.begin(), v.begin(), v.end());
}

void EcPublicKeyCert::dump(const EccGfpPoint& pub, std::vector<char>& v)
{
	pub.get(v);
	v.insert(v.begin(), '\x04');
}

void EcPrivateKeyCert::parse(const BasePtr& b, Bignum& priv, KeyAlgoType& algo, EccGfpPoint& pub)
{
	if (!b || !b->is_seq())					throw_err("ec private parse: base not sequence");

	if (b->contain().size() != 4)			throw_err("ec private parse: wrong sequence size");

	auto& seq = b->contain();

	if (!seq[0]->is_integer() || seq[0]->integer() != 1)	throw_err("ec private parse: version error");

	if (!seq[1]->is_octet_string())			throw_err("ec private parse: private key error");

	SecVecChar v;
	seq[1]->string_get(v);

	priv.set(v.data(), v.size());

	if (!seq[2]->context_class() || seq[2]->id() != 0)			throw_err("ec private parse: param error");
	BasePtr oid = DerBase::context_to_explicit(seq[2]);
	EcParametersCert::parse(oid, algo);

	if (!seq[3]->context_class() || seq[3]->id() != 1)			throw_err("ec private parse: pub key error");
	BasePtr pkey = DerBase::context_to_explicit(seq[3]);
	EcPublicKeyCert::parse(pkey, algo, pub);
}

BasePtr EcPrivateKeyCert::dump(const Bignum& priv, const KeyAlgoType& algo, const EccGfpPoint& pub)
{
	auto seq = new DerSequence;
	BasePtr ret(seq);
	
	auto ver = new DerInteger;
	seq->emplace_back(ver);
	ver->data() = 1;

	SecVecChar v;
	v.resize(priv.len());
	priv.get(v.data(), v.size());
	auto pk = new DerOctetString;
	seq->emplace_back(pk);
	pk->set(v);

	BasePtr oid = EcParametersCert::dump(algo);
	seq->push_back(DerBase::explicit_to_context(oid, 0));

	BasePtr bst = EcPublicKeyCert::dump(pub);
	seq->push_back(DerBase::explicit_to_context(bst, 1));
	
	return ret;
}
