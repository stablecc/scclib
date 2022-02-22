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
#include "util/iostream.h"
#include "util/rwtimer.h"
#include "util/rwcounter.h"
#include "util/rwloopbuf.h"
#include "util/iopipeline.h"
#include <streambuf>
#include <iostream>
#include <system_error>
#include <stdexcept>
#include <algorithm>

using namespace scc::util;

///
/// stream buffers
///

class InStreambuf : public virtual std::streambuf
{
	std::shared_ptr<Reader> m_shared;
	Reader* m_reader;
	std::vector<char> m_recv;
	std::string m_fail;
	size_t m_resize_recv;

	InStreambuf(const InStreambuf&) = delete;
	void operator=(const InStreambuf&) = delete;

	size_t read()
	{
		if (!m_reader)		throw std::runtime_error("InStreambuf not initialized");

		size_t len;

		try
		{
			m_fail.clear();
			len = m_reader->read(m_recv.data(), m_recv.size());
//std::cerr << "istrm read len=" << len << std::endl;
		}
		catch (std::exception& ex)
		{
			m_fail = ex.what();
			throw ex;
		}

		return len;
	}

protected:

	/*
		Ensures that at least one character is available in the input area by updating the pointers to the input area (if needed)
		and reading more data in from the input sequence (if applicable). 
	*/
	virtual int_type underflow()	// buffer is empty, fill it
	{
		if (m_resize_recv)
		{
			m_recv.resize(m_resize_recv);
			m_resize_recv = 0;
		}

		size_t len = read();
//std::cerr << "istrm underflow: read len=" << len << std::endl;

		if (!len)
		{
			return traits_type::eof();			// signal eof
		}

		// set the get pointer to beginning, len data
		setg((char*)m_recv.data(), (char*)m_recv.data(), (char*)m_recv.data() + len);

		return traits_type::to_int_type(m_recv[0]);	// return the first char in the buffer
	}
	
	/*
		For input streams, this typically empties the get area and forces a re-read from the associated sequence to pick up recent changes
		
		For output streams, this typically results in writing the contents of the put area into the associated sequence, 
		i.e. flushing of the output buffer.
	*/
	virtual int sync()
	{
//		size_t curp = gptr()-eback(), endp = egptr()-eback();
//std::cerr << "istrm sync, cur: " << curp << " end: " << endp << std::endl;
//		setg((char*)m_recv.data(), (char*)m_recv.data(), (char*)m_recv.data());		// empty the get area, if there was any data it is lost

	/*
		it looks like the correct behavior here is to do nothing. Devices like files with a single file pointer would have different behavior,
		but doing anything here messes up sockets, character rw buffer.
	*/
		return 0;
	}

public:
	InStreambuf(Reader& reader, size_t recv_buf_sz) : m_reader(&reader), m_resize_recv(0)
	{
		m_recv.resize(recv_buf_sz);

		// set the get pointer to beginning, no data
		setg((char*)m_recv.data(), (char*)m_recv.data(), (char*)m_recv.data());
	}
	InStreambuf(const std::shared_ptr<Reader>& reader, size_t recv_buf_sz) : m_shared(reader), m_reader(m_shared.get()), m_resize_recv(0)
	{
		m_recv.resize(recv_buf_sz);

		// set the get pointer to beginning, no data
		setg((char*)m_recv.data(), (char*)m_recv.data(), (char*)m_recv.data());
	}
	~InStreambuf() {}

	void read_reset(const std::shared_ptr<Reader>& reader)
	{
		m_shared = reader;
		m_reader = m_shared.get();
	}

	std::shared_ptr<Reader> read_shared() const
	{
		return m_shared;
	}

	size_t recvbuf_size() const
	{
		if (m_resize_recv)		return m_resize_recv;		// in case we have not underflowed yet

		return m_recv.size();
	}

	void recvbuf_size(size_t sz)
	{
		if (sz == 0)			throw std::runtime_error("input stream empty buffer");

		m_resize_recv = sz;		// will resize the next time we underflow (after all the buffered data is consumed)
								// this ensures that the buffer can be extended or reduced safely regardless of buffer contents
	}

