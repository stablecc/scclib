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
#include "util/logger.h"
#include <string>
#include <functional>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_set>
#include <cassert>
#include <mutex>
#include <system_error>

/** \addtogroup util_logger
	@{ */
/** \ref util_logger implementation \file */
/** @} */

using namespace scc::util;

std::mutex s_mx;			// some stream objects (like fstream) are not thread safe

class LogStreambuf : public std::streambuf
{
	bool m_on;
	std::string m_id;
	std::string m_ts;
	bool m_utc;
	unsigned int m_multmax;
	unsigned int m_multcur;
	unsigned int m_maxline;
	
	std::string m_line;
	
	bool m_cout;
	bool m_clog;
	bool m_cerr;

	std::unordered_set<std::shared_ptr<std::ostream>> m_strms;

	LogStreambuf(const LogStreambuf&) = delete;
	void operator=(const LogStreambuf&) = delete;

	// add a prefix and emit the line to all streams
	void emit() noexcept
	{
		if (!m_on || m_line.empty())	return;

		// output: [ID] TIMESTAMP <line>

		std::stringstream out;

		bool emit_pre;

		if (m_id.empty() && m_ts.empty())
		{
			emit_pre = false;
		}
		else
		{
			emit_pre = true;
			
			if (m_multmax > 0 && (m_line[0] == '\t' || m_line[0] == ' '))
			{
				if (++m_multcur < m_multmax)
				{
					emit_pre = false;			// inside a multiline log, don't emit prefix
				}
				else
				{
					m_multcur = 0;
				}
			}
		}

		if (emit_pre)
		{
			if (m_id.size())
			{
				out << "[" << m_id << "] ";
			}

			if (m_ts.size())
			{
				auto t = std::time(nullptr);
				struct tm tbuf;
				if (m_utc)
				{
					out << std::put_time(gmtime_r(&t, &tbuf), m_ts.c_str());
				}
				else
				{
					out << std::put_time(localtime_r(&t, &tbuf), m_ts.c_str());
				}
				out << " ";
			}
		}

		out << m_line << '\n';

		if (m_cout)
		{
			std::cout << out.str();
			std::cout.flush();
		}

		if (m_cerr)
		{
			std::cerr << out.str();
			std::cerr.flush();
		}

		if (m_clog)
		{
			std::clog << out.str();
			std::clog.flush();
		}

		std::lock_guard<std::mutex> lk(s_mx);
		for (auto os : m_strms)
		{
			*os << out.str();
			os->flush();
		}
	}

protected:

	int_type overflow(int_type c)
	{
		if (traits_type::eq_int_type(traits_type::eof(), c))
		{
			if (m_line.size())
			{
				emit();
				m_line.clear();
			}

			return c;
		}
		
		if (c == '\n')
		{
			emit();
			m_line.clear();
			return c;
		}
		
		m_line.push_back(c);
		
		if (m_line.size() == m_maxline)
		{
			emit();
			m_line.clear();
		}
		return c;
	}

	int sync()
	{
		if (m_line.size())
		{
			emit();
			m_line.clear();
		}
		return 0;
	}

public:

	LogStreambuf()
	{
		clear();
	}

	LogStreambuf(unsigned int max)
	{
		clear(max);
	}

	virtual ~LogStreambuf() {}

	void add(const std::shared_ptr<std::ostream>& os)
	{
		std::lock_guard<std::mutex> lk(s_mx);
		if (m_strms.find(os) == m_strms.end())
		{
			m_strms.insert(os);
		}
	}

	void remove(const std::shared_ptr<std::ostream>& os)
	{
		std::lock_guard<std::mutex> lk(s_mx);
		auto it = m_strms.find(os);
		if (it != m_strms.end())
		{
			m_strms.erase(it);
		}
	}

	void add_cout()
	{
		m_cout = true;
	}

	void dup(void* rdbufp)
	{
		LogStreambuf& b = *static_cast<LogStreambuf*>(rdbufp);

		m_on = b.m_on;
		m_id = b.m_id;
		m_ts =  b.m_ts;
		m_utc = b.m_utc;
		m_multmax = b.m_multmax;
		m_multcur = b.m_multcur;
		
		m_maxline = b.m_maxline;
		m_line.reserve(m_maxline);
		m_line = b.m_line;
		
		m_cout = b.m_cout;
		m_cerr = b.m_cerr;
		m_clog = b.m_clog;

		std::lock_guard<std::mutex> lk(s_mx);
		m_strms = b.m_strms;
	}

