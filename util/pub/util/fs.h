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
#ifndef _SCC_UTIL_FS_H
#define _SCC_UTIL_FS_H

#include <string>
#include <map>
#include <functional>
#include <ostream>
#include <system_error>

namespace scc::util
{

/** \addtogroup util
	@{
*/

/** \defgroup util_fs Filesystem utilities
	@{

	File system utility class for common operations.
*/

/** File system utilities.
	\file
*/

/** File type. */
enum class FileType
{
	unknown=	0,  ///< unknown or does not exist
	reg=		1,  ///< regular file
	dir=		2,  ///< directory
	link=		4,  ///< symbolic link
	sock=		8,  ///< socket
	block=		16, ///< block device
	chr=		32, ///< character device
	fifo=		64  ///< FIFO
};

/** File stat. */
struct FileStat
{
	FileType 	type;			///< File type
	unsigned 	mode;			///< File mode 07777 format (bits/user/group/all 4=read, 2=write, 3=execute, or uid/gid/sticky)
	int 		uid;			///< User id
	int 		gid;			///< Group id
	uint64_t	size;			///< Size of file
	uint64_t 	alloc_size;		///< Real allocated size on disk
	uint64_t 	access_time;	///< Last access time; reads, etc. (ns since epoch)
	uint64_t 	mod_time;		///< Last modification time; writes, etc. (ns since epoch)
	uint64_t 	change_time;	///< Last status change time; writes, permission changes, etc. (ns since epoch)
	uint64_t	inode;			///< Inode number
	uint16_t	num_links;		///< Number of hard links to this inode

	FileStat() : type(FileType::unknown), mode(0), uid(0), gid(0), size(0), alloc_size(0),
		access_time(0), mod_time(0), change_time(0) {}
};

/** Default scan filter returns true for all files, except "." or ".." directories.
*/
bool default_scan_filter(const std::string&, FileType);

/** Filesystem utility.

	Utility to manipulate files and the filesystem.
*/
class Filesystem
{
	static void remove_file(const std::string&, std::system_error*);
	static void remove_dir(const std::string&, std::system_error*);
	static void remove_all(const std::string&, const FileType&, std::system_error*);

	Filesystem() {}
public:
	
	/** Scan a directory, and return a map of names and file types. Does not scan recursively.

		\param dirname The directory name.
		\param filter Scan filter function which is called with parameters (name, FileType), and returns true if the file should be included.
		\param err If set, on error will set this value and the exception will not be thrown.

		\returns map<name, FileType> Map of files found, empty if no files were found.

		Example in \ref scclib/util/unittest/fs.cc
		\snippet scclib/util/unittest/fs.cc Scan directory
	*/
	static std::map<std::string, FileType> scan_dir(const std::string&,
		std::function<bool(const std::string&, FileType)> = default_scan_filter, std::system_error* = nullptr);

	/** Get the file type. On error, throws an exception or sets err if provided.
	*/
	static FileType file_type(const std::string&, std::system_error* = nullptr);
	
	/** Get the file stat. On error, throws an exception or sets err if provided.
	*/
	static FileStat file_stat(const std::string&, std::system_error* = nullptr);
	
	/** Set the file mode.
		\param name File name.
		\param mode File mode, e.g. 0664.
		\param err Exception override.

		File mode can be specified in octal format:
			- 0400	read by owner
			- 0200	write by owner
			- 0100	execute/search directory by owner
			- 0040	read by group
			- 0020	write by group
			- 0010	execute/search directory by group
			- 0004	read by group
			- 0002	write by group
			- 0001	execute/search directory by group

		Calling this api on a symbolic link will result in an error.

		On error, throws an exception or sets err if provided.
	*/
	static void set_mode(const std::string&, unsigned, std::system_error* = nullptr);
	
	/** Set the file times.

		Times are in nanoseconds since epoch (divide by 1e9 to get time_t)

		\param name File name.
		\param access_time File access time.
		\param mod_time File modification time.
		\param err Exception override.

		On error, throws an exception or sets err if provided.
	*/
	static void set_times(const std::string&, uint64_t, uint64_t, std::system_error* = nullptr);
	
	/** Set the user id and/or group id.

		If either value is < 0, it will not be set.

		\param name File name.
		\param uid User id.
		\param gid Group id.
		\param err Exception override.
	*/
	static void set_ids(const std::string&, int, int, std::system_error* = nullptr);
	
	/** Set the file size.

		\param name File name.
		\param size New file size.
		\param err Exception override.

		If size is less than current size, file is truncated.

		If size is greater than current size, a sparse file is created.

		Example in \ref scclib/util/unittest/fs.cc
		\snippet scclib/util/unittest/fs.cc Sparse file
	*/
	static void set_size(const std::string&, off_t, std::system_error* = nullptr);