	std::string recv_fail() const
	{
		return m_fail;
	}
	void clear_recv_fail()
	{
		m_fail.clear();
	}
};

class OutStreambuf : public virtual std::streambuf
{
	std::shared_ptr<Writer> m_shared;
	Writer* m_writer;
	std::vector<char> m_send;
	std::string m_fail;

	OutStreambuf(const OutStreambuf&) = delete;
	void operator=(const OutStreambuf&) = delete;

	bool send(size_t len)
	{
		if (!m_writer)		throw std::runtime_error("OutStreambuf not initialized");

//std::cerr << "ostrm send len=" << len << std::endl;
		if (!len)			return true;

		size_t left = len;
		char* loc = (char*)m_send.data();
		
		while (left)
		{
			size_t sent;

			try
			{
				m_fail.clear();
//std::cerr << "ostrm write left=" << left << std::endl;
				sent = m_writer->write(loc, left);

				if (sent == 0)
				{
					return false;		// eof condition
				}
			}
			catch (std::exception& ex)
			{
				m_fail = ex.what();
				throw ex;
			}

			left -= sent;
			loc += sent;
		}

		// set put area to the full buffer
		setp((char*)m_send.data(), (char*)m_send.data() + m_send.size() - 1);

		return true;
	}

protected:

	/*
		Ensures that there is space at the put area for at least one character by saving some initial
		subsequence of characters starting at pbase() to the output sequence and updating the pointers to the put area (if needed). 
	*/
	virtual int_type overflow(int_type c)  // buffer is full, send it
	{
		if (traits_type::eq_int_type(traits_type::eof(), c))	// got an eof, don't send anything
		{
//std::cerr << "ostrm overflow: eof" << std::endl;
			return c;
		}

//std::cerr << "ostrm overflow char=" << (isprint(c)?(char)c:'.') << " sending len=" << m_send.size() << std::endl;
		traits_type::assign(*pptr(), c);  // store the last character in the buffer

		if (!send(m_send.size()))
		{
			return traits_type::eof();
		}

		return c;
	}

	/*
		For input streams, this typically empties the get area and forces a re-read from the associated sequence to pick up recent changes
		
		For output streams, this typically results in writing the contents of the put area into the associated sequence, 
		i.e. flushing of the output buffer.
	*/
	virtual int sync()
	{
		// flush the buffer
		size_t len = pptr() - pbase();
//std::cerr << "ostrm sync len=" << len << std::endl;
		if (!send(len))
		{
			return -1;
		}
		return 0;
	}

public:
	OutStreambuf(Writer& writer, size_t send_buf_sz) : m_writer(&writer)
	{
		m_send.resize(send_buf_sz);

		// set put area to full buffer
		setp((char*)m_send.data(), (char*)m_send.data() + m_send.size() - 1);
	}
	OutStreambuf(const std::shared_ptr<Writer>& writer, size_t send_buf_sz) : m_shared(writer), m_writer(m_shared.get())
	{
		m_send.resize(send_buf_sz);

		// set put area to full buffer
		setp((char*)m_send.data(), (char*)m_send.data() + m_send.size() - 1);
	}
	~OutStreambuf() {}

	void write_reset(const std::shared_ptr<Writer>& writer)
	{
		m_shared = writer;
		m_writer = m_shared.get();
	}

	std::shared_ptr<Writer> write_shared() const
	{
		return m_shared;
	}

	size_t sendbuf_size() const
	{
		return m_send.size();
	}

	void sendbuf_size(size_t sz)
	{
		if (sz == 0)			throw std::runtime_error("output stream empty buffer");

		sync();				 // send any partial data
		
		m_send.resize(sz);

		// set put area to full buffer
		setp((char*)m_send.data(), (char*)m_send.data() + m_send.size() - 1);
	}

	std::string send_fail() const
	{
		return m_fail;
	}
	void clear_send_fail()
	{
		m_fail.clear();
	}
};

