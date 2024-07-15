/*
 *  cam_sbigrotator.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2013 Bret McKee
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
 *    Neither the name of Bret Mckee, Dad Dog Development Ltd nor the names of its
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

#if defined(SBIG) && defined(SBIGROTATOR_CAMERA)

#include "cam_sbigrotator.h"
#include "cam_sbig.h"

class CameraSBIGRotator : public GuideCamera
{
private:
    GuideCamera *m_pSubcamera;
    double m_raAngle;
    bool m_mirror;

public:
    CameraSBIGRotator();
    ~CameraSBIGRotator();

    bool Capture(int duration, usImage& img, int options, const wxRect& subframe) override;
    bool ST4PulseGuideScope(int direction, int duration) override;
    bool Connect(const wxString& camId) override;
    bool Disconnect() override;
    bool ST4HasNonGuiMove() override;
    bool HasNonGuiCapture() override;
    wxByte BitsPerPixel() override;
};

CameraSBIGRotator::CameraSBIGRotator()
{
    m_pSubcamera = nullptr;
    m_raAngle = 0.0;
    m_mirror = false;

    Connected = false;
    Name = _T("SBIG Rotator Camera");
}

CameraSBIGRotator::~CameraSBIGRotator()
{
    delete m_pSubcamera;
}

bool CameraSBIGRotator::Connect(const wxString& camId)
{
    bool bError = false;

    try
    {
        wxString raAngle = wxGetTextFromUser(_("Enter RA Angle (in degrees)"),_("RA angle"), _T("0.0"));
        double temp;

        if (raAngle.length() == 0 || !raAngle.ToDouble(&temp))
        {
            throw ERROR_INFO("invalid raAngle");
        }

        m_raAngle = radians(temp);

        wxArrayString choices;

        choices.Add(wxString::Format("Unmirrored (%.2f)", temp - 90));
        choices.Add(wxString::Format("Mirrored (%.2f)", temp + 90));

        int idx = wxGetSingleChoiceIndex(_("Choose Dec Angle"), _("Dec Angle"),
                                              choices);
        if (idx == -1)
            return true;

        m_mirror = (idx == 1);

        m_pSubcamera = SBIGCameraFactory::MakeSBIGCamera();

        bError = m_pSubcamera->Connect(camId);
        Connected = m_pSubcamera->Connected;

        FullSize = m_pSubcamera->FullSize;
        m_hasGuideOutput = m_pSubcamera->ST4HasGuideOutput();
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);

        bError = true;
    }

    return bError;
}

bool CameraSBIGRotator::HasNonGuiCapture()
{
    return m_pSubcamera && m_pSubcamera->HasNonGuiCapture();
}

bool CameraSBIGRotator::ST4HasNonGuiMove()
{
    return m_pSubcamera && m_pSubcamera->ST4HasNonGuiMove();
}

wxByte CameraSBIGRotator::BitsPerPixel()
{
    return m_pSubcamera ? m_pSubcamera->BitsPerPixel() : 0;
}

bool CameraSBIGRotator::Disconnect()
{
    m_pSubcamera->Disconnect();
    Connected = m_pSubcamera->Connected;
    return false;
}

bool CameraSBIGRotator::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool bError = m_pSubcamera->Capture(duration, img, options, subframe);

    img.Rotate(m_raAngle, m_mirror);

    return bError;
}

bool CameraSBIGRotator::ST4PulseGuideScope (int direction, int duration)
{
    return m_pSubcamera->ST4PulseGuideScope(direction, duration);
}

GuideCamera *SBIGRotatorCameraFactory::MakeSBIGRotatorCamera()
{
    return new CameraSBIGRotator();
}

#endif // SBIGROTATOR_CAMERA_H_INCLUDED
