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
#include <iostream>
#include "util/poller.h"
#include "util/event.h"
#include <chrono>


/** \addtogroup util_poller
	@{ */
/** Tests for \ref util_poller \file */
/** \example util/unittest/poller.cc */
/** @} */

using std::cout;
using std::endl;
using scc::util::Event;
using scc::util::Poller;

std::chrono::milliseconds zero(0);
std::chrono::milliseconds tenth(100);

TEST(Poller, timeout)
{
	Event e;
	Poller p;
	p.set(e, Poller::input);
	p.wait(tenth);
	ASSERT_EQ(p.event(e), 0);
}

TEST(Poller, write)
{
	Event e;
	Poller p;
	p.set(e, Poller::output);
	p.wait(tenth);
	ASSERT_EQ(p.event(e) & Poller::output, Poller::output);
}

TEST(Poller, read)
{
	Event e;
	Poller p;
	e.write(1);
	p.set(e, Poller::input);
	p.wait(tenth);
	ASSERT_EQ(p.event(e) & Poller::input, Poller::input);
}

TEST(Poller, Writeread)
{
	Event e;
	Poller p;
	p.set(e, Poller::input|Poller::output);

	p.wait(tenth);
	ASSERT_EQ(p.event(e) & Poller::input, 0);
	ASSERT_EQ(p.event(e) & Poller::output, Poller::output);
	e.write(1);

	p.wait(tenth);
	ASSERT_EQ(p.event(e) & Poller::input, Poller::input);
	ASSERT_EQ(p.event(e) & Poller::output, Poller::output);
}
