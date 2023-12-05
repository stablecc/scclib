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
#include <crypto/hash.h>
#include <crypto/secvec.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <cstring>
#include <ippcp.h>

using namespace scc::crypto;

class HashBase
{
	const IppsHashMethod* m_alg;
	int m_size;
	SecVecUchar m_buf;
public:
	HashBase(Hash::Algorithm alg) : m_size(Hash::alg_size(alg))
	{
		switch (alg)
		{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			case Hash::md5_type: m_alg = ippsHashMethod_MD5(); break;
			case Hash::sha1_type: m_alg = ippsHashMethod_SHA1(); break;
#pragma GCC diagnostic pop
			case Hash::sha224_type: m_alg = ippsHashMethod_SHA224(); break;
			case Hash::sha256_type: m_alg = ippsHashMethod_SHA256(); break;
			case Hash::sha384_type: m_alg = ippsHashMethod_SHA384(); break;
			case Hash::sha512_type: m_alg = ippsHashMethod_SHA512(); break;
			case Hash::sha512_224_type: m_alg = ippsHashMethod_SHA512_224(); break;
			case Hash::sha512_256_type: m_alg = ippsHashMethod_SHA512_256(); break;
			case Hash::sm3_type: m_alg = ippsHashMethod_SM3(); break;
			default: throw std::runtime_error("unknown hash type"); break;
		}

		int r, bsz;
		if ((r = ippsHashGetSize_rmf(&bsz)) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHashGetSize_rmf")+ippcpGetStatusString(r));
		}
		m_buf.resize(bsz);
		reset();
	}
	
	virtual ~HashBase()
	{
	}
	
	IppsHashState_rmf* ctx()
	{
		return (IppsHashState_rmf*)m_buf.data();
	}
	
	void reset()
	{
		int r;
		if ((r = ippsHashInit_rmf(ctx(), m_alg))!= ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHashInit_rmf")+ippcpGetStatusString(r));
		}
	}
	
	void update(const void* data, int len)
	{
		int r;
		if ((r = ippsHashUpdate_rmf(reinterpret_cast<const uint8_t*>(data), len, ctx())) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHashUpdate_rmf")+ippcpGetStatusString(r));
		}
	}
	
	int get_tag(void* data, int len)
	{
		if (len < 1 || len > m_size)
		{
			throw std::runtime_error("get_tag() buffer size must be between 1 and hash size");
		}

		int r;
		if ((r = ippsHashGetTag_rmf(reinterpret_cast<uint8_t*>(data), len, ctx())) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHashGetTag_rmf")+ippcpGetStatusString(r));
		}

		return len;
	}
	
	int final(void* data, int len)
	{
		if (len != m_size)
		{
			throw std::runtime_error("final() buffer size must be hash size");
		}

		int r;
		if ((r = ippsHashFinal_rmf(reinterpret_cast<uint8_t*>(data), ctx())) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHashFinal_rmf")+ippcpGetStatusString(r));
		}
		return m_size;
	}
	
	int size() const { return m_size; }
};

Hash::Hash(Algorithm alg) : m_ptr(new HashBase(alg)), m_alg(alg)
{
}

Hash::~Hash()
{
}

Hash::Hash(Hash&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
}

Hash& Hash::operator=(Hash&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
	return *this;
}

bool Hash::supported(Algorithm alg)
{
	switch (alg)
	{
		case Hash::md5_type:
		case Hash::sha1_type:
		case Hash::sha224_type:
		case Hash::sha256_type:
		case Hash::sha384_type:
		case Hash::sha512_type:
		case Hash::sha512_224_type:
		case Hash::sha512_256_type:
		case Hash::sm3_type:
			return true;
		default: 
			return false;
	}
}

void Hash::reset()
{
	if (!m_ptr) 	throw std::runtime_error("hash invalid");
	m_ptr->reset();
}

void Hash::update(const void* loc, int len)
{
	if (!m_ptr) 	throw std::runtime_error("hash invalid");
	m_ptr->update(loc, len);
}

int Hash::get_tag(void* loc, int len)
{
	if (!m_ptr) 	throw std::runtime_error("hash invalid");
	return m_ptr->get_tag(loc, len);
}

int Hash::final(void* loc, int len)
{
	if (!m_ptr) 	throw std::runtime_error("hash invalid");
	return m_ptr->final(loc, len);
}

