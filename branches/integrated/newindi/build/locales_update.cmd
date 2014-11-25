echo off
REM
REM Internationalization (i18n) is managed in wxWidget with GNU gettext.
REM gettext is available for Windows at http://gnuwin32.sourceforge.net/packages/gettext.htm
REM
REM After installing gettext, add the the GnuWin32\bin in your PATH.
REM
REM Script to extract new string to translate from PHD source code and
REM update every localisation messages.po file. Tranlsating string should
REM use the _() macro.
REM
xgettext *.cpp *.h -C --from-code=CP1252 --keyword="_" --keyword="wxPLURAL:1,2" --keyword="wxTRANSLATE"
cd locale
copy /Y messages.pot messages-old.pot
msgmerge messages-old.pot ..\messages.po -o messages.pot
for /D %%d in (*.*) do (
    cd %%d
    copy /Y messages.po messages-old.po
    msgmerge messages-old.po ..\messages.pot -o messages.po
    cd ..
)


cd..
