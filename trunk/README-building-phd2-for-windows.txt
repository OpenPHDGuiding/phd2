Set up a build environment from scratch on MS-Windows to compile PHD2.


1) Get the Visual Studio C compiler.
2) Install wxWidgets
3) Install fitsio
4) Get PHD2 sources
5) Compile PHD2


1) If you do not yet have a version of Visual Studio then get a free copy at
   http://www.microsoft.com/visualstudio/eng/downloads#d-2010-express

2) Download wxWidgets 2.9.4 from http://sourceforge.net/projects/wxwindows/files/
   In order to get a static copy of wx working, you need to build it yourself.
   Install it in <install path>, and cd into the build directory:

   <install path>\wxWidgets-2.9.4\build\msw

   The following step might not be needed, at least not on WinXP.
   Load the VC variables so nmake and the command line compilers are in the path:
   "\Program Files (x86)\Microsoft Visual Studio 10.0\vc\bin\vcvars32.bat"

   Do two command line builds, one for release, one for debug:

   nmake -f makefile.vc BUILD=release SHARED=0
   nmake -f makefile.vc BUILD=debug  SHARED=0

   Set a WXWIN environment variable to point to <install path> so that
   the PHD2 build will find the bits.

3) Download the CFITSIO Software Library from http://heasarc.gsfc.nasa.gov/fitsio/
   Extract in C:\dev\fits
   Set an environment variable CFITSIO to C:\dev\fits

4) Get PHD2 sources from trunk branch at http://code.google.com/p/open-phd-guiding/

5) Open phd2.sln in Visual Studio, select release or build target and build.