class IoStreambuf : public InStreambuf, public OutStreambuf
{

public:
	IoStreambuf(Reader& reader, Writer& writer, size_t recv_buf_sz, size_t send_buf_sz)
		: InStreambuf(reader, recv_buf_sz), OutStreambuf(writer, send_buf_sz)
	{
	}
	IoStreambuf(const std::shared_ptr<Reader>& reader, const std::shared_ptr<Writer>& writer, size_t recv_buf_sz, size_t send_buf_sz)
		: InStreambuf(reader, recv_buf_sz), OutStreambuf(writer, send_buf_sz)
	{
	}
	~IoStreambuf() {}

	virtual int sync()
	{
		if (InStreambuf::sync() == -1)		return -1;
		if (OutStreambuf::sync() == -1)		return -1;
		return 0;
	}
};

InStream::InStream(Reader& reader, size_t recv_buf_sz)
{
	if (recv_buf_sz == 0)			throw std::runtime_error("InStream empty buffer");

	rdbuf(new InStreambuf(reader, recv_buf_sz));
}

InStream::InStream(const std::shared_ptr<Reader>& reader, size_t recv_buf_sz)
{
	if (!reader)					throw std::runtime_error("InStream null pointer");

	if (recv_buf_sz == 0)			throw std::runtime_error("InStream empty buffer");

	rdbuf(new InStreambuf(reader, recv_buf_sz));
}

InStream::~InStream()
{
	delete rdbuf();
}

void InStream::read_reset(const std::shared_ptr<Reader>& reader)
{
	if (!reader)					throw std::runtime_error("InStream reset with null shared");

	InStreambuf* sb = dynamic_cast<InStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->read_reset(reader);
}

std::shared_ptr<Reader> InStream::read_shared() const
{
	InStreambuf* sb = dynamic_cast<InStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->read_shared();
}

InStream::InStream(InStream&& other)
{
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
}

InStream& InStream::operator=(InStream&& other)
{
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
	return *this;
}

size_t InStream::recvbuf_size() const
{
	InStreambuf* sb = dynamic_cast<InStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->recvbuf_size();
}

void InStream::recvbuf_size(size_t sz)
{
	if (sz == 0)			throw std::runtime_error("InStream empty buffer");

	InStreambuf* sb = dynamic_cast<InStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->recvbuf_size(sz);
}

std::string InStream::recv_fail() const
{
	InStreambuf* sb = dynamic_cast<InStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->recv_fail();
}

void InStream::clear(std::ios::iostate state)
{
	std::istream::clear(state);

	if (state != std::ios::goodbit)				return;		// only clear if the stream is reset

	InStreambuf* sb = dynamic_cast<InStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->clear_recv_fail();
}

OutStream::OutStream(Writer& writer, size_t send_buf_sz)
{
	if (send_buf_sz == 0)			throw std::runtime_error("OutStream empty buffer");

	rdbuf(new OutStreambuf(writer, send_buf_sz));
}

OutStream::OutStream(const std::shared_ptr<Writer>& writer, size_t send_buf_sz)
{
	if (!writer)					throw std::runtime_error("OutStream null pointer");

	if (send_buf_sz == 0)			throw std::runtime_error("OutStream empty buffer");

	rdbuf(new OutStreambuf(writer, send_buf_sz));
}

OutStream::~OutStream()
{
	delete rdbuf();
}

void OutStream::write_reset(const std::shared_ptr<Writer>& writer)
{
	if (!writer)					throw std::runtime_error("OutStream reset with null shared");

	OutStreambuf* sb = dynamic_cast<OutStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->write_reset(writer);
}

std::shared_ptr<Writer> OutStream::write_shared() const
{
	OutStreambuf* sb = dynamic_cast<OutStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->write_shared();
}

OutStream::OutStream(OutStream&& other)
{
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
}

OutStream& OutStream::operator=(OutStream&& other)
{
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
	return *this;
}

size_t OutStream::sendbuf_size() const
{
	OutStreambuf* sb = dynamic_cast<OutStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->sendbuf_size();
}

