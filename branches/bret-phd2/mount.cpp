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

#ifdef BRET_TEST_TRANSLATE
/*
 * TestTranslation() is a routine to do some sanity checking on the transform routines, which
 * have been a source of headaches of late.
 *
 */
static void TestTranslation(void)
{
    static bool bTested = false;

    if (!bTested)
    {
        double angles[] = {
            -11*M_PI/12.0,
            -10*M_PI/12.0,
             -9*M_PI/12.0,
             -8*M_PI/12.0,
             -7*M_PI/12.0,
             -6*M_PI/12.0,
             -5*M_PI/12.0,
             -4*M_PI/12.0,
             -3*M_PI/12.0,
             -2*M_PI/12.0,
             -1*M_PI/12.0,
              0*M_PI/12.0,
              1*M_PI/12.0,
              2*M_PI/12.0,
              3*M_PI/12.0,
              4*M_PI/12.0,
              5*M_PI/12.0,
              6*M_PI/12.0,
              7*M_PI/12.0,
              8*M_PI/12.0,
              9*M_PI/12.0,
             10*M_PI/12.0,
             11*M_PI/12.0,
             12*M_PI/12.0,
        };

        for(int i=0;i<sizeof(angles)/sizeof(angles[0]);i++)
        {
            for(int inverted=0;inverted<2;inverted++)
            {
                double xAngle = angles[i];
                double yAngle;

                if (inverted)
                {
                    yAngle = xAngle-M_PI/2.0;
                }
                else
                {
                    yAngle = xAngle+M_PI/2.0;
                }

                // normalize the angles since real callers of setCalibration
                // will get the angle from atan2().

                xAngle = atan2(sin(xAngle), cos(xAngle));
                yAngle = atan2(sin(yAngle), cos(yAngle));

                SetCalibration(xAngle, yAngle, 1.0, 1.0);

                for(int j=0;j<sizeof(angles)/sizeof(angles[0]);j++)
                {
                    PHD_Point p0(cos(angles[j]), sin(angles[j]));
                    PHD_Point p1, p2;

                    if (TransformCameraCoordinatesToMountCoordinates(p0, p1))
                    {
                        assert(false);
                    }

                    if (TransformMountCoordinatesToCameraCoordinates(p1, p2))
                    {
                        assert(false);
                    }

                    assert(fabs(p0.X - p2.X) < 0.01);
                    assert(fabs(p0.Y - p2.Y) < 0.01);
                }
            }
        }

        bTested = true;
        ClearCalibration();
    }
}
#endif

Mount::Mount(void)
{
    m_connected = false;
    m_requestCount = 0;

    m_pYGuideAlgorithm = NULL;
    m_pXGuideAlgorithm = NULL;
    m_guidingEnabled = true;

    ClearCalibration();
#ifdef BRET_TEST_TRANSLATE
    TestTranslation();
#endif
}

Mount::~Mount()
{
    delete m_pXGuideAlgorithm;
    delete m_pYGuideAlgorithm;
}


bool Mount::HasNonGuiMove(void)
{
    return false;
}

bool Mount::SynchronousOnly(void)
{
    return false;
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

GUIDE_ALGORITHM Mount::GetXGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pXGuideAlgorithm);
}

void Mount::SetXGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    delete m_pXGuideAlgorithm;

    if (SetGuideAlgorithm(guideAlgorithm, &m_pXGuideAlgorithm))
    {
        SetGuideAlgorithm(defaultAlgorithm, &m_pXGuideAlgorithm);
    }
}

GUIDE_ALGORITHM Mount::GetYGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pYGuideAlgorithm);
}

void Mount::SetYGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    delete m_pYGuideAlgorithm;

    if (SetGuideAlgorithm(guideAlgorithm, &m_pYGuideAlgorithm))
    {
        SetGuideAlgorithm(defaultAlgorithm, &m_pYGuideAlgorithm);
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

double Mount::yAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_yAngle;
    }

    return dReturn;
}

double Mount::xAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_xAngle;
    }

    return dReturn;
}

double Mount::yRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_yRate;
    }

    return dReturn;
}

double Mount::xRate()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_xRate;
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

/*
 * The transform code has proven tricky to get right.  For future generations (and
 * for me the next time I try to work on it), I'm going to put some notes here.
 *
 * The goal of TransformCameraCoordinatesToMountCoordinates is to transform a camera
 * pixel * coordinate into an x and y, and TransformMountCoordinatesToCameraCoordinates
 * does * the reverse, converting a mount x and y into pixels coordinates.
 *
 * Note: If a mount's x and y axis are not perfectly perpendicular, the reverse transform
 * will not be able to accuratey reverse the forward transform. The amount of inaccuracy
 * depends upon the perpendicular error.
 *
 * Instead of using cos() to get the x coordinate and sin() to get the y,
 * we keep 2 angles, one for X and one for Y, and use cosine on both of angles
 * to get X and Y.
 *
 * This has two benefits -- it transparently deals with the calibration for a mount
 * does not showing the x and y as separated by 90 degrees. It also avoids issues caused
 * by mirrors in the light path -- we can simply ignore the relative alignment and
 * everything just works.
 *
 * I have had an converstation with Craig about this, and he confirmed that reasoning.
 *
 * Even though I think I understand it now, this quote that I put in when I was sure I
 * didn't still seems relevant, so I'm leaving it.
 *
 * In the words of Stevie Wonder, noted Computer Scientist and  singer/songwriter
 * of some repute:
 *
 * "When you believe in things that you don't understand
 * Then you suffer
 * Superstition ain't the way"
 *
 */

