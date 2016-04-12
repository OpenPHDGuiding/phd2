/////////////////////////////////////////////////////////////////////////////
// Name:        vcapwin.h - wxVideoCaptureWindowBase
// Author:      John Labenski
// Created:     7/06/2001
// Modified:    01/14/03
// Copyright:   John Labenski
// License:     wxWidgets V2.0
/////////////////////////////////////////////////////////////////////////////
//
// Usage notes:
// Link against vfw.lib
// Read the header and the cpp file to figure out what something does
// Test "problems" against MSW vidcap.exe to see if the same problem occurs

#ifndef __WX_VCAPWIN_H__
#define __WX_VCAPWIN_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "vcapwin.h"
#endif

#include "wx/defs.h"
#include "wx/event.h"
#include "wx/scrolwin.h"
#include "wx/image.h"

// These are our DLL macros (see the contrib libs like wxPlot)
#ifdef WXMAKINGDLL_VIDCAP
    #define WXDLLIMPEXP_VIDCAP WXEXPORT
    #define WXDLLIMPEXP_DATA_VIDCAP(type) WXEXPORT type
#elif defined(WXUSINGDLL)
    #define WXDLLIMPEXP_VIDCAP WXIMPORT
    #define WXDLLIMPEXP_DATA_VIDCAP(type) WXIMPORT type
#else // not making nor using DLL
    #define WXDLLIMPEXP_VIDCAP
    #define WXDLLIMPEXP_DATA_VIDCAP(type) type
#endif

#if defined(__WXMSW__)
    #define WXVIDCAP_MSW_VFW
    #define WXVIDCAP_AVI_SUPPORT
    #define WXVIDCAP_AUDIO_SUPPORT
#elif defined(__linux__)
    #define WXVIDCAP_LINUX_V4L
#endif

#if defined(WXVIDCAP_LINUX_V4L)
    typedef wxUint32 FOURCC;
    #define BI_RGB 0
#endif

// classes used by the wxVideoCaptureWindow system
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowBase;
class WXDLLIMPEXP_VIDCAP wxVideoCaptureEvent;
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowCaptureSingleFramesDialog;
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowAudioFormatDialog;
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowCapturePreferencesDialog;
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowCustomVideoFormatDialog;

//----------------------------------------------------------------------------
// wxVideoCaptureWindow #defines and globals
//----------------------------------------------------------------------------
// use a timer to view the frames as wxImages, it kinda blocks the GUI
// if set to 0 then use OnIdle to start the timer as a OneShot to get rate
// if set to 1 then just set the timer to the preview rate, not so good
#define USE_PREVIEW_wxIMAGE_TIMER 0

// array of some predefined "standard" video widths, max 1024
// wxVideoCaptureWindow::SetVideoFormat( width...
#define wxVIDCAP_VIDEO_WIDTHS_COUNT 19
extern const unsigned int wxVIDCAP_VIDEO_WIDTHS[wxVIDCAP_VIDEO_WIDTHS_COUNT];
    //={80,128,160,176,180,192,240,320,352,360,384,400,480,640,704,720,768,800,1024};

// array of some predefined "standard" video heights, max 768
// wxVideoCaptureWindow::SetVideoFormat( width, height...
#define wxVIDCAP_VIDEO_HEIGHTS_COUNT 13
extern const unsigned int wxVIDCAP_VIDEO_HEIGHTS[wxVIDCAP_VIDEO_HEIGHTS_COUNT];
    //={60, 96, 120, 144, 180, 240, 288, 300, 360, 480, 576, 600, 768 };

// Make a FOURCC (DWORD-32bit uint) from a string, STRING_TO_FOURCC("UYVV")
//    MUST have four characters, even for "  Y8" for example
//    this is NOT the preferred way, but used in struct above, see below
#define STRING_TO_FOURCC(f) (((f)[0])|((f)[1]<<8)|((f)[2]<<16)|((f)[3]<<24))
// Make a FOURCC from 4 chars, Chars_To_FOURCC('U','Y','V','V')
// #define CharsToFOURCC(a,b,c,d) ((a)|(b)<<8|(c)<<16|(d)<<24)

