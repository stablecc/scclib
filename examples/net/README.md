# networking example

## ntest

Networking test.

### build

Using the project workspace:
```
$ bazel build :ntest
```

Using from a local workspace:
```
$ bazel build @com_stablecc_scclib//examples/net:ntest
```

Use the output of the bazel build command to find the location of the ntest executable,
or use (or `bazel run`).

### informational

List addresses and interfaces.
```
$ ntest # list available commands
$ ntest -I # list available network interfaces
$ ntest -A # list available addresses
```

Hostname resolution:
```
$ ntest -R google.com localhost
* resolving google.com
* ipv6 2607:f8b0:4002:c09::66 port: 0 scope_id: 0 flags: type-unicast scope-global
* ipv6 2607:f8b0:4002:c09::8a port: 0 scope_id: 0 flags: type-unicast scope-global
* ipv6 2607:f8b0:4002:c09::65 port: 0 scope_id: 0 flags: type-unicast scope-global
* ipv6 2607:f8b0:4002:c09::8b port: 0 scope_id: 0 flags: type-unicast scope-global
* resolving localhost
* ipv4 ::ffff:127.0.0.1 port: 0 scope_id: 0 flags: type-loop
```

## test connection

Test a connection:
```
$ ntest -T google.com 443
* google.com --> ipv6 2607:f8b0:4007:80a::200e port: 0 scope_id: 0 flags: type-unicast scope-global
* connecting with no timeout
* connected OK
$ net -T google.com 44 1
* google.com --> ipv6 2607:f8b0:4007:815::200e port: 0 scope_id: 0 flags: type-unicast scope-global
* connecting with 1 second timeout...
* connection failed: timed out
```

## tcp client and echo server

start a server to listen on any address:
```
$ ntest -l :: 1234
```

start a client to connect to various addresses:
```
$ ntest ::1 1234 # loopback
$ ntest ::ffff:192.168.1.70 1234 # ipv4
$ ntest 2600:<your global address> 1234 # global ipv6 address
$ ntest -s 2 fe80:<your link local address> 1234 # link local ipv6 address, specifying the interface id
```

## udp client and echo server

start a server to listen on any address:
```
$ ntest -l -u :: 1234
```

start a client to send messages to various addresses:
```
$ ntest -u ::1 1234 # loopback
$ ntest -u ::ffff:192.168.1.70 1234 # ipv4 address
$ ntest -u <global address beginning with 2600> 1234 # global ipv6 address
$ ntest -s 2 -u <link local address beginning with fe80> 1234 # link local ipv6 address, specifying the interface id
```
