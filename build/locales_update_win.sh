#!/bin/sh

# locales_update.sh - update translation files in the source tree

# usage: locales_update.sh [build_tree_location]

set -x

here=$(cd "$(dirname $0)"; /bin/pwd)
SRC=$(cd "$here"/..; /bin/pwd)

BUILDTREE=$SRC/tmp
if [ -n "$1" ]; then
    BUILDTREE=$1
    shift
fi

CMD="/c/Windows/System32/cmd.exe"
CMAKE=$(cygpath -w '/c/Program Files/CMake/bin/cmake.exe')
B=$(cygpath -w "$BUILDTREE")

set -e
"$CMD" /c "$CMAKE" --build "$B" --target extract_locales_from_sources
"$CMD" /c "$CMAKE" --build "$B" --target update_locales_in_source_tree
