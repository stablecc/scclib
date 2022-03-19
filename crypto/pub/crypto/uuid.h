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
#ifndef _SCC_CRYPTO_UUID_H
#define _SCC_CRYPTO_UUID_H

#include <string>
#include <ostream>
#include <cstring>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_uuid Universally unique identifier (uuid)
	@{

	Provides a randomly generated UUID.

	https://tools.ietf.org/html/rfc4122
*/

/** Universally Unique Identifier (uuid).
	\file uuid.h
*/

/** Universally unique identifier (uuid).

	Uuid's are 36 digit strings, with 8-4-4-12 hex encoded digits. All hex digits are lower case.

	If an invalid uuid is constructed or assigned, the value is set to Uuid::zero.
*/
class Uuid
{
	std::string m_uuid;
	void assign(const std::string&);

public:
	Uuid()
	{
		generate();
	}

	virtual ~Uuid()
	{
		explicit_bzero(m_uuid.data(), m_uuid.size());
	}
	Uuid(Uuid&&) = delete;		// move not allowed
	Uuid(Uuid& b)
	{
		m_uuid = b.m_uuid;
	}
	/** Copy assignment operator. */
	Uuid& operator =(const Uuid& b)
	{
		m_uuid = b.m_uuid;
		return *this;
	}
	/** String copy assignment operator.
		
		If an invalid uuid is passed, sets to zero without error.
	*/
	Uuid& operator =(const std::string& b)
	{
		assign(b);
		return *this;
	}

	/** String initializer.
	
		If an invalid uuid is passed, sets to zero without error.
	*/
	Uuid(const std::string& s)
	{
		assign(s);
	}

	/** Generate a new uuid.
	
		Returns the new value.
	*/
	std::string generate();

	/** A uuid may be used as a string. */
	operator std::string() const { return m_uuid; }

	/** Return the uuid value. */
	std::string val() const { return m_uuid; }

	/** Equality operator. */
	bool operator ==(const Uuid &b) const
	{
		return m_uuid == b.m_uuid;
	}
	/** Not equals operator. */
	bool operator !=(const Uuid &b) const
	{
		return m_uuid != b.m_uuid;
	}

	/** Zero uuid is 00000000-0000-0000-000000000000 */
	static const std::string zero;
};

/** Googletest printer. */
void PrintTo(const Uuid&, std::ostream*);

/** @} */
/** @} */

}

/** Print the uuid to stream. */
std::ostream& operator <<(std::ostream&, const scc::crypto::Uuid&);

#endif