// Return a FOURCC given a wxString
extern FOURCC wxStringToFOURCC(const wxString &string);
// Return a FOURCC given a regular string of chars
extern FOURCC StringToFOURCC( const char *s );

// Return a wxString given a FOURCC
extern wxString FOURCCTowxString(FOURCC fourcc);
// Return a string of chars given a FOURCC
extern const char* FOURCCToString(FOURCC fourcc);

#define wxNullFOURCC FOURCC(-1)

class WXDLLIMPEXP_VIDCAP wxVideoCaptureFormat
{
public :
    wxVideoCaptureFormat() { m_fourcc = wxNullFOURCC; m_bpp = 0; m_v4l1_palette = -1; }

    wxVideoCaptureFormat(const wxString &description, FOURCC fourcc, int bpp, int v4l1_palette)
    {
        m_description = description;
        m_fourcc = fourcc;
        m_bpp = bpp;
        m_v4l1_palette = v4l1_palette;
    }

    wxString m_description;// common name of the format
    FOURCC m_fourcc;       // MMIO data type for BITMAPINFOHEADER->biCompression
    int m_bpp;             // bits per pixel for BITMAPINFOHEADER->biBitCount
    int m_v4l1_palette;    // the type the v4l1 thinks it is (v4l2 uses fourcc ?)
};

#include "wx/dynarray.h"
WX_DECLARE_OBJARRAY(wxVideoCaptureFormat, wxArrayVideoCaptureFormat);


// structure to hold the rest of the BITMAPINFOHEADER format parameters
typedef struct wxvidcap_video_format_struct
{
    FOURCC fourcc;    // MMIO data type for BITMAPINFOHEADER->biCompression
    int bpp;          // bits per pixel for BITMAPINFOHEADER->biBitCount
    const char *description; // common name of the format
} wxvidcap_video_format_typedef;
/*
// array of some predefined "standard" video formats
#define wxVIDCAP_VIDEO_FORMATS_COUNT 25
extern const wxvidcap_video_format_typedef wxVIDCAP_VIDEO_FORMATS[wxVIDCAP_VIDEO_FORMATS_COUNT];

// constants to specify video formats from wxVIDCAP_VIDEO_FORMATS[one below]
// used in wxVideoCaptureWindow::[Get/Set]VideoFormat( width, height, format..
enum wxVIDCAP_VIDEO_FORMAT_Type
{
    wxVIDCAP_VIDEO_FORMAT_UNKNOWN = -1, // some weird format, there are MANY
    wxVIDCAP_VIDEO_FORMAT_BI_RGB_8 = 0, // 8-bit RGB
    wxVIDCAP_VIDEO_FORMAT_BI_RGB_16,    // 16-bit RBG
    wxVIDCAP_VIDEO_FORMAT_BI_RGB_24,    // 24-bit RGB
    wxVIDCAP_VIDEO_FORMAT_BI_RGB_32,    // 32-bit RGB
    wxVIDCAP_VIDEO_FORMAT_RGB555,       // 555 16 bit RGB
    wxVIDCAP_VIDEO_FORMAT_RGB565,       // 565 16 bit RGB
    wxVIDCAP_VIDEO_FORMAT_Y8,           // Y8 Single Y plane, greyscale
    wxVIDCAP_VIDEO_FORMAT_GREY,         // v4l GREY fake fourcc I think
    wxVIDCAP_VIDEO_FORMAT_CYUV,         // CYUV Inverted YUV 4:2:2
    wxVIDCAP_VIDEO_FORMAT_UYVY,         // UYVY YUV 4:2:2 interleaved
    wxVIDCAP_VIDEO_FORMAT_YUYV,         // YUYV YUV 4:2:2 interleaved
    wxVIDCAP_VIDEO_FORMAT_YUY2,         // YUY2 YUV 4:2:2 interleaved
    wxVIDCAP_VIDEO_FORMAT_YV12,         // YV12 YUV 4:2:0 planar
    wxVIDCAP_VIDEO_FORMAT_I420,         // I420 YUV 4:2:0 planar
    wxVIDCAP_VIDEO_FORMAT_IYUV,         // IYUV YUV 4:2:0 planar
    wxVIDCAP_VIDEO_FORMAT_Y41P,         // Y41P YUV 4:1:1 planar
    wxVIDCAP_VIDEO_FORMAT_YVU9,         // YVU9 YUV planar (Indeo Raw) = YUV9
    wxVIDCAP_VIDEO_FORMAT_YUV9,         // YUV9 YUV planar (Indeo Raw) = YVU9
    wxVIDCAP_VIDEO_FORMAT_MJPG,         // MJPG Motion JPEG
    wxVIDCAP_VIDEO_FORMAT_dmb1,         // dmb1 Matrox MJPEG
    wxVIDCAP_VIDEO_FORMAT_JBYR,         // JBYR 20 bit se401 Kensington/Phillips
    wxVIDCAP_VIDEO_FORMAT_ABYR,         // ABYR 8 bit se401 Kensington/Phillips
    wxVIDCAP_VIDEO_FORMAT_VCR1,         // VCR1 ATI VCR 1.0
    wxVIDCAP_VIDEO_FORMAT_VCR2,         // VCR2 ATI VCR 2.0
    wxVIDCAP_VIDEO_FORMAT_VYUY,         // VYUY ATI Packed YUV Data
    wxVIDCAP_VIDEO_FORMAT_Y411          // Supposedly packed YUV 4:1:1 ???
};
*/
// wxVideoCaptureWindow::[Get/Set]AudioFormat( channels, ...
enum wxVIDCAP_AUDIO_CHANNELS_Type
{
    wxVIDCAP_AUDIO_MONO   = 1,
    wxVIDCAP_AUDIO_STEREO = 2
};

