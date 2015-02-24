echo off
REM
REM Internationalization (i18n) is managed in wxWidget with GNU gettext
REM gettext is available for Windows at http://gnuwin32.sourceforge.net/packages/gettext.htm
REM
REM After installing gettext, add the the GnuWin32\bin in your PATH
REM
REM Script to compile localisition messages.po file to messages.mo file
REM usable by PHD Guiding to display localised interface
REM
cd locale
for /D %%d in (*.*) do (
    cd %%d
    msgfmt messages.po
    cd ..
)


cd..