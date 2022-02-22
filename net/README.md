# scc::net Networking Module

See \ref net.

This module uses ipv6 networking. For ipv4 addresses, use ipv4-embedded notation, for example:
```
sockaddr.host("::ffff:127.0.0.1");
```

Ipv4 broadcast networking is not supported.

## Build

To build the library module use the following:
```
bazel build @com_stablecc_scclib/net:lib
```

## Test

Tests are under the unittest directory:
```
bazel test @com_stablecc_scclib//net/unittest:all_tests
```

## Dependencies

This module depends on the scclib [util](../util) module.  Does not depend on any linux package.