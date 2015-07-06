#!/bin/sh

#
# Internationalization (i18n) is managed in wxWidget with GNU gettext
# gettext is available for Windows at http://gnuwin32.sourceforge.net/packages/gettext.htm
#
# After installing gettext, add the the GnuWin32\bin in your PATH
#
# Script to compile localisition messages.po file to messages.mo file
# usable by PHD Guiding to display localised interface
#

here=$(cd "$(dirname "$0")"; /bin/pwd)
SRC=$(cd "$here"/..; /bin/pwd)

set -x

cd $SRC/locale
for d in *; do
    if [ -d "$d" ]; then
        (
            cd $d
            msgfmt messages.po
        )
    fi
done
