#!/bin/bash

set -ex

zip=$1
if [ ! -f "$zip" ]; then
    echo "usage: $0 toupcamsdk.zip" >&2
    exit 1
fi
zip=$(cd $(dirname "$zip"); /bin/pwd)/$(basename "$zip")

SRC=$(cd $(dirname "$0")/..; /bin/pwd)

TMP=/tmp/toupcamsdk.$$
trap "rm -rf $TMP" INT TERM QUIT EXIT

mkdir -p $TMP
cd $TMP
unzip "$zip"

# includes
cp ./inc/toupcam.h "$SRC"/cameras/

# libs
cp ./win/x86/toupcam.dll "$SRC"/WinLibs/
cp ./win/x86/toupcam.lib "$SRC"/cameras/

# TODO: linux, Mac
