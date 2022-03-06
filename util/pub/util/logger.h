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
#ifndef _SYS_UTIL_LOGGER_H
#define _SYS_UTIL_LOGGER_H

#include <string>
#include <ostream>
#include <memory>

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \defgroup util_logger Thread safe logging
	@{
	
	Logging utility which can share std::ostream objects between multiple threads.
*/

/** Thread safe logging.
	\file
*/

/** Thread-safe stream logger.

	Logger classes may be used as generic output streams, and shared streams attached to loggers will not
	overwrite (clobber) log lines from from other threads.
	
	For example, one thread writing:

	 log line 1 from thread 1
	 log line 2 from thread 1

	and another thread writing:

	 log line 1 from thread 2
	 log line 2 from thread 2

	will result in exactly 4 log lines output to all attached streams.

	Log lines have the form 
	
	    [ID] TIMESTAMP <line>. Multiple log lines (beginning with spaces or tabs)
	
	Multiline logs (beginning with space or tab) will be consolidated into a single log line up to a maximum number
	of lines.

	Separate threads should each create their own logger (loggers themselves are not thread safe).

	Example from \ref scclib/net/unittest/logger.cc Write multithreaded logs to console and stringstream.
	\snippet scclib/net/unittest/logger.cc Multithreaded log test using multiple loggers

*/
class Logger : public std::ostream
{
protected:

	void dup(const Logger& b);

public:
	/**	Create a logger with maximum line length (default 256).

		The minimum line length does not include the prefix, if any.
	*/
	Logger(unsigned int = 256);
	virtual ~Logger();
	/**	Create a logger with an attached stream.
	*/
	Logger(const std::shared_ptr<std::ostream>& os, unsigned int ll = 256) : Logger(ll)
	{
		add(os);
	}
	/** Copy construct logger, using original streams and settings. */
	Logger(const Logger&);
	/** Copy assign logger, using original streams and settings. */
	Logger& operator=(const Logger&);
	/** Move construct logger. Original will be reset to defaults. */
	Logger(Logger&&);
	/** Move assign logger. Original will be reset to defaults. */
	Logger& operator=(Logger&&);

	/** Clear the logger and reset to defaults.
		
		Defaults:
		* enabled with no streams attached
		* no id
		* no timestamp
		* multiline logging mode off
	*/
	void clear();

	/** Add a shared stream.
	
		The stream should inherit from basic_ios (have a streambuf).
	*/
	void add(const std::shared_ptr<std::ostream>&);
	/** Remove shared stream. */
	void remove(const std::shared_ptr<std::ostream>&);

	/** Add std::cout console stream. */
	void add_cout();
	/** Remove std::cout console stream. */
	void remove_cout();
	/** Add std::clog (buffered error) console stream. */
	void add_clog();
	/** Remove std::clog (buffered error) console stream. */
	void remove_clog();
	/** Add std::cerr (unbuffered error) console stream. */
	void add_cerr();
	/** Remove std::cerr (unbuffered error) console stream. */
	void remove_cerr();

	/** Set the id.
	
		Setting to "" means do not emit an id.
	*/
	void id(const std::string& id="");
	/** Get the current id string. */
	std::string id() const;

	/** Set a numeric id. */
	void id(unsigned int i)
	{
		id(std::to_string(i));
	}

	/** Set the timestamp format to emit. Follows std::put_time format.

		Setting to "" means do not emit a timestamp.
	*/
	void timestamp(const std::string& ts="");
	/** Get the current timestamp format. */
	std::string timestamp() const;

	/** Set to a standard timestamp.
	
		Example: Oct 1 2020 12:00:00 UTC
	*/
	void timestamp_std(bool utc_on=false)
	{
		utc(utc_on);

		if (utc_on)
		{
			timestamp("%b %d %Y %T UTC");
		}
		else
		{
			timestamp("%b %d %Y %T");
		}
	}

	/** Set iso 8601 timestamp.
		
		Example: 2020-10-01T12:00:00Z
	*/
	void timestamp_iso(bool utc_on=true)
	{
		utc(utc_on);

		if (utc_on)
		{
			timestamp("%FT%TZ");
		}
		else
		{
			timestamp("%FT%T%z");
		}
	}

	/** Set utc mode. */
	void utc(bool on);
	/** Get utc mode. */
	bool utc() const;

	/** Enable or disable this stream logger. */
	void enable(bool on);
	/** Is the logger enabled? */
	bool enable() const;

	/** Set the maximum line length (default is 256).
	
		Lines that exceed this length will be emitted as separate lines.
	*/
	void max_line(unsigned int);
	/** Get the maximum line length. */
	unsigned int max_line() const;

	/**	Set multiline mode.
	
		Sets the maximum number of multilines allowed (including the first line). Setting to 0 disables multline mode.
	*/
	void multiline(unsigned int max=0);
	/** Get maximum number of multiline lines.
	
		0 means multiline mode is off.
	*/
	unsigned int multiline() const;
};

/** @} */
/** @} */
}

#endif
