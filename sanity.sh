#!/bin/bash

# basic sanity test
# you can run bazel clean --expunge first to have a completely clean environment

bazel test @com_stablecc_scclib//util/unittest:sccutilunit # unit tests for util module
bazel test @com_stablecc_scclib//net/unittest:sccnetunit # unit tests for net module
bazel build @com_stablecc_scclib//examples/net:ntest # network test example executable
