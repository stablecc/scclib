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
#include "util/poller.h"
#include <sys/epoll.h>
#include <system_error>
#include <sstream>
#include "util/safe_clib.h"

/** \addtogroup util_poller
	@{ */
/** \ref util_poller implementation \file */
/** @} */

using namespace scc::util;

Poller::Poller()
{
	m_fd = epoll_create1(0);
}

Poller::~Poller()
{
	safe_close(m_fd);
}

void Poller::set(int fd, int flags)
{
	epoll_event ev;
	int  eflags = 0;

	if (flags & Poller::input)
	{
		eflags |= EPOLLIN;
	}
	if (flags & Poller::output)
	{
		eflags |= EPOLLOUT;
	}
	if (flags & Poller::read_hup)
	{
		eflags |= EPOLLRDHUP;
	}
	if (flags & Poller::priority)
	{
		eflags |= EPOLLPRI;
	}

	if (eflags == 0)
	{
		throw std::runtime_error("No poll events specified");
	}

	ev.events = eflags;
	ev.data.fd = fd;

	if (m_polls.find(fd) == m_polls.end())
	{
		if (epoll_ctl(m_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			throw std::system_error(errno, std::system_category());
		}
	}
	else
	{
		if (epoll_ctl(m_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
		{
			throw std::system_error(errno, std::system_category());
		}
	}
	m_polls[fd] = eflags;
}

void Poller::remove(int fd)
{
	if (m_polls.find(fd) != m_polls.end())
	{
		if (epoll_ctl(m_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
		{
			throw std::system_error(errno, std::system_category());
		}
		m_polls.erase(fd);
	}
}

void Poller::wait(int timeout_ms)
{
	epoll_event evs[m_polls.size()];
	int n;
	do
	{
		n = epoll_wait(m_fd, evs, m_polls.size(), timeout_ms);
	}
	while (n == -1 && errno == EINTR);
	if (n == -1)
	{
		std::stringstream st;
		st << "epoll_wait()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	m_events.clear();
	for (int i = 0; i < n; i++)
	{
		if (m_polls.find(evs[i].data.fd) == m_polls.end())
		{
			throw std::runtime_error("invalid poll event returned");
		}
		m_events[evs[i].data.fd] = converteflags(evs[i].events);
	}
}

int Poller::converteflags(int flags)
{
	int r = 0;

	if (flags & EPOLLIN)
	{
		r |= Poller::input;
	}
	if (flags & EPOLLOUT)
	{
		r |= Poller::output;
	}
	if (flags & EPOLLRDHUP)
	{
		r |= Poller::read_hup;
	}
	if (flags & EPOLLPRI)
	{
		r |= Poller::priority;
	}
	if (flags & EPOLLHUP)
	{
		r |= Poller::hup;
	}
	if (flags & EPOLLERR)
	{
		r |= Poller::error;
	}
	
	return r;
}

int Poller::event(int fd)
{
	auto e = m_events.find(fd);
	if (e == m_events.end())
	{
		return 0;
	}
	return e->second;
}

