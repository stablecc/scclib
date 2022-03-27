#!/bin/bash

IPPCMD='--define ipp=off'
if [[ "$1" == "ipp" ]]; then
IPPCMD='--define ipp=on'
fi

set -e # fail on error
trap 'echo "last command had exit code $?"' EXIT # run on exit
set -x # xtrace

#
# runs all tests and builds all apps in scclib and associated projects
#
# this script assumes that either OpenSSL or Intel IPP and IPPCP is installed
#
# to run against IPP, add "ipp" to the command line
#
# you can run bazel clean --expunge first to have a completely clean environment

# scclib libraries

bazel test $IPPCMD @com_stablecc_scclib//crypto/unittest:scccryptounit
bazel test $IPPCMD @com_stablecc_scclib//encode/unittest:sccencodeunit
bazel test $IPPCMD @com_stablecc_scclib//net/unittest:sccnetunit
bazel test $IPPCMD @com_stablecc_scclib//util/unittest:sccutilunit

# scclib examples

bazel build $IPPCMD @com_stablecc_scclib//examples/net:ntest

# associated projects

if [[ "$1" == "ipp" ]]; then
bazel test $IPPCMD @com_stablecc_scclib_ipp//unittest:importippunit
bazel test $IPPCMD @com_stablecc_scclib_ippcp//unittest:importippcpunit
else
bazel test $IPPCMD @com_stablecc_scclib_openssl//unittest:importopensslunit
fi
bazel test $IPPCMD @com_stablecc_scclib_sqlite//sqlite/unittest:importsqliteunit
bazel test $IPPCMD @com_stablecc_scclib_sqlite//unittest:sccsqliteunit
bazel test $IPPCMD @com_stablecc_scclib_zlib//unittest:importzlibunit

set +x
trap '' EXIT
echo "Everything worked!"
printf '\xf0\x9f\x99\x82\n'
