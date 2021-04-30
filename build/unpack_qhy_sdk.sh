#!/bin/bash

set -ex

function unpack_win {
    zip=$1
    zip=$(cd $(dirname "$zip"); /bin/pwd)/$(basename "$zip")

    unzip "$zip"

    sdk=$TMP/$(ls $TMP | head -1)

    (
        cd "$sdk"/include
        cp config.h qhyccd_config.h
        for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
            sed -e 's/config\.h/qhyccd_config.h/' -e 's/#include \"cyapi.h\"//' $f > "$SRC"/cameras/qhyccd/$f
        done
    )

    # Packaging starting with April 2020 SDK release
    cp "$sdk"/x86/qhyccd.dll "$SRC"/WinLibs/
    cp "$sdk"/x86/tbb.dll "$SRC"/WinLibs/
    cp "$sdk"/x86/qhyccd.lib "$SRC"/cameras/
}

function unpack_mac {
    f=$1
    dir=$2

    tar xfz $f

    sdk=$TMP/$(ls $TMP | head -1)
    cd $sdk

    (
        cd usr/local/include
        cp config.h qhyccd_config.h
        for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
            sed -e 's/config\.h/qhyccd_config.h/' -e 's/#include \"cyapi.h\"//' $f > "$SRC"/cameras/qhyccd/$f
        done
    )
    cp usr/local/lib/libqhyccd.dylib "$SRC"/cameras/qhyccdlibs/mac/$dir
}

function unpack_linux {
    f=$1
    arch=$2

    tar xfz $f

    sdk=$TMP/$(ls $TMP | head -1)
    cd $sdk

    (
        cd usr/local/include
        cp config.h qhyccd_config.h
        for f in qhyccd.h qhyccd_config.h qhyccdcamdef.h qhyccderr.h qhyccdstruct.h; do
            sed -e 's/config\.h/qhyccd_config.h/' -e 's/#include \"cyapi.h\"//' $f > "$SRC"/cameras/qhyccd/$f
        done
    )
    cp usr/local/lib/libqhyccd.a "$SRC"/cameras/qhyccdlibs/linux/$arch/
}

# === main ===

SRC=$(cd $(dirname "$0")/..; /bin/pwd)

TMP=$(mktemp -d /tmp/qhysdk.XXXX)
trap "rm -rf $TMP" INT TERM QUIT EXIT

cd $TMP

for f in $*; do
    if [ ! -f "$f" ]; then
        echo "usage: $0 FILE..." >&2
        exit 1
    fi

    case $(basename $f) in
        sdk_macMix*.tgz)	unpack_mac $f;;
        sdk_arm32*.tgz)		unpack_linux $f armv7 ;;
        sdk_linux32*.tgz)	unpack_linux $f x86_32 ;;
        sdk_linux64*.tgz)	unpack_linux $f x86_64 ;;
        sdk_WinMix*.zip)	unpack_win $f;;
        *) echo "Unsupported arch. Add support for it!" >&2 ; exit 1 ;;
    esac
done
