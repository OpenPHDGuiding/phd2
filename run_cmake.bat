@mkdir tmp
@cd tmp
cmake -Wno-dev -G "Visual Studio 17" -A Win32 "-DwxWidgets_PREFIX_DIRECTORY=%WXWIN%" ..
@cd ..