	/** Map of file sparseness.

		\param name File name.
		\param err Exception override.
		\returns Map of start and end offsets of holes (unwritten zero regions) in file.

		A sparse file will contain data only in blocks which have been written.
		When data is read from an unwritten "hole", 0 is returned.

		For example, a 16 K sparse file with a single 0 written at the front and end will have map:
			
			<4096, 12287>   - block from 4096 to 12287 is hole

		This file reads all 0, but takes up 8192 bytes on disk instead of 16384.

		Example in \ref scclib/util/unittest/fs.cc
		\snippet scclib/util/unittest/fs.cc Sparse file
	*/
	static std::map<off_t, off_t> sparse_map(const std::string&, std::system_error* = nullptr);

	/** Remove the file or directory. On error, throws an exception or sets err if provided.
	*/
	static void remove(const std::string&, std::system_error* = nullptr);

	/** Rename the file or directory. On error, throws an exception or sets err if provided.
		\param old_fn Old filename.
		\param new_fn New Filename.
	*/
	static void rename(const std::string&, const std::string&, std::system_error* = nullptr);

	/** Recursively removes the file or directory. On error, throws an exception or sets err if provided.
	*/
	static void remove_all(const std::string& fn, std::system_error* err = nullptr)
	{
		auto ty = file_type(fn, err);
		if (ty == FileType::unknown) return;
		remove_all(fn, ty, err);
	}
	
	/** Create a directory. On error, throws an exception or sets err if provided.

		\param name File name.
		\param err Exception override.

		The directory will be created if necessary with mode 0700.
	*/
	static void create_dir(const std::string&, std::system_error* = nullptr);
	
	/** Create a regular file.

		\param name File name.
		\param err Exception override.

		The file will be created if necessary with mode 0600.

		If the file exists, and cannot be opened for writing, throws an error.

		On error, throws an exception or sets err if provided.
	*/
	static void create_reg(const std::string&, std::system_error* = nullptr);
	
	/** Create a temporary regular file.

		Return the filename, which will be prefix plus a random string.

		On error, throws an exception or sets err if provided.
	*/
	static std::string create_tmp_reg(const std::string&, std::system_error* = nullptr);

	/** Create a symbolic link.

		\param target Target of the link.
		\param name Path name of the link file.
		\param err Exception override.

		E.g. create_symlink("../reg", "dir/link") creates a symlink: dir/link -> ../reg

		On error, throws an exception or sets err if provided.
	*/
	static void create_symlink(const std::string&, const std::string&, std::system_error* = nullptr);
	
	/** Create a hard link.

		Results in a second inode that points to the original file.

		\param orig_name Original file name.
		\param new_name New file name.
		\param err Exception override.

		On error, throws an exception or sets err if provided.
	*/
	static void create_link(const std::string&, const std::string&, std::system_error* = nullptr);
	
	/** Create a named pipe (FIFO). On error, throws an exception or sets err if provided.

		The file will be created if necessary with mode 0600.
	*/
	static void create_fifo(const std::string&, std::system_error* = nullptr);
	
	/** Change working directory. On error, throws an exception or sets err if provided.
	*/
	static void change_dir(const std::string&, std::system_error* = nullptr);
	
	/** Get working directory. On error, throws an exception or sets err if provided.
	*/
	static std::string get_current_dir(std::system_error* = nullptr);

	/** Read the location of a symbolic link target.

		\param name The symbolic link name
		\param err Exception override.
		\returns target_name File name of the symbolic link target

		Example if "dir/link" -> "dir/regular", read_symlink(dir/link) will return "dir/regular"

		On error, throws an exception or sets err if provided.
	*/
	static std::string read_symlink(const std::string&, std::system_error* = nullptr);

	/** Normalize a path with base directory.

		If path is relative, will append to the base dir, and return a normalized version.

		If path is absolute, ignores base_dir, and returns a normalized version of path.

		Normalized version removes all consecutive slashes, and unneccesary . and .. directories.

		Returned paths starting with:
			- "/" is an absolute path
			- "/.." is an invalid absolute path
			- "." is a relative path starting in base directory
			- ".." is never returned ("./.." may be returned)
			- any other is relative

		An input of "" paths will result in output "."

		Conserves trailing slash, if any.

		\param base_dir Base directory
		\param path Path
	*/
	static std::string norm_path(const std::string&, const std::string&) noexcept;

	/** Normalize a path.
		\param path Path
	*/
	static std::string norm_path(const std::string& path) noexcept
	{
		return norm_path("", path);
	}
};
/** @} */
/** @} */
}

/** \addtogroup util
	@{
*/
/** \addtogroup util_fs
	@{
*/
std::ostream& operator<<(std::ostream&, scc::util::FileStat);
std::ostream& operator<<(std::ostream&, scc::util::FileType);
/** @} */
/** @} */

#endif
