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
#ifndef _SCC_UTIL_IOSTREAM_H
#define _SCC_UTIL_IOSTREAM_H

#include <istream>
#include <string>
#include <memory>
#include <util/iobase.h>

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \addtogroup util_iostream
	@{
*/

/** Base input/output stream classes.
	\file
*/

/** Input stream wrapper for reader.

	Makes \ref Reader objects compatible with std::istream.
*/
class InStream : public std::istream
{
public:
	/** Create reader stream.
		Does not assume ownership of object. Useful for temporary use, otherwise use the shared pointer form.
		\param reader Reader.
		\param recv_buf_sz Size of receive buffer. Number of bytes to be buffered from the underlying reader.
	*/
	InStream(Reader&, size_t = 1024);
	/** Create reader stream.
		\param reader Reader shared pointer.
		\param recv_buf_sz Size of receive buffer. Number of bytes to be buffered from the underlying reader.
	*/
	InStream(const std::shared_ptr<Reader>&, size_t = 1024);
	/** No default construct. */
	InStream() = delete;
	/** Copy construct not allowed. */
	InStream(const InStream&) = delete;
	/** Copy assign not allowed. */
	InStream& operator=(const InStream&) = delete;
	/** Move construct. */
	InStream(InStream&&);
	/** Move assign. */
	InStream& operator=(InStream&&);
	virtual ~InStream();

	void read_reset(const std::shared_ptr<Reader>&);
	std::shared_ptr<Reader> read_shared() const;

	/** Size of receive buffer. */
	virtual size_t recvbuf_size() const;
	/** Resize the receive buffer. Clears any data currently in the input buffer. */
	virtual void recvbuf_size(size_t);
	/** Failure message from the input stream.
		
		When an exception is thrown during an underlying read, the exception value is set in recv_fail(),
		and the bad bit is set.

		When a read returns 0, the eof bit is set.
	*/
	virtual std::string recv_fail() const;

	void clear(std::ios::iostate = std::ios::goodbit);
};

/** Output stream wrapper for writer.

	Makes \ref Writer objects compatible with std::ostream.
*/
class OutStream : public std::ostream
{
public:
	/** Create writer stream.
		Does not assume ownership of object. Useful for temporary use, otherwise use the shared pointer form.
		\param writer Writer.
		\param send_buf_sz Size of send buffer. Number of bytes to be buffered before writing to the underlying writer.
	*/
	OutStream(Writer&, size_t = 1024);
	/** Create writer stream.
		\param writer Writer shared pointer.
		\param send_buf_sz Size of send buffer. Number of bytes to be buffered before writing to the underlying writer.
	*/
	OutStream(const std::shared_ptr<Writer>&, size_t = 1024);
	/** No default construct. */
	OutStream() = delete;
	/** Copy construct not allowed. */
	OutStream(const OutStream&) = delete;
	/** Copy assign not allowed. */
	OutStream& operator=(const OutStream&) = delete;
	/** Move construct. */
	OutStream(OutStream&&);
	/** Move assign. */
	OutStream& operator=(OutStream&&);
	virtual ~OutStream();

	void write_reset(const std::shared_ptr<Writer>&);
	std::shared_ptr<Writer> write_shared() const;

	/** Size of send buffer. */
	size_t sendbuf_size() const;
	/** Resize the send buffer. This will clear any data currently in the output buffer. */
	void sendbuf_size(size_t);
	/** Failure message from outstream.

		When an exception is thrown during an underlying write, the exception value is set in send_fail(),
		and the bad bit is set.

		When a write returns 0, the eof bit is set.
	*/
	virtual std::string send_fail() const;

	void clear(std::ios::iostate = std::ios::goodbit);
};

/**	Input/output stream wrapper for reader/writer.

	Makes \ref Reader and \ref Writer objects compatible with std::iostream.
*/
class IoStream : public std::iostream
{
public:
	/** Create reader/writer stream.
		Does not assume ownership of objects. Useful for temporary use, otherwise use the shared pointer form.
		\param reader Reader.
		\param writer Writer.
		\param recv_buf_sz Size of receive buffer. Number of bytes to be buffered from the underlying reader.
		\param send_buf_sz Size of send buffer. Number of bytes to be buffered before writing to the underlying writer.
	*/
	IoStream(Reader&, Writer&, size_t = 1024, size_t = 1024);
	/** Create reader/writer stream.
		\param reader Reader shared pointer.
		\param writer Writer shared pointer.
		\param recv_buf_sz Size of receive buffer. Number of bytes to be buffered from the underlying reader.
		\param send_buf_sz Size of send buffer. Number of bytes to be buffered before writing to the underlying writer.
	*/
	IoStream(const std::shared_ptr<Reader>&, const std::shared_ptr<Writer>&, size_t = 1024, size_t = 1024);
	/** No default construct. */
	IoStream() = delete;
	/** Copy construct not allowed. */
	IoStream(const IoStream&) = delete;
	/** Copy assign not allowed. */
	IoStream& operator=(const IoStream&) = delete;
	/** Move construct. */
	IoStream(IoStream&&);
	/** Move assign. */
	IoStream& operator=(IoStream&&);
	virtual ~IoStream();

	void read_reset(const std::shared_ptr<Reader>&);
	void write_reset(const std::shared_ptr<Writer>&);
	std::shared_ptr<Reader> read_shared() const;
	std::shared_ptr<Writer> write_shared() const;

	/** Size of receive buffer. */
	size_t recvbuf_size() const;
	/** Resize the receive buffer. */
	void recvbuf_size(size_t);
	/** Size of send buffer. */
	size_t sendbuf_size() const;
	/** Resize the send buffer. */
	void sendbuf_size(size_t);
	/** Failure message from instream. */
	virtual std::string recv_fail() const;
	/** Failure message from outstream. */
	virtual std::string send_fail() const;

	void clear(std::ios::iostate = std::ios::goodbit);
};

/** @} */
/** @} */
}

#endif
