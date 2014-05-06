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

class GuideCamera;

class CameraConfigDialogPane : public ConfigDialogPane
{
    GuideCamera *m_pCamera;
    wxCheckBox *m_pUseSubframes;
    wxSpinCtrl *m_pCameraGain;
    wxChoice   *m_pPortNum;
    wxSpinCtrl *m_pDelay;
    wxSpinCtrlDouble *m_pPixelSize;

public:
    CameraConfigDialogPane(wxWindow *pParent, GuideCamera *pCamera);
    virtual ~CameraConfigDialogPane(void);

    virtual void LoadValues(void);
    virtual void UnloadValues(void);

    double GetPixelSize(void);
    void SetPixelSize(double val);
};

class GuideCamera :  public wxMessageBoxProxy, public OnboardST4
{
protected:

    virtual int GetCameraGain(void);
    virtual bool SetCameraGain(int cameraGain);
    virtual float GetCameraPixelSize(void);
    virtual bool SetCameraPixelSize(float pixel_size);

    friend class CameraConfigDialogPane;

protected:
    bool            m_hasGuideOutput;

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
    short           Port;
    int             ReadDelay;
    bool            ShutterState;  // false=light, true=dark
    bool            UseSubframes;
    double          PixelSize;

    static wxArrayString List(void);
    static GuideCamera *Factory(wxString choice);

    wxCriticalSection DarkFrameLock; // dark frames can be accessed in the main thread or the camera worker thread
    usImage         *CurrentDarkFrame;
    ExposureImgMap  Darks; // map exposure => dark frame
    DefectMap       *CurrentDefectMap;

    virtual bool HasNonGuiCapture(void);

    virtual bool    Capture(int duration, usImage& img, wxRect subframe = wxRect(0,0,0,0), bool recon=false) = 0;
    virtual bool    Capture(int duration, usImage& img, bool recon) { return Capture(duration, img, wxRect(0, 0, 0, 0), recon); }

    virtual bool    Connect() = 0;                  // Opens up and connects to camera
    virtual bool    Disconnect() = 0;               // Disconnects, unloading any DLLs loaded by Connect
    virtual void    InitCapture();                  // Gets run at the start of any loop (e.g., reset stream, set gain, etc).

    virtual bool    ST4HasGuideOutput(void);
    virtual bool    ST4HostConnected(void);
    virtual bool    ST4HasNonGuiMove(void);
    virtual bool    ST4PulseGuideScope(int direction, int duration);

    CameraConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    virtual void    ShowPropertyDialog() { return; }

    virtual wxString GetSettingsSummary();
    void            AddDark(usImage *dark);
    void            SelectDark(int exposureDuration);
    void            SetDefectMap(DefectMap *newMap);
    void            ClearDefectMap(void);
    void            ClearDarks(void);

    void            SubtractDark(usImage& img);

    GuideCamera(void);
    virtual ~GuideCamera(void);
};

#endif /* CAMERA_H_INCLUDED */
