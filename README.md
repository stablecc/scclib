\mainpage scclib documentation
# stable cloud computing library (scclib)

Standalone c++ general purpose library designed to work in any system with the following:
* [Linux](https://www.linux.org/).
* [The Bazel build system](https://bazel.build/).
* A C++ compiler with support for Standard C++ 17. See
[cppreference](https://en.cppreference.com/w/cpp/17).
* [OpenSSL](https://www.openssl.org) version 1.1.0 or later (current version is 1.1.1).
* Optionally, the [Intel IPP Cryptography Library](https://github.com/intel/ipp-crypto).

The library was should be compatible with many POSIX emulation environments such as
[cygwin](https://www.cygwin.com/), but it was developed and tested
primarily on Linux distributions with [GCC](https://gcc.gnu.org) 9+. It was developed using Bazel 4.2+, but should be compatible
with older versions of Bazel.

An alternative [build system](make/) using GNU make is available, for systems without Bazel.

## quick start

Install the [build essentials and OpenSSL development packages](dev_packages.md), [Install bazel](install_bazel.md),
download the source code, and run the following from the ```scclib``` directory:
```
$ bazel clean --expunge
$ ./sanity.sh
```
If you have Intel IPP and IPPCP on your system, try:
```
$ ./sanity.sh ipp
```

## installation in a local workspace

1. Make sure the [build essentials and OpenSSL development packages](dev_packages.md) are installed and up-to-date.

2. [Install bazel](install_bazel.md) on your local system.

3. Set up a workspace, either [remotely](workspace_remote.md) or [locally](workspace_local.md).

4. Run the sanity tests to make sure things are working (using [sanity.sh](sanity.sh)).


## documentation

Current documentation is available on
[stablecc.github.io](https://stablecc.github.io/scclib-doxygen/).

### generating doxygen documentation

Scclib code is fully documented using [doxygen](https://www.doxygen.nl/index.html).

When you have doxygen available, run the following command line after downloading the source
code:
```
$ doxygen doxygen.conf
```

The docs will be created in the `docs/` directory:
```
$ ls docs/html/index.html 
docs/html/index.html
```

## licensing

Original source:
* [BSD 3-Clause License](lic/bsd_3_clause.txt)

External and redistributable:
* [openssl](lic/openssl.txt)
* [ipp and ippcp](lic/intel.txt)
* [zlib](lic/zlib.txt)
* [sqlite](lic/sqlite.txt)
