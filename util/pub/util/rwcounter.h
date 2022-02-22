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
#ifndef _SCC_UTIL_RWCOUNTER_H
#define _SCC_UTIL_RWCOUNTER_H

#include <atomic>
#include <memory>
#include "util/iobase.h"

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \addtogroup util_iostream
	@{
*/

/** Read/write counter.
	\file
*/

/** Adds byte count to a read stream.

	Example from \ref scclib/util/unittest/iohelper.cc
	\snippet scclib/util/unittest/iohelper.cc Counter and timer
*/
class ReadCounter : public PipelineReader
{
	Reader* m_reader;
	std::shared_ptr<Reader> m_shared;
	std::atomic_uint64_t m_count;
	std::atomic_uint64_t m_calls;

public:
	/** Reads will return 0 until reset. */
	ReadCounter();
	/** Chain using a reference. Does not assume ownership. */
	ReadCounter(Reader&);
	/** Chain using a shared pointer. */
	ReadCounter(const std::shared_ptr<Reader>&);

	virtual ~ReadCounter() {}

	/** Reset the chained reader. */
	void read_reset(Reader&);
	/** Reset the chained reader. */
	virtual void read_reset(const std::shared_ptr<Reader>&);
	virtual std::shared_ptr<Reader> read_shared() const { return m_shared; }

	virtual size_t read(void*, size_t);

	uint64_t read_count() const { return m_count; }
	void read_count(uint64_t v) { m_count = v; }
	void read_count_reset() { m_count = 0; }
	uint64_t read_calls() const { return m_calls; }
	void read_calls(uint64_t v) { m_calls = v; }
	void read_calls_reset() { m_calls = 0; }
};

/** Adds byte count to a write stream.

	Example from \ref scclib/util/unittest/iohelper.cc
	\snippet scclib/util/unittest/iohelper.cc Counter and timer
*/
class WriteCounter : public PipelineWriter
{
	Writer* m_writer;
	std::shared_ptr<Writer> m_shared;
	std::atomic_uint64_t m_count;
	std::atomic_uint64_t m_calls;

public:
	/** Writes will return 0 until reset. */
	WriteCounter();
	/** Chain using a reference. Does not assume ownership. */
	WriteCounter(Writer&);
	/** Chain using a shared pointer. */
	WriteCounter(const std::shared_ptr<Writer>&);

	virtual ~WriteCounter() {}

	/** Reset the chained writer. */
	void write_reset(Writer&);
	/** Reset the chained writer. */
	virtual void write_reset(const std::shared_ptr<Writer>&);
	virtual std::shared_ptr<Writer> write_shared() const { return m_shared; }

	virtual size_t write(const void*, size_t);

	uint64_t write_count() const { return m_count; }
	void write_count(uint64_t v) { m_count = v; }
	void write_count_reset() { m_count = 0; }
	uint64_t write_calls() const { return m_calls; }
	void write_calls(uint64_t v) { m_calls = v; }
	void write_calls_reset() { m_calls = 0; }
};

/** Adds byte count to a read/write stream.

	Example from \ref scclib/util/unittest/iohelper.cc
	\snippet scclib/util/unittest/iohelper.cc Counter and timer
*/
class RwCounter : public ReadCounter, public WriteCounter
{
public:
	/** Reads and writes will return 0 until reset. */
	RwCounter();
	/** Chain using references. Does not assume ownership. */
	RwCounter(Reader&, Writer&);
	/** Chain using shared pointers. */
	RwCounter(const std::shared_ptr<Reader>&, const std::shared_ptr<Writer>&);

	virtual ~RwCounter() {}
};

/** @} */
/** @} */
}

#endif