// wxVideoCaptureWindow::[Get/Set]AudioFormat( channels, bitspersample, ...
enum wxVIDCAP_AUDIO_BITS_Type
{
    wxVIDCAP_AUDIO_8BITS  = 8,
    wxVIDCAP_AUDIO_16BITS = 16
};

// wxVideoCaptureWindow::[Get/Set]AudioFormat( channels, bitspersample, samplespersecond
enum wxVIDCAP_AUDIO_SAMPLES_Type
{
    wxVIDCAP_AUDIO_8000HZ  = 8000,
    wxVIDCAP_AUDIO_11025HZ = 11025,
    wxVIDCAP_AUDIO_16000HZ = 16000,
    wxVIDCAP_AUDIO_22050HZ = 22050,
    wxVIDCAP_AUDIO_24000HZ = 24000,
    wxVIDCAP_AUDIO_32000HZ = 32000,
    wxVIDCAP_AUDIO_44100HZ = 44100,
    wxVIDCAP_AUDIO_48000HZ = 48000
};

// array of the above "standard" audio rates, in same order
#define wxVIDCAP_AUDIO_SAMPLESPERSEC_COUNT 8
extern const long int wxVIDCAP_AUDIO_SAMPLESPERSEC[wxVIDCAP_AUDIO_SAMPLESPERSEC_COUNT];
    // = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000 };

//----------------------------------------------------------------------------
// wxVideoCaptureEvent : events for the wxVideoCaptureWindow
//----------------------------------------------------------------------------
// This is derived from a wxEvent, so you must subclass your wxVideoCaptureWindow
//    to get any of these events, this should be faster.
//    If you want the parent to get events change to wxCommandEvent

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_VIDEO,        wxEVT_USER_FIRST + 3001)
    DECLARE_EVENT_TYPE(wxEVT_VIDEO_STATUS, wxEVT_USER_FIRST + 3002)
    DECLARE_EVENT_TYPE(wxEVT_VIDEO_FRAME,  wxEVT_USER_FIRST + 3003)
    DECLARE_EVENT_TYPE(wxEVT_VIDEO_STREAM, wxEVT_USER_FIRST + 3004)
    DECLARE_EVENT_TYPE(wxEVT_VIDEO_ERROR,  wxEVT_USER_FIRST + 3005)
