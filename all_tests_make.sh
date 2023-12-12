#!/bin/bash

# Full clean and test of scclib and associated libraries
#
# Use MAKE_TARGET=release to build and test release versions.
# Use IPP=on to use IPP (otherwise the system will use openssl).

SCCBIN=sccbld-bin
SCCOBJ=sccbld-obj

if [[ "$1" == "cleanall" ]]; then
set -x
rm -rf ../${SCCBIN}/ ../${SCCOBJ}/
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
MAKE="make IPP=on"
# ipp
$MAKE -C ../scclib-ipp/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/ipp_unit${BIN_PRE}
# ippcp
$MAKE -C ../scclib-ippcp/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/ippcp_unit${BIN_PRE}
else
MAKE=make
# openssl
$MAKE -C ../scclib-openssl/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/openssl_unit${BIN_PRE}
fi
# sqlite
$MAKE -C ../scclib-sqlite/sqlite/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/import_sqlite_unit${BIN_PRE}
$MAKE -C ../scclib-sqlite/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/sqlite_unit${BIN_PRE}
# zlib
$MAKE -C ../scclib-zlib/unittest $MAKE_TARGET; (cd ../scclib-zlib/unittest; LD_LIBRARY_PATH=../../${SCCBIN} ../../${SCCBIN}/zlib_unit${BIN_PRE})

# scclib libraries

# crypto
$MAKE -C crypto/unittest $MAKE_TARGET; (cd crypto/unittest; LD_LIBRARY_PATH=../../../${SCCBIN} ../../../${SCCBIN}/crypto_unit${BIN_PRE})
# encode
$MAKE -C encode/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/encode_unit${BIN_PRE}
# net
$MAKE -C net/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/net_unit${BIN_PRE}
# util
$MAKE -C util/unittest $MAKE_TARGET; LD_LIBRARY_PATH=../${SCCBIN} ../${SCCBIN}/util_unit${BIN_PRE}

set +x
trap '' EXIT
echo "Everything worked!"
printf "\u2705\n"
