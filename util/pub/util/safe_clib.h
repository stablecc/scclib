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
#ifndef _SCC_SYS_SAFE_CLIB_H
#define _SCC_SYS_SAFE_CLIB_H

#include <cstdio>
#include <sys/types.h>

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \defgroup util_safeclib Signal-safe C library wrapper
	@{

	Provides signal safe wrapper for commonly used C library APIs.

	Generally, APIs which are capable of EINTR error when interrupted a signal are included.

	See https://man7.org/linux/man-pages/man7/signal.7.html

	An example which uses safe clib apis to write into a sparse file,
	from \ref scclib/util/unittest/fs.cc
	\snippet scclib/util/unittest/fs.cc Sparse file
*/

/** Signal-safe C library wrapper.
	\file safe_clib.h
*/

/** Signal safe close. */
int safe_close(int fd);
/** Signal safe close, throws system_error on error. */
int safe_close_throw(int fd);
/** Signal safe read. */
ssize_t safe_read(int fd, void *buf, size_t count);
/** Signal safe read, throws system_error on error. */
ssize_t safe_read_throw(int fd, void *buf, size_t count);
/** Signal safe write. */
ssize_t safe_write(int fd, const void *buf, size_t count);
/** Signal safe write, throws system_error on error. */
ssize_t safe_write_throw(int fd, const void *buf, size_t count);
/** Signal safe dup. */
int safe_dup(int oldfd);
/** Signal safe dup, throws system_error on error. */
int safe_dup_throw(int oldfd);
/** Signal safe dup2. */
int safe_dup2(int oldfd, int newfd);
/** Signal safe dup2, throws system_error on error. */
int safe_dup2_throw(int oldfd, int newfd);

/** Signal safe open. */
int safe_open(const char *pathname, int flags);
/** Signal safe open. */
int safe_open(const char *pathname, int flags, mode_t mode);
/** Signal safe open, throws system_error on error. */
int safe_open_throw(const char *pathname, int flags);
/** Signal safe open, throws system_error on error. */
int safe_open_throw(const char *pathname, int flags, mode_t mode);

/** Signal safe fopen. */
FILE* safe_fopen(const char *pathname, const char *mode);
/** Signal safe fopen, throws system_error on error. */
FILE* safe_fopen_throw(const char *pathname, const char *mode);
/** Signal safe fclose. */
int safe_fclose(FILE* stream);
/** Signal safe fclose, throws system_error on error. */
int safe_fclose_throw(FILE* stream);
/** Signal safe fscanf. */
int safe_fscanf(FILE *stream, const char *format, ...);
/** Signal safe fscanf, throw on error. */
int safe_fscanf_throw(FILE *stream, const char *format, ...);
/** Signal safe getline. */
ssize_t safe_getline(char **lineptr, size_t *n, FILE *stream);
/** Signal safe getline, throw on error. */
ssize_t safe_getline_throw(char **lineptr, size_t *n, FILE *stream);
/** Signal safe truncate. */
int safe_truncate(const char *path, off_t length);
/** Signal safe truncate, throw on error. */
int safe_truncate_throw(const char *path, off_t length);
/** Signal safe ftruncate. */
int safe_ftruncate(int fd, off_t length);
/** Signal safe ftruncate, throw on error. */
int safe_ftruncate_throw(int fd, off_t length);

/** Signal safe wait. */
int safe_wait(int *wstatus);
/** Signal safe waitpid. */
int safe_waitpid(pid_t pid, int *wstatus, int options);
/** Signal safe wait, throws system_error on error. */
int safe_wait_throw(int *wstatus);
/** Signal safe waitpid, throws system_error on error. */
int safe_waitpid_throw(pid_t pid, int *wstatus, int options);

/** @} */
/** @} */
}

#endif