void OutStream::sendbuf_size(size_t sz)
{
	OutStreambuf* sb = dynamic_cast<OutStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->sendbuf_size(sz);
}

std::string OutStream::send_fail() const
{
	OutStreambuf* sb = dynamic_cast<OutStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->send_fail();
}

void OutStream::clear(std::ios::iostate state)
{
	std::ostream::clear(state);

	if (state != std::ios::goodbit)				return;		// only clear if the stream is reset

	OutStreambuf* sb = dynamic_cast<OutStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->clear_send_fail();
}

IoStream::IoStream(Reader& reader, Writer& writer, size_t recv_buf_sz, size_t send_buf_sz)
{
	if (send_buf_sz == 0 || recv_buf_sz == 0)			throw std::runtime_error("IoStream empty buffer");

	rdbuf(new IoStreambuf(reader, writer, recv_buf_sz, send_buf_sz));
}

IoStream::IoStream(const std::shared_ptr<Reader>& reader, const std::shared_ptr<Writer>& writer, size_t recv_buf_sz, size_t send_buf_sz)
{
	if (!reader || !writer)								throw std::runtime_error("IoStream null pointer(s)");

	if (send_buf_sz == 0 || recv_buf_sz == 0)			throw std::runtime_error("IoStream empty buffer");

	rdbuf(new IoStreambuf(reader, writer, recv_buf_sz, send_buf_sz));
}

IoStream::~IoStream()
{
	delete rdbuf();
}

void IoStream::read_reset(const std::shared_ptr<Reader>& reader)
{
	if (!reader)					throw std::runtime_error("IoStream read reset with null shared");

	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->read_reset(reader);
}

void IoStream::write_reset(const std::shared_ptr<Writer>& writer)
{
	if (!writer)					throw std::runtime_error("IoStream write reset with null shared");

	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->write_reset(writer);
}

std::shared_ptr<Reader> IoStream::read_shared() const
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->read_shared();
}

std::shared_ptr<Writer> IoStream::write_shared() const
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->write_shared();
}

IoStream::IoStream(IoStream&& other)
{
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
}

IoStream& IoStream::operator=(IoStream&& other)
{
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
	return *this;
}

size_t IoStream::recvbuf_size() const
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->recvbuf_size();
}

size_t IoStream::sendbuf_size() const
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->sendbuf_size();
}

void IoStream::recvbuf_size(size_t sz)
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->recvbuf_size(sz);
}

void IoStream::sendbuf_size(size_t sz)
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->sendbuf_size(sz);
}

std::string IoStream::recv_fail() const
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->recv_fail();
}

std::string IoStream::send_fail() const
{
	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	return sb->send_fail();
}

void IoStream::clear(std::ios::iostate state)
{
	std::iostream::clear(state);

	if (state != std::ios::goodbit)				return;		// only clear if the stream is reset

	IoStreambuf* sb = dynamic_cast<IoStreambuf*>(rdbuf());
	if (sb == nullptr)
	{
		throw std::runtime_error("invalid streambuffer");
	}
	sb->clear_send_fail();
	sb->clear_recv_fail();
}

///
/// timers
///

ReadTimer::ReadTimer() : m_reader(nullptr), m_ticks{0}, m_calls{0}
{
}

ReadTimer::ReadTimer(Reader& r) : m_reader(nullptr), m_ticks{0}, m_calls{0}
{
	read_reset(r);
}

ReadTimer::ReadTimer(const std::shared_ptr<Reader>& r) : m_reader(nullptr), m_ticks{0}, m_calls{0}
{
	read_reset(r);
}

size_t ReadTimer::read(void* loc, size_t len)
{
	if (!m_reader)			return 0;

	auto t1 = std::chrono::high_resolution_clock::now();
	size_t rd = m_reader->read(loc, len);
	auto t2 = std::chrono::high_resolution_clock::now();
	m_ticks += std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
	m_calls++;
	return rd;
}

