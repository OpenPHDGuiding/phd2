/*
 *  mount.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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

Mount::Mount(void)
{
    m_connected = false;
    m_requestCount = 0;

    m_pDecGuideAlgorithm = NULL;
    m_pRaGuideAlgorithm = NULL;
    m_guidingEnabled = true;

    ClearCalibration();
}

Mount::~Mount()
{
    delete m_pRaGuideAlgorithm;
    delete m_pDecGuideAlgorithm;
}

bool Mount::GetGuidingEnabled(void)
{
    return m_guidingEnabled;
}

void Mount::SetGuidingEnabled(bool guidingEnabled)
{
    m_guidingEnabled = guidingEnabled;
}

GUIDE_ALGORITHM Mount::GetGuideAlgorithm(GuideAlgorithm *pAlgorithm)
{
    GUIDE_ALGORITHM ret = GUIDE_ALGORITHM_NONE;

    if (pAlgorithm)
    {
        ret = pAlgorithm->Algorithm();
    }
    return ret;
}

bool Mount::SetGuideAlgorithm(int guideAlgorithm, GuideAlgorithm** ppAlgorithm)
{
    bool bError = false;

    try
    {
        switch (guideAlgorithm)
        {
            case GUIDE_ALGORITHM_IDENTITY:
            case GUIDE_ALGORITHM_HYSTERESIS:
            case GUIDE_ALGORITHM_LOWPASS:
            case GUIDE_ALGORITHM_LOWPASS2:
            case GUIDE_ALGORITHM_RESIST_SWITCH:
                break;
            case GUIDE_ALGORITHM_NONE:
            default:
                throw ERROR_INFO("invalid guideAlgorithm");
                break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        guideAlgorithm = GUIDE_ALGORITHM_IDENTITY;
    }

    switch (guideAlgorithm)
    {
        case GUIDE_ALGORITHM_IDENTITY:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmIdentity();
            break;
        case GUIDE_ALGORITHM_HYSTERESIS:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmHysteresis();
            break;
        case GUIDE_ALGORITHM_LOWPASS:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass();
            break;
        case GUIDE_ALGORITHM_LOWPASS2:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass2();
            break;
        case GUIDE_ALGORITHM_RESIST_SWITCH:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmResistSwitch();
            break;
        case GUIDE_ALGORITHM_NONE:
        default:
            assert(false);
            break;
    }

    return bError;
}

GUIDE_ALGORITHM Mount::GetRaGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pRaGuideAlgorithm);
}

void Mount::SetRaGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    delete m_pRaGuideAlgorithm;

    if (SetGuideAlgorithm(guideAlgorithm, &m_pRaGuideAlgorithm))
    {
        SetGuideAlgorithm(defaultAlgorithm, &m_pRaGuideAlgorithm);
    }
}

GUIDE_ALGORITHM Mount::GetDecGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pDecGuideAlgorithm);
}

void Mount::SetDecGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    delete m_pDecGuideAlgorithm;

    if (SetGuideAlgorithm(guideAlgorithm, &m_pDecGuideAlgorithm))
    {
        SetGuideAlgorithm(defaultAlgorithm, &m_pDecGuideAlgorithm);
    }
}

const wxString &Mount::Name(void)
{
    return m_Name;
}

bool Mount::IsConnected()
{
    bool bReturn = m_connected;

    return bReturn;
}

bool Mount::IsBusy(void)
{
    return m_requestCount > 0;
}

void Mount::IncrementRequestCount(void)
{
    m_requestCount++;

    // for the moment we never enqueue requests if the mount is busy, but we can
    // enqueue them two at a time.  There is no reason we can't, it's just that
    // right now we don't, and this might catch an error
    assert(m_requestCount <= 2);

}

void Mount::DecrementRequestCount(void)
{
    assert(m_requestCount > 0);
    m_requestCount--;
}

bool Mount::IsCalibrated()
{
    bool bReturn = false;

    if (IsConnected())
    {
        bReturn = m_calibrated;
    }

    return bReturn;
}

void Mount::ClearCalibration(void)
{
    m_calibrated = false;
}

double Mount::DecAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_decAngle;
    }

    return dReturn;
}

double Mount::RaAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_raAngle;
    }

    return dReturn;
}

double Mount::DecRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_decRate;
    }

    return dReturn;
}

double Mount::RaRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_raRate;
    }

    return dReturn;
}

bool Mount::Connect(void)
{
    m_connected = true;

    return false;
}

bool Mount::Disconnect(void)
{
    m_connected = false;

    return false;
}

bool Mount::TransformCameraCoordinatesToMountCoordinates(const PHD_Point& vectorEndpoint,
                                                         double& raDistance,
                                                         double& decDistance)
{
    bool bError = false;

    try
    {
        if (!vectorEndpoint.IsValid())
        {
            throw ERROR_INFO("invalid vectorEndPoint");
        }

        PHD_Point origin(0,0);
        double theta = origin.Angle(vectorEndpoint);
        double hyp   = origin.Distance(vectorEndpoint);

        // Convert theta and hyp into RA and DEC

        raDistance  = cos(m_raAngle -  theta) * hyp;
        decDistance = cos(m_decAngle - theta) * hyp;

        Debug.AddLine("CameraToMount -- cameraX=%.2f cameraY=%.2f hyp=%.2f theta=%.2f mountX=%.2lf mountY=%.2lf",
                vectorEndpoint.X, vectorEndpoint.Y, hyp, theta, raDistance, decDistance);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool Mount::TransformMoutCoordinatesToCameraCoordinates(const double raDistance,
                                                        const double decDistance,
                                                        PHD_Point& vectorEndpoint)
{
    bool bError = false;

    try
    {
        PHD_Point origin(0,0);
        PHD_Point mountPosition(raDistance, decDistance);

        double hyp = origin.Distance(mountPosition);
        double theta = origin.Angle(mountPosition);
        double cameraX = cos(m_raAngle -  theta) * hyp;
        double cameraY = cos(m_decAngle - theta) * hyp;

        vectorEndpoint.SetXY(cameraX, cameraY);

        Debug.AddLine("MountToCamera -- mountX=%.2f mountY=%.2f hyp=%.2f theta=%.2f cameraX=%.2f, cameraY=%.2f",
                raDistance, decDistance, hyp, theta, cameraX, cameraY);

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        vectorEndpoint.Invalidate();
    }

    return bError;
}

bool Mount::Move(const PHD_Point& vectorEndpoint, bool normalMove)
{
    bool bError = false;

    try
    {
        double raDistance, decDistance;

        if (TransformCameraCoordinatesToMountCoordinates(vectorEndpoint, raDistance, decDistance))
        {
            throw ERROR_INFO("Unable to transform camera coordinates");
        }

        Debug.AddLine(wxString::Format("Moving (%.2lf, %.2lf) raw raDistance=%.2lf decDistance=%.2lf", vectorEndpoint.X, vectorEndpoint.Y, raDistance, decDistance));


        if (normalMove)
        {
            pFrame->GraphLog->AppendData(vectorEndpoint.X, vectorEndpoint.Y, raDistance, decDistance);

            // Feed the raw distances to the guide algorithms

            if (m_pRaGuideAlgorithm)
            {
                raDistance = m_pRaGuideAlgorithm->result(raDistance);
            }

            if (m_pDecGuideAlgorithm)
            {
                decDistance = m_pDecGuideAlgorithm->result(decDistance);
            }
        }


        // Figure out the guide directions based on the (possibly) updated distances
        GUIDE_DIRECTION raDirection = raDistance > 0 ? EAST : WEST;
        GUIDE_DIRECTION decDirection = decDistance > 0 ? SOUTH : NORTH;

        double actualRaAmount  = Move(raDirection,  fabs(raDistance/m_raRate), normalMove);

        if (actualRaAmount >= 0.5)
        {
            wxString msg = wxString::Format("%c dist=%.2f dur=%.0f", (raDirection==EAST)?'E':'W', raDistance, actualRaAmount);
            pFrame->SetStatusText(msg, 1, (int)actualRaAmount);
            Debug.AddLine(msg);
        }

        double actualDecAmount = Move(decDirection, fabs(decDistance/m_decRate), normalMove);

        if (actualDecAmount >= 0.5)
        {
            wxString msg = wxString::Format("%c dist=%.2f dur=%.0f" , (decDirection==SOUTH)?'S':'N', decDistance, actualDecAmount);
            pFrame->SetStatusText(msg, 1, (int)actualDecAmount);
            Debug.AddLine(msg);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Mount::SetCalibration(double raAngle, double decAngle, double raRate, double decRate)
{
    Debug.AddLine("Mount::SetCalibration -- raAngle=%.2lf decAngle=%.2lf raRate=%.2lf decRate=%.2lf", raAngle, decAngle, raRate, decRate);
    m_decAngle = decAngle;
    m_raAngle  = raAngle;
    m_decRate  = decRate;
    m_raRate   = raRate;
    m_calibrated = true;
}

bool Mount::FlipCalibration(void)
{
    bool bError = false;

    if (!IsCalibrated())
    {
        pFrame->SetStatusText(_T("No CAL"));
        bError = true;
    }
    else
    {
        double orig = m_raAngle;

        m_raAngle += PI;
        if (m_raAngle > PI)
        {
            m_raAngle -= 2*PI;
        }
        pFrame->SetStatusText(wxString::Format(_T("CAL: %.2f -> %.2f"), orig, m_raAngle),0);
    }

    return bError;
}

void Mount::ClearHistory(void)
{
    if (m_pRaGuideAlgorithm)
    {
        m_pRaGuideAlgorithm->reset();
    }

    if (m_pDecGuideAlgorithm)
    {
        m_pDecGuideAlgorithm->reset();
    }
}

#if 0
ConfigDialogPane *Mount::GetConfigDialogPane(wxWindow *pParent)
{
    return new MountConfigDialogPane(pParent, this);
}
#endif


Mount::MountConfigDialogPane::MountConfigDialogPane(wxWindow *pParent, wxString title, Mount *pMount)
    : ConfigDialogPane(title + " Settings", pParent)
{
    int width;
    m_pMount = pMount;

    m_pRecalibrate = new wxCheckBox(pParent ,wxID_ANY,_T("Force calibration"),wxPoint(-1,-1),wxSize(75,-1));

    DoAdd(m_pRecalibrate, _T("Check to clear any previous calibration and force PHD to recalibrate"));


    m_pEnableGuide = new wxCheckBox(pParent, wxID_ANY,_T("Enable Guide Output"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pEnableGuide, _T("Should mount guide commands be issued"));

    wxString raAlgorithms[] = {
        _T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
    };

    width = StringArrayWidth(raAlgorithms, WXSIZEOF(raAlgorithms));
    m_pRaGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(raAlgorithms), raAlgorithms);
    DoAdd(_T("RA Algorithm"), m_pRaGuideAlgorithm,
          _T("Which Guide Algorithm to use for Right Ascention"));

    if (!m_pMount->m_pRaGuideAlgorithm)
    {
        m_pRaGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pRaGuideAlgorithmConfigDialogPane  = m_pMount->m_pRaGuideAlgorithm->GetConfigDialogPane(pParent);
        DoAdd(m_pRaGuideAlgorithmConfigDialogPane);
    }

    wxString decAlgorithms[] = {
        _T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
    };

    width = StringArrayWidth(decAlgorithms, WXSIZEOF(decAlgorithms));
    m_pDecGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(decAlgorithms), decAlgorithms);
    DoAdd(_T("Declination Algorithm"), m_pDecGuideAlgorithm,
          _T("Which Guide Algorithm to use for Declination"));

    if (!pMount->m_pDecGuideAlgorithm)
    {
        m_pDecGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pDecGuideAlgorithmConfigDialogPane  = pMount->m_pDecGuideAlgorithm->GetConfigDialogPane(pParent);
        DoAdd(m_pDecGuideAlgorithmConfigDialogPane);
    }
}

Mount::MountConfigDialogPane::~MountConfigDialogPane(void)
{
}

void Mount::MountConfigDialogPane::LoadValues(void)
{
    m_pRecalibrate->SetValue(!m_pMount->IsCalibrated());

    m_pRaGuideAlgorithm->SetSelection(m_pMount->GetRaGuideAlgorithm());
    m_pDecGuideAlgorithm->SetSelection(m_pMount->GetDecGuideAlgorithm());
    m_pEnableGuide->SetValue(m_pMount->GetGuidingEnabled());

    if (m_pRaGuideAlgorithmConfigDialogPane)
    {
        m_pRaGuideAlgorithmConfigDialogPane->LoadValues();
    }

    if (m_pDecGuideAlgorithmConfigDialogPane)
    {
        m_pDecGuideAlgorithmConfigDialogPane->LoadValues();
    }
}

void Mount::MountConfigDialogPane::UnloadValues(void)
{
    if (m_pRecalibrate->GetValue())
    {
        m_pMount->ClearCalibration();
    }

    m_pMount->SetGuidingEnabled(m_pEnableGuide->GetValue());

    // note these two have to be before the SetXxxAlgorithm calls, because if we
    // changed the algorithm, the current one will get freed, and if we make
    // these two calls after that, bad things happen
    if (m_pRaGuideAlgorithmConfigDialogPane)
    {
        m_pRaGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    if (m_pDecGuideAlgorithmConfigDialogPane)
    {
        m_pDecGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    m_pMount->SetRaGuideAlgorithm(m_pRaGuideAlgorithm->GetSelection());
    m_pMount->SetDecGuideAlgorithm(m_pDecGuideAlgorithm->GetSelection());
}
