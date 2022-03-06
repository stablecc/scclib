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
#ifndef _SCC_UTIL_IOBASE_H
#define _SCC_UTIL_IOBASE_H

#include <memory>

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \defgroup util_iostream input/output stream utilities
	@{

	Allows wrapping of generic streaming objects into std::iostream.

	Can create pipeline objects that add behavior to the stream.

	Example from \ref scclib/util/unittest/iostream.cc of basic stream:
	\snippet scclib/util/unittest/iostream.cc I/O stream
	
	Example from \ref scclib/util/unittest/iohelper.cc showing chaining of reader/writers:
	\snippet scclib/util/unittest/iohelper.cc Chained test

	Example with counter and timer:
	\snippet scclib/util/unittest/iohelper.cc Counter and timer
*/

/** Input/output stream base reader/writer interface classes.
	\file
*/

/** Interface class for objects which can be read.
*/
struct Reader
{
	virtual ~Reader() {}
	/** Read interface. Read a block of data to a maximum. Return number of bytes read. */
	virtual size_t read(void*, size_t) = 0;
};

/** Pipeline reader to carry out processing in a pipeline (chain of readers).
*/
struct PipelineReader : public Reader
{
	virtual ~PipelineReader() {}
	/** Reset interface. The underlying reader is reset and will be used for all future reads. */
	virtual void read_reset(const std::shared_ptr<Reader>&) = 0;
	virtual std::shared_ptr<Reader> read_shared() const = 0;
};

/** Interface class for objects which can be written.
*/
struct Writer
{
	virtual ~Writer() {}
	/** Write interface. Write a block of data to a maximum. Return number of bytes written. */
	virtual size_t write(const void*, size_t) = 0;
};

/** Pipeline writer to carry out processing in a pipeline (chain of writers).
*/
struct PipelineWriter : public Writer
{
	virtual ~PipelineWriter() {}
	/** Reset interface. The underlying writer is reset and will be used for all future writes. */
	virtual void write_reset(const std::shared_ptr<Writer>&) = 0;
	virtual std::shared_ptr<Writer> write_shared() const = 0;
};

/** Pipeline reader which flows through all data.
*/
struct FlowThroughPipelineReader : public PipelineReader
{
	std::shared_ptr<Reader> reader;

	FlowThroughPipelineReader() : reader(nullptr) {}
	FlowThroughPipelineReader(const std::shared_ptr<Reader>& r) : reader(r) {}
	virtual ~FlowThroughPipelineReader() {}
	virtual size_t read(void* loc, size_t len)
	{
		if (reader)		return reader->read(loc, len);
		else			return 0;
	}
	virtual void read_reset(const std::shared_ptr<Reader>& r)
	{
		reader = r;
	}
	virtual std::shared_ptr<Reader> read_shared() const { return reader; };
};

/** Pipeline writer which flows through all data.
*/
struct FlowThroughPipelineWriter : public PipelineWriter
{
	std::shared_ptr<Writer> writer;

	FlowThroughPipelineWriter() : writer(nullptr) {}
	FlowThroughPipelineWriter(const std::shared_ptr<Writer>& w) : writer(w) {}
	virtual ~FlowThroughPipelineWriter() {}
	virtual size_t write(const void* loc, size_t len)
	{
		if (writer)		return writer->write(loc, len);
		else			return 0;
	}
	virtual void write_reset(const std::shared_ptr<Writer>& w)
	{
		writer = w;
	}
	virtual std::shared_ptr<Writer> write_shared() const { return writer; };
};
/** @} */
/** @} */
}

#endif