END_DECLARE_EVENT_TYPES()


class WXDLLIMPEXP_VIDCAP wxVideoCaptureEvent : public wxEvent
{
public:
    wxVideoCaptureEvent( wxEventType commandType = wxEVT_NULL,
                         wxVideoCaptureWindowBase *vidCapWin = NULL,
                         int Id = -1 );

    virtual ~wxVideoCaptureEvent() {}

    virtual wxEvent *Clone() const { return new wxVideoCaptureEvent(*this); }

    // wxEVT_VIDEO_STATUS uses this
    // for capSetCallbackOnStatus, anytime the status changes
    wxString GetStatusText() { return m_statustext; }
    void SetStatusText( const wxString &statustext ) { m_statustext = statustext; }

    // wxEVT_VIDEO_ERROR uses this
    // for capSetCallbackOnError, anytime nonfatal errors are generated
    wxString GetErrorText() { return m_errortext; }
    void SetErrorText( const wxString &errortext ) { m_errortext = errortext; }

    // wxEVT_VIDEO_FRAME uses this
    // for capSetCallbackOnFrame, generated whenever previewing
    unsigned long int GetFrameNumber() { return m_framenumber; }
    void SetFrameNumber( unsigned long int num ) { m_framenumber = num; }

    unsigned int GetFramerateMS() { return m_frameratems; }
    void SetFrameRateMS( unsigned int num ) { m_frameratems = num; }

    wxString m_statustext;
    wxString m_errortext;
    unsigned long int m_framenumber;
    unsigned int m_frameratems;

private:
    DECLARE_DYNAMIC_CLASS(wxVideoCaptureEvent)
};

typedef void (wxEvtHandler::*wxVideoCaptureEventFunction)(wxVideoCaptureEvent&);

// whenever the status of the capture device changes
// for MSW capSetCallbackOnStatus(m_hWndC, VFW_VideoStatusCallbackProc);
#define EVT_VIDEO_STATUS(id, func) DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_STATUS, id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject *)NULL),
// whenever a preview frame is available from the device
// for MSW capSetCallbackOnFrame(m_hWndC, VFW_VideoFrameCallbackProc);
#define EVT_VIDEO_FRAME(id, func)  DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_FRAME, id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject *)NULL),
// whenver a frame is about to be written to an AVI file
// for MSW capSetCallbackOnStream(m_hWndC, VFW_VideoStreamCallbackProc);
#define EVT_VIDEO_STREAM(id, func) DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_STREAM, id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject *)NULL),
// whenver a nonfatal error occurs
// for MSW capSetCallbackOnError(m_hWndC, VFW_VideoErrorCallbackProc);
#define EVT_VIDEO_ERROR(id, func)  DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_ERROR, id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject *)NULL),
// for all events, whenever anything happens
#define EVT_VIDEO(id, func) \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_STATUS, id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject*)NULL), \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_FRAME,  id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject*)NULL), \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_STREAM, id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject*)NULL), \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_VIDEO_ERROR,  id, wxID_ANY, (wxObjectEventFunction)(wxEventFunction)(wxVideoCaptureEventFunction)&func, (wxObject*)NULL),

