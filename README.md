# Stable Cloud Computing library (scclib)

Standalone C++ general purpose library, designed to run in any system with the following:
* Linux 2.6.33 (or better)
* [The Bazel build system](https://bazel.build/)
* A compiler that supports Standard C++ 17

## Quick start

Set up bazel (see [how to install bazel](bazel.md)). A fairly easy way to set up bazel is using
one of the [pre-built releases](https://github.com/bazelbuild/bazel/releases/tag/5.0.0) available
in github.

Set up a workspace (or use the [scclib workspace](WORKSPACE.bazel)).

Build one of the the [examples](examples/).

## Adding scclib to an existing workspace

Add the http_archive rule to your workspace:
```
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
```

Add dependencies:
```
http_archive(
    name = "rules_python",
    sha256 = "98b3c592faea9636ac8444bfd9de7f3fb4c60590932d6e6ac5946e3f8dbd5ff6",
    strip_prefix = "rules_python-ed6cc8f2c3692a6a7f013ff8bc185ba77eb9b4d2",
    urls = ["https://github.com/bazelbuild/rules_python/archive/ed6cc8f2c3692a6a7f013ff8bc185ba77eb9b4d2.zip"],
)
http_archive(
    name = "com_google_googletest",
    sha256 = "12ef65654dc01ab40f6f33f9d02c04f2097d2cd9fbe48dc6001b29543583b0ad",
    strip_prefix = "googletest-8d51ffdfab10b3fba636ae69bc03da4b54f8c235",
    urls = ["https://github.com/google/googletest/archive/8d51ffdfab10b3fba636ae69bc03da4b54f8c235.zip"],
)
http_archive(
    name = "com_github_google_benchmark",
    sha256 = "62e2f2e6d8a744d67e4bbc212fcfd06647080de4253c97ad5c6749e09faf2cb0",
    strip_prefix = "benchmark-0baacde3618ca617da95375e0af13ce1baadea47",
    urls = ["https://github.com/google/benchmark/archive/0baacde3618ca617da95375e0af13ce1baadea47.zip"],
)
```

To add using a local repository:
```
local_repository(
    name = "com_stablecc_scclib",
    path = "your/path/to/scclib",
)
```

To add using the github mirror archive:
```
http_archive(
    name = "com_stablecc_scclib",
    strip_prefix = "scclib-master",
    urls = ["https://github.com/stablecc/scclib/archive/master.zip"],
)
```
