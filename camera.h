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

class GuideCamera :  public wxMessageBoxProxy
{
protected:
    class CameraConfigDialogPane : public ConfigDialogPane
    {
        GuideCamera *m_pCamera;
        wxCheckBox *m_pUseSubframes;
        wxSpinCtrl *m_pCameraGain;
        wxChoice   *m_pPortNum;
        wxSpinCtrl *m_pDelay;
        wxTextCtrl *m_pPixelSize;
    public:
        CameraConfigDialogPane(wxWindow *pParent, GuideCamera *pCamera);
        virtual ~CameraConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };

    virtual int GetCameraGain(void);
    virtual bool SetCameraGain(int cameraGain);
    virtual float GetCameraPixelSize(void);
    virtual bool SetCameraPixelSize(float pixel_size);

    friend class CameraConfigDialogPane;

public:
    int             GuideCameraGain;
    wxString        Name;                   // User-friendly name
    wxSize          FullSize;           // Size of current image
    bool            Connected;
    bool            HasGuiderOutput;
    bool            HasPropertyDialog;
    bool            HasPortNum;
    bool            HasDelayParam;
    bool            HasGainControl;
    bool            HasShutter;
    bool            HasSubframes;
    short           Port;
    int             Delay;
    bool            ShutterState;  // false=light, true=dark
    bool            UseSubframes;
    double          PixelSize;

    bool            HaveDark;
    int             DarkDur;
    usImage         CurrentDarkFrame;

    virtual bool HasNonGuiCapture(void) { return false; }

    virtual bool    Capture(int duration, usImage& img, wxRect subframe = wxRect(0,0,0,0), bool recon=false) = 0;
    virtual bool    Capture(int duration, usImage& img, bool recon) { return Capture(duration, img, wxRect(0, 0, 0, 0), recon); }

    virtual bool    Connect() { return true; }      // Opens up and connects to camera
    virtual bool    Disconnect() { return true; }   // Disconnects, unloading any DLLs loaded by Connect
    virtual void    InitCapture() { return; }       // Gets run at the start of any loop (e.g., reset stream, set gain, etc).

    virtual bool    HasNonGuiMove(void) { return false; }
    virtual bool    PulseGuideScope (int direction, int duration) { return true; }

    ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    virtual void    ShowPropertyDialog() { return; }

    virtual wxString GetSettingsSummary();

    GuideCamera(void);
    virtual ~GuideCamera(void) {};
};

#endif /* CAMERA_H_INCLUDED */
