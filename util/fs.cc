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
#include "util/fs.h"
#include <system_error>
#include <dirent.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <ios>
#include <fcntl.h>
#include <libgen.h>
#include <string>
#include <vector>
#include "util/safe_clib.h"

/** \addtogroup util_fs
	@{ */
/** \ref util_fs implementation \file */
/** @} */

const uint64_t NS=1000000000;

using namespace scc::util;

std::ostream& operator<<(std::ostream& os, FileType t)
{
	switch (t)
	{
		case FileType::reg:		os << "regular file"; break;
		case FileType::dir:		os << "directory"; break;
		case FileType::link:	os << "symbolic link"; break;
		case FileType::sock:	os << "socket"; break;
		case FileType::block:	os << "block device"; break;
		case FileType::chr:		os << "character device"; break;
		case FileType::fifo:	os << "fifo"; break;
		default:				os << "unknown"; break;
	} 
	return os;
}

std::ostream& operator<<(std::ostream& os, FileStat s)
{
	os << s.type << " sz: " << s.size << " alloc: " << s.alloc_size << " ino: " << s.inode << " (" << s.num_links << ")\n";
	os << "mode: ";
	os << (s.mode&S_ISUID ? 'u' : '-');
	os << (s.mode&S_ISGID ? 'g' : '-');
	os << (s.mode&S_ISVTX ? 's' : '-') << " ";
	os << (s.mode&S_IRUSR ? 'r' : '-');
	os << (s.mode&S_IWUSR ? 'w' : '-');
	os << (s.mode&S_IXUSR ? 'x' : '-') << " ";
	os << (s.mode&S_IRGRP ? 'r' : '-');
	os << (s.mode&S_IWGRP ? 'w' : '-');
	os << (s.mode&S_IXGRP ? 'x' : '-') << " ";
	os << (s.mode&S_IROTH ? 'r' : '-');
	os << (s.mode&S_IWOTH ? 'w' : '-');
	os << (s.mode&S_IXOTH ? 'x' : '-') << " (" << std::oct << s.mode << std::dec << ") ";
	os << "own: " << s.uid << ":" << s.gid << "\n";

	using ns = std::chrono::nanoseconds;
	using sclock = std::chrono::system_clock;
	using tpoint = std::chrono::time_point<sclock, ns>;

	tpoint p(ns(s.access_time));
	time_t t = sclock::to_time_t(p);
	os << "access: " << std::put_time(std::localtime(&t), "%D %T") << " (" << s.access_time << ")\n";
	p = tpoint(ns(s.mod_time));
	t = sclock::to_time_t(p);
	os << "modify: " << std::put_time(std::localtime(&t), "%D %T") << " (" << s.mod_time << ")\n";
	p = tpoint(ns(s.change_time));
	t = sclock::to_time_t(p);
	os << "change: " << std::put_time(std::localtime(&t), "%D %T") << " (" << s.change_time << ")";

	return os;
}

bool scc::util::default_scan_filter(const std::string& name, FileType type)
{
	if (type == FileType::dir && (name == "." || name == ".."))
	{
		return false;
	}
	return true;
}

std::map<std::string, FileType> Filesystem::scan_dir(const std::string& dirname,
	std::function<bool(const std::string&, FileType)> filter, std::system_error* err)
{
	std::map<std::string, FileType> ret;

	DIR* d = opendir(dirname.c_str());
	if (d == nullptr)
	{
		if (errno == ENOENT || errno == ENOTDIR)
		{
			return ret;
		}
		auto e = std::system_error(errno, std::system_category(), "scan_dir()");
		if (err)
		{
			*err = e;
			return ret;
		}
		throw e;
	}

	errno = 0;
	dirent* ent;
	while ((ent = readdir(d)))
	{
		FileType type = FileType::unknown;
		switch (ent->d_type)
		{
			case DT_REG:	type = FileType::reg; break;
			case DT_DIR:	type = FileType::dir; break;
			case DT_LNK:	type = FileType::link; break;
			case DT_SOCK:	type = FileType::sock; break;
			case DT_BLK:	type = FileType::block; break;
			case DT_CHR:	type = FileType::chr; break;
			case DT_FIFO:	type = FileType::fifo; break;
		} 

		if (filter(ent->d_name, type))
		{
			ret[ent->d_name] = type;
		}
	}
	if (errno)
	{
		closedir(d);
		auto e = std::system_error(errno, std::system_category(), "scan_dir()");
		if (err)
		{
			*err = e;
			return ret;
		}
		throw e;
	}
	closedir(d);

	return ret;
}

