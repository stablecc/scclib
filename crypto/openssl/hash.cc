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

class HashBase
{
	const EVP_MD* m_md;
	EVP_MD_CTX* m_ctx;
	int m_size;
public:
	HashBase(const std::string& n, int size) : m_md(EVP_get_digestbyname(n.c_str())), m_ctx(EVP_MD_CTX_create()), m_size(size)
	{
		if (!m_md)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << n << " not found: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
		if (!m_ctx)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << n << " ctx new failed: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
		reset();
		assert(EVP_MD_size(m_md) == m_size);
	}

	virtual ~HashBase()
	{
		EVP_MD_CTX_destroy(m_ctx);
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

Hash::Hash(int alg) : m_ptr(nullptr), m_alg(alg), m_size(0)
{
	switch (alg)
	{
		case Hash::md5_type: m_ptr.reset(new HashBase("md5", Hash::md5_size)); break;
		case Hash::sha1_type: m_ptr.reset(new HashBase("sha1", Hash::sha1_size)); break;
		case Hash::sha224_type: m_ptr.reset(new HashBase("sha224", Hash::sha224_size)); break;
		case Hash::sha256_type: m_ptr.reset(new HashBase("sha256", Hash::sha256_size)); break;
		case Hash::sha384_type: m_ptr.reset(new HashBase("sha384", Hash::sha384_size)); break;
		case Hash::sha512_type: m_ptr.reset(new HashBase("sha512", Hash::sha512_size)); break;
		case Hash::sha512_224_type: m_ptr.reset(new HashBase("sha512-224", Hash::sha512_224_size)); break;
		case Hash::sha512_256_type: m_ptr.reset(new HashBase("sha512-256", Hash::sha512_256_size)); break;
		case Hash::sm3_type: m_ptr.reset(new HashBase("sm3", Hash::sm3_size)); break;
		default: throw std::runtime_error("unknown hash type"); break;
	}
	m_size = m_ptr->size();
}

Hash::~Hash()
{
}

Hash::Hash(Hash&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
	m_size = other.m_size;
}

Hash& Hash::operator=(Hash&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
	m_size = other.m_size;
	return *this;
}

bool Hash::supported(int type)
{
	switch (type)
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

HashReader::HashReader(scc::util::Reader& rd, Hash& chk) : m_rd(rd), m_chk(chk)
{
}

HashReader::~HashReader()
{
}

size_t HashReader::read(void* loc, size_t len)
{
	m_buf.resize(len);

	int got = m_rd.read(m_buf.data(), len);
	if (got == 0)
	{
		return 0;
	}
	m_chk.update((char*)m_buf.data(), got);
	memcpy(loc, (char*)m_buf.data(), got);
	return got;
}

HashWriter::HashWriter(scc::util::Writer& wr, Hash& chk) : m_wr(wr), m_chk(chk)
{
}

HashWriter::~HashWriter()
{
}

size_t HashWriter::write(const void* loc, size_t len)
{
	int put = m_wr.write(loc, len);
	if (put == 0)
	{
		return 0;
	}
	m_chk.update(loc, put);
	return put;
}

class HmacBase
{
	const EVP_MD* m_md;
	HMAC_CTX* m_ctx;
	int m_size;
public:
	HmacBase(const std::string& n, int size) : m_md(EVP_get_digestbyname(n.c_str())), m_size(size)
	{
		m_ctx = HMAC_CTX_new();

		if (!m_md)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << n << " not found: " << ERR_error_string(e, nullptr);
			throw std::runtime_error(s.str());
		}
		if (!m_ctx)
		{
			auto e = ERR_get_error();
			std::stringstream s;
			s << n << " hmac ctx new failed: " << ERR_error_string(e, nullptr);
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

Hmac::Hmac(const void* key, int len, int hash_alg) : m_ptr(nullptr), m_alg(hash_alg)
{
	switch (hash_alg)
	{
		case Hash::md5_type: m_ptr.reset(new HmacBase("md5", Hash::md5_size)); break;
		case Hash::sha1_type: m_ptr.reset(new HmacBase("sha1", Hash::sha1_size)); break;
		case Hash::sha224_type: m_ptr.reset(new HmacBase("sha224", Hash::sha224_size)); break;
		case Hash::sha256_type: m_ptr.reset(new HmacBase("sha256", Hash::sha256_size)); break;
		case Hash::sha384_type: m_ptr.reset(new HmacBase("sha384", Hash::sha384_size)); break;
		case Hash::sha512_type: m_ptr.reset(new HmacBase("sha512", Hash::sha512_size)); break;
		case Hash::sha512_224_type: m_ptr.reset(new HmacBase("sha512-224", Hash::sha512_224_size)); break;
		case Hash::sha512_256_type: m_ptr.reset(new HmacBase("sha512-256", Hash::sha512_256_size)); break;
		case Hash::sm3_type: m_ptr.reset(new HmacBase("sm3", Hash::sm3_size)); break;
		default: throw std::runtime_error("unknown hmac type"); break;
	}
	m_size = m_ptr->size();
	m_ptr->init(key, len);
}

Hmac::~Hmac()
{
}

Hmac::Hmac(Hmac&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
	m_size = other.m_size;
}

Hmac& Hmac::operator=(Hmac&& other)
{
	m_ptr.reset(other.m_ptr.release());
	m_alg = other.m_alg;
	m_size = other.m_size;
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
