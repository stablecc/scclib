# Using scclib locally

Scclib source code can be downloaded from the
[github public repository](https://github.com/stablecc/scclib/archive/refs/heads/master.zip)

## Using the project workspace

Build and run from the project root directory, using the [project workspace](WORKSPACE.bazel).

## Using as a local repository

Add the following to your WORKSPACE or WORKSPACE.bazel file, to provide the com_stablecc_scclib
repository locally to other projects:
```
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "com_stablecc_scclib",
    path = "/path/to/scclib",
)

http_archive(
    name = "rules_python",
    sha256 = "f578b22630b8278ad75aa2d35128bd9ae8c387bfe687f310eda0e24404d2e6c8",
    strip_prefix = "rules_python-b842276b79320b320998a0db47181b93716babed",
    urls = ["https://github.com/bazelbuild/rules_python/archive/b842276b79320b320998a0db47181b93716babed.zip"],
)

http_archive(
    name = "com_google_googletest",
    sha256 = "8daa1a71395892f7c1ec5f7cb5b099a02e606be720d62f1a6a98f8f8898ec826",
    strip_prefix = "googletest-e2239ee6043f73722e7aa812a459f54a28552929",
    urls = ["https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip"],
)

http_archive(
    name = "com_github_google_benchmark",
    sha256 = "3b156bac7800f67858afe6ec2c280e291da70c9fc55377cb47ac11cc83f3128e",
    strip_prefix = "benchmark-0d98dba29d66e93259db7daa53a9327df767a415",
    urls = ["https://github.com/google/benchmark/archive/0d98dba29d66e93259db7daa53a9327df767a415.zip"],
)
```