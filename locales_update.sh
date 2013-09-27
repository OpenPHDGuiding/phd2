#!/bin/sh

#
# Internationalization (i18n) is managed in wxWidgets with GNU gettext.
# gettext is available for Windows at http://gnuwin32.sourceforge.net/packages/gettext.htm
#
# After installing gettext, add the the GnuWin32\bin in your PATH.
#
# Script to extract new string to translate from PHD source code and
# update every localisation messages.po file. Tranlsating string should
# use the _() macro.
#

set -x

xgettext *.cpp -C --from-code=CP1252 --keyword="_" --keyword="_NOTRANS" --keyword="wxPLURAL:1,2"
cd locale
cp messages.pot messages-old.pot
msgmerge messages-old.pot ../messages.po -o messages.pot
for f in */messages.po; do
    (
        cd $(dirname "$f")
        cp messages.po messages-old.po
        msgmerge messages-old.po ../messages.pot -o messages.po
    )
done
