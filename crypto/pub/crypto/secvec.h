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
#ifndef _SCC_CRYPTO_SECVEC_H
#define _SCC_CRYPTO_SECVEC_H

#include <vector>
#include <cstring>
#include <iostream>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_secvec Secure vector
	@{

	Secure (automatically erased) vector.
*/

/** Secure vector.
	\file
*/

/** Secure vector helper. A secure vector whose memory is zeroed when destroyed, cleared or resized.
*/
template <typename T>
class SecVec : public std::vector<T>
{
public:
	SecVec(typename std::vector<T>::size_type = 0);
	SecVec(typename std::vector<T>::size_type, const typename std::vector<T>::value_type&);
	template <class InputIt>
	SecVec(InputIt begin, InputIt end) : std::vector<T>(begin, end) {}
	virtual ~SecVec();

	/** Zero the original vector and clear it. */
	void clear() noexcept;

	/** Resize the vector.
	
		If the new size is less than the original size, zero the extra bytes before setting them.
	*/
	void resize(typename std::vector<T>::size_type);

	/** Resize the vector to an explicit value.
	
		If the new size is less than the original size, zero the extra bytes before setting them.
	*/
	void resize(typename std::vector<T>::size_type, const typename std::vector<T>::value_type&);
};

using SecVecUchar = SecVec<unsigned char>;
using SecVecChar = SecVec<char>;

/** @} */
/** @} */

}

/** Secure vector stream read helper. Reads the stream by character until eof().
	Throws exception on any stream error except eof().
*/
template <typename T>
std::istream& operator >>(std::istream&, scc::crypto::SecVec<T>&);

/** Secure vector stream write helper. Throws exception on any stream error.
*/
template <typename T>
std::ostream& operator <<(std::ostream&, const scc::crypto::SecVec<T>&);

#endif
