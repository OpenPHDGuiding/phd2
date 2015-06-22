#!/bin/bash
#
# usage: build_help_pdf.sh 
#
#    build the pdf PHD2 documentation from help source files
#
# Require:  wkhtmltopdf
#           available from http://wkhtmltopdf.org/
#

build=$(cd $(dirname $0); /bin/pwd)
top="$build/.."
help="$top/help"

TMP=/tmp/phd2help$$
mkdir "$TMP"
trap "rm -rf $TMP" 2 3 15

# Get the list of html files in the right order from PHD2GuideHelp.hhc
fl=$(grep '<param name="Local" value=' $help/PHD2GuideHelp.hhc | grep 'htm">'| cut -d\" -f4| awk '!x[$0]++ {print "'$help\/'" $0}')

# generate the title page
phdversion=$("$build"/get_phd_version "$top"/phd.h)
# TODO: make hires icon 
convert $top/icons/phd.xpm -resize 200x200 $TMP/phd.png
dt='Edited: '$(LC_ALL=C date '+%B %d %Y')
cat > $TMP/00_title.html << EOF
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8" />
</head>
<body>
<center>
<br/><br/><br/><br/><br/><br/><br/><br/>
<H1> PHD2 Guiding 
<br/>
$phdversion
</H1>
<H2>Documentation
</H2>
<H3>
$dt
<br/>
</H3>
<br/><br/>
<br/><br/>
<br/><br/>
<img src="$TMP/phd.png">
</center>
</body>
</html>
EOF

# generate the default toc
wkhtmltopdf --dump-default-toc-xsl > $TMP/toc.xsl

# create pdf
if [ -z $DISPLAY ]; then useX=''; else useX='--use-xserver' ;fi
wkhtmltopdf $useX --dpi 96 --enable-toc-back-links  --enable-external-links --enable-internal-links --footer-right '[page]' $TMP/00_title.html toc --xsl-style-sheet $TMP/toc.xsl $fl $top/PHD2.pdf

# cleanup
rm $TMP/phd.png $TMP/toc.xsl $TMP/00_title.html
rmdir $TMP

