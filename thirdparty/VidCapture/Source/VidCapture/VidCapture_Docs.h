/// \file VidCapture_Docs.h
/// \brief Main Documentation page for VidCapture.
///
/// There is no active source here - just documentation.

///
/// \mainpage CodeVis VidCapture (v0.30)
/// <CENTER>
/// Simplified Video Capture for Web Cameras.<BR>
/// <i>
/// Copyright &copy; 2003-2004 by 
/// <a href="http://www.codevis.com/mellison.html">Michael Ellison</a> (mike@codevis.com).
/// </i><BR>
/// </CENTER>
/// <P>
/// <hr>
/// <a name="toc"></a>
/// <h2>Table of Contents</h2>
/// <UL>
/// <LI> <a href="#overview">Overview</a>
/// <LI> <a href="#project">Including VidCapture in a Project</a>
/// <LI> <a href="#using">Using VidCapture Library</a>
/// <LI> <a href="#dll">Using VidCapDLL.dll</a>
/// <LI> <a href="#history">VidCapture History</a>
/// <LI> <a href="#future">Future Directions...</a>
/// <LI> <a href="#credits">Credits</a>
/// <LI> <a href="#license">License Agreement</a>
/// <LI> Source Documentation (generated with <a href="http://www.doxygen.org">doxygen</a>)
///      <UL>
///         <LI> <a href="hierarchy.html">Class&nbsp;Hierarchy</a> 
///         <LI> <a  href="classes.html">Alphabetical&nbsp;List</a> 
///         <LI> <a  href="annotated.html">Compound&nbsp;List</a> 
///         <LI> <a  href="files.html">File&nbsp;List</a> 
///         <LI> <a  href="functions.html">Compound&nbsp;Members</a> 
///         <LI> <a  href="globals.html">File&nbsp;Members</a>
///      </UL>
/// <LI>Downloads and Support
///   <UL>
///      <LI><A href="https://sourceforge.net/project/showfiles.php?group_id=100837">Download latest CodeVis VidCapture library</a>
///      <LI><A href="http://sourceforge.net/projects/vidcapture/">VidCapture's SourceForge Page</a>
///      <LI> Go to the <a href="http://www.codevis.com/phpBB2/index.php">
///         CodeVis Public Forums</a>
///      <LI> Go to <a href="http://www.codevis.com">CodeVis</a>.
///   </UL>
/// </UL>
///
/// <P>
/// <a href="#top">^Top</a>
/// <hr>
/// <a name="overview"></a>
/// <h2>Overview</h2>
/// VidCapture provides a simple interface to capture images from web cameras or other
/// supported video capture devices.  All you have to do is initialize it,
/// choose a device, and start capturing - no confusing filter graphs, 
/// input pins, or IUnknowns to deal with.  VidCapture also returns the images 
/// in an easy to use but lightweight class so you can process them efficiently.
/// <P>
/// It is geared towards computer vision and image processing applications.
/// As such, it does not directly provide support for previewing, capturing 
/// the video as an AVI, streaming the video to a VHS recorder while adding
/// subtitles, or any of the multitude of other things people might
/// want to do with video capture that make DirectShow so much fun to 
/// work with.
/// <P>
/// <I>VidCapture just gives you the image data in the format you want as
/// quickly and as painlessly as possible.</i>
/// <P>
/// VidCapture may be used in commercial and non-commercial programs
/// provided that you read and abide by the <a href="#license">license
/// agreement</a> found at the end of this documentation.
/// <P>
/// If you need support for VidCapture, please first check the documentation,
/// then check the <a href="http://sourceforge.net/projects/vidcapture/">
/// project page on SourceForge</a> to see if any information is available in
/// the forums or documentation there. There may also be information available 
/// on the <a href="http://www.codevis.com/phpBB2/index.php">CodeVis Public
/// Forums</a>.  If you can't find a solution in the above places, feel free to 
/// <a href="mailto:mike@codevis.com">email me</a>.  If you'd like VidCapture 
/// integrated into an existing system or need any sort of vision system
/// designed and built,  <a href="http://www.codevis.com/mellison.html"> I 
/// may be available for contracting or hire</a>.
/// <P>
/// For the latest released version of VidCapture, 
/// <a href="http://sourceforge.net/project/showfiles.php?group_id=100837">click
/// here</a>.  For the latest development version, you can use CVS to grab
/// it directly from SourceForge.  For help grabbing the source from CVS,
/// <a href="http://sourceforge.net/cvs/?group_id=100837">click here</a>.
/// <P>
/// <h3>Features at a glance</h3>
/// <TABLE><TR><TH>Feature</TH><TH>Library function or reference</TH><TH>DLL Function</TH></TR>
/// <TR><TD><B>Capture device enumeration and selection</B>
///     </TD><TD>
///         <small><em>CVVidCapture::GetNumDevices(), CVVidCapture::GetDeviceInfo()</em></small>
///     </TD>
///     <TD>
///         <small><em>CVGetNumDevices(), CVGetDeviceName()</em></small>
///     </TD>
///     </TR>
/// <TR><TD><B>Capture resolution enumeration and selection</B>
///     </TD><TD> 
///       <small><em>CVVidCapture::GetNumSupportedModes()<BR> 
///       CVVidCapture::GetModeInfo()<BR>CVVidCapture::SetMode()</em></small>
///     </TD>
///     <TD>
///         <small><em>CVDevGetNumModes(), CVDevGetModeInfo()</em></small>
///     </TD>
///     </TR>
/// <TR><TD><B>Single frame grabs with selectable image formats</B>
///     </TD><TD>
///       <small><em>CVVidCapture::Grab()</em></small>
///     </TD>
///     <TD>
///         <small><em>CVDevGrabImage()</em></small>
///     </TD>
///     </TR>
/// <TR><TD><B>Continuous capture mode with selectable image formats</B>
///     </TD><TD>
///        <small><em>CVVidCapture::StartImageCap()</em></small>
///     </TD>
///     <TD>
///         <small><em>CVDevStartCap()</em></small>
///     </TD>
///     </TR>
/// <TR><TD><B>Selectable image formats<BR>
///         <UL>
///            <LI>8bit grey
///            <LI>24bit RGB
///            <LI>floating point RGB
///         </UL></B>
///     </TD><TD>
///          <small><em>CVImage<BR>CVImageGrey<BR>CVImageRGB24<BR>CVImageRGBFloat</em></small>
///     </TD>
///     <TD>
///         <small><em>CVImageStructs.h</em></small>
///     </TD>
///     </TR>
/// <TR><TD><B>Raw continuous capture mode</B>
///     </TD><TD>
///        <small><em>CVVidCaptureDSWin32::StartRawCap()</em></small>
///     </TD>
///     <TD>
///         <small><em>not supported</em></small>
///     </TD>
///     </TR>
/// <TR><TD><B>Image import/export to .PPM, .PGM, and additional formats.</B>
///     </TD><TD>
///          <small><em>CVImage::Save(), CVImage::Load()</em></small>
///     </TD><TD>
///       <small><em>CVSaveImage(), CVLoadImage(), CVIMAGESTRUCT</em></small>
///     </TD></TR>
/// <TR><TD><B>Reference-counted image class supporting sub-imaging for easy 
///            image manipulation</B>
///     </TD><TD>
///     <small><em>CVImage</em></small>
///     </TD><TD>
///       <small><em>not supported</em></small>
///     </TD></TR>
/// </TABLE>
///
/// <P>
/// <a href="#top">^Top</a>
/// <hr>
/// <a name="project"></a>
/// <h2>Including VidCapture in a Project</h2>
/// Originally (version 0.1), VidCapture was distributed as a set of
/// C++ files that you added to your project.  You can still do that -
/// just grab the files from the ./Source/VidCapture directory from the
/// install and you're ready to go.
/// <P>
/// Now, however, the recommend method for C++ programmers is to use the
/// static library included with the install - VidCapLib.lib. This will
/// give you the most control over the video capture library without
/// having to recompile all the time.
/// <P>
/// For other languages, there is now a .DLL available (VidCapDll.dll)
/// that you can use that has a C-style interface.  The symbols in the
/// DLL are undecorated and it uses Pascal (standard Windows API)
/// calling conventions, so it should be useable from just about any
/// serious language available on Windows. Instructions for using the
/// DLL library are <a href="#dll">further down the page</a>.
/// <P>
/// If you're using the files directly or compiling the static lib, you may
/// define CVRES_VIDCAP_OFFSET to offset all of the result codes if you wish
/// to use it alongside a similar error system without collisions. See
/// CVRes.h for details.  HOWEVER... If you're using the DLL, I'd recommend
/// against this - if you do offset the error codes, you can not use future
/// default releases of the DLL and your program may conflict with others
/// using the VidCapDLL.
/// <P>
/// VidCapture is setup so that documentation can automatically be
/// generated. To generate the documentation from the source, you'll need
/// <a href="http://www.doxygen.org">doxygen</a> installed. Open
/// a command prompt and go to the ./Project directory of the VidCapture
/// install, then run:<P>
/// <CODE>doxygen Doxyfile.cfg</CODE>
/// <P>
/// The output is placed in the ./Build/Docs/html folder.
/// <P>
/// <h2>Including the C++ Static Library...</h2>
/// <P>
/// This overview details using the static library (VidCapLib.lib) in a
/// C++ project.  If you want to use the .DLL, go <a href="#dll">here</a>.
/// <P>
/// <h3>DirectX SDK</h3>
/// First, you'll need the <a href="http://msdn.microsoft.com/library/default.asp?url=/downloads/list/directx.asp">
/// latest DirectX SDK</a> installed  in order to compile.  
/// I've been using DirectX 9 for development. Make sure your #include 
/// directories are set up to let the SDK take precedent over any older
/// versions you may have.
///
/// <h3>Multithreaded Runtime</h3>
/// You will need to use multithreaded runtime libraries when compiling.
/// <P>
/// In Visual Studio 6.0:
/// <OL>
/// <LI>Go to <b>Project->Settings</b> on the menu bar.
/// <LI>Make sure your project is selected under <b>Settings For</b>.
/// <LI>Select the <B>C++</B> tab
/// <LI>Select <b>Code Generation</b> from the <b>Category</b> pull down.
/// <LI>Select <b>Win32 Debug</b> from the <b>Settings For:</b> pull down.
/// <LI>Select <b>Debug Multithreaded</b> for the debug runtime library.
/// <LI>Select <b>Win32 Release</b> from the <b>Settings For:</B> pull down.
/// <LI>Select <b>Multithreaded</b> for the release mode runtime library.
/// </OL>
///
///
/// <P>
/// <h3>Required Link Libraries</h3>
/// Required link libraries:<BR>
/// <UL>
///   <LI><B>VidCapLib.lib</B> ( or <B>VidCapLib_db.lib</B> for debugging )
///   <LI>kernel32.lib 
///   <LI>ole32.lib 
///   <LI>oleaut32.lib 
///   <LI>Strmiids.lib 
///   <LI>Quartz.lib 
///   <LI>and whatever multithreaded C++ runtime libraries your compiler uses... <BR>
///       (tested with Visual C++ 6.0)
/// </UL>
/// For Visual C++ 6.0, these are set in the <b>Project->Settings</b> 
/// menu under the <b>Link</b> tab in the <b>Object/Library modules</b>
/// box.<BR>
///
/// <h3>Includes</h3>
/// All of the necessary headers for using the VidCapture are included
/// by the VidCapture.h header file.  Just add the line:<P>
/// <Code>#include "VidCapture.h"</CODE><P>
/// at the top of your .cpp and you should be set.  
/// You may need to add the path to the file to your
/// preprocessor's #include directories.
///
/// <P>
/// <a href="#top">^Top</a>
/// <hr>
/// <a name="using"></a>
/// <h2>Using the VidCapture Library (VidCapLib.lib)</h2>
/// You may want to check out the Example.cpp sample program for
/// a simple example of how to use the VidCapture library - most
/// of the functionality is covered in it.
/// <P>
/// <h3>Steps required to capture video</h3>
/// <OL>
/// <LI>Acquire a CVVidCapture object by calling CVPlatform::AcquireVideoCapture().<BR>
///     To do this, use the code:<P>
///     <CODE>CVVidCapture* vidCap = CVPlatform::GetPlatform()->AcquireVideoCapture();</CODE><P>
///     You can also just instantiate a CVVidCaptureDSWin32 object, but using the platform 
///     manager will make it easier to modify later on.
/// <BR><BR>
/// <LI>Call CVVidCapture::Init() to initialize the video capture subsystem.
/// <BR><BR>
/// <LI>Call CVVidCapture::GetNumDevices() to retrieve the number of devices
///     available.  Then call CVVidCapture::GetDeviceInfo() to retrieve the 
///     information for each device.
/// <BR><BR>
/// <LI>Call CVVidCapture::Connect() with the desired device name.
/// <BR><BR>
/// <LI>Call CVVidCapture::GetNumSupportedModes() to retrieve the number of 
///     video modes that are supported on the connected device. Then
///     call CVVidCapture::GetModeInfo() for each index value to get information
///     about the nodes.
/// <BR><BR>
/// <LI>Select the desired video mode by calling SetMode().
/// <BR><BR>
/// <LI>Call CVVidCapture::GetPropertyInfo() with the properties from 
///     CVVidCapture::CAMERA_PROPERTY to retrieve information about 
///     the properties the camera supports such as brightness, contrast,
///     and hue. You can also modify the properties while capturing.
/// <BR><BR>
/// <LI>Set any properties you want to change by calling CVVidCapture::SetProperty().
/// <BR><BR>
/// <LI>Start capturing video by calling CVVidCapture::StartImageCap(). You'll need to supply
///     it with a callback to be called on each frame - see 
///     CVVidCapture::CVVIDCAP_CALLBACK for the callback definition.
/// <BR><BR>
/// <LI>On each callback, first check the status code passed in.  If it's an error,
///     (e.g. CVSUCCESS(status) yields a false result), then you'll probably 
///     need to notify your main thread that it's halted so
///     you can do an orderly shutdown on the capture (e.g. call CVVidCapture::Stop() and
///     CVVidCapture::Disconnect() ), figure out what went wrong, and try again.  
///     <P>
///     <i>You still most certainly need to call CVVidCapture::Stop() from the main thread - NOT
///     from the callback. Calling it from the callback will result in a deadlock!</i>
///     <P>
///     Usually, if you get an error here, what's happened
///     is that the camera's USB cable got yanked (status will be CVRES_VIDCAP_CAPTURE_DEVICE_DISCONNECTED in this case).
///     Other possibilities include low memory conditions and hardware failure.
/// <BR><BR>
/// <LI>Process images as desired within the callback.  By default, the CVImage
///     object that the callback receives will be released when the callback exits.
///     However, you can call CVImage::AddRef() on the image and keep it around for
///     later processing outside of the callback - just make sure to call 
///     CVImage::ReleaseImage() on it when you are done.
/// <BR><BR>
/// <LI>Stop capturing by calling CVVidCapture::Stop().
/// <BR><BR>
/// <LI>Disconnect the capture device by calling CVVidCapture::Disconnect().
/// <BR><BR>
/// <LI>Uninitialize the video capture subsystem by calling CVVidCapture::Uninit().
/// <BR><BR>
/// <LI>Free the CVVidCapture object.<BR>
///     If you allocated it with CVPlatform as recommended in step 1, then
///     call:<P>
///     <CODE>
///         CVPlatform::GetPlatform()->Release(vidCap);
///     </CODE><P>
///      <i>If you constructed a CVVidCaptureDSWin32 object directly, just delete it.</i>
/// </OL>
/// <P>
/// <h3>Some things to watch out for...</h3>
/// <UL>
/// <LI>VidCapture uses COM (Common Object Model) to talk to DirectX.  
///     The CVVidCapture::Init() function initializes COM
///     with CoInitializeEx(0, COINIT_MULTITHREADED). If you are using 
///     apartment mode, you may want to change this. See the 
///     <a href="http://msdn.microsoft.com/library/default.asp?url=/library/en-us/com/htm/cmf_a2c_5iyg.asp">
///     MSDN documentation for CoInitializeEx()</a> for more information.
///
/// <LI>VidCapture is currently set up to only have one thread using a specific
///     CVVidCapture object at a time.  If you want to call into an object from
///     multiple threads, I'd suggest serializing the calls.  Calls into a single
///     CVImage object should be serialized as well.  In practice, I wouldn't expect
///     either to be a problem, which is why the classes themselves aren't serialized.
///     <BR>
///
/// <LI>Check the result codes!  There are lots of them, and they'll probably help quite
///     a bit if you encounter an error.  The CVRES result codes are defined in
///     CVRes.h, and result codes for the various subsystems are enumerated in
///     CVResFile.h, CVResImage.h, and CVResVidCap.h.<BR>
///
/// <LI>Try not to put a lot of heavy processing or other lengthy code in the callback.
///     If you need to take a lot of time for each image, create a thread-safe queue,
///     call CVImage::AddRef() on the images in the callback and add them to the
///     queue.  Then pull them from the queue as you are ready for them from another
///     thread. Remember to call CVImage::ReleaseImage() on the images when you're
///     done with them if you use this method so you don't create a huge memory leak.
///
///     Also, you'll probably want to place a size limit on your queue if your processing
///     is slower than the capture speed and drop frames once you've hit your limit to
///     avoid filling up all available memory with images.
///
///     Alternatively, you can just use CVImage::Save() to save the images to disk
///     and load them back in for processing later at your leisure.<BR>
///
/// <LI>Do NOT call CVVidCapture::Stop() from within a capture callback to stop the
///     image capture - it will cause a deadlock. Instead, just return 'false' from
///     the callback and the capture will be stopped.  You still need to call
///     CVVidCapture::Stop() from another thread (not the capture callback) to free
///     up the resources.
/// 
/// </UL>
/// <P>
/// <hr>
/// <a name="dll"></a>
/// <h2>Using the VidCapture DLL (VidCapDll.dll)</h2>
/// The VidCapture DLL has a slightly different interface, as it is
/// written to be used from C and other non-object oriented languages.
/// <P>
/// You may want to check out the VidCapDllTest.c sample program for
/// a simple example of how to use the VidCapture DLL - most
/// of the functionality is covered in it.
/// <P>
/// If you're using the DLL from C or C++, you'll want to link to the
/// VidCapDll.lib (or VidCapDll_db.lib for debugging) library in your project.
/// This will cause the DLL to implicitly load when you run your program.
/// <P>
/// You may also load the library manually using LoadLibrary() and retrieve
/// the address of each function from the DLL by calling GetProcAddress()
/// if you wish.  If you do this - none of the functions names are decorated, and
/// they do NOT have a preceding underscore (_).  However, while this method is
/// preferable in some cases it is otherwise beyond the scope of this documentation.
/// <P>
/// <h3>Steps required to capture video with the DLL</h3>
/// <P> 
/// <OL>
/// <LI>Load the DLL.  If you linked against VidCapDLL.lib, this is automatic.<BR><BR>
/// <LI>Acquire a video capture system handle (CVVIDCAPSYSTEM) by calling CVAcquireVidCap().<BR><BR>
/// <LI>Get the number of devices available with CVGetNumDevices(), then find the one you want
///     by calling CVGetDeviceName(). Remember to set the nameBufLen to the full size of the name
///     buffer for each call to CVGetDeviceName(), as when it returns it will be set to the 
///     actual length of the last name returned.<BR><BR>
/// <LI>Connect to the desired device by calling CVDevConnect() with the device name.<BR><BR>
/// <LI>Retrieve parameters for any properties you may wish to modify with CVDevGetProperty(),
///     and set them with CVDevSetProperty(). Note that this may also be done during
///     video capture.<BR><BR>
/// <LI>Get the number of available modes with CVDevGetNumModes(), then retrieve information about
///     each mode by calling CVDevGetModeInfo().<BR><BR>
/// <LI>Begin a grab by calling CVDevStartCap() with the desired mode index.  
///     Keep the capture handle it returns! This will begin
///     a capture on a seperate thread - your callback will be called each time an image becomes available.<BR><BR>
/// <LI>Process the images in your callback as it is called for each frame (see CVIMAGESTRUCT).  The CVIMAGESTRUCT
///     structures and their associated image data are <em>ONLY</em> valid during the callback.  If you need to
///     perform any processing that is too slow to place in the callback, you MUST copy the data out.  Unlike the
///     C++ VidCapLib, the DLL does not currently offer reference counting on images. <BR><BR>
/// <LI>During the grab, you may prematurely terminate the capture from the callback by
///     returning FALSE.  However, you'll still need to call CVDevStopCap() to clean up.<BR><BR>
/// <LI>Call CVDevStopCap() when you're finished grabbing images. This halts the capture if it is still
///     going, then cleans up any memory or objects used by the capture.<BR><BR>
/// <LI>Disconnect from the connected device by calling CVDevDisconnect().<BR><BR>
/// <LI>Free the video capture system handle with CVReleaseVidCap().<BR><BR>
/// </OL>
/// <h3>Some things to watch out for...</h3>
/// <UL>
/// <LI>VidCapture is currently set up to only have one thread using a specific
///     CVVIDCAPSYSTEM handle at a time.  If you want to call into an object from
///     multiple threads, I'd suggest serializing the calls.
///     <BR>
///
/// <LI>Check the result codes!  There are lots of them, and they'll probably help quite
///     a bit if you encounter an error.  The CVRES result codes are defined in
///     CVRes.h, and result codes for the various subsystems are enumerated in
///     CVResFile.h, CVResImage.h, CVResVidCap.h, and CVResDll.h.<BR>
///
/// <LI>Try not to put a lot of heavy processing or other lengthy code in the callback.
///     If you need to take a lot of time for each image, copy the data out of the
///     CVIMAGESTRUCT (including making a copy of the pixel data!) and queue it
///     for processing.
///
///     Alternatively, you can just use CVSaveImage()to save the images to disk
///     and load them back in for processing later at your leisure.<BR>
///
/// <LI>Do NOT call CVDevStopCap() from within a capture callback to stop the
///     image capture - it will cause a deadlock. Instead, just return 'FALSE' from
///     the callback and the capture will be stopped.  You still need to call
///     CVDevStopCap() from another thread (not the capture callback) to free
///     up the resources.
/// 
/// </UL>
/// <a href="#top">^Top</a>
/// <hr>
/// <a name="history"></a>
/// <h2>VidCapture History</h2>
/// <h3>Version 0.30 (3/01/04)</h3>
/// <UL>
/// <LI> Added support for additional input video formats (YUV, I420, etc.)
/// <LI> Changed enumeration/allocation of devices to allow use of multiple identical devices.
/// <LI> Added framerate estimation to mode information
/// <LI> Fixed disconnection bug that could cause a failure on reconnect.
/// <LI> Added a GUI test project.
/// </UL>
/// <h3>Version 0.21 (2/08/04)</h3>
/// No code changes, but now hosted on SourceForge! <A href="http://sourceforge.net/projects/vidcapture/">http://sourceforge.net/projects/vidcapture/</a>
/// <h3>Version 0.21 (1/30/04)</h3>
/// <UL>
/// <LI> Fixed crash that could occur if no devices were attached.<BR>
///      Thanks to John Janecek for finding it!
/// <LI> Added this history to documentation.
/// </UL>
/// <h3>Version 0.20 (1/26/04)</h3>
/// <UL>
/// <LI> Added VidCapDll and VidCapDllTest projects for C-style DLL
/// <LI> Changed the VidCapture project to VidCapLib and built as static libary
/// <LI> Implemented previously defined but unused calls: 
///      CVVidCapture::IsConnected(), CVVidCapture::IsStarted, and CVVidCapture::IsInitialized())
/// <LI> Added VidCapDll documentation
/// </UL>
/// <h3>Version 0.10</h3>
/// <UL>
/// <LI>Initial Release of C++ code
/// </UL>
/// <a href="#top">^Top</a>
/// <hr>
/// <a name="future"></a>
/// <h2>Future Directions...</h2>
/// I can't (and won't) make any promises about any future upgrades, support,
/// or anything else regarding VidCapture. However, there are some directions
/// I'm currently hoping to take it.
/// <P>
/// <i>WARNING: In future versions the interface may change dramatically.</i>
/// <P>
/// As of version 0.2, VidCapture also includes a .DLL version with a C-style
/// interface. So at least one of the wishlist features is done :)
/// <P>
/// I'm still hoping to make it available as an ActiveX control and possibly
/// as a .NET control as well - it all depends on my 'free' time.
/// <P>
/// At a later date, I hope to make a more complete CVImage library with  
/// image processing support similar to the ones I wrote for 
/// the <a href="http://www.codevis.com/proj_scanner.html">3D Scanner</a>
/// project.  In fact, a large part of the reason I wrote VidCapture was to replace
/// the old code used there.  I also plan to make the library multiplatform,
/// supporting at least Mac OSX and Linux.  For now, if you want to do image processing
/// using the images (beyond just capturing them) you might want to take a look 
/// at the CVImage::GetMaxPixel() function to see how it templates and handles the
/// image processing generically while supporting the offsets and widths of 
/// sub images.
/// <P>
/// If you have any comments, suggestions, or want to lend a hand in the development,
/// please email me at mike@codevis.com.
/// <P>
/// <a href="#top">^Top</a>
/// <hr>
/// <a name="credits"></a>
/// <h2>Credits</h2>
/// Many thanks to Blair MacIntyre of Georgia Tech for providing equipment and 
/// helpful suggestions!
/// <P>
/// I want to thank Dimitri van Heesch for writing
/// an excellent documentation tool called <a href="http://www.doxygen.org">
/// doxygen</a>.  It is what has made the documentation you are reading
/// possible, and I couldn't recommend it more highly for documenting new
/// projects.
/// <P>
/// For installation of VidCapture, I'm using <a href="http://www.nullsoft.com">
/// NullSoft's</a> <a href="http://nsis.sourceforge.net">Scriptable Install System</a> (NSIS).
/// <P>
/// The reference material used for developing VidCapture came mostly from
/// <a href="http://msdn.microsoft.com">MSDN</a> and Microsoft's <a href="http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/htm/directshow.asp">
/// DirectShow Documentation</a>.
/// <P>
/// <a href="http://www.amazon.com/exec/obidos/tg/detail/-/0735618216/002-6804222-2560820?v=glance">
/// Programming DirectShow for Digital Video and Television</a> by Mark D. Pesce was also very helpful
/// on a lot of the basic concepts for interfacing with DirectShow.
/// <P>
/// CodeVis VidCapture was written by <a href="http://www.codevis.com/mellison.html">Michael Ellison</a>,
/// mike@codevis.com.
/// <P>
/// <a href="#top">^Top</a>
/// <hr>
/// <a name="license"></a>
/// <h2>License Agreement</h2>
///  <CENTER>
///  CodeVis's Free License<BR>
///  <a href="http://www.codevis.com">www.codevis.com</a><BR>
///  <BR>
///  <i>Copyright (c) 2003-2004 by <a href="http://www.codevis.com/mellison.html">
///     Michael Ellison</a> (mike@codevis.com).<BR>
///  All Rights Reserved.</i><BR>
///  <BR>
///  <TABLE border=0><TR><TD>
///  You may use this software in source and/or binary form, with or without
///  modification, for commercial or non-commercial purposes, provided that 
///  you comply with the following conditions:
/// 
///  <UL>
///  <LI>Redistributions of source code must retain the above copyright notice,
///      this list of conditions and the following disclaimer. 
/// 
///  <LI>Redistributions of modified source must be clearly marked as modified,
///  and due notice must be placed in the modified source indicating the
///  type of modification(s) and the name(s) of the person(s) performing
///  said modification(s).
///  </UL>
///
///  <b>This software is provided by the copyright holders and contributors 
///  "as is" and any express or implied warranties, including, but not 
///  limited to, the implied warranties of merchantability and fitness for
///  a particular purpose are disclaimed. In no event shall the copyright 
///  owner or contributors be liable for any direct, indirect, incidental, 
///  special, exemplary, or consequential damages (including, but not limited 
///  to, procurement of substitute goods or services; loss of use, data, or 
///  profits; or business interruption) however caused and on any theory of 
///  liability, whether in contract, strict liability, or tort (including 
///  negligence or otherwise) arising in any way out of the use of this 
///  software, even if advised of the possibility of such damage.</b>
///
/// </TD></TR></TABLE></CENTER>
/// <a href="#top">^Top</a>

