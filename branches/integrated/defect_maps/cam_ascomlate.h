/*
 *  cam_ascomlate.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2009-2010 Craig Stark.
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

#ifndef CAM_ASCOMLATE_H_INCLUDED
#define CAM_ASCOMLATE_H_INCLUDED

#if defined (ASCOM_LATECAMERA)

class DispatchObj;
class DispatchClass;

class Camera_ASCOMLateClass : public GuideCamera, protected ASCOM_COMMON
{
public:
    virtual bool Capture(int duration, usImage& img, wxRect subframe = wxRect(0,0,0,0), bool recon=false);
    virtual bool HasNonGuiCapture(void);
    bool    Connect(void);
    bool    Disconnect(void);
    void    ShowPropertyDialog(void);
    bool    ST4PulseGuideScope(int direction, int duration);

    bool    Color;

    Camera_ASCOMLateClass(const wxString& choice);
    ~Camera_ASCOMLateClass();
    static wxArrayString EnumAscomCameras();

private:
#ifdef __WINDOWS__
    IGlobalInterfaceTable* m_pIGlobalInterfaceTable;
    DWORD m_dwCookie;
    DISPID dispid_setxbin, dispid_setybin;  // Frequently used IDs
    DISPID dispid_startx, dispid_starty;
    DISPID dispid_numx, dispid_numy;
    DISPID dispid_startexposure, dispid_stopexposure;
    DISPID dispid_imageready, dispid_imagearray;
    DISPID dispid_ispulseguiding, dispid_pulseguide;

    bool Create(DispatchObj *obj, DispatchClass *cls);

    bool ASCOM_SetBin(int mode, EXCEPINFO *excep);
    bool ASCOM_SetROI(int startx, int starty, int numx, int numy, EXCEPINFO *excep);
    bool ASCOM_StartExposure(double duration, bool light, EXCEPINFO *excep);
    bool ASCOM_StopExposure(EXCEPINFO *excep);
    bool ASCOM_ImageReady(bool *ready, EXCEPINFO *excep);
    bool ASCOM_Image(usImage& Image, bool useSubframe, wxRect subframe, EXCEPINFO *excep);
    bool ASCOM_IsMoving(void);

    bool ST4HasNonGuiMove(void);

    int DriverVersion;

    wxString m_choice; // name of chosen camera

#endif
};
#endif // defined (ASCOM_LATECAMERA)

#endif //CAM_ASCOMLATE_H_INCLUDED
