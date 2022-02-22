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
#ifndef _SCC_UTIL_FILEDESC_H
#define _SCC_UTIL_FILEDESC_H

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \defgroup util_filedesc File descriptor wrapper
	@{

	Wraps an open file descriptor, providing safe duplication and destruction.
*/

/** File descriptor wrapper.
	\file
*/

/** File descriptor wrapper.

	Assumes ownership of an open file descriptor.

	Wraps a file descriptor object and allows safe duplication and destruction.
	Safe for signal interruption.

	Copy operations duplicate the file descriptor.

	Example of using FileDesc to protect a file descriptor opened with
	the clib open api, in \ref scclib/util/unittest/fs.cc
	\snippet scclib/util/unittest/fs.cc Sparse file
*/
class FileDesc
{
	int m_fd;

public:
	FileDesc() : m_fd(-1) {}
	/** Wrap an open file descriptor. */
	explicit FileDesc(int fd) : m_fd{fd < 0 ? -1 : fd} {}
	virtual ~FileDesc()
	{
		close();
	}
	/** Copy construct by duplication. */
	FileDesc(const FileDesc& other)
	{
		dup(other.m_fd);
	}
	/** Copy assign by duplication. */
	FileDesc& operator=(const FileDesc& other)
	{
		dup(other.m_fd);	 // copy by duplication
		return *this;
	}
	/** Move construct. */
	FileDesc(FileDesc&& other)
	{
		m_fd = other.m_fd;
		other.m_fd = -1;
	}
	/** Move assign. */
	FileDesc& operator=(FileDesc&& other)
	{
		m_fd = other.m_fd;
		other.m_fd = -1;
		return *this;
	}

	/** Cast const file descriptor to int. */
	operator int() const { return m_fd; }
	/** Return the file descriptor. */
	int fd() const { return m_fd; }

	/** Signal safe close. */
	void close();

	/** Duplicate a file descriptor.

		This descriptor is now a duplicate of the input descriptor.

		If the input file descriptor is invalid, this descriptor will also be invalid.
	*/
	void dup(int);
};
/** @} */
/** @} */
}

#endif
