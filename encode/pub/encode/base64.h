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
#ifndef _SCC_ENCODE_BASE64_H
#define _SCC_ENCODE_BASE64_H

#include <string>
#include <cstring>
#include <vector>

namespace scc::encode {

/** \addtogroup encode
	@{
*/

/** \defgroup encode_base64 Base 64 encoding
	@{
	
	Base64 string encoding for binary data.

	Implements "Base 64 Encoding", from https://tools.ietf.org/html/rfc4648
*/

/** Convert from binary to base64 string.
	\param vin input binary data
	\param s output base64 encoded string

	See https://tools.ietf.org/html/rfc4648
*/
template<class T>
void base64_encode(const std::vector<T>&, std::string&) noexcept;

/** Convert from string to base64 string.
	\param s input string
	\retval base64_string base64 encoded string
*/
std::string str_to_base64(const std::string& s) noexcept;

/** Convert from base64 string to binary.
	\param s input base 64 string
	\param v output binary data
	\retval true if conversion successful
	\retval false if conversion failed due to invalid input format
*/
template <class T>
bool base64_decode(const std::string&, std::vector<T>&) noexcept;

/** Convert from base64 string to string.
	\param s input base 64 string
	\retval base64_string if conversion succeeded
	\retval empty_string if conversion failed
*/
std::string base64_to_str(const std::string&) noexcept;


/** Convert from base64 to base64url.

	See https://tools.ietf.org/html/rfc4648

	base64url replaces, '+' with '-', '/' with '_', and removes any padding '=' characters.

	It is also known as "Base 64 with URL and Filename Safe Encoding".
*/
std::string base64_to_base64url(const std::string&) noexcept;

/** Convert from base64url to base64.
*/
std::string base64url_to_base64(const std::string&) noexcept;

/** @} */
/** @} */
}

#endif
