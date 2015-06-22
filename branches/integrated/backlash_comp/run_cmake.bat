@mkdir tmp
@cd tmp
cmake -Wno-dev -G "Visual Studio 12" -DwxWidgets_PREFIX_DIRECTORY=%WXWIN% -DOpenCVRoot=%OPENCV_DIR% ..
@cd ..
