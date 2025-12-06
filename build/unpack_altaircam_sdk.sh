#!/bin/bash

set -ex

zip=$1
if [ ! -f "$zip" ]; then
    echo "usage: $0 altaircamsdk.zip" >&2
    exit 1
fi
zip=$(cd $(dirname "$zip"); /bin/pwd)/$(basename "$zip")

SRC=$(cd $(dirname "$0")/..; /bin/pwd)

TMP=/tmp/altaircamsdk.$$
success=

cleanup () {
    rm -r "$TMP"
    if [[ ! $success ]]; then
        echo "Failed!"
        exit 1
    fi
}
trap cleanup INT TERM QUIT EXIT

mkdir -p $TMP
cd $TMP
unzip "$zip"
# contents may be in a sub-folder
if [ -d altaircamsdk.* ]; then
    cd altaircamsdk.*
elif [ -d altairsdk.* ]; then
    cd altairsdk.*
elif [ -d *SDK_ALTAIR* ]; then
    cd *SDK_ALTAIR*
elif [ -f ./inc/altaircam.h ]; then
    : no sub-folder
else
    echo "unexpected zip contents" >&2
    exit 1
fi

# newer sdk is packaged as zip files within the zip file
(
    shopt -s nullglob
    for f in *.zip; do
        unzip "$f"
    done
)

# includes
cp ./inc/altaircam.h "$SRC"/cameras/altair/include/

# windows libs
cp ./win/x86/altaircam.dll "$SRC"/cameras/altair/win/x86/
cp ./win/x86/altaircam.lib "$SRC"/cameras/altair/win/x86/
cp ./win/x64/altaircam.dll "$SRC"/cameras/altair/win/x64/
cp ./win/x64/altaircam.lib "$SRC"/cameras/altair/win/x64/

# mac lib
cp ./mac/libaltaircam.dylib "$SRC"/cameras/altair/mac/

# linux libs
cp -r ./linux/{arm64,armel,armhf,x86,x64} "$SRC"/cameras/altair/linux/

success=1
echo "Done"
