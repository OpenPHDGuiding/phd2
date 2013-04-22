In order to get a static copy of wx working, you need to build it yourself.

For 2.9.4, I downloaded the windows source installer from:
    http://sourceforge.net/projects/wxwindows/files/2.9.4/wxMSW-2.9.4-Setup.exe/download

Installed it in <install path>, and cd'ed into the build directory:

<install path>\wxWidgets-2.9.4\build\msw

load the VC variables so nmake and the command line compilers are
in the path:

"\Program Files (x86)\Microsoft Visual Studio 10.0\vc\bin\vcvars32.bat"

then do two command line builds, one for release, one for debug:

nmake -f makefile.vc BUILD=release SHARED=0
nmake -f makefile.vc BUILD=debug  SHARED=0

set the WXWIN environment variable to point to 
<install path> so that the PHD2 build will pick find the bits.
