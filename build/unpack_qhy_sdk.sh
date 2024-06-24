#!/bin/bash

set -ex

SRC=$(cd $(dirname "$0")/..; /bin/pwd)

TMP=$(mktemp -d --tmpdir -t qhysdk.XXXXXXXX)
trap "rm -rf '$TMP'" INT TERM QUIT EXIT

cd "$TMP"

_install_headers () (
    include=$1
    shift
    cd "$include"
    sed -E 's/^#define QHYCCD_PCIE_SUPPORT[[:space:]]+1[[:space:]]*$/#define QHYCCD_PCIE_SUPPORT 0/' \
        <config.h >qhyccd_config.h
    for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
        sed -E \
            -e 's/config\.h/qhyccd_config\.h/' \
            -e 's,^[[:space:]]*#[[:space:]]*include[[:space:]]+"cyapi\.h",//#include "cyapi.h",' \
            "$f" > "$SRC"/cameras/"$f"
    done
)

_unpack_win_dir () {
    sdk=$1
    shift

    _install_headers "$sdk"/include

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
        cp "$sdk"/lib/qhyccd.lib "$SRC"/cameras/
    elif [ -f "$sdk"/x86/qhyccd.dll ]; then
        # MYQ's style packaging
        cp "$sdk"/x86/qhyccd.dll "$SRC"/WinLibs/
        cp "$sdk"/x86/tbb.dll "$SRC"/WinLibs/
        cp "$sdk"/x86/qhyccd.lib "$SRC"/cameras/
    fi

    rm -rf "$sdk"
}

unpack_win () {
    zip=$1
    zip=$(cd $(dirname "$zip"); /bin/pwd)/$(basename "$zip")
    unzip "$zip"
    sdk=$TMP/$(ls "$TMP" | head -1)
    _unpack_win_dir "$sdk"
}

unpack_win_dir () {
    dir=$1
    shift
    # make a copy of the source dir since the sdk will be modified in place
    tar cf - -C "$dir" . | (cd "$TMP" && tar xf -)
    _unpack_win_dir "$TMP"
}

unpack_mac () {
    f=$1
    dir=$2
    tar xfz "$f"
    sdk=$TMP/$(ls "$TMP" | head -1)
    _install_headers "$sdk"/usr/local/include
    cp "$sdk"/usr/local/lib/libqhyccd.dylib "$SRC"/cameras/qhyccdlibs/mac/"$dir"/
    rm -rf "$sdk"
}

unpack_linux () {
    f=$1
    arch=$2
    tar xfz "$f"
    sdk=$TMP/$(ls "$TMP" | head -1)
    _install_headers "$sdk"/usr/local/include
    cp "$sdk"/usr/local/lib/libqhyccd.a "$SRC"/cameras/qhyccdlibs/linux/"$arch"/
    rm -rf "$sdk"
}

unpack_qxx () (
    # QinXiaoXu's qhyccd_lib_header_*.zip packaging 4/2023

    set -e
    set -o pipefail

    unzip "$1"
    cd qhyccd_lib_header_*

    # headers
    (
        cd headers
        sed -E 's/^#define QHYCCD_PCIE_SUPPORT[[:space:]]+1[[:space:]]*$/#define QHYCCD_PCIE_SUPPORT 0/' \
            <config.h >qhyccd_config.h
        for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
            sed -E \
                -e 's/config\.h/qhyccd_config\.h/' \
                -e 's,^[[:space:]]*#[[:space:]]*include[[:space:]]+"cyapi.h",//#include "cyapi.h",' \
                "$f" > "$SRC"/cameras/"$f"
        done
    )

    # libqhyccd20230510_arm32.tar::libqhyccd.a => cameras/qhyccdlibs/linux/armv7/libqhyccd.a
    mkdir linux-arm32
    (
        cd linux-arm32
        tar xf ../libqhyccd20*_arm32.tar
        install -m 0644 libqhyccd.a "$SRC"/cameras/qhyccdlibs/linux/armv7/
    )

    # libqhyccd20*_linux64.tar::libqhyccd.a => cameras/qhyccdlibs/linux/x86_64/libqhyccd.a
    mkdir linux-x86_64
    (
        cd linux-x86_64
        tar xf ../libqhyccd20*_linux64.tar
        install -m 0644 libqhyccd.a "$SRC"/cameras/qhyccdlibs/linux/x86_64/
    )

    # libqhyccd20230510_mac64.zip::libqhyccd.23.5.10.17.dylib => cameras/qhyccdlibs/mac/x86_64/libqhyccd.dylib
    mkdir mac-x86_64
    (
        cd mac-x86_64
        unzip ../libqhyccd20*_mac64.zip
        find . -type l | xargs rm
        if (($(ls *.dylib | wc -l) != 1)); then
            echo "too many dylib files!" >&2
            exit 1
        fi
        install -m 0755 *.dylib "$SRC"/cameras/qhyccdlibs/mac/x86_64/libqhyccd.dylib
    )

    # windows
    mkdir win32
    (
        cd win32
        unzip ../libqhyccd20*_windows.zip
        install -m 0755 x86/qhyccd.dll "$SRC"/WinLibs/
        install -m 0644 x86/qhyccd.lib "$SRC"/cameras/
    )
)

# === main ===

umask 022

for f in "$@"; do
    if [[ -d $f ]]; then
        unpack_win_dir "$f"
        continue
    fi
    if [[ ! -f $f ]]; then
        echo "usage: $0 FILE..." >&2
        exit 1
    fi
    case $(basename "$f") in
        # current packaging as of 6/2024
        sdk_WinMix*.zip)   unpack_win   "$f" ;;
        sdk_macMix*tgz)    unpack_mac   "$f" universal ;;
        sdk_arm32*tgz)     unpack_linux "$f" arm32 ;;
        sdk_Arm64*tgz)     unpack_linux "$f" arm64 ;;
        sdk_linux64_*tgz)  unpack_linux "$f" x86_64 ;;
        # these are obsolete as of 6/2024 and can be removed
        qhyccd_lib_header_*.zip) unpack_qxx "$f" ;;
        MAC*64*tgz)        unpack_mac   "$f" x86_64 ;;
        MAC*tgz)           unpack_mac   "$f" x86_32 ;;
        *zip)              unpack_win   "$f" ;;
        RPI*)              unpack_linux "$f" armv7 ;;
        LINUX_X64_qhyccd*) unpack_linux "$f" x86_64 ;;
        LINUX_qhyccd*)     unpack_linux "$f" x86_32 ;;
        *) echo "TODO" >&2 ; exit 1 ;;
    esac
done
