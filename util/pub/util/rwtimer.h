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
#ifndef _SCC_UTIL_RWTIMER_H
#define _SCC_UTIL_RWTIMER_H

#include <atomic>
#include <chrono>
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

/** Read/write timer.
	\file
*/

/** Adds timer to a read stream.

	Example from \ref scclib/util/unittest/iohelper.cc
	\snippet scclib/util/unittest/iohelper.cc Counter and timer
*/

class ReadTimer : public PipelineReader
{
	Reader* m_reader;
	std::shared_ptr<Reader> m_shared;
	std::atomic_int_least64_t m_ticks;
	std::atomic_uint_least64_t m_calls;

public:
	/** Reads return 0 until reset. */
	ReadTimer();
	/** Chain using a reference. Does not assume ownership. */
	ReadTimer(Reader&);
	/** Chain using a shared pointer. */
	ReadTimer(const std::shared_ptr<Reader>&);
	
	virtual ~ReadTimer() {}

	/** Reset the chained reader. */
	void read_reset(Reader&);
	/** Reset the chained reader. */
	virtual void read_reset(const std::shared_ptr<Reader>&);
	virtual std::shared_ptr<Reader> read_shared() const { return m_shared; }

	/** Read and update time over underlying read. */
	virtual size_t read(void*, size_t);

	std::chrono::nanoseconds read_dur() const { return std::chrono::nanoseconds(m_ticks); }
	void read_dur(std::chrono::nanoseconds v) { m_ticks = v.count(); }
	void read_dur_reset() { m_ticks = 0; }

	uint64_t read_calls() const { return m_calls; }
	void read_calls_reset() { m_calls = 0; }
};

/** Adds timer to a write stream.

	Example from \ref scclib/util/unittest/iohelper.cc
	\snippet scclib/util/unittest/iohelper.cc Counter and timer
*/
class WriteTimer : public PipelineWriter
{
	Writer* m_writer;
	std::shared_ptr<Writer> m_shared;
	std::atomic_int_least64_t m_ticks;
	std::atomic_uint_least64_t m_calls;

public:
	/** Writes return 0 until reset. */
	WriteTimer();
	/** Chain using a reference. Does not assume ownership. */
	WriteTimer(Writer&);
	/** Chain using a shared pointer. */
	WriteTimer(const std::shared_ptr<Writer>&);

	virtual ~WriteTimer() {}

	/** Reset the chained writer. */
	void write_reset(Writer&);
	/** Reset the chained writer. */
	virtual void write_reset(const std::shared_ptr<Writer>&);
	virtual std::shared_ptr<Writer> write_shared() const { return m_shared; }

	/** Write and update time over underlying write. */
	virtual size_t write(const void*, size_t);

	std::chrono::nanoseconds write_dur() const { return std::chrono::nanoseconds(m_ticks); }
	void write_dur(std::chrono::nanoseconds v) { m_ticks = v.count(); }
	void write_dur_reset() { m_ticks = 0; }

	uint64_t write_calls() const { return m_calls; }
	void write_calls_reset() { m_calls = 0; }
};

/** Adds byte count to a read/write stream.

	Example from \ref scclib/util/unittest/iohelper.cc
	\snippet scclib/util/unittest/iohelper.cc Counter and timer
*/
class RwTimer : public ReadTimer, public WriteTimer
{
public:
	/** Reads and writes return 0 until reset. */
	RwTimer();
	/** Chain using references. Does not assume ownership. */
	RwTimer(Reader&, Writer&);
	/** Chain using shared pointers. */
	RwTimer(const std::shared_ptr<Reader>&, const std::shared_ptr<Writer>&);
	
	virtual ~RwTimer() {}
};

/** @} */
/** @} */
}

#endif