bool Mount::TransformCameraCoordinatesToMountCoordinates(const PHD_Point& cameraVectorEndpoint,
                                                         PHD_Point& mountVectorEndpoint)
{
    bool bError = false;

    try
    {
        if (!cameraVectorEndpoint.IsValid())
        {
            throw ERROR_INFO("invalid cameraVectorEndPoint");
        }

        double hyp   = cameraVectorEndpoint.Distance();
        double cameraTheta = cameraVectorEndpoint.Angle();

        // Convert theta and hyp into X and Y

        mountVectorEndpoint.SetXY(
            cos(cameraTheta + m_xAngle)  * hyp,
            cos(cameraTheta + m_yAngle) * (m_negateForward?-1.0:1.0) * hyp
            );

        Debug.AddLine("CameraToMount -- m_xAngle=%.2f m_yAngle=%.2f m_neg=%d", 
                m_xAngle, m_yAngle, m_negateForward);
        Debug.AddLine("CameraToMount -- cameraX=%.2f cameraY=%.2f hyp=%.2f cameraTheta=%.2f mountX=%.2lf mountY=%.2lf",
                cameraVectorEndpoint.X, cameraVectorEndpoint.Y, hyp, cameraTheta, mountVectorEndpoint.X, mountVectorEndpoint.Y);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        mountVectorEndpoint.Invalidate();
    }

    return bError;
}

bool Mount::TransformMountCoordinatesToCameraCoordinates(const PHD_Point& mountVectorEndpoint,
                                                        PHD_Point& cameraVectorEndpoint)
{
    bool bError = false;

    try
    {
        if (!mountVectorEndpoint.IsValid())
        {
            throw ERROR_INFO("invalid mountVectorEndPoint");
        }

        double hyp = mountVectorEndpoint.Distance();
        double mountTheta = mountVectorEndpoint.Angle();


        cameraVectorEndpoint.SetXY(
                cos(mountTheta - m_xAngle)  * hyp,
                cos(mountTheta - m_yAngle) * (!m_negateForward?-1.0:1.0) * hyp
                );

        Debug.AddLine("MountToCamera -- m_xAngle=%.2f m_yAngle=%.2f m_neg=%d", 
                m_xAngle, m_yAngle, !m_negateForward);
        Debug.AddLine("MountToCamera -- mountX=%.2f mountY=%.2f hyp=%.2f mountTheta=%.2f cameraX=%.2f, cameraY=%.2f",
                mountVectorEndpoint.X, mountVectorEndpoint.Y, hyp, mountTheta, cameraVectorEndpoint.X, cameraVectorEndpoint.Y);

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        cameraVectorEndpoint.Invalidate();
    }

    return bError;
}