//----------------------------------------------------------------------------
// wxVideoCaptureWindowBase : window for viewing/recording streaming video or snapshots
//----------------------------------------------------------------------------
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowBase: public wxScrolledWindow
{
public:
    wxVideoCaptureWindowBase() : wxScrolledWindow() { Reset(true); }
    wxVideoCaptureWindowBase( wxWindow *parent, wxWindowID id = -1,
                              const wxPoint &pos = wxDefaultPosition,
                              const wxSize &size = wxDefaultSize,
                              long style = wxSIMPLE_BORDER,
                              const wxString &name = wxT("wxVideoCaptureWindow"));

    virtual ~wxVideoCaptureWindowBase();

    bool Create( wxWindow *parent, wxWindowID id = -1,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 long style = wxSIMPLE_BORDER,
                 const wxString &name = wxT("wxVideoCaptureWindow"));

    // ----------------------------------------------------------------------
    // Device descriptions & versions, get and enumerate
    // ----------------------------------------------------------------------

    // Enumerate all available device names & versions
    //   This is called at creation and shouldn't need to be called again
    // void EnumerateDevices() = 0;

    // Get a single description of a device, -1 is current device
    //   VFW - supports only [0..9]
    virtual wxString GetDeviceName(int index = -1) const;
    // Get a single device version, [0..9], -1 is current device
    //   VFW - supports only [0..9]
    virtual wxString GetDeviceVersion(int index = -1) const;
    // how many devices were detected?
    virtual int GetDeviceCount() const { return m_deviceNames.GetCount(); }

    // ----------------------------------------------------------------------
    // Connect or Disconnect to device
    // ----------------------------------------------------------------------

    // are we connected to a device now?
    virtual bool IsDeviceConnected() const { return m_deviceIndex > -1; }

    // true if successfully initialized/connected/ready to capture
    // DON'T TRUST w/ VFW always seems to return true, so that's why we have IsDeviceConnected()
    // FIXME : maybe there's some use to this? probably should just delete it
    virtual bool IsDeviceInitialized() { return IsDeviceConnected(); }

    // index of this cap device
    // returns -1 if not connected, so be sure to check IsDeviceConnected()
    //   VFW - the devices [0..9] are listed in system.ini
    virtual int GetDeviceIndex() { return m_deviceIndex; }

    // Connect to one of the available devices, returns sucess
    //    VFW supports only [0..9]
    virtual bool DeviceConnect(int index) = 0;
    // disconnect from the device, returns sucess
    virtual bool DeviceDisconnect() = 0;

    // ----------------------------------------------------------------------
    // Display dialogs to set/get video characteristics
    // ----------------------------------------------------------------------

    // Dialog to adjust video contrast/intensity... and change source
    //   VFW - driver supplies the dialog, also select source
    //   V4L - custom dialog to change the source FIXME
    virtual bool HasVideoSourceDialog() { return false; }
    // Show the dialog
    virtual void VideoSourceDialog() {}

    // Dialog to display the format (size)
    //  VFW - driver supplies the dialog
    //  V4L - use the VideoCustomFormatDialog
    virtual bool HasVideoFormatDialog() { return false; }
    // Show the dialog
    virtual void VideoFormatDialog() {}
    // Dialog for setting the frame size, bpp, and compression "by hand"
    //   VFW - WARNING: this can crash your system if you choose an unsupported format
    //                  I'm pretty sure this is correct, but the driver itself fails
    //   V4L - no problems :)
    virtual void VideoCustomFormatDialog() = 0;

    // Dialog for redisplay of video from cap frame buffer? Huh? saturation, brightness...
    //   VFW - driver supplies dialog, for overlay only?
    //   V4L - not sure what to make of this FIXME
    virtual bool HasVideoDisplayDialog() { return false; }
    // Show the dialog
    virtual void VideoDisplayDialog() {}

    // Dialog to display all know device characteristics
    //   each system displays different things, basicly just a dump of all available settings
    virtual void PropertiesDialog() = 0;

    // A formatted string of all know properties of the video system
    //   this is the string that's printed in the PropertiesDialog
    virtual wxString GetPropertiesString() = 0;

    // ----------------------------------------------------------------------
    // Video characteristics and manipulation
    // ----------------------------------------------------------------------

    // the width of the capture image
    virtual int GetImageWidth() { return m_imageSize.x; }
    // the height of the capture image
    virtual int GetImageHeight() { return m_imageSize.y; }

    // Get the Max/Min allowed video sizes
    //   VFW max size is 1024X768, min ???, no way to find these parameters
    //   V4L supports a method to get the Max/Min size
    virtual wxSize GetMinImageSize() { return m_minImageSize; }
    virtual wxSize GetMaxImageSize() { return m_maxImageSize; }

    // get the video format characteristics, returns sucess
    //   format is a 4 character string, "UYVY" or whatever comes out
    virtual bool GetVideoFormat( int *width, int *height, int *bpp, FOURCC *fourcc ) = 0;

    //***********************************************************************
    // WARNING! - Video For Windows - (Video 4 Linux - works fine :)
    // SetVideoFormat is not for the faint of heart or people w/o saved data!
    // at worst it'll crash your system, some drivers don't gracefully fail
    // Known culprits - Kensington
    // No problem - Pinnacle (bt878)
    //***********************************************************************
    // attempt to set the video format the device puts out, returns sucess
    //  -1 for width/height/bpp and format=-1 uses current value
    //  FOURCC is the 4 chararacter code "UYUV" or whatever
    virtual bool SetVideoFormat( int width, int height, int bpp, FOURCC fourcc ) = 0;

    // default driver palette being used, then true
    //   VFW - supported, but not sure what you would do with this
    //   V4L - unsupported
    virtual bool IsUsingDefaultPalette() { return true; }

    // true if driver can create palettes
    //   VFW - supported, not sure what you'd do with itself
    //   V4L - unsupported
    virtual bool DriverSuppliesPalettes() { return false; }

    // ----------------------------------------------------------------------
    // Capture Preview and Overlay
    // ----------------------------------------------------------------------

    // turn software previewing on/off, returns sucess, on = false turns both off
    //   VFW - wxpreview = false uses the VFW preview capabilities
    //         wxpreview = true, uses the callback to decompress frames to m_wximage
    //   V4L - wxpreview is ignored as it always uses wxImages
    virtual bool Preview(bool on, bool wxpreview = false) = 0;

    // true if displaying using software preview method
    virtual bool IsPreviewing() { return m_previewing || m_preview_wximage; }

    // true if previewing using wxImages & OnPaint
    virtual bool IsPreviewingwxImage() const { return m_preview_wximage; }

    // scale preview window to window's size (no scrollbars), returns sucess
    // if overlaying then just center the window
    virtual bool PreviewScaled(bool fit_window) { m_previewscaled = fit_window; return true; }

    // video is scaled to the capture window size Preview, or centered Overlay
    virtual bool IsPreviewScaled() { return m_previewscaled; }

    // set the number of milliseconds per frames to *try* to capture at, returns sucess
    // this has NOTHING to do with how many you'll actually get
    // if > than hardware capability, default = 66ms or 15fps
    // bool SetPreviewRateMS( unsigned int msperframe = 66 );
    bool DoSetPreviewRateMS( unsigned int msperframe = 66 ) { m_previewmsperframe = msperframe; return true; }

    // currently set PreviewRate, not necessarily correct if set too fast
    virtual unsigned int GetPreviewRateMS() const { return m_previewmsperframe; }
    // actual measured PreviewRate when previewing
    virtual unsigned int GetActualPreviewRateMS() const { return m_actualpreviewmsperframe; }

    // function stub YOU override to do image processing of the preview frames
    // when Previewing with wxImages, OnIdle/PreviewwxImageTimer grabs a frame
    // which calls the MSW callback which calls CallbackOnFrame which fills the
    // m_wximage and then calls this and if it returns true it calls Refresh()
    // to show the frame
    virtual bool ProcesswxImageFrame() { return true; }

    // device supports hardware video overlay
    virtual bool HasOverlay() { return m_has_overlay; }
    // use video card hardware overlay, ie. pci framegrabbers, returns sucess
    // automatically turns off preview if necessary
    virtual bool Overlay(bool on) = 0;
    // true if displaying using hardware video overlay method
    virtual bool IsOverlaying() { return m_overlaying; }

    // ----------------------------------------------------------------------
    // Capture single frames, take snapshots of streaming video
    // ----------------------------------------------------------------------

    // single stop action snapshot to window, stops previewing, returns sucess
    virtual bool SnapshotToWindow() = 0;
    // capture a single frame to the clipboard, returns sucess
    virtual bool SnapshotToClipboard() = 0;
    // single snapshot of video, save it as a DIB (.BMP) file, returns sucess
    virtual bool SnapshotToBMP( const wxString &filename ) = 0;
    // take a single snapshot and fill this image, doesn't need to be Created
    virtual bool SnapshotTowxImage( wxImage &image) = 0;
    // take a single snapshot and fill m_wximage see GetwxImage()
    virtual bool SnapshotTowxImage() = 0;
    // get the m_wximage, can use in conjunction with SnapshotTowxImage()
    //  or previewing w/ wxImages methods
    virtual wxImage GetwxImage();

    // ----------------------------------------------------------------------
    // Capture (append) single video frames to an AVI file
    // ----------------------------------------------------------------------

    // NOTE : None of this is implemeted for V4L

    // ----------------------------------------------------------------------
    // Capture streaming video to an AVI file
    // ----------------------------------------------------------------------

    // NOTE : None of this is implemeted for V4L

    // ----------------------------------------------------------------------
    // Capture file settings, filename to capture video to
    // ----------------------------------------------------------------------

    // NOTE : None of this is implemeted for V4L

    // ----------------------------------------------------------------------
    // Audio Setup
    // ----------------------------------------------------------------------

    // NOTE : None of this is implemeted for V4L


    long int GetFileSizeInKB( const wxString &filename );

    //-------------------------------------------------------------------------
    // wxVideoCaptureFormat manipulation - predefined list of FOURCCs and descriptions
    //   The lists are different for VFW and V4L
    //   VFW - any format that the device wants to put out is possible
    //         so a generic list of come common types is provided
    //   V4L - seems like only a fixed number of formats are supported and that
    //         the FOURCC codes are "made up" so only those are provided
    //-------------------------------------------------------------------------
    int GetVideoCaptureFormatCount();
    wxArrayVideoCaptureFormat &GetVideoCaptureFormatArray();
    wxVideoCaptureFormat GetVideoCaptureFormat(int index);
    int FindVideoCaptureFormatFOURCC(FOURCC fourcc);
    int FindVideoCaptureFormatV4Lpalette(int v4lpalette);
    void RegisterVideoCaptureFormat(wxVideoCaptureFormat *new_VideoFormat);

protected :

    // resets the member vars to a disconnected state
    //   if !full then user settings are left alone
    //   call when disconnecting
    virtual void Reset(bool full = false);


    void CreateVideoCaptureFormatArray(); // don't need to call, used internally

    // -----------------------------------------------------------------------
    // member vars

    wxSize m_clientSize;            // from wxWindow::GetClientSize

    wxArrayString m_deviceNames;    // device names from EnumerateDevices
    wxArrayString m_deviceVersions; // device versions from EnumerateDevices

    int m_deviceIndex;              // current index of the device in use or -1

    bool m_previewing;              // currently previewing
    bool m_preview_wximage;         // m_previewing is true, but displaying using OnDraw w/ m_wximage
    bool m_previewscaled;           // scale the preview window fullsize
    unsigned int m_previewmsperframe;       // # milliseconds between preview frames
    unsigned int m_actualpreviewmsperframe; // measured ms between preview frames

    bool m_has_overlay;             // can use hardware overlay for display
    bool m_overlaying;              // currently overlaying

    unsigned long int m_framenumber;  // # of frames, since preview start

    wxImage m_wximage;              // wximage to hold the streaming video

    wxSize m_imageSize;             // size of the video
    wxSize m_maxImageSize;          // min available capture size
    wxSize m_minImageSize;          // max available capture size

private:
    DECLARE_ABSTRACT_CLASS(wxVideoCaptureWindowBase);
};

#if defined(WXVIDCAP_MSW_VFW)
    #include "wx/vidcap/vcap_vfw.h"
    typedef wxVideoCaptureWindowVFW wxVideoCaptureWindow;
#elif defined(WXVIDCAP_LINUX_V4L)
    #include "wx/vidcap/vcap_v4l.h"
    typedef wxVideoCaptureWindowV4L wxVideoCaptureWindow;
#endif

#endif //__WX_VCAPWIN_H__