FileType Filesystem::file_type(const std::string& name, std::system_error* err)
{
	FileType type = FileType::unknown;
	struct stat st;

	if (lstat(name.c_str(), &st) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "file_type()");
		if (err)
		{
			*err = e;
			return type;
		}
		throw e;
	}

	switch (st.st_mode & S_IFMT)
	{
		case S_IFREG:	type = FileType::reg; break;
		case S_IFDIR:	type = FileType::dir; break;
		case S_IFLNK:	type = FileType::link; break;
		case S_IFSOCK:	type = FileType::sock; break;
		case S_IFBLK:	type = FileType::block; break;
		case S_IFCHR:	type = FileType::chr; break;
		case S_IFIFO:	type = FileType::fifo; break;
	}

	return type;
}

void Filesystem::remove_dir(const std::string& dirn, std::system_error* err)
{
	if (rmdir(dirn.c_str()) == -1)
	{
		if (err) *err = std::system_error(errno, std::system_category(), "remove_dir()");
		return;
	}
}

void Filesystem::remove_file(const std::string& name, std::system_error* err)
{
	if (unlink(name.c_str()) == -1)
	{
		if (err) *err = std::system_error(errno, std::system_category(), "remove_file()");
	}
}

void Filesystem::remove(const std::string& name, std::system_error* err)
{
	auto ty = file_type(name, err);
	if (ty == FileType::unknown)
	{
		return;
	}
	if (ty == FileType::dir)
	{
		remove_dir(name, err);
		return;
	}
	remove_file(name, err);
}

// Recursive.
void Filesystem::remove_all(const std::string& name, const FileType& ty, std::system_error* err)
{
	if (ty == FileType::dir)
	{
		for (auto& f : scan_dir(name))
		{
			std::stringstream newf;
			newf << name << "/" << f.first;
			remove_all(newf.str(), f.second, err);
		}
	}
	if (ty == FileType::dir)
	{
		remove_dir(name, err);
	}
	else
	{
		remove_file(name, err);
	}
}

