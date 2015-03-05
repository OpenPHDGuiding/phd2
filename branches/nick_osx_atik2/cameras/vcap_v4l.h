/////////////////////////////////////////////////////////////////////////////
// Name:        vcap_v4l.h - wxVideoCaptureWindow using Linux V4L2 API
// Author:      John Labenski
// Created:     7/06/2001
// Modified:    01/14/03
// Copyright:   John Labenski
// License:     wxWidgets V2.0
/////////////////////////////////////////////////////////////////////////////
//
// Usage notes:

#ifndef __WX_VCAP_V4L_H__
#define __WX_VCAP_V4L_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "vcap_v4l.h"
#endif

#include "wx/defs.h"
#include "wx/timer.h"

class wxTimerEvent;
class wxScrollWinEvent;
class wxSizeEvent;
class wxMoveEvent;
class wxPaintEvent;

#include <linux/videodev.h>

//----------------------------------------------------------------------------
// wxVideoCaptureWindow : window for viewing/recording streaming video or snapshots
//----------------------------------------------------------------------------
class WXDLLIMPEXP_VIDCAP wxVideoCaptureWindowV4L: public wxVideoCaptureWindowBase
{
public:
    wxVideoCaptureWindowV4L() : wxVideoCaptureWindowBase() {}
    wxVideoCaptureWindowV4L( wxWindow *parent, wxWindowID id = -1,
                             const wxPoint &pos = wxDefaultPosition,
                             const wxSize &size = wxDefaultSize,
                             long style = wxSIMPLE_BORDER,
                             const wxString &name = wxT("wxVideoCaptureWindow"));

    bool Create( wxWindow *parent, wxWindowID id = -1,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 long style = wxSIMPLE_BORDER,
                 const wxString &name = wxT("wxVideoCaptureWindow"));

    virtual ~wxVideoCaptureWindowV4L();

    // ----------------------------------------------------------------------
    // Device descriptions & versions, get and enumerate
    // ----------------------------------------------------------------------

    void EnumerateDevices();

    // ----------------------------------------------------------------------
    // Connect or Disconnect to device
    // ----------------------------------------------------------------------

    bool DeviceConnect(int index);
    bool DeviceDisconnect();

    // ----------------------------------------------------------------------
    // Display dialogs to set/get video characteristics
    // ----------------------------------------------------------------------

    void VideoCustomFormatDialog();
    void PropertiesDialog();
    wxString GetPropertiesString();

    // ----------------------------------------------------------------------
    // Video characteristics and manipulation
    // ----------------------------------------------------------------------

    bool GetVideoFormat( int *width, int *height, int *bpp, FOURCC *fourcc );
    bool SetVideoFormat( int width, int height, int bpp, FOURCC fourcc );

    // ----------------------------------------------------------------------
    // Capture Preview and Overlay
    // ----------------------------------------------------------------------

    void OnPreviewwxImageTimer(wxTimerEvent& event); // get frames

    bool Preview(bool onoff, bool wxpreview = false);
    bool Overlay(bool WXUNUSED(on)) { return false; }

    bool SetPreviewRateMS( unsigned int msperframe = 66 )
    { return DoSetPreviewRateMS( msperframe ); }

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

    // NOT IMPLEMENTED

    // ----------------------------------------------------------------------
    // Capture streaming video to an AVI file
    // ----------------------------------------------------------------------

    // NOT IMPLEMENTED

    // ----------------------------------------------------------------------
    // Capture file settings, filename to capture video to
    // ----------------------------------------------------------------------

    // NOT IMPLEMENTED

    // ----------------------------------------------------------------------
    // Audio Setup
    // ----------------------------------------------------------------------

    // NOT IMPLEMENTED

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

    // called by wxWindow's EVT_MOVE, make Overlay window follow
    void OnMove( wxMoveEvent &event );

    // draw the frames when using wxImages preview from EVT_PAINT
    void OnDraw( wxPaintEvent &event );

    // ----------------------------------------------------------------------
    // Platform dependent video conversion
    // ----------------------------------------------------------------------

    // FIXME add conversion here!

    // ----------------------------------------------------------------------
    // Member Variables
    // ----------------------------------------------------------------------

    // Generic variables
    wxArrayString m_deviceFilenames; // all device files from EnumerateDevices

    unsigned char *m_bmpdata;       // big 'ole temp storage for DIB

    bool m_grab_wximage;            // grab a single frame into m_wximage
    bool m_getting_wximage;         // true when filling the m_wximage

    wxTimer m_preview_wximage_timer; // for preview rate adjustment

    wxString m_capturefilename;

    wxString m_statustext;          // MSW status messages
    wxString m_errortext;           // MSW error messages

    // V4L specific variables

    // safe - open and close a device (can call w/o checks)
    int m_fd_device;      // the device m_fd_device = open("/dev/video",O_RDWR)
    bool open_device(const wxString &filename);
    bool close_device();

    // safe - map and unmap the shared video memory (can call w/o checks)
    void *m_map;            // memory map of the
    int m_map_size;         // size of memory map
    bool mmap_mem();        // mmap the memory to m_map
    bool munmap_mem();      // mumap the memory from m_map

    // safe - ioctl function
    int xioctl(int fd, int request, void *arg) const;

    // v4l structs
    struct video_mbuf       m_video_mbuf; //
    struct video_mmap       m_video_mmap;
    struct video_capability m_video_capability;
    struct video_window     m_video_window;
    struct video_picture    m_video_picture;
    struct video_buffer     m_video_buffer;
    struct video_channel    m_video_channel;
    struct video_clip       m_video_clip;
    struct video_capture    m_video_capture;
    struct video_unit       m_video_unit;
    struct video_tuner      m_video_tuner;

    // simple functions to set the v4l structs
    inline int V4L_Set_video_buffer();
    inline int V4L_Set_video_window();
    inline int V4L_Set_videoSource(int);
    inline int V4L_Set_video_picture();
    inline int V4L_Set_video_tuner();

    // simple functions to get the v4l structs
    inline int V4L_Get_video_capability();
    inline int V4L_Get_video_buffer();
    inline int V4L_Get_video_window();
    inline int V4L_Get_video_channel();
    inline int V4L_Get_video_picture();
    inline int V4L_Get_video_tuner();
    inline int V4L_Get_video_mbuf();

    // methods to display device information
    void print_video_capability();
    void print_video_window();
    void print_video_channel();
    void print_video_mbuf();

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxVideoCaptureWindowV4L)
};

#endif //__WX_VCAP_V4L_H__