void ReadTimer::read_reset(Reader& r)
{
	m_shared.reset();
	m_reader = &r;
	m_ticks = 0;
	m_calls = 0;
}

void ReadTimer::read_reset(const std::shared_ptr<Reader>& r)
{
	if (!r)
	{
		throw std::runtime_error("ReadTimer reset with null pointer");
	}
	m_shared = r;
	m_reader = m_shared.get();
	m_ticks = 0;
	m_calls = 0;
}

WriteTimer::WriteTimer() : m_writer(0), m_ticks{0}, m_calls{0}
{
}

WriteTimer::WriteTimer(Writer& w) : m_writer(0), m_ticks{0}, m_calls{0}
{
	write_reset(w);
}

WriteTimer::WriteTimer(const std::shared_ptr<Writer>& w) : m_writer(0), m_ticks{0}, m_calls{0}
{
	write_reset(w);
}

size_t WriteTimer::write(const void* loc, size_t len)
{
	if (!m_writer)			return 0;

	auto t1 = std::chrono::high_resolution_clock::now();
	size_t wr = m_writer->write(loc, len);
	auto t2 = std::chrono::high_resolution_clock::now();
	m_ticks += std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
	m_calls++;
	return wr;
}

void WriteTimer::write_reset(Writer& w)
{
	m_shared.reset();
	m_writer = &w;
	m_ticks = 0;
	m_calls = 0;
}
void WriteTimer::write_reset(const std::shared_ptr<Writer>& w)
{
	if (!w)
	{
		throw std::runtime_error("WriteTimer reset with null pointer");
	}
	m_shared = w;
	m_writer = m_shared.get();
	m_ticks = 0;
	m_calls = 0;
}

RwTimer::RwTimer() : ReadTimer(), WriteTimer()
{
}

RwTimer::RwTimer(Reader& r, Writer& w) : ReadTimer(r), WriteTimer(w)
{
}

RwTimer::RwTimer(const std::shared_ptr<Reader>& r, const std::shared_ptr<Writer>& w) : ReadTimer(r), WriteTimer(w)
{
}

///
/// counters
///

ReadCounter::ReadCounter() : m_reader(nullptr), m_count{0}, m_calls{0}
{
}

ReadCounter::ReadCounter(Reader& r) : m_reader(nullptr), m_count{0}, m_calls{0}
{
	read_reset(r);
}

ReadCounter::ReadCounter(const std::shared_ptr<Reader>& r) : m_reader(nullptr), m_count{0}, m_calls{0}
{
	read_reset(r);
}

size_t ReadCounter::read(void* loc, size_t len)
{
	if (!m_reader)		return 0;

	size_t rd = m_reader->read(loc, len);
	m_count += rd;
	m_calls++;
	return rd;
}

void ReadCounter::read_reset(Reader& r)
{
	m_shared.reset();
	m_reader = &r;
	m_count = 0;
	m_calls = 0;
}

void ReadCounter::read_reset(const std::shared_ptr<Reader>& r)
{
	if (!r)
	{
		throw std::runtime_error("ReadCounter reset with null pointer");
	}
	m_shared = r;
	m_reader = m_shared.get();
	m_count = 0;
	m_calls = 0;
}

WriteCounter::WriteCounter() : m_writer(nullptr), m_count{0}, m_calls{0}
{
}

WriteCounter::WriteCounter(Writer& w) : m_writer(nullptr), m_count{0}, m_calls{0}
{
	write_reset(w);
}

WriteCounter::WriteCounter(const std::shared_ptr<Writer>& w) : m_writer(nullptr), m_count{0}, m_calls{0}
{
	write_reset(w);
}

size_t WriteCounter::write(const void* loc, size_t len)
{
	if (!m_writer)			return 0;

	size_t wr = m_writer->write(loc, len);
	m_count += wr;
	m_calls++;
	return wr;
}

void WriteCounter::write_reset(Writer& w)
{
	m_shared.reset();
	m_writer = &w;
	m_count = 0;
	m_calls = 0;
}

