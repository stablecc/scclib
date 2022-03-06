# GNU make

A build system using [GNU make](https://www.gnu.org/software/make/) is provided for
environments where Bazel is not available.

Shared libaries and applications can be built from their directories, with output to **bin/** and
**obj/** directories relative to the project root.

General make help:
```
$ make help
```

Debug and release builds:
```
$ make # or make debug
$ make release
```

## code coverage (gcov)

Code coverage
[instruments](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html) the code using the
**--coverage -fprofile-arcs -ftest-coverage** options, and links with **-lgcov**.
The [gcov](https://gcc.gnu.org/onlinedocs/gcc/Invoking-Gcov.html#Invoking-Gcov) application can
be used to analyze the code.

An example using gcov from the **examples/net** directory:
```
$ make cleanall
$ make gcov
$ LD_LIBRARY_PATH=../../bin ../../bin/ntest_d -I # list interfaces
$ LD_LIBRARY_PATH=../../bin ../../bin/ntest_d -T google.com 443 # test connect to google
$ pushd ../../net/ # go to the source directory for net
$ gcov -fjmr -o ../obj/sccnet_d * # create .gcov output files
File 'net_if.cc'
Lines executed:91.53% of 118
Creating 'net_if.cc.gcov'
$ less net_if.cc.gcov # take a look
$ rm *.gcov
$ popd
$ make cleanall
```

## googletest

Running unit tests requires a [git submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules)
snapshot of googletest.

After downloading source code, initialize the submodules:
```
$ git submodule init
Submodule 'import/googletest' (https://github.com/stablecc/googletest.git) registered for path 'import/googletest'
$ git submodule update
Cloning into '/home/mike/src/p4.stablecc.com/mike_mikedev_open/scc2/import/googletest'...
```

Now can run unit tests, for example from **net/unittest** directory:
```
```