	void clear(int max = 256)
	{
	
		m_on = true;
		m_id = "";
		m_ts =  "";
		m_utc = false;
		m_multmax = 0;
		m_multcur = 0;

		m_maxline = max;
		m_line.reserve(m_maxline);
		m_line.clear();
		
		m_cout = false;
		m_cerr = false;
		m_clog = false;

		std::lock_guard<std::mutex> lk(s_mx);
		m_strms.clear();
	}

	void id(const std::string& i)
	{
		m_id = i;
	}
	std::string id() const
	{
		return m_id;
	}

	void timestamp(const std::string& ts)
	{
		m_ts = ts;
	}
	std::string timestamp() const
	{
		return m_ts;
	}

	void utc(bool on)
	{
		m_utc = on;
	}
	bool utc() const
	{
		return m_utc;
	}

	void enable(bool on)
	{
		m_on = on;
	}
	bool enable() const
	{
		return m_on;
	}

	void multiline(unsigned int max)
	{
		m_multmax = max;
		m_multcur = 0;
	}
	unsigned multiline() const
	{
		return m_multmax;
	}

	void max_line(unsigned v)
	{
		m_maxline = v;
	}
	unsigned max_line() const
	{
		return m_maxline;
	}
};

Logger::Logger(unsigned int max)
{
	if (!max)
	{
		throw std::runtime_error("logger max line length must be non-zero");
	}

	rdbuf(new LogStreambuf(max));		// each logger contains a stream buffer (which must be thread safe)
}

Logger::~Logger()
{
	delete rdbuf();
}

Logger::Logger(const Logger& b)			// copy operations must create a streambuf and duplicate
{
	rdbuf(new LogStreambuf());
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	rb->dup(b.rdbuf());
}

Logger& Logger::operator=(const Logger& b)
{
	rdbuf(new LogStreambuf());
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	rb->dup(b.rdbuf());
	return *this;
}

Logger::Logger(Logger&& b)				// move operations must create a streambuf, duplicate, and clear the original logger
{
	rdbuf(new LogStreambuf());
	LogStreambuf* rba = static_cast<LogStreambuf*>(rdbuf());
	LogStreambuf* rbb = static_cast<LogStreambuf*>(b.rdbuf());
	assert(rba);
	assert(rbb);
	rba->dup(rbb);
	rbb->clear();
}

Logger& Logger::operator=(Logger&& b)
{
	LogStreambuf* rba = static_cast<LogStreambuf*>(rdbuf());
	LogStreambuf* rbb = static_cast<LogStreambuf*>(b.rdbuf());
	assert(rba);
	assert(rbb);
	rba->dup(rbb);
	rbb->clear();
	return *this;
}

void Logger::dup(const Logger& b)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->dup(b.rdbuf());
}

void Logger::clear()
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->clear();
}

void Logger::add(const std::shared_ptr<std::ostream>& os)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->add(os);
}

void Logger::remove(const std::shared_ptr<std::ostream>& os)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->remove(os);
}

void Logger::add_cout()
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->add_cout();
}

void Logger::id(const std::string& i)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->id(i);
}

std::string Logger::id() const
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	return rb->id();
}

void Logger::timestamp(const std::string& ts)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->timestamp(ts);
}

std::string Logger::timestamp() const
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	return rb->timestamp();
}

void Logger::utc(bool on)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->utc(on);
}

bool Logger::utc() const
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	return rb->utc();
}

void Logger::enable(bool on)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->enable(on);
}

bool Logger::enable() const
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	return rb->enable();
}

void Logger::multiline(unsigned v)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->multiline(v);
}

unsigned Logger::multiline() const
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	return rb->multiline();
}

void Logger::max_line(unsigned v)
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	rb->max_line(v);
}

unsigned Logger::max_line() const
{
	LogStreambuf* rb = static_cast<LogStreambuf*>(rdbuf());
	assert(rb);
	return rb->max_line();
}
