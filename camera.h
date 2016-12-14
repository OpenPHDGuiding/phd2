/*
 *  camera.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

typedef std::map<int, usImage *> ExposureImgMap; // map exposure to image
class DefectMap;

enum PropDlgType
{
    PROPDLG_NONE = 0,
    PROPDLG_WHEN_CONNECTED = (1 << 0),    // property dialog available when connected
    PROPDLG_WHEN_DISCONNECTED = (1 << 1), // property dialog available when disconnected
    PROPDLG_ANY = (PROPDLG_WHEN_CONNECTED | PROPDLG_WHEN_DISCONNECTED),
};

extern wxSize UNDEFINED_FRAME_SIZE;

class GuideCamera;

class CameraConfigDialogPane : public ConfigDialogPane
{
public:
    CameraConfigDialogPane(wxWindow *pParent, GuideCamera *pCamera);
    virtual ~CameraConfigDialogPane(void) {};

    void LayoutControls(GuideCamera *pCamera, BrainCtrlIdMap& CtrlMap);
    virtual void LoadValues(void) {};
    virtual void UnloadValues(void) {};
};

class CameraConfigDialogCtrlSet : public ConfigDialogCtrlSet
{
    GuideCamera *m_pCamera;
    wxCheckBox *m_pUseSubframes;
    wxSpinCtrl *m_pCameraGain;
    wxSpinCtrl *m_timeoutVal;
    wxChoice   *m_pPortNum;
    wxSpinCtrl *m_pDelay;
    wxSpinCtrlDouble *m_pPixelSize;
    wxChoice *m_binning;
    wxCheckBox *m_coolerOn;
    wxSpinCtrl *m_coolerSetpt;

public:
    CameraConfigDialogCtrlSet(wxWindow *pParent, GuideCamera *pCamera, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~CameraConfigDialogCtrlSet() {};
    virtual void LoadValues(void);
    virtual void UnloadValues(void);

    double GetPixelSize(void);
    void SetPixelSize(double val);
    int GetBinning(void);
    void SetBinning(int val);
};

enum CaptureOptionBits
{
    CAPTURE_SUBTRACT_DARK = 1 << 0,
    CAPTURE_RECON         = 1 << 1,    // debayer and/or deinterlace as required

    CAPTURE_LIGHT = CAPTURE_SUBTRACT_DARK | CAPTURE_RECON,
    CAPTURE_DARK = 0,
    CAPTURE_BPM_REVIEW = CAPTURE_SUBTRACT_DARK,
};

class GuideCamera :  public wxMessageBoxProxy, public OnboardST4
{
    friend class CameraConfigDialogPane;
    friend class CameraConfigDialogCtrlSet;

    double          m_pixelSize;

protected:
    bool            m_hasGuideOutput;
    int             m_timeoutMs;

public:
    int             GuideCameraGain;
    wxString        Name;                   // User-friendly name
    wxSize          FullSize;           // Size of current image
    bool            Connected;
    PropDlgType     PropertyDialogType;
    bool            HasPortNum;
    bool            HasDelayParam;
    bool            HasGainControl;
    bool            HasShutter;
    bool            HasSubframes;
    wxByte          MaxBinning;
    wxByte          Binning;
    short           Port;
    int             ReadDelay;
    bool            ShutterClosed;  // false=light, true=dark
    bool            UseSubframes;
    bool            HasCooler;

    wxCriticalSection DarkFrameLock; // dark frames can be accessed in the main thread or the camera worker thread
    usImage        *CurrentDarkFrame;
    ExposureImgMap  Darks; // map exposure => dark frame
    DefectMap      *CurrentDefectMap;

    static wxArrayString List(void);
    static GuideCamera *Factory(const wxString& choice);

    GuideCamera(void);
    virtual ~GuideCamera(void);

    virtual bool HasNonGuiCapture(void) = 0;
    virtual wxByte BitsPerPixel(void) = 0;

    static bool Capture(GuideCamera *camera, int duration, usImage& img, int captureOptions, const wxRect& subframe);
    static bool Capture(GuideCamera *camera, int duration, usImage& img, int captureOptions) { return Capture(camera, duration, img, captureOptions, wxRect(0, 0, 0, 0)); }

    virtual bool HandleSelectCameraButtonClick(wxCommandEvent& evt);
    static const wxString DEFAULT_CAMERA_ID;
    virtual bool    EnumCameras(wxArrayString& names, wxArrayString& ids);

    // Opens up and connects to camera. cameraId identifies which camera to connect to if
    // there is more than one camera present
    virtual bool    Connect(const wxString& cameraId) = 0;
    virtual bool    Disconnect() = 0;               // Disconnects, unloading any DLLs loaded by Connect
    virtual void    InitCapture();                  // Gets run at the start of any loop (e.g., reset stream, set gain, etc).

    virtual bool    ST4HasGuideOutput(void);
    virtual bool    ST4HostConnected(void);
    virtual bool    ST4HasNonGuiMove(void);
    virtual bool    ST4PulseGuideScope(int direction, int duration);

    CameraConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    CameraConfigDialogCtrlSet *GetConfigDlgCtrlSet(wxWindow *pParent, GuideCamera *pCamera, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);

    static void GetBinningOpts(int maxBin, wxArrayString *opts);
    void GetBinningOpts(wxArrayString *opts);

    virtual void    ShowPropertyDialog() { return; }
    bool            SetCameraPixelSize(double pixel_size);
    double          GetCameraPixelSize(void) const;
    virtual bool    GetDevicePixelSize(double *devPixelSize);           // Value from device/driver or error return

    virtual bool    SetCoolerOn(bool on);
    virtual bool    SetCoolerSetpoint(double temperature);
    virtual bool    GetCoolerStatus(bool *on, double *setpoint, double *power, double *temperature);

    virtual wxString GetSettingsSummary();
    void            AddDark(usImage *dark);
    void            SelectDark(int exposureDuration);
    void            SetDefectMap(DefectMap *newMap);
    void            ClearDefectMap(void);
    void            ClearDarks(void);

    void            SubtractDark(usImage& img);
    void            GetDarklibProperties(int *pNumDarks, double *pMinExp, double *pMaxExp);

    virtual const wxSize& DarkFrameSize() { return FullSize; }

    static double GetProfilePixelSize(void);

protected:

    virtual bool Capture(int duration, usImage& img, int captureOptions, const wxRect& subframe) = 0;
    int GetCameraGain(void);
    bool SetCameraGain(int cameraGain);
    bool SetBinning(int binning);
    int GetTimeoutMs(void) const;
    void SetTimeoutMs(int timeoutMs);

    enum CaptureFailType {
        CAPT_FAIL_MEMORY,
        CAPT_FAIL_TIMEOUT,
    };
    enum ReconnectType {
        NO_RECONNECT,
        RECONNECT,
    };
    void DisconnectWithAlert(CaptureFailType type);
    void DisconnectWithAlert(const wxString& msg, ReconnectType reconnect);
};

inline int GuideCamera::GetTimeoutMs(void) const
{
    return m_timeoutMs;
}

inline void GuideCamera::GetBinningOpts(wxArrayString *opts)
{
    GetBinningOpts(MaxBinning, opts);
}

inline double GuideCamera::GetCameraPixelSize(void) const
{
    return m_pixelSize;
}

inline bool GuideCamera::GetDevicePixelSize(double *devPixelSize)
{
    return true;                // Return an error, the device/driver can't report pixel size
}

#endif /* CAMERA_H_INCLUDED */
