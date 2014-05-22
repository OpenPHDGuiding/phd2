#!/bin/sh

# usage: build_help.sh [-w]
#
#    build PHD2GuideHelp.zip from help source files
#
# Options:
#    -w      also build help files for the web site, man.tar.gz
#

set -e

do_web=
if [ "$1" = "-w" ]; then
    do_web=1
fi

build=$(cd $(dirname $0); /bin/pwd)
top="$build/.."
help="$top/help"

TMP=/tmp/phd2help$$
mkdir "$TMP"
trap "rm -rf $TMP" 2 3 15

cp -p "$help"/* "$TMP"/

(
    cd "$TMP"

    # generate the hhk (index) file
    "$build"/build_help_hhk

    # rebuild the help zip file
    rm -f "$top"/PHD2GuideHelp.zip
    zip -r "$top"/PHD2GuideHelp.zip .

    if [ -n "$do_web" ]; then
        echo "building web pages..."

        # generate the HTML table of contents page
        phdversion=$("$build"/get_phd_version "$top"/phd.h)
        "$build"/build_help_toc_html "$phdversion" > index.html

        # generate the online help files
        tar cfz "$top"/man.tar.gz *.html *.htm *.png

        echo "done"
    fi
)

rm -rf "$TMP"
