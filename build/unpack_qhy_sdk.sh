#!/bin/bash

set -ex

zip=$1
if [ ! -f "$zip" ]; then
    echo "usage: $0 qhyccdlibsX.Y.Z.zip" >&2
    exit 1
fi
zip=$(cd $(dirname "$zip"); /bin/pwd)/$(basename "$zip")

SRC=$(cd $(dirname "$0")/..; /bin/pwd)

TMP=/tmp/qhysdk.$$
trap "rm -rf $TMP" INT TERM QUIT EXIT

mkdir -p $TMP
cd $TMP
unzip "$zip"

sdk=$TMP/$(ls $TMP | head -1)

for f in qhyccd.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
    cp "$sdk/include/$f" "$SRC"/cameras/
done

cp "$sdk"/Linux/i386/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/x86_32/
cp "$sdk"/Linux/x86-64/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/x86_64/
cp "$sdk"/Linux/armv6/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/armv6/
cp "$sdk"/OSX/universal/libqhy.a "$SRC"/cameras/qhyccdlibs/mac/x86_32/
cp "$sdk"/Windows/x86/vc12/qhyccd.dll    "$SRC"/WinLibs/
cp "$sdk"/Windows/x86/vc12/qhyccd.lib    "$SRC"/cameras/
