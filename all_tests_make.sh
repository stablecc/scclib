#!/bin/bash

# Full clean and test of scclib and associated libraries
#
# Use MAKE_TARGET=release to build and test release versions.
# Use IPP=on to use IPP (otherwise the system will use openssl).

if [[ "$1" == "cleanall" ]]; then
set -x
rm -rf ../sccbin/ ../sccobj/
exit
fi

set -e # fail on error
trap 'echo "last command had failure code $?"; printf "\u274c\n"' EXIT # run on exit
set -x # xtrace

if [[ "$MAKE_TARGET" == "release" ]]; then
unset BIN_PRE
else
MAKE_TARGET='debug'
BIN_PRE=_d
fi

# associated projects

if [[ "$IPP" == "on" ]]; then
# ipp
make -C ../scclib-ipp/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/ipp_unit${BIN_PRE}
# ippcp
make -C ../scclib-ippcp/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/ippcp_unit${BIN_PRE}
else
# openssl
make -C ../scclib-openssl/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/openssl_unit${BIN_PRE}
fi
# sqlite
make -C ../scclib-sqlite/sqlite/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/openssl_unit${BIN_PRE}
make -C ../scclib-sqlite/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/openssl_unit${BIN_PRE}
# zlib
make -C ../scclib-zlib/unittest $MAKE_TARGET; (cd ../scclib-zlib/unittest; LD_LIBRARY_PATH=../../sccbin ../../sccbin/zlib_unit${BIN_PRE})

# scclib libraries

# crypto
make -C crypto/unittest $MAKE_TARGET; (cd crypto/unittest; LD_LIBRARY_PATH=../../../sccbin ../../../sccbin/crypto_unit${BIN_PRE})
# encode
make -C encode/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/encode_unit${BIN_PRE}
# net
make -C net/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/net_unit${BIN_PRE}
# util
make -C util/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../sccbin ../sccbin/util_unit${BIN_PRE}

set +x
trap '' EXIT
echo "Everything worked!"
printf "\u2705\n"
