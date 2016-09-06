/*
*  cam_Altair.cpp
*  PHD Guiding
*
*  Created by Robin Glover.
*  Copyright (c) 2014 Robin Glover.
*  All rights reserved.
*
*  This source code is distributed under the following "BSD" license
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*    Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*    Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*    Neither the name of Craig Stark, Stark Labs nor the names of its
*     contributors may be used to endorse or promote products derived from
*     this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
*/
#include "phd.h"

#ifdef ALTAIR

#include "cam_Altair.h"

#ifdef __WINDOWS__

#ifdef OS_WINDOWS
// troubleshooting with the libusb definitions
#  undef OS_WINDOWS
#endif

# include <Shlwapi.h>
# include <DelayImp.h>
#endif

class AltairCameraDlg : public wxDialog
{
public:

    wxCheckBox* m_reduceRes;
    AltairCameraDlg(wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& title = _("Altair Camera Settings"),
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(268, 133), long style = wxDEFAULT_DIALOG_STYLE);
    ~AltairCameraDlg() { }
};

AltairCameraDlg::AltairCameraDlg(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{


    SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer *bSizer12 = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Settings")), wxHORIZONTAL);

    m_reduceRes = new wxCheckBox(this, wxID_ANY, wxT("Reduced Resolution (by ~20%)"), wxDefaultPosition, wxDefaultSize, 0);
    sbSizer3->Add(m_reduceRes, 0, wxALL, 5);
    bSizer12->Add(sbSizer3, 1, wxEXPAND, 5);

    wxStdDialogButtonSizer* sdbSizer2 = new wxStdDialogButtonSizer();
    wxButton *sdbSizer2OK = new wxButton(this, wxID_OK);
    wxButton* sdbSizer2Cancel = new wxButton(this, wxID_CANCEL);
    sdbSizer2->AddButton(sdbSizer2OK);
    sdbSizer2->AddButton(sdbSizer2Cancel);
    sdbSizer2->Realize();
    bSizer12->Add(sdbSizer2, 0, wxALL | wxEXPAND, 5);

    SetSizer(bSizer12);
    Layout();

    Centre(wxBOTH);
}

Camera_Altair::Camera_Altair()
    : m_buffer(0),
    m_capturing(false)
{
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;

    Name = _T("Altair Camera");
    Connected = false;
    m_hasGuideOutput = true;
    HasSubframes = false;
    HasGainControl = true; // workaround: ok to set to false later, but brain dialog will frash if we start false then change to true later when the camera is connected
}

Camera_Altair::~Camera_Altair()
{
    delete[] m_buffer;
}

wxByte Camera_Altair::BitsPerPixel()
{
    return 8;
}

inline static int cam_gain(int minval, int maxval, int pct)
{
    return minval + pct * (maxval - minval) / 100;
}

inline static int gain_pct(int minval, int maxval, int val)
{
    return (val - minval) * 100 / (maxval - minval);
}

bool Camera_Altair::EnumCameras(wxArrayString& names, wxArrayString& ids)
{
	AltaircamInst ai[ALTAIRCAM_MAX];

    unsigned int numCameras = Altaircam_Enum(ai);

    for (int i = 0; i < numCameras; i++)
    {
        names.Add(ai[i].displayname);
        ids.Add(ai[i].id);
    }

    return false;
}

bool Camera_Altair::Connect(const wxString& camIdArg)
{
	AltaircamInst ai[ALTAIRCAM_MAX];
    unsigned int numCameras = Altaircam_Enum(ai);
    if (numCameras == 0)
    {
        wxMessageBox(_T("No Altair cameras detected."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }
    wxString camId(camIdArg);
    if (camId == DEFAULT_CAMERA_ID || numCameras == 1)
        camId = ai[0].id;

	bool found = false;
	for (int i=0; i<numCameras; i++)
	{
		if (camId == ai[i].id)
		{
			found = true;
			break;
		}
	}
	if (!found)
	if (m_handle == NULL)
	{
		wxMessageBox(_("Specified Altair Camera not found."), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

    m_handle = Altaircam_Open(camId);
    if ( m_handle == NULL)
    {
        wxMessageBox(_("Failed to open Altair Camera."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    Connected = true;
    bool hasROI = false;
    bool hasSkip = false;

    for (int i = 0; i < numCameras; i++)
    {
        if (ai[i].id == camId)
        {
            Name = ai[i].displayname;
            hasROI = (ai[i].model->flag & ALTAIRCAM_FLAG_ROI_HARDWARE) != 0;
            hasSkip = (ai[i].model->flag & ALTAIRCAM_FLAG_BINSKIP_SUPPORTED) != 0;
            break;
        }
    }

    int width, height;
    if (FAILED(Altaircam_get_Resolution(m_handle, 0, &width, &height)))
    {
        Disconnect();
        wxMessageBox(_("Failed to get camera resolution for Altair Camera."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

	delete[] m_buffer; 
	m_buffer = new unsigned char[width * height]; // new SDK has issues with some ROI functions needing full resolution buffer size


    ReduceResolution = pConfig->Profile.GetBoolean("/camera/Altair/ReduceResolution", false);
    if (hasROI && ReduceResolution)
    {
        width *= 0.8;
        height *= 0.8;
    }

    FullSize.x = width;
    FullSize.y = height;


    float xSize, ySize;
    m_devicePixelSize = 3.75; // for all cameras so far....
    if (Altaircam_get_PixelSize(m_handle, 0, &xSize, &ySize) == 0)
    {
        m_devicePixelSize = xSize;
    }

    wxYield();

    HasGainControl = false;

    unsigned short min, max, def;
    if (SUCCEEDED(Altaircam_get_ExpoAGainRange(m_handle, &min, &max, &def)))
    {
        m_minGain = min;
        m_maxGain = max;
        HasGainControl = max > min;
    }

	Altaircam_put_AutoExpoEnable(m_handle, FALSE);

    Altaircam_put_Speed(m_handle, 0);
    Altaircam_put_RealTime(m_handle, TRUE);

    wxYield();

    m_frame = wxRect(FullSize);
    Debug.Write(wxString::Format("Altair: frame (%d,%d)+(%d,%d)\n", m_frame.x, m_frame.y, m_frame.width, m_frame.height));

    if (hasROI && ReduceResolution)
    {
        Altaircam_put_Roi(m_handle, 0, 0, width, height);
    }

    if (hasSkip)
        Altaircam_put_Mode(m_handle, 0);

    Altaircam_put_Option(m_handle, ALTAIRCAM_OPTION_RAW, 0);
	Altaircam_put_Option(m_handle, ALTAIRCAM_OPTION_AGAIN, 0);


    return false;
}

bool Camera_Altair::StopCapture(void)
{
    if (m_capturing)
    {
        Debug.AddLine("Altair: stopcapture");
        Altaircam_Stop(m_handle);
        m_capturing = false;
    }
    return true;
}

void Camera_Altair::FrameReady(void)
{
    m_frameReady = true;
}

bool Camera_Altair::GetDevicePixelSize(double *devPixelSize)
{
    if (!Connected)
        return true;

    *devPixelSize = m_devicePixelSize;
    return false;                               // Pixel size is known in any case
}

void Camera_Altair::ShowPropertyDialog()
{
    AltairCameraDlg dlg(wxGetApp().GetTopWindow());
    bool value = pConfig->Profile.GetBoolean("/camera/Altair/ReduceResolution", false);
    dlg.m_reduceRes->SetValue(value);
    if (dlg.ShowModal() == wxID_OK)
    {
        ReduceResolution = dlg.m_reduceRes->GetValue();
        pConfig->Profile.SetBoolean("/camera/Altair/ReduceResolution", ReduceResolution);
    }
}

bool Camera_Altair::Disconnect()
{
    StopCapture();
    Altaircam_Close(m_handle);

    Connected = false;

    delete[] m_buffer;
    m_buffer = 0;

    return false;
}

inline static int round_down(int v, int m)
{
    return v & ~(m - 1);
}

inline static int round_up(int v, int m)
{
    return round_down(v + m - 1, m);
}


void __stdcall CameraCallback(unsigned nEvent, void* pCallbackCtx)
{
    if (nEvent == ALTAIRCAM_EVENT_IMAGE)
    {
        Camera_Altair* pCam = (Camera_Altair*)pCallbackCtx;
        pCam->FrameReady();
    }
}
//static void flush_buffered_image(int cameraId, usImage& img)
//{
//    enum { NUM_IMAGE_BUFFERS = 2 }; // camera has 2 internal frame buffers
//
//    // clear buffered frames if any
//
//    for (unsigned int num_cleared = 0; num_cleared < NUM_IMAGE_BUFFERS; num_cleared++)
//    {
//        ASI_ERROR_CODE status = ASIGetVideoData(cameraId, (unsigned char *) img.ImageData, img.NPixels * sizeof(unsigned short), 0);
//        if (status != ASI_SUCCESS)
//            break; // no more buffered frames
//
//        Debug.Write(wxString::Format("Altair: getimagedata clearbuf %u ret %d\n", num_cleared + 1, status));
//    }
//}

bool Camera_Altair::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    if (img.Init(FullSize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    wxRect frame;
    frame = wxRect(FullSize);

    long exposureUS = duration * 1000;
    unsigned int cur_exp;
    if (Altaircam_get_ExpoTime(m_handle, &cur_exp) == 0 &&
        cur_exp != exposureUS)
    {
        Debug.Write(wxString::Format("Altair: set CONTROL_EXPOSURE %d\n", exposureUS));
        Altaircam_put_ExpoTime(m_handle, exposureUS);
    }

    long new_gain = cam_gain(m_minGain, m_maxGain, GuideCameraGain);
    unsigned short cur_gain;
    if (Altaircam_get_ExpoAGain(m_handle, &cur_gain) == 0 &&
        new_gain != cur_gain)
    {
        Debug.Write(wxString::Format("Altair: set CONTROL_GAIN %d%% %d\n", GuideCameraGain, new_gain));
        Altaircam_put_ExpoAGain(m_handle, new_gain);
    }


    // the camera and/or driver will buffer frames and return the oldest frame,
    // which could be quite stale. read out all buffered frames so the frame we
    // get is current

    //flush_buffered_image(m_handle, img);
    unsigned int width, height;
    while (SUCCEEDED(Altaircam_PullImage(m_handle, m_buffer, 8, &width, &height)))
    {
        
    }


    if (!m_capturing)
    {
        Debug.AddLine("Altair: startcapture");
        HRESULT result = Altaircam_StartPullModeWithCallback(m_handle, CameraCallback, this);
        if (result != 0)
        {
            Debug.Write(wxString::Format("Altaircam_StartPullModeWithCallback failed with code %d\n", result));
            return true;
        }
        m_capturing = true;
		m_frameReady = false;
	}

    int frameSize = frame.GetWidth() * frame.GetHeight();

    int poll = wxMin(duration, 100);

    CameraWatchdog watchdog(duration, duration + GetTimeoutMs() + 10000); // total timeout is 2 * duration + 15s (typically)

	// do not wait here, as we will miss a frame most likely, leading to poor flow of frames.

  /*  if (WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY) &&
        (WorkerThread::TerminateRequested() || StopCapture()))
    {
        return true;
    }*/

    while (true)
    {
        if (m_frameReady)
        {
            m_frameReady = false;
            
            if (SUCCEEDED(Altaircam_PullImage(m_handle, m_buffer, 8, &width, &height)))
                break;
        }
        WorkerThread::MilliSleep(poll, WorkerThread::INT_ANY);
        if (WorkerThread::InterruptRequested())
        {
            StopCapture();
            return true;
        }
        if (watchdog.Expired())
        {
            Debug.AddLine("Altair: getimagedata failed");
            StopCapture();
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    for (int i = 0; i < img.NPixels; i++)
        img.ImageData[i] = m_buffer[i];

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    return false;
}

inline static int GetAltairDirection(int direction)
{
    switch (direction)
    {
    default:
    case NORTH:
        return 0;
    case EAST:
        return 2;
    case WEST:
        return 3;
    case SOUTH:
        return 1;
    }
}

bool Camera_Altair::ST4PulseGuideScope(int direction, int duration)
{
    int d = GetAltairDirection(direction);
    Altaircam_ST4PlusGuide(m_handle, d, duration);

    return false;
}

void  Camera_Altair::ClearGuidePort()
{
    Altaircam_ST4PlusGuide(m_handle, 0,0);
}

#endif // Altaircam_ASI
