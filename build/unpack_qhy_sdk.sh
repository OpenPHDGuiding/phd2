#!/bin/bash

set -ex

SRC=$(cd $(dirname "$0")/..; /bin/pwd)

TMP=/tmp/qhysdk.$$
trap "rm -rf $TMP" INT TERM QUIT EXIT

mkdir -p $TMP
cd $TMP

unpack_win () {
    zip=$1
    zip=$(cd $(dirname "$zip"); /bin/pwd)/$(basename "$zip")

    unzip "$zip"

    sdk=$TMP/$(ls $TMP | head -1)

    (
        cd "$sdk"/include
        sed -E 's/^#define QHYCCD_PCIE_SUPPORT[[:space:]]+1[[:space:]]*$/#define QHYCCD_PCIE_SUPPORT 0/' \
            <config.h >qhyccd_config.h
        for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
            sed -e s/config.h/qhyccd_config.h/ $f > "$SRC"/cameras/$f
        done
    )

    if [ -d "$sdk"/Windows ]; then
        # LZR's style packaging
        cp "$sdk"/Linux/i386/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/x86_32/
        cp "$sdk"/Linux/x86-64/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/x86_64/
        cp "$sdk"/Linux/armv6/libqhy.a "$SRC"/cameras/qhyccdlibs/linux/armv6/
        cp "$sdk"/OSX/universal/libqhy.a "$SRC"/cameras/qhyccdlibs/mac/x86_32/
        cp "$sdk"/Windows/x86/vc12/qhyccd.dll    "$SRC"/WinLibs/
        cp "$sdk"/Windows/x86/vc12/qhyccd.lib    "$SRC"/cameras/
    elif [ -f "$sdk"/qhyccd.dll ]; then
        # QXX's style packaging (Windows only)
        cp "$sdk"/qhyccd.dll "$SRC"/WinLibs/
        cp "$sdk"/tbb.dll "$SRC"/WinLibs/
        cp "$sdk"/lib/qhyccd.lib "$SRC"/Cameras/
    elif [ -f "$sdk"/x86/qhyccd.dll ]; then
        # MYQ's style packaging
        cp "$sdk"/x86/qhyccd.dll "$SRC"/WinLibs/
        cp "$sdk"/x86/tbb.dll "$SRC"/WinLibs/
        cp "$sdk"/x86/qhyccd.lib "$SRC"/Cameras/
    fi
}

unpack_mac () {
    f=$1
    dir=$2

    tar xfz $f

    sdk=$TMP/$(ls $TMP | head -1)
    cd $sdk

    (
        cd usr/local/include
        sed -E 's/^#define QHYCCD_PCIE_SUPPORT[[:space:]]+1[[:space:]]*$/#define QHYCCD_PCIE_SUPPORT 0/' \
            <config.h >qhyccd_config.h
        for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
            sed -e s/config.h/qhyccd_config.h/ $f > "$SRC"/cameras/$f
        done
    )
    cp usr/local/lib/libqhyccd.dylib "$SRC"/cameras/qhyccdlibs/mac/$dir/
}

unpack_linux () {
    f=$1
    arch=$2

    tar xfz $f

    sdk=$TMP/$(ls $TMP | head -1)
    cd $sdk

    (
        cd usr/local/include
        sed -E 's/^#define QHYCCD_PCIE_SUPPORT[[:space:]]+1[[:space:]]*$/#define QHYCCD_PCIE_SUPPORT 0/' \
            <config.h >qhyccd_config.h
        for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
            sed -e s/config.h/qhyccd_config.h/ $f > "$SRC"/cameras/$f
        done
    )
    cp usr/local/lib/libqhyccd.a "$SRC"/cameras/qhyccdlibs/linux/$arch/
}

# === main ===

for f in $*; do
    if [ ! -f "$f" ]; then
        echo "usage: $0 FILE..." >&2
        exit 1
    fi
    case $(basename $f) in
        MAC*64*tgz)        unpack_mac   $f x86_64 ;;
        MAC*tgz)           unpack_mac   $f x86_32 ;;
        *zip)              unpack_win   $f ;;
        RPI*)              unpack_linux $f armv7 ;;
        LINUX_X64_qhyccd*) unpack_linux $f x86_64 ;;
        *) echo "TODO" >&2 ; exit 1 ;;
    esac
done
