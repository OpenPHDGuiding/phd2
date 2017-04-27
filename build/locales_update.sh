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

set -e
cmake --build "$BUILDTREE" --target extract_locales_from_sources
cmake --build "$BUILDTREE" --target update_locales_in_source_tree
