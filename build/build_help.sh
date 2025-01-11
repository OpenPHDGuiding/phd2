#!/bin/sh

# usage: build_help.sh [-w] [build_tree]
#
#    build PHD2GuideHelp.zip and man.tar.gz from help source files
#
# Options:
#    -w      also build help files for the web site, man.tar.gz
#

set -e

do_web=
if [ "$1" = "-w" ]; then
    do_web=1
    shift
fi

if [ -z "$do_web" ]; then
    echo "build_help.sh is obselete. The help .zip files are now built automatically by the cmake-generated build system" >&2
    exit 1
fi

build=$(cd $(dirname $0); /bin/pwd)
top="$build/.."

BUILDTREE=$top/tmp
if [ -n "$1" ]; then
    BUILDTREE=$1
    shift
fi

TMP=/tmp/phd2help$$
trap "rm -rf $TMP" 2 3 15

help=$top/help

mkdir -p "$TMP"
cp -p "$help"/* "$TMP"/

(
    cd "$TMP"

    echo "building web pages..."

    # generate the HTML table of contents page
    phdversion=$("$build"/get_phd_version "$top"/src/phd.h)
    "$build"/build_help_toc_html "$phdversion" > index.html

    # generate the online help files
    tar cfz "$BUILDTREE"/man.tar.gz *.html *.htm *.png

    echo "done"
)

rm -rf "$TMP"

echo "building PDF user manual..."
"$build"/build_help_pdf.sh
echo "done"
