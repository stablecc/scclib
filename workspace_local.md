# Using scclib locally

Scclib source code can be downloaded from the
[http repository](https://github.com/stablecc/scclib/archive/refs/heads/master.zip), and
added to any workspace file
by adding `local_repository` lines:
```
local_repository(
    name = "com_stablecc_scclib",
    path = "/path/to/scclib",
)
```
