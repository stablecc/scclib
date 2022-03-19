# networking scc::net

See \ref net.

This module uses ipv6 networking. For ipv4 addresses, use ipv4-embedded notation, for example:
```
sockaddr.host("::ffff:127.0.0.1");
```

Ipv4 broadcast networking is not supported.

## dependencies

This module depends on the scclib [util](../util) module.  Does not depend on any linux package.
