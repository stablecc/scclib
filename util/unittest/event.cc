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
#include <gtest/gtest.h>
#include <cstring>
#include <iostream>
#include "util/event.h"
#include <system_error>
#include <cerrno>

using std::cout;
using std::endl;
using scc::util::Event;

/** \addtogroup util_event
	@{ */
/** Tests for \ref util_event \file */
/** \example util/unittest/event.cc */
/** @} */

TEST(event_test, blocking)
{
	//! [Blocking event]
	Event ev;
	ev.write(1);
	ev.write(2);
	ASSERT_EQ(ev.read(), 3);
	//! [Blocking event]
}

TEST(event_test, nonblocking)
{
	//! [Non-blocking event]
	Event ev(Event::nonblocking);
	ev.write(1);
	ev.write(2);
	ASSERT_EQ(ev.read(), 3);
	ASSERT_THROW(ev.read(), std::system_error);
	ASSERT_EQ(errno, EAGAIN); // most recent system error was EAGAIN "unavailable"
	//! [Non-blocking event]
}

TEST(event_test, nonblocking_semaphore)
{
	//! [Non-blocking semaphore event]
	Event ev(Event::nonblocking|Event::semaphore);
	ASSERT_THROW(ev.read(), std::system_error);
	ev.write(1);
	ev.write(2);
	ASSERT_EQ(ev.read(), 1);
	ASSERT_EQ(ev.read(), 1);
	ASSERT_EQ(ev.read(), 1);
	//! [Non-blocking semaphore event]
}
