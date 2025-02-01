#!/bin/bash
#
# usage: build_help_pdf.sh [build_tree]
#
#    build the pdf PHD2 documentation from help source files
#
# Require:  wkhtmltopdf
#           available from http://wkhtmltopdf.org/
#

build=$(cd $(dirname $0); /bin/pwd)
top=$(realpath "$build/..")
help="$top"/help

BUILDTREE=$top/tmp
if [ -n "$1" ]; then
    BUILDTREE=$1
    shift
fi

TMP=/tmp/phd2help$$
mkdir "$TMP"
trap "rm -rf '$TMP'" 2 3 15

# Get the list of html files in the right order from PHD2GuideHelp.hhc
files=$(grep '<param name="Local" value=' $help/PHD2GuideHelp.hhc \
          | grep 'htm">' | cut -d\" -f4 | awk '!x[$0]++ {print "'$help\/'" $0}')

phdversion=$("$build"/get_phd_version "$top"/src/phd.h)
date=$(LC_ALL=C date '+%B %d, %Y')

# create pdf
if [ -z "$DISPLAY" ]; then
    useX=''
else
    useX='--use-xserver'
fi

titlepg="$TMP"/00_title.html
tocxsl="$TMP"/toc.xsl
output="$BUILDTREE"/PHD2_User_Guide.pdf
tocxsl_native=$tocxsl
logo="$top"/icons/phd2_128.png

case $(uname -o) in
    Cygwin)
        # The Cygwin port of wkhtmltopdf does not work (2016/1/20)
        # Under Cygwin we can use the Win32 native port, but we have
        # to go through some contortions to pass DOS-style paths

        wkhtmltopdf="/c/Program Files/wkhtmltopdf/bin/wkhtmltopdf.exe"

        touch "$titlepg"
        titlepg=$(cygpath -d "$titlepg")

        touch "$tocxsl"
        tocxsl=$(cygpath -d "$tocxsl")

        s=""
        for f in $files; do
            d=$(cygpath -d "$f")
            s="$s $d"
        done
        files=$s

        touch "$output"
        output=$(cygpath -d "$output")

        logo=$(cygpath -d "$logo")
        ;;
    *)
        wkhtmltopdf="wkhtmltopdf"
        ;;
esac

# generate the title page

cat >"$titlepg" <<EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
  <head>
    <meta content="text/html; charset=utf-8" http-equiv="content-type">
    <title>Cover_Page</title>
  </head>
  <body>
    <center>
      <br><br><br><br><br><br><br><br><br><br><br><br>
      <img src="file:///${logo}">
      <br><br><br><br><br>
      <h1>PHD2 v${phdversion}</h1>
      <h1>User Guide</h1>
      <br><br><br><br><br><br><br><br><br>
      <br><br><br><br><br><br><br><br><br>
      <h2>${date}</h2>
    </center>
  </body>
</html>
EOF

# generate the default toc
"$wkhtmltopdf" --dump-default-toc-xsl >"$tocxsl_native"

"$wkhtmltopdf" -q $useX --dpi 96 \
               --allow "$top" \
               --allow "$TMP" \
               --enable-toc-back-links --enable-external-links \
               --enable-internal-links --footer-right '[page]' \
               cover "$titlepg" toc --xsl-style-sheet "$tocxsl" \
               $files "$output"

# cleanup
rm -rf "$TMP"
