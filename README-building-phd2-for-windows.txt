Set up a build environment from scratch on MS-Windows to compile and run PHD2.

0) Install PHD1
1) Get the Visual Studio C compiler
2) Install Visual Leak Detector
3) Install wxWidgets
4) Install fitsio
5) Install opencv
6) Get PHD2 sources
7) Compile PHD2

0) PHD2 uses some DLLs that are distributed with PHD1, so you'll need
   to download and install PHD1 before you can run PHD2. I find it
   easiest to set my PATH environment variable to include the PHD1
   directory. That allows PHD2 to to find the required DLLs.

1) If you do not yet have a version of Visual Studio 2010 then get a
   free copy at
   http://www.microsoft.com/visualstudio/eng/downloads#d-2010-express

   The download you want is "Visual C++ 2010 Express".

2) Download and install Visual Leak Detector (VLD) Version 2.3.0
   from https://vld.codeplex.com/

   The VC++ project files assume you have VLD installed in the default
   location:

       c:\program files (x86)\visual leak detector\

   so make sure you install it there.

3) Download wxWidgets 3.0.0 from http://sourceforge.net/projects/wxwindows/files/
   In order to get a static copy of wx working, you need to build it yourself.
   Install it in <install path>, and cd into the build directory:

   <install path>\wxWidgets-3.0.0\build\msw

   The following step might not be needed, at least not on WinXP.
   Load the VC variables so nmake and the command line compilers are in the path:
   "\Program Files (x86)\Microsoft Visual Studio 10.0\vc\bin\vcvars32.bat"

   Do two command line builds, one for release, one for debug:

   nmake -f makefile.vc BUILD=release SHARED=0
   nmake -f makefile.vc BUILD=debug  SHARED=0

   Set a WXWIN environment variable to point to <install path> so that
   the PHD2 build will find the bits.

4) Download the CFITSIO V3.340 Software Library from
   http://heasarc.gsfc.nasa.gov/fitsio/
   Extract in C:\dev\fits
   Set an environment variable CFITSIO to C:\dev\fits

5) Download OpenCV 2.4.5 from http://opencv.org/.  Install it, and set
   the OPENCV_DIR environment variable to point to the build\
   subdirectory of where you put it.  For example, if you install it
   in C:\opencv, set OPENCV_DIR to C:\opencv\build

   You will have to copy 3 Debug and Release DLLs from
   opencv-2.4.5\build\x86\vc10\bin

   The files you need to copy are: 
        Debug: opencv_core245d.dll  opencv_highgui245d.dll  opencv_imgproc245d.dll
        Release: opencv_core245.dll  opencv_highgui245.dll  opencv_imgproc245.dll

   You can either copy these to the Debug\ and Release\ build
   directories that VC creates, or copy them to your PHD1 install
   directory. If you copy them to the PHD1 install directory, make
   sure that directory is in your PATH environment variable.

6) Get PHD2 sources from trunk branch at
   http://code.google.com/p/open-phd-guiding/

7) Open phd2.sln in Visual Studio, select the Release or Debug build
   target and build.
