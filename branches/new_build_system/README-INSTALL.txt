 
On Linux
Needed: wx3.0

In a shell
sudo apt-get install wx3.0-i18n
sudo apt-get install wx3.0-headers
 
 
 
 
 
On OSX

Installation of wxWidgets

cd path/to/wxwidgets
mkdir build-cocoa
cd build-cocoa
../configure --enable-universal_binary=i386,x86_64 --disable-shared --with-libpng=builtin --with-cocoa --prefix=$my_prefix \
             --with-macosx-sdk=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/ \
             --with-macosx-version-min=10.7 \
             CXXFLAGS="-stdlib=libc++ -std=c++11" \
             OBJCXXFLAGS="-stdlib=libc++ -std=c++11" \
             CPPFLAGS="-stdlib=libc++"  \
             LDFLAGS="-stdlib=libc++" CXX=clang++ CXXCPP="clang++ -E" CC=clang CPP="clang -E"
make -j8
make install