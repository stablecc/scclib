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
#include <crypto/cipher.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <crypto/secvec.h>
#include <ippcp.h>

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

using namespace scc::crypto;

class Gcm : public CipherBase
{
	int m_sz;
	SecVecChar m_buf;
	IppsAES_GCMState* m_state;
	bool aad_called;
public:
	Gcm(const void* key_loc, int key_len) : aad_called(false)
	{
		IppStatus s = ippsAES_GCMGetSize(&m_sz);
		throw_ipp_err("ippsAES_GCMGetSize", s);

		m_buf.resize(m_sz);
 		m_state = (IppsAES_GCMState*)m_buf.data();

		s = ippsAES_GCMInit((const Ipp8u*)key_loc, key_len, m_state, m_sz);
		throw_ipp_err("ippsAES_GCMInit", s);
	}
	virtual ~Gcm()
	{
	}
	void reset(const void* nonce_loc, int nonce_len, const void* aad_loc, int aad_len)
	{
		IppStatus s = ippsAES_GCMReset(m_state);
		throw_ipp_err("ippsAES_GCMReset", s);

		s = ippsAES_GCMProcessIV((const Ipp8u*)nonce_loc, nonce_len, m_state);
		throw_ipp_err("ippsAES_GCMProcessIV", s);

		aad(aad_loc, aad_len);	// note: this must be called with nullptr if there is no additional data
	}
	void aad(const void* loc, int len)
	{
		aad_called = true;
		
		IppStatus s = ippsAES_GCMProcessAAD((const Ipp8u*)loc, len, m_state);
		throw_ipp_err(" ippsAES_GCMProcessAAD", s);
	}
	void encrypt(const void* msg_loc, int msg_len, void* cipher_loc, int cipher_len)
	{
		if (msg_len <= 0 || cipher_len < msg_len)
		{
			throw std::runtime_error("encrypt buffer size error");
		}
		IppStatus s = ippsAES_GCMEncrypt((const Ipp8u*)msg_loc, (Ipp8u*)cipher_loc, msg_len, m_state);
		throw_ipp_err("ippsAES_GCMEncrypt", s);
	}
	void decrypt(const void* cipher_loc, int cipher_len, void* msg_loc, int msg_len)
	{	
		if (cipher_len <= 0 || msg_len < cipher_len)
		{
			throw std::runtime_error("decrypt buffer size error");
		}
		IppStatus s = ippsAES_GCMDecrypt((const Ipp8u*)cipher_loc, (Ipp8u*)msg_loc, cipher_len, m_state);
		throw_ipp_err("ippsAES_GCMDecrypt", s);
	}
	void auth_tag(void* loc, int len)
	{
		explicit_bzero(loc, len);
		IppStatus s = ippsAES_GCMGetTag((Ipp8u*)loc, len, m_state);
		throw_ipp_err("ippsAES_GCMGetTag", s);
	}
};

class Ccm : public CipherBase
{
	int m_sz;
	SecVecChar m_buf;
	IppsAES_CCMState* m_state;
public:
	Ccm(const void* key_loc, int key_len, int tag_len)
	{
		IppStatus s = ::ippsAES_CCMGetSize(&m_sz);
		throw_ipp_err("ippsAES_CCMGetSize", s);

		m_buf.resize(m_sz);
 		m_state = (IppsAES_CCMState*)m_buf.data();

		s = ippsAES_CCMInit((const Ipp8u*)key_loc, key_len, m_state, m_sz);
		throw_ipp_err("ippsAES_CCMInit", s);

		s = ippsAES_CCMTagLen(tag_len, m_state);
		throw_ipp_err("ippsAES_CCMTagLen", s);
	}
	virtual ~Ccm()
	{
	}
	void reset(const void* nonce_loc, int nonce_len, const void* aad_loc, int aad_len)
	{
		// fool the ipp api which requires a message length (which is only used as a counter for encryption/decryption)
		IppStatus s = ippsAES_CCMMessageLen(0xffffffffffffffff, m_state);
		throw_ipp_err("ippsAES_CCMMessageLen", s);

		s = ippsAES_CCMStart((const Ipp8u*)nonce_loc, nonce_len, (const Ipp8u*)aad_loc, aad_len, m_state);
		throw_ipp_err("ippsAES_CCMStart", s);
	}
	void aad(const void* loc, int len)
	{
		throw std::runtime_error("ccm mode does not allow additional authenticated data");
	}
	void encrypt(const void* msg_loc, int msg_len, void* cipher_loc, int cipher_len)
	{
		if (msg_len <= 0 || cipher_len < msg_len)
		{
			throw std::runtime_error("encrypt buffer size error");
		}
		IppStatus s = ippsAES_CCMEncrypt((const Ipp8u*)msg_loc, (Ipp8u*)cipher_loc, msg_len, m_state);
		throw_ipp_err("ippsAES_CCMEncrypt", s);
	}
	void decrypt(const void* cipher_loc, int cipher_len, void* msg_loc, int msg_len)
	{	
		if (cipher_len <= 0 || msg_len < cipher_len)
		{
			throw std::runtime_error("decrypt buffer size error");
		}
		IppStatus s = ippsAES_CCMDecrypt((const Ipp8u*)cipher_loc, (Ipp8u*)msg_loc, cipher_len, m_state);
		throw_ipp_err("ippsAES_CCMDecrypt", s);
	}
	void auth_tag(void* loc, int len)
	{
		explicit_bzero(loc, len);
		IppStatus s = ippsAES_CCMGetTag((Ipp8u*)loc, len, m_state);
		throw_ipp_err("ippsAES_CCMGetTag", s);
	}
};

Cipher::Cipher(Cipher::Type type, const void* key_loc, int key_len, int tag_len) : m_ctx(nullptr), m_type(type)
{
	switch (m_type)
	{
		case Cipher::aes_gcm_type:
			m_ctx = new Gcm(key_loc, key_len);
		break;
		case Cipher::aes_ccm_type:
			m_ctx = new Ccm(key_loc, key_len, tag_len);
		break;
		default: throw_err("unknown cipher type"); break;
	}
}

Cipher::~Cipher()
{
	switch (m_type)
	{
		case Cipher::aes_gcm_type:
			delete reinterpret_cast<Gcm*>(m_ctx);
		break;
		case Cipher::aes_ccm_type:
			delete reinterpret_cast<Ccm*>(m_ctx);
		break;
	}
}