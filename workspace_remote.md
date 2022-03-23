# Using scclib remotely

Scclib can be added to any workspace using the
[github public repository](https://github.com/stablecc/scclib)
by adding `http_archive` lines:
```
http_archive(
    name = "com_stablecc_scclib",
    strip_prefix = "scclib-master",
    urls = ["https://github.com/stablecc/scclib/archive/refs/heads/master.zip"],
)
```
