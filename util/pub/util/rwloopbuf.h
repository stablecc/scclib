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
#ifndef _SCC_UTIL_RWLOOPBUF_H
#define _SCC_UTIL_RWLOOPBUF_H

#include <vector>
#include <string>
#include <cstring>
#include "util/iobase.h"

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \addtogroup util_iostream
	@{
*/

/** Loopback read/write buffer.
	\file
*/

/** Loopback read/write stream buffer.
*/
class RwLoopBuffer : public Reader, public Writer, public std::vector<char>
{
	size_t m_idx;
public:
	RwLoopBuffer();
	virtual ~RwLoopBuffer() {}

	size_t idx() const { return m_idx; }

	/** Create as a string. */
	RwLoopBuffer(const std::string& b)
	{
		set(b);
	}

	/** Create as a vector. */
	RwLoopBuffer(const std::vector<char>& v)
	{
		set(v);
	}

	/** Set to a string. */
	RwLoopBuffer& operator=(const std::string& b)
	{
		set(b.data(), b.size());
		return *this;
	}

	/** Empty and reset. */
	void clear()
	{
		m_idx = 0;
		std::vector<char>::clear();
	}

	/** Clear and set for reading. */
	void set(const void* loc, size_t len)
	{
		clear();
		write(loc, len);
	}

	/** Clear and set for reading. */
	void set(const std::vector<char>& v)
	{
		set(v.data(), v.size());
	}

	/** Clear and set for reading. */
	void set(const std::string& v)
	{
		set(v.data(), v.size());
	}

	/** \ref Reader read. */
	size_t read(void* loc, size_t len)
	{
		if (m_idx == size())
		{
			return 0;
		}
		int rd = len < size()-m_idx ? len : size()-m_idx;
		memcpy(loc, (char*)data()+m_idx, rd);
		m_idx += rd;
		return rd;
	}

	/** Return the current read location. */
	size_t const read_loc()
	{
		return m_idx;
	}
	
	/** \ref Writer write. */
	size_t write(const void* loc, size_t len)
	{
		insert(end(), (char*)loc, (char*)loc+len);
		return len;
	}

	/** Return the current write location. */
	size_t const write_loc()
	{
		return size();
	}

	/** Read as a string. */
	std::string str()
	{
		if (size() == 0)
		{
			return "";
		}
		return std::string((char*)data()+m_idx, (char*)data()+size());
	}

	/** Read as a vector. */
	std::vector<char> vec() const
	{
		if (size() == 0)
		{
			return std::vector<char>();
		}
		return std::vector<char>((char*)data()+m_idx, (char*)data()+size());
	}
};

/** @} */
/** @} */
}

#endif