void WriteCounter::write_reset(const std::shared_ptr<Writer>& w)
{
	if (!w)
	{
		throw std::runtime_error("WriteCounter reset with null pointer");
	}
	m_shared = w;
	m_writer = m_shared.get();
	m_count = 0;
	m_calls = 0;
}

RwCounter::RwCounter() : ReadCounter(), WriteCounter()
{
}

RwCounter::RwCounter(Reader& r, Writer& w) : ReadCounter(r), WriteCounter(w)
{
}

RwCounter::RwCounter(const std::shared_ptr<Reader>& r, const std::shared_ptr<Writer>& w) : ReadCounter(r), WriteCounter(w)
{
}

///
/// rw loop buffer
///

RwLoopBuffer::RwLoopBuffer() : m_idx(0)
{
}

///
/// pipelines
///

InChain::InChain(const std::shared_ptr<Reader>& r)
	: rd_base(r)
{}

void InChain::rd_del(const std::shared_ptr<PipelineReader>& r)
{
	auto it = std::find_if(rd_chain.begin(), rd_chain.end(), [&r](auto& v) { return v == r; });

	if (it != rd_chain.end())		rd_chain.erase(it);

	rd_fix_chain();
}

std::shared_ptr<Reader> InChain::rd_fix_chain()
{
	if (rd_chain.empty())
	{
		return rd_base;
	}

	std::shared_ptr<PipelineReader> last;

	for (auto it = rd_chain.rbegin(); it != rd_chain.rend(); ++it)
	{
		if (it == rd_chain.rbegin())		(*it)->read_reset(rd_base);		// last points to base
		else								(*it)->read_reset(last);		// points to next in pipeline
		
		last = *it;
	}
	return last;		// last is first
}

OutChain::OutChain(const std::shared_ptr<Writer>& w)
	: wr_base(w)
{}

void OutChain::wr_del(const std::shared_ptr<PipelineWriter>& w)
{
	auto it = std::find_if(wr_chain.begin(), wr_chain.end(), [&w](auto& v) { return v == w; });

	if (it != wr_chain.end())		wr_chain.erase(it);

	wr_fix_chain();
}

std::shared_ptr<Writer> OutChain::wr_fix_chain()
{
	if (wr_chain.empty())
	{
		return wr_base;
	}

	std::shared_ptr<PipelineWriter> last;

	for (auto it = wr_chain.rbegin(); it != wr_chain.rend(); ++it)
	{
		if (it == wr_chain.rbegin())		(*it)->write_reset(wr_base);	// last points to base
		else								(*it)->write_reset(last);		// points to next in pipeline
		
		last = *it;
	}
	return last;		// last is first
}

InPipeline::InPipeline(const std::shared_ptr<Reader>& r, size_t rd_bufsz) : InChain(r), InStream(r, rd_bufsz)
{}

std::shared_ptr<Reader> InPipeline::rd_fix_chain()
{
	auto first = InChain::rd_fix_chain();
	read_reset(first);						// reset my stream class to the first in the chain
	return first;
}

OutPipeline::OutPipeline(const std::shared_ptr<Writer>& w, size_t wr_bufsz) : OutChain(w), OutStream(w, wr_bufsz)
{}

std::shared_ptr<Writer> OutPipeline::wr_fix_chain()
{
	auto first = OutChain::wr_fix_chain();
	write_reset(first);						// reset my stream class to the first in the chain
	return first;
}

IoPipeline::IoPipeline(const std::shared_ptr<Reader>& r, const std::shared_ptr<Writer>& w, size_t rdbuf, size_t wrbuf)
	: InChain(r), OutChain(w), IoStream(r, w, rdbuf, wrbuf)
{}

std::shared_ptr<Reader> IoPipeline::rd_fix_chain()
{
	auto first = InChain::rd_fix_chain();
	read_reset(first);						// reset my stream class to the first in the chain
	return first;
}

std::shared_ptr<Writer> IoPipeline::wr_fix_chain()
{
	auto first = OutChain::wr_fix_chain();
	write_reset(first);						// reset my stream class to the first in the chain
	return first;
}
