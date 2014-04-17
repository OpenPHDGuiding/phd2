/////////////////////////////////////////////////////////////////////////////
// Name:        vcap_vfw.h - wxVideoCaptureWindow using MSW VFW 1.1 API
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

#ifndef __WX_VCAP_VFW_H__
#define __WX_VCAP_VFW_H__

#include <windows.h>    // MSW headers
#include <windowsx.h>   // for GlobalAllocPtr to set audio parameters
#include <vfw.h>        // Video For Windows 1.1
#include "wx/timer.h"

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "vcap_vfw.h"
#endif

//----------------------------------------------------------------------------
// wxVideoCaptureWindow : window for viewing/recording streaming video or snapshots
//----------------------------------------------------------------------------
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowVFW: public wxVideoCaptureWindowBase
{
public:
    wxVideoCaptureWindowVFW() : wxVideoCaptureWindowBase() {}
    wxVideoCaptureWindowVFW( wxWindow *parent, wxWindowID id = -1,
                             const wxPoint &pos = wxDefaultPosition,
                             const wxSize &size = wxDefaultSize,
                             long style = wxSIMPLE_BORDER,
                             const wxString &name = wxT("wxVideoCaptureWindow") );

    bool Create( wxWindow *parent, wxWindowID id = -1,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 long style = wxSIMPLE_BORDER,
                 const wxString &name = wxT("wxVideoCaptureWindow") );

    virtual ~wxVideoCaptureWindowVFW();

    // ----------------------------------------------------------------------
    // Device descriptions & versions, get and enumerate
    // ----------------------------------------------------------------------

    void EnumerateDevices();

    // ----------------------------------------------------------------------
    // Connect or Disconnect to device
    // ----------------------------------------------------------------------

    bool IsDeviceInitialized();
    bool DeviceConnect(int index);
    bool DeviceDisconnect();

    // ----------------------------------------------------------------------
    // Display dialogs to set/get video characteristics
    // ----------------------------------------------------------------------

    bool HasVideoSourceDialog();
    void VideoSourceDialog();

    bool HasVideoFormatDialog();
    void VideoFormatDialog();
    void VideoCustomFormatDialog();

    bool HasVideoDisplayDialog();
    void VideoDisplayDialog();

    // Dialog to show the available compression codecs used for capture
    //   VFW - MSW system dialog
    void VideoCompressionDialog();

    // Dialog to setup most of the capture preferences
    //   shows only whats available on each system
    void CapturePreferencesDialog();

    // Dialog to set the audio channels, bits/sample, samples/second
    //   VFW - works only if HasAudioHardware is true, ie you have a sound card
    virtual void AudioFormatDialog();

    void PropertiesDialog();
    wxString GetPropertiesString();

    // ----------------------------------------------------------------------
    // Video characteristics and manipulation
    // ----------------------------------------------------------------------

    bool GetVideoFormat( int *width, int *height, int *bpp, FOURCC *fourcc );

    //***********************************************************************
    // WARNING! - Video For Windows
    // SetVideoFormat is not for the faint of heart or people w/o saved data!
    // at worst it'll crash your system, some drivers don't gracefully fail.
    // There doesn't seem to be a way to get supported values for a device.
    // This function lets you set them to anything, use the VideoFormatDialog
    // which the driver supplies to be on the safe side.
    bool SetVideoFormat( int width, int height, int bpp, FOURCC fourcc );

    bool IsUsingDefaultPalette();
    bool DriverSuppliesPalettes();

    // ----------------------------------------------------------------------
    // Capture Preview and Overlay
    // ----------------------------------------------------------------------

    void OnPreviewwxImageTimer(wxTimerEvent& event); // get frames

    bool Preview(bool on, bool wxpreview = false);
    bool PreviewScaled(bool on);
    bool SetPreviewRateMS( unsigned int msperframe = 66 );
    bool Overlay(bool on);

    // ----------------------------------------------------------------------
    // Capture single frames, take snapshots of streaming video
    // ----------------------------------------------------------------------

    bool SnapshotToWindow();
    bool SnapshotToClipboard();
    bool SnapshotToBMP( const wxString &filename );
    bool SnapshotTowxImage( wxImage &image);
    bool SnapshotTowxImage();

    // ----------------------------------------------------------------------
    // Capture (append) single video frames to an AVI file
    // ----------------------------------------------------------------------

    // Overview: These frames will be saved to the SetCaptureFilename() file
    // USE: CaptureSingleFramesToFileOpen then CaptureSingleFrames...
    //   (see SetAviMaxIndexEntries() ) then CaptureSingleFramesToFileClose

    //   SaveCapturedFileAs() can be used to extract video to new file
    //   this is useful only if the the file is larger than the # of frames
    //   eg. you've called SetCaptureFilesizeMB to create a larger than necessary file

    // get how many frames we've taken, reset to -1 when closed
    int GetCapturedFramesToFileCount() { return m_capsingleframecount; }

    // open an AVI file to capture single frames into it, returns sucess
    bool CaptureSingleFramesToFileOpen();
    // capture single frames into the AVI file, returns sucess
    bool CaptureSingleFramesToFile();
    // close the AVI file when done, returns sucess
    bool CaptureSingleFramesToFileClose();

    // a simple dialog to capture single frames to an AVI file
    //   it sets up the file and just click ok to save another frame
    void CaptureSingleFramesToFileDialog();

    // ----------------------------------------------------------------------
    // Capture streaming video to an AVI file
    // ----------------------------------------------------------------------

    // record video segment, returns sucess
    // starting/stopping controlled by setting up capture parameters
    bool CaptureVideoToFile();

    // set microsecond/frame to record with, default = 66667us = 15fps
    // not necessarily what you'll get if fps set too high
    unsigned long int GetMicroSecPerFrameRequested();
    void SetMicroSecPerFrameRequested( unsigned long int framespersec);

    // number of frames processed during current/recent capture
    //   includes dropped frames
    unsigned long int GetCapturedVideoFramesCount();
    // number of frames dropped during current/recent capture
    //      when frame is dropped the previous frame is put into the AVI file
    //      therefore does not affect syncronization, w/ audio & timing
    unsigned long int GetCapturedVideoFramesDropped();

    // time elapsed since the start of current/recent capture
    unsigned long int GetCaptureTimeElapsedMS();
    // capture in progress? then true
    bool IsCapturingNow();

    // keycode to terminate streaming video, default = VK_ESCAPE
    // NOTE use RegisterHotKey() for system wide key, NOT IMPLEMENTED, NOT TESTED
    unsigned int GetAbortKey();
    void SetAbortKey( unsigned int key );
    // abort video capture on left mouse button, default = true
    bool GetAbortLeftMouse();
    void SetAbortLeftMouse( bool leftmouse );
    // abort video capture on right mouse button, default = true
    bool GetAbortRightMouse();
    void SetAbortRightMouse( bool rightmouse );

    // only capture for a fixed number of seconds
    bool GetTimeLimitedCapture();
    void SetTimeLimitedCapture( bool usetimelimit );
    // number of seconds to capture video for
    unsigned int GetCaptureTimeLimit();
    void SetCaptureTimeLimit( unsigned int timelimit );

    // popup "OK" dialog to initiate capture, default = false
    bool GetDialogInitiatedCapture();
    void SetDialogInitiatedCapture( bool usedialog );

    // max allowed % of frames dropped, default=10 [0..100], popup error dialog
    // note: dropped frames are replaced by the last grabbed frame in AVI file
    unsigned int GetMaxAllowedFramesDropped();
    void SetMaxAllowedFramesDropped(unsigned int maxdrop );

    // these two must use capSetCallbackOnYield, I think?
    // stop capturing video & audio to AVI file, close file and end
    // UNTESTED
    bool CaptureVideoToFileStop();
    // abort capturing video to AVI file, "discard" data
    // notes: capture operation must yield to use this macro
    //        image data is retained, audio is not
    // UNTESTED
    bool CaptureVideoToFileAbort();

    // The default values are probably fine, but who knows?
    // actual number of video buffers allocated in the memory heap
    unsigned int GetNumVideoBuffersAllocated();
    // max number of video buffers to allocate, if 0 then autocalc, dunno?
    unsigned int GetNumVideoBuffers();
    void SetNumVideoBuffers( unsigned int vidbufs );

    // use background thread to capture, controls NOT disabled, default=false
    // see VFW_CallbackOnCaptureYield also VFW's capSetCallbackOnYield...
    // NOT TESTED
    bool GetUseThreadToCapture();
    void SetUseThreadToCapture( bool usethread );

    // ??? does this work? and for what device?
    // double resolution capture, (height&width*2)
    //      "enable if hardware doesn't support decimation & using RGB" huh?
    bool GetStepCaptureAt2x();
    void SetStepCaptureAt2x( bool cap2x);
    // ??? does this work? and for what device?
    // number of times a frame is sampled for average frame, typically 5
    unsigned int GetStepCaptureAverageFrames();
    void SetStepCaptureAverageFrames( unsigned int aveframes );

    // max number of index entries in AVI file [1800..324,000]
    //      each frame or audio buffer uses 1 index, sets max num of frames
    //      default = 34,952 (32K frames + proportional num of audio buffers)
    unsigned long int GetAviMaxIndexEntries();
    void SetAviMaxIndexEntries( unsigned long int maxindex);

    // Logical block size in an AVI file in bytes
    //  0 = current sector size, recommended default=2K
    //  if larger than the data structure, the rest is filled w/ RIFF "JUNK"
    //  64,128,256,512,1K,2K,4K,8K,16K,32K are valid... I think?
    unsigned int GetChunkGranularity();
    void SetChunkGranularity( unsigned int chunkbytes );

    // NOTE capFileSetInfoChunk can embed "data" in an AVI file, why bother?

    // ----------------------------------------------------------------------
    // Capture file settings, filename to capture video to
    // ----------------------------------------------------------------------
    // default file for VFW is c:\capture.avi
    // Overview: SetCaptureFilename() sets the file to capture AVI video to.
    // SetCaptureFilesizeMB() preallocates space for the video, make > needed
    // SaveCapturedFileAs() then copies only the actual video to a new file
    // or just set the filesize < than the needed, but take performance hit

    // Set the name of a capture file, used with SetCaptureFilesizeMB
    //     MSW wants you to set the name, preallocate space, capture and then
    //     use SaveCapturedFileAs() to extract the video to a new file.
    //     The idea is a permanent capture file exists for best performance

    // valid capture file exists, then true
    bool CaptureFileExists();
    // returns the name of the file to capture to AVI file
    wxString GetCaptureFilename();
    // set the name of the file to capture AVI file to, returns sucess
    //  doesn't create/open/allocate
    bool SetCaptureFilename(const wxString &capfilename);
    // Get the capture filename from MSW AVIcap window, returns sucess
    //  internal use only
    bool VFW_GetCaptureFilename();
    // dialog to ask the user what capture filename to use, returns sucess
    bool SetCaptureFilenameDialog();

    // preallocate space for the capture file, returns sucess
    // this is to get a fixed "contiguous" file to capture to,
    // however the user must run a defrag program to guarantee it's contiguous
    // if the captured data is smaller, then the rest of the file is just junk
    // if set to zero or smaller than the actual capture size or if the file
    // doesn't exist there be a performance loss...
    bool SetCaptureFilesizeMB(unsigned int filesizeMB=0 );
    // dialog to preallocate space for video capture, returns sucess
    bool SetCaptureFileSizeDialog();

    // extract actual data from the SetCaptureFilename(), save it to new file
    // this is used to create a file that is correctly sized to the data size
    bool SaveCapturedFileAs( const wxString &filename );

    // I guess that newer versions of wxWidgets will have these? 2.3?
    // get the amount of free disk space in KB
    long int GetFreeDiskSpaceInKB( const wxString &filepath );

    // ----------------------------------------------------------------------
    // Audio Setup
    // ----------------------------------------------------------------------
    // device has waveform-audio, then true (usually means a sound card)
    bool HasAudioHardware();

    // set the audio recording properties, see #defines above
    // channels = 1, 2 (mono, stereo)
    // bitspersample = 8, 16
    // samplespersecond = 8000,11025,16000,22050,24000,32000,44100,48000 (freq)
    // note: CD = stereo, 16bit, 44100
    //       Radio = mono, 8bit, 22050
    //       telephone = mono, 8bit, 11025
    bool SetAudioFormat( int channels = 1,
                         int bitspersample = 8,
                         long int samplespersecond = 11025 );

    // get the audio format, see SetAudioFormat, returns sucess
    bool GetAudioFormat( int *channels ,
                         int *bitspersample ,
                         long int *samplespersecond );

    // capture audio too, default = true if device has audio capability
    // actually the device is your sound card, I guess?
    bool GetCaptureAudio();
    void SetCaptureAudio( bool capaudio );

    // max number of audio buffers, (max = 10), if 0 then autocalc, default = 4
    unsigned int GetNumAudioBuffers();
    void SetNumAudioBuffers( unsigned int audiobufs );

    // Number of audio buffers allocated
    unsigned int GetNumAudioBuffersAllocated();
    // audio buffer size, default = 0 = max of either .5sec or 10Kb
    unsigned long int GetAudioBufferSize();
    void SetAudioBufferSize( unsigned long int audiobufsize );

    // audio stream controls clock for AVI file
    //      video duration is forced to match audio duration
    //      if false they can differ in length
    bool GetAudioStreamMaster();
    void SetAudioStreamMaster( bool audiomaster);

    // number of waveform-audio samples processed during current/recent capture
    unsigned long int GetCapturedWaveSamplesCount();

    // ----------------------------------------------------------------------
    // MSW VFW Callbacks call these wxVideoCaptureWindow functions
    //   SetCallbackOnXXX turns them on/off, check out what they do first
    // ----------------------------------------------------------------------

    bool VFW_SetCallbackOnError(bool on);
    // called when a nonfatal error occurs
    virtual bool VFW_CallbackOnError(const wxString &errortext, int errorid);

    bool VFW_SetCallbackOnStatus(bool on);
    // called when the device's status changes (usually)
    virtual bool VFW_CallbackOnStatus(const wxString &statustext, int statusid);

    bool VFW_SetCallbackFrame(bool on);
    // called when preview frames are available, previewing or not
    virtual bool VFW_CallbackOnFrame(LPVIDEOHDR lpVHdr);

    bool VFW_SetCallbackOnCaptureYield(bool on);
    // called during capture (between frames) to update the gui
    virtual bool VFW_CallbackOnCaptureYield();

    bool VFW_SetCallbackOnCaptureControl(bool on);
    // called after preroll (capture ready) and between capture frames
    virtual bool VFW_CallbackOnCaptureControl(int nState);

    bool VFW_SetCallbackOnVideoStream(bool on);
    // called before video frames are written to an AVI file
    virtual bool VFW_CallbackOnVideoStream(LPVIDEOHDR lpVHdr);

    bool VFW_SetCallbackOnWaveStream(bool on);
    // called during capture before audio buffers written, can be modified here
    virtual bool VFW_CallbackOnWaveStream(LPWAVEHDR lpWHdr);

protected:

    // ----------------------------------------------------------------------
    // Implementation
    // ----------------------------------------------------------------------

    void OnCloseWindow(wxCloseEvent &event);
    void OnIdle( wxIdleEvent &event );

    // ----------------------------------------------------------------------
    // Size & Position functions
    // ----------------------------------------------------------------------

    virtual void DoSetSize(int x, int y, int width, int height,
                           int sizeFlags = wxSIZE_AUTO);
    // adjust the scrollbars, use to generally refresh too
    void DoSizeWindow();

    // move m_hWndC when EVT_SCROLLWIN occurs when Overlaying
    void OnScrollWin( wxScrollWinEvent &event );

    // scroll the m_hWndC to this point, necessary for Overlay only
    bool VFW_ScrollHWND(int x, int y);

    // x,y offset of the upperleft corner of MSW capture window
    wxPoint VFW_GetHWNDViewStart();

    // called by wxWindow's EVT_MOVE, make Overlay window follow
    void OnMove( wxMoveEvent &event );

    // draw the frames when using wxImages preview from EVT_PAINT
    void OnDraw( wxPaintEvent &event );

    // ----------------------------------------------------------------------
    // Platform dependent video conversion
    // ----------------------------------------------------------------------

    // decompress a video frame into the m_wximage member variable, returns sucess
    // Device Dependent Bitmap -> 24bpp Device Independent Bitmap -> m_wximage
    bool VFW_DDBtoDIB(LPVIDEOHDR lpVHdr);

    // ----------------------------------------------------------------------
    // Member Variables
    // ----------------------------------------------------------------------

    // Generic variables

    unsigned char *m_bmpdata;       // big 'ole temp storage for DIB

    bool m_grab_wximage;            // grab a single frame into m_wximage
    bool m_getting_wximage;         // true when filling the m_wximage

    wxTimer m_preview_wximage_timer; // for preview rate adjustment

    wxString m_capturefilename;

    long int m_capsingleframecount; // number of frames taken,
                                    // -1 when closed, >= 0 if open

    wxString m_statustext;          // MSW status messages
    wxString m_errortext;           // MSW error messages

    // MSW specific variables
    HWND m_hWndC;       // this is the VFW vidcap HWND, it does everything

    HIC m_hic_compressor;           // handle to device (de)compressor

    BITMAPINFO *m_lpBmpInfo24bpp;   // bitmap header for 24bpp bitmaps
    BITMAPINFO *m_lpBmpInfo;        // storage for video bitmap header
    BITMAPINFO *m_lpBmpInfoLast;    // storage for video bitmap header

    // ------------------------------------------------------------------------
    CAPDRIVERCAPS m_CAPDRIVERCAPS;  // Capture Driver Capabilities
    bool VFW_GetCAPDRIVERCAPS();     // used internally

    // Parts of CAPDRIVERCAPS that are not used
    // HANDLE hVideo[In,Out,ExitIn,ExtOut] are unused in Win32, always NULL

    // ------------------------------------------------------------------------
    CAPSTATUS m_CAPSTATUS;           // Current status of the capture driver
    bool VFW_GetCAPSTATUS();          // used internally

    // Parts of CAPSTATUS that are not used
    // HPALETTE hPalCurrent - Current palette in use
    // DWORD dwReturn - Error code after any operation, 32 bit unsigned int, StatusCallback does this

    // ------------------------------------------------------------------------
    CAPTUREPARMS m_CAPTUREPARMS;   // Capture Parameters
    bool VFW_GetCAPTUREPARMS();     // used internally
    bool VFW_SetCAPTUREPARMS();     // used internally

    // Parts of CAPTUREPARMS that are unused
    // BOOL fUsingDOSMemory - unused in win32
    // BOOL fMCIControl - controlling a MCI compatible device, VCR, laserdisc
    // BOOL fStepMCIDevice - MCI device step capture if true, false is streaming
    // DWORD dwMCIStartTime - starting position in ms, for MCI device capture
    // DWORD dwMCIStopTime - stopping position in ms for MCI device capture
    // BOOL fDisableWriteCache - not used in Win32

private:
    void Init();
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxVideoCaptureWindowVFW)
};

#include "wx/msw/winundef.h"

#endif //__WX_VCAP_VFW_H__
