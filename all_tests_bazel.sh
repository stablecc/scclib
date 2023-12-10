#!/bin/bash

# Full clean and test of scclib and associated libraries
#
# Use IPP=on to use IPP (otherwise the system will use openssl).
# Run bazel clean --expunge first to have a completely clean environment.

if [[ "$1" == "cleanall" ]]; then
set -x
bazel clean --expunge
exit
fi

if [[ "$IPP" == "on" ]]; then
IPPCMD='--define ipp=on'
else
IPPCMD='--define ipp=off'
fi

set -e # fail on error
trap 'echo "last command had failure code $?"; printf "\u274c\n"' EXIT # run on exit
set -x # xtrace

# associated projects

if [[ "$1" == "ipp" ]]; then
# ipp
bazel test $IPPCMD @com_stablecc_scclib_ipp//unittest:importippunit
# ippcp
bazel test $IPPCMD @com_stablecc_scclib_ippcp//unittest:importippcpunit
else
# openssl
bazel test $IPPCMD @com_stablecc_scclib_openssl//unittest:importopensslunit
fi
# sqlite
bazel test $IPPCMD @com_stablecc_scclib_sqlite//sqlite/unittest:importsqliteunit
bazel test $IPPCMD @com_stablecc_scclib_sqlite//unittest:sccsqliteunit
# zlib
bazel test $IPPCMD @com_stablecc_scclib_zlib//unittest:importzlibunit

# scclib libraries

# crypto
bazel test $IPPCMD @com_stablecc_scclib//crypto/unittest:scccryptounit
# encode
bazel test $IPPCMD @com_stablecc_scclib//encode/unittest:sccencodeunit
# net
bazel test $IPPCMD @com_stablecc_scclib//net/unittest:sccnetunit
# util
bazel test $IPPCMD @com_stablecc_scclib//util/unittest:sccutilunit

set +x
trap '' EXIT
echo "Everything worked!"
printf "\u2705\n"
