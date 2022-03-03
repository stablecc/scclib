# Stable Cloud Computing library (scclib)

Standalone C++ general purpose library which should work in any system with the following:
* [Linux](https://www.linux.org/)
* [The Bazel build system](https://bazel.build/)
* A compiler with support for Standard C++ 17. See
[cppreference](https://en.cppreference.com/w/cpp/17).

The library should be compatible with Windows POSIX emulation environments such as
[cygwin](https://www.cygwin.com/), but it was developed and tested on Linux
(with GCC 9+). Likewise, it should work with older versions of bazel, but was
developed using Bazel 4.2+.

## Installation

1. [Install bazel](install_bazel.md) on your local system.

2. Set up a workspace, either [remotely](workspace_remote.md) or [locally](workspace_local.md).

3. Run sanity tests to make sure things are working:
```
bazel test @com_stablecc_scclib//util/unittest:sccutilunit # unit tests for util module
bazel test @com_stablecc_scclib//net/unittest:sccnetunit # unit tests for net module
bazel build @com_stablecc_scclib//examples/net:ntest # network test example executable
```

## Documentation

Scclib code is fully documented using [doxygen](https://www.doxygen.nl/index.html).

When you have doxygen available, run the following command line after downloading the source
code:
```
$ doxygen doxygen.conf
```

The docs will be created in the docs/ directory:
```
$ ls docs/html/index.html 
docs/html/index.html
```

Current documentation is available on
[stablecc.github.io](https://stablecc.github.io/scclib-doxygen/).
