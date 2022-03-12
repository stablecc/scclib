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
#include <util/safe_clib.h>
#include <system_error>
#include <sstream>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstdio>

/** \addtogroup util_safeclib
	@{ */
/** \ref util_safeclib implementation \file */
/** @} */

using namespace scc::util;

int scc::util::safe_truncate(const char *path, off_t length)
{
	int r;
	do
	{
		r = ::truncate(path, length);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_truncate_throw(const char *path, off_t length)
{
	int r = truncate(path, length);
	if (r == -1)
	{
		std::stringstream st;
		st << "truncate(" << path << "," << length << ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_ftruncate(int fd, off_t length)
{
	int r;
	do
	{
		r = ::ftruncate(fd, length);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_ftruncate_throw(int fd, off_t length)
{
	int r = ftruncate(fd, length);
	if (r == -1)
	{
		std::stringstream st;
		st << "ftruncate(" << fd << "," << length << ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_close(int fd)
{
	int r;
	do
	{
		r = ::close(fd);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_close_throw(int fd)
{
	int r = safe_close(fd);
	if (r == -1)
	{
		std::stringstream st;
		st << "close(" << fd << ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

ssize_t scc::util::safe_read(int fd, void *buf, size_t count)
{
	ssize_t r;
	do
	{
		r = ::read(fd, buf, count);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

ssize_t scc::util::safe_read_throw(int fd, void *buf, size_t count)
{
	ssize_t r = safe_read(fd, buf, count);
	if (r == -1)
	{
		std::stringstream st;
		st << "read(" << fd << ", ...)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

ssize_t scc::util::safe_write(int fd, const void *buf, size_t count)
{
	ssize_t r;
	do
	{
		r = ::write(fd, buf, count);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

ssize_t scc::util::safe_write_throw(int fd, const void *buf, size_t count)
{
	ssize_t r = safe_write(fd, buf, count);
	if (r == -1)
	{
		std::stringstream st;
		st << "write(" << fd << ", ...)";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_dup(int oldfd)
{
	int r;
	do
	{
		r = ::dup(oldfd);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_dup_throw(int oldfd)
{
	int r = safe_dup(oldfd);
	if (r == -1)
	{
		std::stringstream st;
		st << "dup(" << oldfd << ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_dup2(int oldfd, int newfd)
{
	int r;
	do
	{
		r = ::dup2(oldfd, newfd);
	}
	while (r == -1 && (errno == EINTR||errno == EBUSY));
	return r;
}

int scc::util::safe_dup2_throw(int oldfd, int newfd)
{
	int r = safe_dup2(oldfd, newfd);
	if (r == -1)
	{
		std::stringstream st;
		st << "dup2(" << oldfd << "," << newfd << ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_open(const char *pathname, int flags)
{
	int r;
	do
	{
		r = ::open(pathname, flags);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_open_throw(const char *pathname, int flags)
{
	int r = safe_open(pathname, flags);
	if (r == -1)
	{
		std::stringstream st;
		st << "open(" << pathname << "," << flags << ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_open(const char *pathname, int flags, mode_t mode)
{
	int r;
	do
	{
		r = ::open(pathname, flags, mode);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_open_throw(const char *pathname, int flags, mode_t mode)
{
	int r = safe_open(pathname, flags, mode);
	if (r == -1)
	{
		std::stringstream st;
		st << "open(" << pathname << "," << flags << "," << mode <<  ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

FILE* scc::util::safe_fopen(const char *pathname, const char *mode)
{
	FILE* r;
	do
	{
		r = ::fopen(pathname, mode);
	}
	while (r == nullptr && errno == EINTR);
	return r;
}

FILE* scc::util::safe_fopen_throw(const char *pathname, const char *mode)
{
	FILE* r = safe_fopen(pathname, mode);
	if (r == nullptr)
	{
		std::stringstream st;
		st << "fopen(" << pathname << "," << mode << ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_fclose(FILE* stream)
{
	int r;
	do
	{
		r = ::fclose(stream);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_fclose_throw(FILE* stream)
{
	int r = safe_fclose(stream);
	if (r == -1)
	{
		std::stringstream st;
		st << "fclose()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

static int safe_vfscanf(FILE *stream, const char *format, va_list ap)
{
	int r;
	do
	{
		if (ferror(stream) && errno == EINTR)
		{
			clearerr(stream);
		}
		r = ::vfscanf(stream, format, ap);
	}
	while (r == EOF && ferror(stream) && errno == EINTR);
	return r;
}

int scc::util::safe_fscanf(FILE *stream, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	int r = safe_vfscanf(stream, format, va);
	va_end(va);
	return r;
}

int scc::util::safe_fscanf_throw(FILE *stream, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	int r = safe_vfscanf(stream, format, va);
	va_end(va);
	if (r == EOF && ferror(stream))
	{
		std::stringstream st;
		st << "fscanf()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

ssize_t scc::util::safe_getline(char **lineptr, size_t *n, FILE *stream)
{
	ssize_t r;
	do
	{
		r = ::getline(lineptr, n, stream);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

ssize_t scc::util::safe_getline_throw(char **lineptr, size_t *n, FILE *stream)
{
	ssize_t r = safe_getline(lineptr, n, stream);
	if (r == -1)
	{
		std::stringstream st;
		st << "getline()";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}

int scc::util::safe_wait(int *wstatus)
{
	return safe_waitpid(-1, wstatus, 0);
}

int scc::util::safe_waitpid(pid_t pid, int *wstatus, int options)
{
	int r;
	do
	{
		r = ::waitpid(pid, wstatus, options);
	}
	while (r == -1 && errno == EINTR);
	return r;
}

int scc::util::safe_wait_throw(int *wstatus)
{
	return safe_waitpid_throw(-1, wstatus, 0);
}

int scc::util::safe_waitpid_throw(pid_t pid, int *wstatus, int options)
{
	int r = safe_waitpid(pid, wstatus, options);
	if (r == -1)
	{
		std::stringstream st;
		st << "waitpid(" << pid <<  ")";
		throw std::system_error(errno, std::system_category(), st.str());
	}
	return r;
}
