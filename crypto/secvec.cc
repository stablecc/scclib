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
#include <crypto/secvec.h>

using namespace scc::crypto;

template <typename T>
SecVec<T>::SecVec(typename std::vector<T>::size_type sz) : std::vector<T>(sz)
{
}

template <typename T>
SecVec<T>::SecVec(typename std::vector<T>::size_type sz, const typename std::vector<T>::value_type& value) : std::vector<T>(sz, value)
{
}

template <typename T>
SecVec<T>::~SecVec()
{
	explicit_bzero(std::vector<T>::data(), std::vector<T>::size());
}

template <typename T>
void SecVec<T>::clear() noexcept
{
	explicit_bzero(std::vector<T>::data(), std::vector<T>::size());
	std::vector<T>::clear();
}

template <typename T>
void SecVec<T>::resize(typename std::vector<T>::size_type count)
{
	if (std::vector<T>::size() > count)
	{
		explicit_bzero(std::vector<T>::data()+count, std::vector<T>::size()-count);
	}
	std::vector<T>::resize(count);
}

template <typename T>
void SecVec<T>::resize(typename std::vector<T>::size_type count, const typename std::vector<T>::value_type& value)
{
	if (std::vector<T>::size() > count)
	{
		explicit_bzero(std::vector<T>::data()+count, std::vector<T>::size()-count);
	}
	std::vector<T>::resize(count, value);
}

template SecVec<unsigned char>::SecVec(typename std::vector<unsigned char>::size_type);
template SecVec<unsigned char>::SecVec(typename std::vector<unsigned char>::size_type, const typename std::vector<unsigned char>::value_type&);
template SecVec<unsigned char>::~SecVec();
template void SecVec<unsigned char>::clear() noexcept;
template void SecVec<unsigned char>::resize(typename std::vector<unsigned char>::size_type);
template void SecVec<unsigned char>::resize(typename std::vector<unsigned char>::size_type, const typename std::vector<unsigned char>::value_type&);

template SecVec<char>::SecVec(typename std::vector<char>::size_type);
template SecVec<char>::SecVec(typename std::vector<char>::size_type, const typename std::vector<char>::value_type&);
template SecVec<char>::~SecVec();
template void SecVec<char>::clear() noexcept;
template void SecVec<char>::resize(typename std::vector<char>::size_type);
template void SecVec<char>::resize(typename std::vector<char>::size_type, const typename std::vector<char>::value_type&);

template <typename T>
std::istream& operator >>(std::istream& is, scc::crypto::SecVec<T>& sv)
{
	char ch;
	while (is.get(ch))
	{
		sv.push_back(ch);
	}
	if (is.fail() && !is.eof())
	{
		throw std::runtime_error("secure vector stream read error");
	}
	return is;
}

template std::istream& operator >><unsigned char>(std::istream& is, scc::crypto::SecVec<unsigned char>& sv);
template std::istream& operator >><char>(std::istream& is, scc::crypto::SecVec<char>& sv);

template <typename T>
std::ostream& operator <<(std::ostream& os, const scc::crypto::SecVec<T>& sv)
{
	os.write((const char*)sv.data(), sv.size()*sizeof(T));
	if (os.fail())
	{
		throw std::runtime_error("secure vector stream write error");
	}
	return os;
}

template std::ostream& operator <<<unsigned char>(std::ostream& os, const scc::crypto::SecVec<unsigned char>& sv);
template std::ostream& operator <<<char>(std::ostream& os, const scc::crypto::SecVec<char>& sv);