class HmacBase
{
	SecVecUchar m_buf;
	SecVecUchar m_pack;
	const IppsHashMethod* m_alg;
	int m_size;
	int m_algsize;
public:
	HmacBase(const void* key, int len, Hash::Algorithm alg) : m_size(Hash::alg_size(alg))
	{
		switch (alg)
		{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
			case Hash::md5_type: m_alg = ippsHashMethod_MD5(); break;
			case Hash::sha1_type: m_alg = ippsHashMethod_SHA1(); break;
#pragma GCC diagnostic pop
			case Hash::sha224_type: m_alg = ippsHashMethod_SHA224(); break;
			case Hash::sha256_type: m_alg = ippsHashMethod_SHA256(); break;
			case Hash::sha384_type: m_alg = ippsHashMethod_SHA384(); break;
			case Hash::sha512_type: m_alg = ippsHashMethod_SHA512(); break;
			case Hash::sha512_224_type: m_alg = ippsHashMethod_SHA512_224(); break;
			case Hash::sha512_256_type: m_alg = ippsHashMethod_SHA512_256(); break;
			case Hash::sm3_type: m_alg = ippsHashMethod_SM3(); break;
			default: throw std::runtime_error("unknown hash type"); break;
		}
		int r;
		if ((r = ippsHMACGetSize_rmf(&m_algsize)) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHMACGetSize_rmf")+ippcpGetStatusString(r));
		}
		m_buf.resize(m_algsize);
		m_pack.resize(m_algsize);
		init(key, len);
	}

	virtual ~HmacBase()
	{
	}
	
	IppsHMACState_rmf* ctx()
	{
		return (IppsHMACState_rmf*)m_buf.data();
	}
	
	void init(const void* key, int len)
	{
		int r;
		if ((r = ippsHMACInit_rmf(reinterpret_cast<const uint8_t*>(key), len, ctx(), m_alg)) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHMACInit_rmf")+ippcpGetStatusString(r));
		}
		if ((r = ippsHMACPack_rmf(ctx(), (uint8_t*)m_pack.data(), m_algsize)) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHMACPack_rmf")+ippcpGetStatusString(r));
		}
	}
	
	void reset()
	{
		int r;
		if ((r = ippsHMACUnpack_rmf((uint8_t*)m_pack.data(), ctx())) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHMACUnpack_rmf")+ippcpGetStatusString(r));
		}
	}
	
	void update(const void* data, int len)
	{
		int r;
		if ((r = ippsHMACUpdate_rmf(reinterpret_cast<const uint8_t*>(data), len, ctx())) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHMACUpdate_rmf")+ippcpGetStatusString(r));
		}
	}
	
	int final(void* data, int len)
	{
		if (len != m_size)
		{
			throw std::runtime_error("hmac::final() len too small");
		}
		int r;
		if ((r = ippsHMACFinal_rmf(reinterpret_cast<uint8_t*>(data), len, ctx())) != ippStsNoErr)
		{
			throw std::runtime_error(std::string("ippsHMACFinal_rmf")+ippcpGetStatusString(r));
		}
		reset();
		return m_size;
	}
	
	int size() const { return m_size; }
};

Hmac::Hmac(const void* key, int len, Hash::Algorithm alg) : m_ptr(new HmacBase(key, len, alg)), m_alg(alg)
{
}

Hmac::~Hmac()
{
}

Hmac::Hmac(Hmac&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
}

Hmac& Hmac::operator=(Hmac&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
	return *this;
}

bool Hmac::supported(int alg)
{
	switch (alg)
	{
		case Hash::md5_type: break;
		case Hash::sha1_type: break;
		case Hash::sha224_type: break;
		case Hash::sha256_type: break;
		case Hash::sha384_type: break;
		case Hash::sha512_type: break;
		case Hash::sha512_224_type: break;
		case Hash::sha512_256_type: break;
		case Hash::sm3_type: break;
		default: 
			return false;
	}
	return true;
}

void Hmac::init(const void* key, int len)
{
	if (!m_ptr) 	throw std::runtime_error("hmac invalid");
	m_ptr->init(key, len);
}

void Hmac::reset()
{
	if (!m_ptr) 	throw std::runtime_error("hmac invalid");
	m_ptr->reset();
}

void Hmac::update(const void* buf, int len)
{
	if (!m_ptr) 	throw std::runtime_error("hmac invalid");
	m_ptr->update(buf, len);
}

int Hmac::final(void* loc, int len)
{
	if (!m_ptr) 	throw std::runtime_error("hmac invalid");
	return m_ptr->final(loc, len);
}
