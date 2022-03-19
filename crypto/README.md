# cryptography scc::crypto

See \ref crypto.

## dependencies

This module depends on the scclib [util](../util) module.

The module also depends on [OpenSSL](https://github.com/stablecc/scclib-openssl) or
[Intel ippcp](https://github.com/stablecc/scclib-ippcp) depending on build parameters.

### ippcp with bazel

OpenSSL is the default. To use ippcp, use the following command line:
```
$ bazel run :scccryptounit --define ipp=on
```
or modify `.bazelrc` with the following:
```
build --define ipp=on
```

Requires the Intel IPP Cryptography library to be installed under `/opt/intel`.

### ippcp with gnu make

OpenSSL is the default. To build the libraries with ippcp, use the following command line:
```
make IPP=on
```

`make cleanall` should be used when changing crypto libraries. The default can be changed in the
[shared makefile](../make/shared.mk).

Requires OpenSSL version 1.1.0 or greater.
