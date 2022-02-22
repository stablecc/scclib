# scc::util General Utilities Module

See \ref util.

## Build

To build the library module use the following:
```
bazel build @com_stablecc_scclib//util:lib
```

## Test

Tests are under the unittest directory:
```
bazel test @com_stablecc_scclib//util/unittest:all_tests
```

## Dependencies

This module does not depend on any scclib library or linux package.