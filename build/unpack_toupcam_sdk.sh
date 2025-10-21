#!/bin/bash

set -ex

zip=$1
if [[ ! -f $zip ]]; then
    echo "usage: $0 toupcamsdk.zip" >&2
    exit 1
fi
zip=$(cd $(dirname "$zip"); /bin/pwd)/$(basename "$zip")

SRC=$(cd $(dirname "$0")/..; /bin/pwd)

TMP=/tmp/toupcamsdk.$$
trap "rm -rf $TMP" INT TERM QUIT EXIT

mkdir -p "$TMP"
cd "$TMP"
unzip "$zip"

DEST=$SRC/cameras/toupcam

# includes
cp ./inc/toupcam.h "$DEST"/include/

# Windows libs
cp ./win/x86/toupcam.dll "$DEST"/win/x86/
cp ./win/x86/toupcam.lib "$DEST"/win/x86/

# Linux libs
for arch in x86 x64 armhf armel arm64; do
    cp ./linux/$arch/libtoupcam.so "$DEST"/linux/"$arch"/
done

# Mac
cp ./mac/x64+arm64/libtoupcam.dylib "$DEST"/mac/
