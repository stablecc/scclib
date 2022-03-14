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
#ifndef _SCC_ENCODE_HEX_H
#define _SCC_ENCODE_HEX_H

#include <string>
#include <vector>

namespace scc::encode {

/** \addtogroup encode
	@{
*/

/** \defgroup encode_hex Binary to hex string converter
	@{

	Converts binary into hexadecimal string.
*/

/** Binary to hex string converter.
	\file
*/

/** Encode to hex from binary.

	Encodes using two characters per byte.

	\param bin Binary input.
	\param hex String output.
	\param lower_case Output lower case flag.
*/

void bin_to_hex(const std::vector<char>&, std::string&, bool = true);
void bin_to_hex(const void*, size_t, std::string&, bool = true);

/** Encode to hex from binary.

	Encodes using two characters per byte.

	\param bin Binary input.
	\param lower_case Output lower case flag.

	\returns Hex string output.
*/
std::string bin_to_hex(const std::vector<char>&, bool = true);
std::string bin_to_hex(const void*, size_t, bool = true);

/** Decode from hex to binary.

	Encodes using two characters per byte.

	\param hex string input.
	\param bin Binary output.
*/
void hex_to_bin(const std::string&, std::vector<char>&);

/** Binary to human readable string.
	\param bin Binary input.
	\param lower_case Output lower case flag.
	\param delimit Delimiter for output (placed between bytes)
	\param limit Limit in bytes (< 0 means no limit)
	\param limit_msg Message to add if limit exceeded
	\param lower_case Use lower case

	Output message of form HH<delimit>HH<delimit>[limit_msg]
*/
std::string bin_to_hexstr(const std::vector<char>&, const std::string& = "", int limit=-1, const std::string& = " +more", bool = true);
std::string bin_to_hexstr(const void*, size_t, const std::string& = "", int limit=-1, const std::string& = " +more", bool = true);

/** @} */
/** @} */

}	// namespace

#endif
