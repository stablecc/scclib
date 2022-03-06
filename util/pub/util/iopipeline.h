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
#ifndef _SCC_UTIL_IOPIPELINE_H
#define _SCC_UTIL_IOPIPELINE_H

#include <memory>
#include <list>
#include "util/iostream.h"

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \addtogroup util_iostream
	@{
*/

/** Input/output streaming pipeline.
	\file
*/

/** Chain of readers base class.
*/
struct InChain
{
	std::shared_ptr<Reader> rd_base;

	/** Create with base reader and read buffer size. */
	InChain(const std::shared_ptr<Reader>&);
	virtual ~InChain() {}

	std::list<std::shared_ptr<PipelineReader>> rd_chain;

	/** Replace the base writer with a new base writer. */
	void rd_replace_base(const std::shared_ptr<PipelineReader>& r)
	{
		r->read_reset(rd_base);
		rd_base = r;
		rd_fix_chain();
	}
	/** Add a reader to the end of the chain (before the base). */
	void rd_add_back(const std::shared_ptr<PipelineReader>& r)
	{
		rd_chain.emplace_back(r);
		rd_fix_chain();
	}
	/** Add a reader to the start of the chain (after the stream). */
	void rd_add_front(const std::shared_ptr<PipelineReader>& r)
	{
		rd_chain.emplace_front(r);
		rd_fix_chain();
	}
	/** Delete a reader from the chain. */
	void rd_del(const std::shared_ptr<PipelineReader>&);
	
	/** Fix the chain, and return the pointer that should be pointed to by the stream. */
	virtual std::shared_ptr<Reader> rd_fix_chain();
};

/** Chain of writers base class.
*/
struct OutChain
{
	std::shared_ptr<Writer> wr_base;

	/** Create with base writer and write buffer size. */
	OutChain(const std::shared_ptr<Writer>&);
	virtual ~OutChain() {}

	std::list<std::shared_ptr<PipelineWriter>> wr_chain;
	
	/** Replace the base writer with a new base writer. */
	void wr_replace_base(const std::shared_ptr<PipelineWriter>& w)
	{
		w->write_reset(wr_base);
		wr_base = w;
		wr_fix_chain();
	}
	/** Add a writer to the end of the chain (before the base). */
	void wr_add_back(const std::shared_ptr<PipelineWriter>& w)
	{
		wr_chain.emplace_back(w);
		wr_fix_chain();
	}
	/** Add a writer to the start of the chain (after the stream). */
	void wr_add_front(const std::shared_ptr<PipelineWriter>& w)
	{
		wr_chain.emplace_front(w);
		wr_fix_chain();
	}
	/** Delete a writer from the chain. */
	void wr_del(const std::shared_ptr<PipelineWriter>&);
	
	/** Fix the chain, and return the pointer that should be pointed to by the stream. */
	virtual std::shared_ptr<Writer> wr_fix_chain();
};

/** Input stream with pipeline of readers.

	Without:		stream reads <-> base
	With chain:		stream reads <-> chain[0] <-> ... <-> chain[N] <-> base
*/
struct InPipeline : public InChain, public InStream
{
	/** Create with base reader and read buffer size. */
	InPipeline(const std::shared_ptr<Reader>&, size_t = 1024);
	virtual ~InPipeline() {}

	std::shared_ptr<Reader> rd_fix_chain();
};

/** Output stream pipeline of writers.

	Without:		stream writes <-> base
	With chain:		stream writes <-> chain[0] <-> ... <-> chain[N] <-> base
*/
struct OutPipeline : public OutChain, public OutStream
{
	/** Create with base writer and write buffer size. */
	OutPipeline(const std::shared_ptr<Writer>&, size_t = 1024);
	virtual ~OutPipeline() {}

	std::shared_ptr<Writer> wr_fix_chain();
};

/** Input/output stream with pipeline of readers and writers.

	Without:		stream reads/writes <-> base
	With chain:		stream reads/writes <-> chain[0] <-> ... <-> chain[N] <-> base
*/
struct IoPipeline : public InChain, public OutChain, public IoStream
{
	/** Create with base reader/writer and buffer sizes. */
	IoPipeline(const std::shared_ptr<Reader>&, const std::shared_ptr<Writer>&, size_t = 1024, size_t = 1024);
	virtual ~IoPipeline() {}

	/** Add to the end of the chain (before the base). */
	void rw_add_back(const std::shared_ptr<PipelineReader>& r, const std::shared_ptr<PipelineWriter>& w)
	{
		rd_add_back(r);
		wr_add_back(w);
	}
	/** Add to the end of the chain (after the stream). */
	void rw_add_front(const std::shared_ptr<PipelineReader>& r, const std::shared_ptr<PipelineWriter>& w)
	{
		rd_add_front(r);
		wr_add_front(w);
	}

	std::shared_ptr<Reader> rd_fix_chain();
	std::shared_ptr<Writer> wr_fix_chain();
};

/** @} */
/** @} */
}

#endif