void Filesystem::rename(const std::string& old_fn, const std::string& new_fn, std::system_error* err)
{
	if (::rename(old_fn.c_str(), new_fn.c_str()) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "rename()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

void Filesystem::create_dir(const std::string& name, std::system_error* err)
{
	if (mkdir(name.c_str(), 0700) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "create_dir()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

void Filesystem::create_reg(const std::string& name, std::system_error* err)
{
	int fd = safe_open(name.c_str(), O_WRONLY|O_CREAT, 0600);
	if (fd == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "create_reg()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
	safe_close(fd);
}

std::string Filesystem::create_tmp_reg(const std::string& prefix, std::system_error* err)
{
	char nam[prefix.size()+7];
	strcpy(nam, prefix.c_str());
	strcpy(nam+prefix.size(), "XXXXXX");
	int fd;
	do
	{
		fd = ::mkstemp(nam);
	}
	while (fd == -1 && errno == EINTR);
	if (fd == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "create_tmp_reg()");
		if (err)
		{
			*err = e;
			return "";
		}
		throw e;
	}
	safe_close(fd);
	return std::string(nam);
}

void Filesystem::create_symlink(const std::string& target, const std::string& name, std::system_error* err)
{
	if (symlink(target.c_str(), name.c_str()) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "create_symlink()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

std::string Filesystem::read_symlink(const std::string& name, std::system_error* err)
{
	char buf[NAME_MAX+1];
	memset(&buf[0], '\0', NAME_MAX+1);

	auto ret = readlink(name.c_str(), buf, NAME_MAX);
	if (ret == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "read_symlink()");
		if (err)
		{
			*err = e;
			return "";
		}
		throw e;
	}
	return std::string(buf);
}

void Filesystem::create_link(const std::string& orig_name, const std::string& new_name, std::system_error* err)
{
	if (link(orig_name.c_str(), new_name.c_str()) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "create_link()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

void Filesystem::create_fifo(const std::string& name, std::system_error* err)
{
	if (mkfifo(name.c_str(), 0600) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "create_fifo()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

FileStat Filesystem::file_stat(const std::string& name, std::system_error* err)
{
	FileStat fs;
	struct stat st;
	if (lstat(name.c_str(), &st) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "file_stat()");
		if (err)
		{
			*err = e;
			return fs;
		}
		throw e;
	}

	switch (st.st_mode & S_IFMT)
	{
		case S_IFREG:	fs.type = FileType::reg; break;
		case S_IFDIR:	fs.type = FileType::dir; break;
		case S_IFLNK:	fs.type = FileType::link; break;
		case S_IFSOCK:	fs.type = FileType::sock; break;
		case S_IFBLK:	fs.type = FileType::block; break;
		case S_IFCHR:	fs.type = FileType::chr; break;
		case S_IFIFO:	fs.type = FileType::fifo; break;
	}

	fs.mode = st.st_mode & ~S_IFMT;
	fs.uid = st.st_uid;
	fs.gid = st.st_gid;
	fs.size = st.st_size;
	fs.alloc_size = st.st_blocks*512;
	fs.access_time = st.st_atim.tv_sec*NS + st.st_atim.tv_nsec;
	fs.mod_time = st.st_mtim.tv_sec*NS + st.st_mtim.tv_nsec;
	fs.change_time = st.st_ctim.tv_sec*NS + st.st_ctim.tv_nsec;
	fs.inode = st.st_ino;
	fs.num_links = st.st_nlink;

	return fs;
}

void Filesystem::change_dir(const std::string& name, std::system_error* err)
{
	if (chdir(name.c_str()) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "change_dir()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

std::string Filesystem::get_current_dir(std::system_error* err)
{
	auto dir = get_current_dir_name();
	if (dir == nullptr)
	{
		auto e = std::system_error(errno, std::system_category(), "get_current_dir()");
		if (err)
		{
			*err = e;
			return "";
		}
		throw e;
	}
	std::string ret(dir);
	free(dir);
	return ret;
}

void Filesystem::set_mode(const std::string& name, unsigned mode, std::system_error* err)
{
	// no symlink follow is not implemented, cannot apply mode to a symlink
	if (fchmodat(AT_FDCWD, name.c_str(), mode, 0) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "set_mode()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

void Filesystem::set_ids(const std::string& name, int uid, int gid, std::system_error* err)
{
	if (uid < 0)		uid = -1;
	if (gid < 0)		gid = -1;

	if (fchownat(AT_FDCWD, name.c_str(), uid, gid, AT_SYMLINK_NOFOLLOW) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "set_ids()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

void Filesystem::set_times(const std::string& name, uint64_t access_time, uint64_t mod_time, std::system_error* err)
{
	struct timespec times[2];
	times[0].tv_sec = (access_time-(access_time%NS))/NS;
	times[0].tv_nsec = access_time%NS;
	times[1].tv_sec = (mod_time-(mod_time%NS))/NS;
	times[1].tv_nsec = mod_time%NS;

	if (utimensat(AT_FDCWD, name.c_str(), &times[0], AT_SYMLINK_NOFOLLOW) == -1)
	{
		auto e = std::system_error(errno, std::system_category(), name);
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}
}

//#include <iostream>
std::map<off_t, off_t>  Filesystem::sparse_map(const std::string& name, std::system_error* err)
{
	std::map<off_t, off_t> ret;

	int fd = scc::util::safe_open(name.c_str(), O_RDONLY);
	if (fd == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "sparse_map()");
		if (err)
		{
			*err = e;
			return ret;
		}
		throw e;
	}

	auto throw_err = [&]()
	{
		auto e = std::system_error(errno, std::system_category(),  "sparse_map()");
		if (err)
		{
			*err = e;
			safe_close(fd);
			return ret;
		}
		safe_close(fd);
		throw e;
		return ret;
	};
	
	struct stat st;
	if (fstat(fd, &st) < 0)		return throw_err();

//std::cout << "size=" << st.st_size << std::endl;

	off_t idx = 0;
	off_t starth = -1;
	while (1)
	{
		off_t h = lseek(fd, idx, SEEK_HOLE);		// find next hole
//std::cout << "idx=" << idx << " hole=" << h << std::endl;

		if (h < 0)
		{
//std::cout << "idx=" << idx << " hole=" << h << std::endl;
			throw_err();
//			break;
		}

		if (h == idx)		// start of hole
		{
			starth = idx;
			
			int d = lseek(fd, idx, SEEK_DATA);		// find next data
//std::cout << "idx=" << idx << " data=" << d << std::endl;

			if (d < 0)		// hole at the end of file
			{
//std::cout << "idx=" << idx << " data=" << d << std::endl;
				ret.emplace(starth, st.st_size-1);
				break;
			}
			
			idx = d;		// go to the start of next data
		}
		else				// start of data
		{
			if (starth >= 0)
			{
				ret.emplace(starth, idx-1);
				starth = -1;
			}

			idx = h;				// go to the start of next hole
		}

		if (h >= st.st_size)		// end of file has an implicit hole
		{
			if (starth >= 0)
			{
				ret.emplace(starth, st.st_size-1);
			}
			break;
		}

	}
	return ret;
}

void Filesystem::set_size(const std::string& name, off_t size, std::system_error* err)
{
	int fd = scc::util::safe_open(name.c_str(), O_WRONLY, 0);
	if (fd == -1)
	{
		auto e = std::system_error(errno, std::system_category(), "set_size()");
		if (err)
		{
			*err = e;
			return;
		}
		throw e;
	}

	auto throw_err = [&]()
	{
		auto e = std::system_error(errno, std::system_category(), "set_size()");
		if (err)
		{
			*err = e;
			safe_close(fd);
		}
		safe_close(fd);
		throw e;
	};

	struct stat st;
	if (fstat(fd, &st) < 0)			throw_err();

	if (size < st.st_size)
	{
		if (safe_ftruncate(fd, size) < 0)		throw_err();
	}
	else if (size > st.st_size)
	{
		// set the file pointer to the desired size
		if (lseek(fd, size-1, SEEK_SET) == -1)		throw_err();
		// write out the sparse file (0 at the end)
		if (safe_write(fd, "", 1) < 0)				throw_err();
	}

	safe_close(fd);
}

std::string Filesystem::norm_path(const std::string& base_dir, const std::string& path) noexcept
{
	char _bd[base_dir.size()+1];
	strcpy(_bd, base_dir.c_str());
	char _bp[base_dir.size()+1];
	strcpy(_bp, base_dir.c_str());

	std::string bd(dirname(_bd));		// these may use the input string as scratch
	std::string bp(basename(_bp));

	char _pd[path.size()+1];
	strcpy(_pd, path.c_str());
	char _pp[path.size()+1];
	strcpy(_pp, path.c_str());
	
	std::string pd(dirname(_pd));
	std::string pp(basename(_pp));

	std::string newp, trail;

	if (pd[0] == '/')		// absolute
	{
		newp = pd + "/" + pp;
	}
	else
	{
		newp = bd + "/" + bp + "/" + pd + "/" + pp;
	}

	if (path.size())
	{
		if (path[path.size()-1] == '/')
		{
			trail = "/";
		}
	}
	else if (base_dir.size() && base_dir[base_dir.size()-1] == '/')
	{
		trail = "/";
	}

	// split up the path
	std::vector<std::string> sp;
	std::stringstream paths(newp);
	std::string token;
	while (std::getline(paths, token, '/'))
	{
		sp.push_back(token);
	}

	// will ignore any sp item which is empty (thus ignoring double //)

	for (unsigned i = 1; i < sp.size(); i++)		// always keep the first entry
	{
		if (sp.size() == 0)
		{
			continue;			// ignore this entry
		}
		else if (sp[i] == ".")
		{
			sp[i] = "";			// erase this entry
		}
		else if (sp[i] == "..")
		{
			int back;
			for (back=i-1; back > 0 && sp[back] == ""; back--)		// go back to the last real entry
			{}
			if (back == 0)
			{
				continue;			// use this entry
			}
			if (sp[back] == "..")
			{
				continue;			// use this entry
			}
			sp[back] = "";			// erase the last entry
			sp[i] = "";				// erase this entry
		}
		// otherwise, use this entry
	}

	std::stringstream ret;
	ret << sp[0];
	for (unsigned i = 1; i < sp.size(); i++)
	{
		if (sp[i].size() > 0)
		{
			ret << "/" << sp[i];
		}
	}

	if (ret.str().size() == 0)		// this was either / or // etc.
	{
		return "/";
	}
	if (ret.str()[ret.str().size()-1] != '/')
	{
		return ret.str()+trail;
	}
	return ret.str();
}
