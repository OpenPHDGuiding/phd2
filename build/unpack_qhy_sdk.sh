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

cp "$sdk"/linux/x86_32/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/x86_32/
cp "$sdk"/linux/x86_64/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/x86_64/
cp "$sdk"/mac/libqhy.a          "$SRC"/cameras/qhyccdlibs/mac/x86_32/
cp "$sdk"/win/dll/qhyccd.dll    "$SRC"/WinLibs/
cp "$sdk"/win/lib/qhyccd.lib    "$SRC"/cameras/

exit

        0  05-18-2016 12:34   qhyccdlibs0.1.8/include/
    20330  05-17-2016 18:24   qhyccdlibs0.1.8/include/CyAPI.h
     3504  05-17-2016 18:24   qhyccdlibs0.1.8/include/CyUSB30_def.h
    27388  05-17-2016 18:24   qhyccdlibs0.1.8/include/qhyccd.h
     9273  05-17-2016 18:24   qhyccdlibs0.1.8/include/qhyccdcamdef.h
     3954  05-17-2016 18:24   qhyccdlibs0.1.8/include/qhyccderr.h
     6273  05-17-2016 18:24   qhyccdlibs0.1.8/include/qhyccdstruct.h
        0  05-18-2016 18:12   qhyccdlibs0.1.8/linux/
        0  05-18-2016 12:35   qhyccdlibs0.1.8/linux/x86_32/
  4116806  05-17-2016 16:34   qhyccdlibs0.1.8/linux/x86_32/libqhy.a
  1563302  05-17-2016 16:33   qhyccdlibs0.1.8/linux/x86_32/libqhy.so.0
        0  05-18-2016 12:35   qhyccdlibs0.1.8/linux/x86_64/
  5396694  05-17-2016 16:29   qhyccdlibs0.1.8/linux/x86_64/libqhy.a
  1762941  05-17-2016 16:29   qhyccdlibs0.1.8/linux/x86_64/libqhy.so.0
        0  05-18-2016 12:35   qhyccdlibs0.1.8/mac/
  3482888  05-17-2016 18:19   qhyccdlibs0.1.8/mac/libqhy.0.dylib
 20060680  05-17-2016 18:19   qhyccdlibs0.1.8/mac/libqhy.a
        0  05-18-2016 18:07   qhyccdlibs0.1.8/win/
        0  05-18-2016 18:07   qhyccdlibs0.1.8/win/dll/
  3721728  05-18-2016 17:27   qhyccdlibs0.1.8/win/dll/qhyccd.dll
        0  05-18-2016 18:07   qhyccdlibs0.1.8/win/lib/
    21008  05-18-2016 17:27   qhyccdlibs0.1.8/win/lib/qhyccd.lib
