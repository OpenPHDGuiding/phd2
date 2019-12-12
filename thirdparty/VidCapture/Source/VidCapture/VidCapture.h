/// \file VidCapture.h
/// \brief Header includes for using the CodeVis VidCapture library.
///

#ifndef _CODEVIS_VIDCAPTURE_H_
#define _CODEVIS_VIDCAPTURE_H

// Result codes
#include "CVResFile.h"     // File result codes
#include "CVResImage.h"    // Imaging result codes
#include "CVResVidCap.h"   // Video capture result codes

// Imaging, Platform, and Video Capture
#include "CVImage.h"       // Imaging class interface
#include "CVPlatform.h"    // Platform-specific creation of classes
#include "CVVidCapture.h"  // Video capture interface

// This one only needs to be #included if you want to do 
// DirectShow specific stuff - e.g. CVVidCaptureDSWin32::StartRawCap()

//#ifdef WIN32
//   #include "CVVidCaptureDSWin32.h" // Win32 DirectShow implementation
//#endif

#endif _CODEVIS_VIDCAPTURE_H_