bool Mount::Move(const PHD_Point& cameraVectorEndpoint, bool normalMove)
{
    bool bError = false;

    try
    {
        PHD_Point mountVectorEndpoint;

        if (TransformCameraCoordinatesToMountCoordinates(cameraVectorEndpoint, mountVectorEndpoint))
        {
            throw ERROR_INFO("Unable to transform camera coordinates");
        }

        double xDistance = mountVectorEndpoint.X;
        double yDistance = mountVectorEndpoint.Y;

        Debug.AddLine(wxString::Format("Moving (%.2lf, %.2lf) raw xDistance=%.2lf yDistance=%.2lf", cameraVectorEndpoint.X, cameraVectorEndpoint.Y, xDistance, yDistance));

        if (normalMove)
        {
            pFrame->GraphLog->AppendData(cameraVectorEndpoint.X, cameraVectorEndpoint.Y, xDistance, yDistance);

            // Feed the raw distances to the guide algorithms

            if (m_pXGuideAlgorithm)
            {
                xDistance = m_pXGuideAlgorithm->result(xDistance);
            }

            if (m_pYGuideAlgorithm)
            {
                yDistance = m_pYGuideAlgorithm->result(yDistance);
            }
        }

        // Figure out the guide directions based on the (possibly) updated distances
        GUIDE_DIRECTION xDirection = xDistance > 0 ? WEST : EAST;
        GUIDE_DIRECTION yDirection = yDistance > 0 ? SOUTH : NORTH;

        double actualXAmount  = Move(xDirection,  fabs(xDistance/m_xRate), normalMove);

        if (actualXAmount < 0.0)
        {
            throw ERROR_INFO("ActualXAmount < 0");
        }

        if (actualXAmount >= 0.5)
        {
            wxString msg = wxString::Format("%c dist=%.2f dur=%.0f", (xDirection==EAST)?'E':'W', xDistance, actualXAmount);
            pFrame->SetStatusText(msg, 1, (int)actualXAmount);
            Debug.AddLine(msg);
        }

        double actualYAmount = Move(yDirection, fabs(yDistance/m_yRate), normalMove);

        if (actualYAmount < 0.0)
        {
            throw ERROR_INFO("ActualYAmount < 0");
        }

        if (actualYAmount >= 0.5)
        {
            wxString msg = wxString::Format("%c dist=%.2f dur=%.0f" , (yDirection==SOUTH)?'S':'N', yDistance, actualYAmount);
            pFrame->SetStatusText(msg, 1, (int)actualYAmount);
            Debug.AddLine(msg);
        }

        GuideLog.GuideStep(this, cameraVectorEndpoint, actualXAmount, xDistance, actualYAmount, yDistance, 0);  // errorCode??
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Mount::SetCalibration(double xAngle, double yAngle, double xRate, double yRate)
{
    Debug.AddLine("Mount::SetCalibration -- xAngle=%.2lf yAngle=%.2lf xRate=%.2lf yRate=%.2lf", xAngle, yAngle, xRate, yRate);

    m_xAngle = xAngle;
    m_yAngle = yAngle;

    // see if the angles have wrapped. To set sign factor, we need to make sure
    // that the two angles are close "enough".
    //
    // For example, if we have +135 degrees and -135 degrees, these angles are
    // 90 degrees apart but to a simple test, they show up as 270 degrees apart.
    // If the angles show up as more than 180 degrees apart, we assume this has
    // happended

    if (fabs(xAngle - yAngle) > M_PI)
    {
        if (xAngle < 0)
        {
            assert(yAngle >= 0);
            xAngle += 2*M_PI;
        }
        else
        {
            assert(yAngle < 0);
            yAngle += 2*M_PI;
        }
    }

    assert(fabs(xAngle - yAngle) <= M_PI);

    m_negateForward = (xAngle < yAngle);

    m_yRate  = yRate;
    m_xRate  = xRate;

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
        double orig = m_xAngle;

        m_xAngle += PI;
        if (m_xAngle > PI)
        {
            m_xAngle -= 2*PI;
        }
        pFrame->SetStatusText(wxString::Format(_T("CAL: %.2f -> %.2f"), orig, m_xAngle),0);
    }

    return bError;
}

void Mount::ClearHistory(void)
{
    if (m_pXGuideAlgorithm)
    {
        m_pXGuideAlgorithm->reset();
    }

    if (m_pYGuideAlgorithm)
    {
        m_pYGuideAlgorithm->reset();
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

    wxString xAlgorithms[] = {
        _T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
    };

    width = StringArrayWidth(xAlgorithms, WXSIZEOF(xAlgorithms));
    m_pXGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(xAlgorithms), xAlgorithms);
    DoAdd(_T("RA Algorithm"), m_pXGuideAlgorithm,
          _T("Which Guide Algorithm to use for Right Ascention"));

    if (!m_pMount->m_pXGuideAlgorithm)
    {
        m_pXGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pXGuideAlgorithmConfigDialogPane  = m_pMount->m_pXGuideAlgorithm->GetConfigDialogPane(pParent);
        DoAdd(m_pXGuideAlgorithmConfigDialogPane);
    }

    wxString yAlgorithms[] = {
        _T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
    };

    width = StringArrayWidth(yAlgorithms, WXSIZEOF(yAlgorithms));
    m_pYGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(yAlgorithms), yAlgorithms);
    DoAdd(_T("Declination Algorithm"), m_pYGuideAlgorithm,
          _T("Which Guide Algorithm to use for Declination"));

    if (!pMount->m_pYGuideAlgorithm)
    {
        m_pYGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pYGuideAlgorithmConfigDialogPane  = pMount->m_pYGuideAlgorithm->GetConfigDialogPane(pParent);
        DoAdd(m_pYGuideAlgorithmConfigDialogPane);
    }
}

Mount::MountConfigDialogPane::~MountConfigDialogPane(void)
{
}

void Mount::MountConfigDialogPane::LoadValues(void)
{
    m_pRecalibrate->SetValue(!m_pMount->IsCalibrated());

    m_pXGuideAlgorithm->SetSelection(m_pMount->GetXGuideAlgorithm());
    m_pYGuideAlgorithm->SetSelection(m_pMount->GetYGuideAlgorithm());
    m_pEnableGuide->SetValue(m_pMount->GetGuidingEnabled());

    if (m_pXGuideAlgorithmConfigDialogPane)
    {
        m_pXGuideAlgorithmConfigDialogPane->LoadValues();
    }

    if (m_pYGuideAlgorithmConfigDialogPane)
    {
        m_pYGuideAlgorithmConfigDialogPane->LoadValues();
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
    if (m_pXGuideAlgorithmConfigDialogPane)
    {
        m_pXGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    if (m_pYGuideAlgorithmConfigDialogPane)
    {
        m_pYGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    m_pMount->SetXGuideAlgorithm(m_pXGuideAlgorithm->GetSelection());
    m_pMount->SetYGuideAlgorithm(m_pYGuideAlgorithm->GetSelection());
}
