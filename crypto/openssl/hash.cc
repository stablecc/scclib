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
#include <cassert>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

using namespace scc::crypto;

struct HashBase
{
	const EVP_MD* m_md;
	EVP_MD_CTX* m_ctx;
	int m_size;

	HashBase(Hash::Algorithm alg) : m_size(Hash::alg_size(alg))
	{
		m_md = digest(alg);
		if (!m_md)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << "digest not found: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
		m_ctx = EVP_MD_CTX_create();
		if (!m_ctx)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << "hash ctx new failed: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
		reset();
		assert(EVP_MD_size(m_md) == m_size);
	}

	virtual ~HashBase()
	{
		EVP_MD_CTX_destroy(m_ctx);
	}

	static const EVP_MD* digest(Hash::Algorithm alg)
	{
		switch (alg)
		{
			case Hash::md5_type: return EVP_get_digestbyname("md5");
			case Hash::sha1_type: return EVP_get_digestbyname("sha1");
			case Hash::sha224_type: return EVP_get_digestbyname("sha224");
			case Hash::sha256_type: return EVP_get_digestbyname("sha256");
			case Hash::sha384_type: return EVP_get_digestbyname("sha384");
			case Hash::sha512_type: return EVP_get_digestbyname("sha512");
			case Hash::sha512_224_type: return EVP_get_digestbyname("sha512-224");
			case Hash::sha512_256_type: return EVP_get_digestbyname("sha512-256");
			case Hash::sm3_type: return EVP_get_digestbyname("sm3");
		}
		return nullptr;
	}

	void reset()
	{
		if (!EVP_DigestInit(m_ctx, m_md))
		{
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
	}

	void update(const void* data, int len)
	{
		if (!EVP_DigestUpdate(m_ctx, data, len))
		{
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
	}
	
	int get_tag(void* data, int len)
	{
		if (len < 1 || len > m_size)
		{
			throw std::runtime_error("get_tag() buffer size must be between 1 and hash size");
		}

		SecVecUchar buf(m_size);
		EVP_MD_CTX* ctx_copy = EVP_MD_CTX_create();
		
		if (!EVP_MD_CTX_copy(ctx_copy, m_ctx))
		{
			EVP_MD_CTX_destroy(ctx_copy);
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
		
		if (!EVP_DigestFinal(ctx_copy, reinterpret_cast<unsigned char*>(&buf[0]), nullptr))
		{
			EVP_MD_CTX_destroy(ctx_copy);
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
		EVP_MD_CTX_destroy(ctx_copy);

		memcpy(data, &buf[0], len);

		return len;
	}

	int final(void* data, int len)
	{
		if (len < m_size)
		{
			throw std::runtime_error("hash final() len too small");
		}

		if (!EVP_DigestFinal(m_ctx, reinterpret_cast<unsigned char*>(data), nullptr))
		{
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
		reset();
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
	return HashBase::digest(alg) != nullptr ? true : false;
}

void Hash::reset()
{
	if (!m_ptr) 	throw std::runtime_error("hash invalid");
	m_ptr->reset();
}

void Hash::update(const void* buf, int len)
{
	if (!m_ptr) 	throw std::runtime_error("hash invalid");
	m_ptr->update(buf, len);
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
	const EVP_MD* m_md;
	HMAC_CTX* m_ctx;
	int m_size;
public:
	HmacBase(Hash::Algorithm alg) : m_size(Hash::alg_size(alg))
	{
		m_md = HashBase::digest(alg);
		if (!m_md)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << "digest not found: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
		m_ctx = HMAC_CTX_new();
		if (!m_ctx)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << "hmac ctx new failed: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
		HMAC_CTX_reset(m_ctx);
		assert(EVP_MD_size(m_md) == m_size);
	}

	virtual ~HmacBase()
	{
		HMAC_CTX_free(m_ctx);
	}

	void init(const void* key, int len)
	{
		if (!HMAC_Init_ex(m_ctx, key, len, m_md, nullptr))
		{
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
	}
	void reset()
	{
		init(nullptr, 0);
	}

	void update(const void* data, int len)
	{
		if (!HMAC_Update(m_ctx, reinterpret_cast<const unsigned char*>(data), len))
		{
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
	}

	int final(void* data, int len)
	{
		if (len < m_size)
		{
			throw std::runtime_error("hmac::final() len too small");
		}

		if (!HMAC_Final(m_ctx, reinterpret_cast<unsigned char*>(data), nullptr))
		{
			auto e = ERR_get_error();
			throw std::runtime_error(ERR_error_string(e, nullptr));
		}
		reset();
		return m_size;
	}

	int size() const { return m_size; }
};

Hmac::Hmac(const void* key, int len, Hash::Algorithm alg) : m_ptr(new HmacBase(alg)), m_alg(alg)
{
	m_ptr->init(key, len);
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
