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

#ifdef TEST_TRANSFORMS
/*
 * TestTransforms() is a routine to do some sanity checking on the transform routines, which
 * have been a source of a huge number of headaches involving getting the reverse transforms
 * to work
 *
 */
void Mount::TestTransforms(void)
{
    static bool bTested = false;

    if (!bTested)
    {
        for(int inverted=0;inverted<2;inverted++)
        {
            // we test every 15 degrees, starting at -195 degrees and
            // ending at 375 degrees

            for(int i=-13;i<14;i++)
            {
                double xAngle = ((double)i) * M_PI/12.0;
                double yAngle;

                yAngle = xAngle+M_PI/2.0;

                if (inverted)
                {
                    yAngle -= M_PI;
                }

                // normalize the angles since real callers of setCalibration
                // will get the angle from atan2().

                xAngle = atan2(sin(xAngle), cos(xAngle));
                yAngle = atan2(sin(yAngle), cos(yAngle));

                Debug.AddLine("xidx=%.2lf, yIdx=%.2lf", xAngle/M_PI*180.0/15, yAngle/M_PI*180.0/15);

                SetCalibration(xAngle, yAngle, 1.0, 1.0);

                for(int j=-13;j<14;j++)
                {
                    double cameraAngle = ((double)i) * M_PI/12.0;
                    cameraAngle = atan2(sin(cameraAngle), cos(cameraAngle));

                    PHD_Point p0(cos(cameraAngle), sin(cameraAngle));
                    assert(fabs((p0.X*p0.X + p0.Y*p0.Y) - 1.00) < 0.01);

                    double p0Angle = p0.Angle();
                    assert(fabs(cameraAngle - p0Angle) < 0.01);

                    PHD_Point p1, p2;

                    if (TransformCameraCoordinatesToMountCoordinates(p0, p1))
                    {
                        assert(false);
                    }

                    assert(fabs((p1.X*p1.X + p1.Y*p1.Y) - 1.00) < 0.01);

                    double adjustedCameraAngle = cameraAngle - xAngle;

                    double cosAdjustedCameraAngle = cos(adjustedCameraAngle);

                    // check that the X value is correct
                    assert(fabs(cosAdjustedCameraAngle - p1.X) < 0.01);

                    double sinAdjustedCameraAngle = sin(adjustedCameraAngle);

                    if (inverted)
                    {
                        sinAdjustedCameraAngle *= -1;
                    }
                    // and check that the Y value is correct;
                    assert(fabs(sinAdjustedCameraAngle - p1.Y) < 0.01);

                    if (TransformMountCoordinatesToCameraCoordinates(p1, p2))
                    {
                        assert(false);
                    }

                    double p2Angle = p2.Angle();
                    //assert(fabs(p0Angle - p2Angle) < 0.01);
                    assert(fabs((p2.X*p2.X + p2.Y*p2.Y) - 1.00) < 0.01);
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
#ifdef TEST_TRANSFORMS
    TestTransforms();
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

bool Mount::CreateGuideAlgorithm(int guideAlgorithm, Mount *mount, GuideAxis axis, GuideAlgorithm** ppAlgorithm)
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
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmIdentity(mount, axis);
            break;
        case GUIDE_ALGORITHM_HYSTERESIS:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmHysteresis(mount, axis);
            break;
        case GUIDE_ALGORITHM_LOWPASS:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass(mount, axis);
            break;
        case GUIDE_ALGORITHM_LOWPASS2:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass2(mount, axis);
            break;
        case GUIDE_ALGORITHM_RESIST_SWITCH:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmResistSwitch(mount, axis);
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

    if (CreateGuideAlgorithm(guideAlgorithm, this, GUIDE_X, &m_pXGuideAlgorithm))
    {
        CreateGuideAlgorithm(defaultAlgorithm, this, GUIDE_X, &m_pXGuideAlgorithm);
        guideAlgorithm = defaultAlgorithm;
    }

    pConfig->SetInt("/" + GetMountClassName() + "/XGuideAlgorithm", guideAlgorithm);
}

GUIDE_ALGORITHM Mount::GetYGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pYGuideAlgorithm);
}

void Mount::SetYGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    delete m_pYGuideAlgorithm;

    if (CreateGuideAlgorithm(guideAlgorithm, this, GUIDE_Y, &m_pYGuideAlgorithm))
    {
        CreateGuideAlgorithm(defaultAlgorithm, this, GUIDE_Y, &m_pYGuideAlgorithm);
        guideAlgorithm = defaultAlgorithm;
    }

    pConfig->SetInt("/" + GetMountClassName() + "/YGuideAlgorithm", guideAlgorithm);
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
    if (pFrame) pFrame->UpdateCalibrationStatus();
}

double Mount::yAngle()
{
    double dReturn = 0;

    if (IsCalibrated())
    {
        dReturn = m_xAngle - m_yAngleError + M_PI/2;
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
    if (pFrame) pFrame->UpdateCalibrationStatus();

    return false;
}

bool Mount::Disconnect(void)
{
    m_connected = false;
    if (pFrame) pFrame->UpdateCalibrationStatus();

    return false;
}

/*
 * The transform code has proven really tricky to get right.  For future generations
 * (and for me the next time I try to work on it), I'm going to put some notes here.
 *
 * The goal of TransformCameraCoordinatesToMountCoordinates is to transform a camera
 * pixel coordinate into an x and y, and TransformMountCoordinatesToCameraCoordinates
 * does the reverse, converting a mount x and y into pixels coordinates.
 *
 * Note: If a mount's x and y axis are not perfectly perpendicular, the reverse transform
 * will not be able to accuratey reverse the forward transform. The amount of inaccuracy
 * depends upon the perpendicular error.
 *
 * The mount is calibrated but moving 1 axis, then the other, watching where the mount
 * started and stopped.  Looking at the coordinates of the start and stop point, we
 * can compute the angle and the speed.
 *
 * The original PHD code used cos() against both the angles, but that code had issues
 * with sign reversal for some of the reverse transforms.  I spent some time looking
 * at that code (OK, a lot of time) and I never managed to get it to work, so I
 * rewrote it.
 *
 * After calibration, we use the x axis angle, and compute the y axis error by
 * subtracting 90 degrees.  If the mount was perfectly perpendicular, the error
 * will either be 0 or 180, depending on whether the axis motion was reversed
 * during calibration.
 *
 * I put this quote in when I was trying to fix the cos/cos code.  Hopefully
 * since  completely rewrote that code, I underand the new code. But I'm going
 * to leave it hear as it still seems relevant, given the amount of trouble
 * I had getting this code to work.
 *
 * In the words of Stevie Wonder, noted Computer Scientist and singer/songwriter
 * of some repute:
 *
 * "When you believe in things that you don't understand
 * Then you suffer
 * Superstition ain't the way"
 *
 * As part of trying to get the transforms to work, I wrote a routine called TestTransforms()
 * which does a bunch of forward/reverse trasnform pairs, checking the results as it goes.
 *
 * If you ever have change the transform functions, it would be wise to #define TEST_TRANSFORMS
 * to make sure that you (at least) didn't break anything that TestTransforms() checks for.
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

        double xAngle = cameraTheta - m_xAngle;
        double yAngle = cameraTheta - (m_xAngle + m_yAngleError);

        // Convert theta and hyp into X and Y

        mountVectorEndpoint.SetXY(
            cos(xAngle) * hyp,
            sin(yAngle) * hyp
            );

        Debug.AddLine("CameraToMount -- cameraTheta (%.2f) - m_xAngle (%.2f) = xAngle (%.2f = %.2f)",
                cameraTheta, m_xAngle, xAngle, atan2(sin(xAngle), cos(xAngle)));
        Debug.AddLine("CameraToMount -- cameraTheta (%.2f) - (m_xAngle (%.2f) + m_yAngleError (%.2f)) = yAngle (%.2f = %.2f)",
                cameraTheta, m_xAngle, m_yAngleError, yAngle, atan2(sin(yAngle), cos(yAngle)));
        Debug.AddLine("CameraToMount -- cameraX=%.2f cameraY=%.2f hyp=%.2f cameraTheta=%.2f mountX=%.2lf mountY=%.2lf, mountTheta=%.2lf",
                cameraVectorEndpoint.X, cameraVectorEndpoint.Y, hyp, cameraTheta, mountVectorEndpoint.X, mountVectorEndpoint.Y,
                mountVectorEndpoint.Angle());
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

        if (fabs(m_yAngleError) > M_PI/2)
        {
            mountTheta = -mountTheta;
        }

        double xAngle = mountTheta + m_xAngle;

        cameraVectorEndpoint.SetXY(
                cos(xAngle) * hyp,
                sin(xAngle) * hyp
                );

        Debug.AddLine("MountToCamera -- mountTheta (%.2f) + m_xAngle (%.2f) = xAngle (%.2f = %.2f)",
                mountTheta, m_xAngle, xAngle, atan2(sin(xAngle), cos(xAngle)));
        Debug.AddLine("MountToCamera -- mountX=%.2f mountY=%.2f hyp=%.2f mountTheta=%.2f cameraX=%.2f, cameraY=%.2f cameraTheta=%.2f",
                mountVectorEndpoint.X, mountVectorEndpoint.Y, hyp, mountTheta, cameraVectorEndpoint.X, cameraVectorEndpoint.Y,
                cameraVectorEndpoint.Angle());

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
            pFrame->pGraphLog->AppendData(cameraVectorEndpoint.X, cameraVectorEndpoint.Y,
                                         xDistance, yDistance);
            pFrame->pTarget->AppendData(xDistance, yDistance);

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
        GUIDE_DIRECTION xDirection = xDistance > 0 ? LEFT : RIGHT;
        GUIDE_DIRECTION yDirection = yDistance > 0 ? DOWN : UP;

        double actualXAmount  = Move(xDirection,  fabs(xDistance/m_xRate), normalMove);

        if (actualXAmount < 0.0)
        {
            throw ERROR_INFO("ActualXAmount < 0");
        }

        if (actualXAmount >= 0.5)
        {
            wxString msg = wxString::Format(_("%s dist=%.2f dur=%.0f"), (xDirection==EAST)?_("E"):_("W"), fabs(xDistance), actualXAmount);
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
            wxString msg = wxString::Format(_("%s dist=%.2f dur=%.0f") , (yDirection==SOUTH)?_("S"):_("N"), fabs(yDistance), actualYAmount);
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

void Mount::SetCalibration(double xAngle, double yAngle, double xRate, double yRate, double declination)
{

    Debug.AddLine("Mount::SetCalibration -- xAngle=%.2lf yAngle=%.2lf xRate=%.4lf yRate=%.4lf dec=%.lf", xAngle, yAngle, xRate, yRate, declination);

    // we do the rates first, since they just get stored
    m_yRate  = yRate;
    m_calXRate = xRate;
    m_calDeclination = declination;
    m_xRate  = m_calXRate;

    // the angles are more difficult because we have to turn yAngle into a yError.
    m_xAngle = xAngle;
    m_yAngleError = (xAngle-yAngle) + M_PI/2;

    m_yAngleError = atan2(sin(m_yAngleError), cos(m_yAngleError));

    Debug.AddLine("Mount::SetCalibration -- sets m_xAngle=%.2lf m_yAngleError=%.2lf",
            m_xAngle, m_yAngleError);

    m_calibrated = true;
    if (pFrame) pFrame->UpdateCalibrationStatus();

    // store calibration data
    wxString prefix = "/" + GetMountClassName() + "/calibration/";
    pConfig->SetString(prefix + "timestamp", wxDateTime::Now().Format());
    pConfig->SetDouble(prefix + "xAngle", xAngle);
    pConfig->SetDouble(prefix + "yAngle", yAngle);
    pConfig->SetDouble(prefix + "xRate", xRate);
    pConfig->SetDouble(prefix + "yRate", yRate);
    pConfig->SetDouble(prefix + "declination", declination);
}

/*
 * Adjust the xRate to reflect the declination.  The default implemenation
 * of GetDeclination returns 0, which makes this code into a no-op
 */

void Mount::AdjustForDeclination(void)
{
    // avoid division by zero and gross errors.  If the user didn't calibrate
    // somewhere near the celestial equater, we don't do this
    if (fabs(m_calDeclination) > (M_PI/2.0)*(2.0/3.0))
    {
        Debug.AddLine("skipping declination adjustment: too far from equator");
    }
    else
    {
        double newDeclination = GetDeclination();
        m_xRate = (m_calXRate/cos(m_calDeclination))*cos(newDeclination);
        Debug.AddLine("adjusted dec rate %.2lf -> %.2lf for dec %.2lf -> dec %.2lf",
                m_calXRate, m_xRate, m_calDeclination, newDeclination);
    }
}

double Mount::GetDeclination(void)
{
    return m_calibrated ? m_calDeclination : 0.0;
}

bool Mount::FlipCalibration(void)
{
    bool bError = false;

    try
    {
        if (!IsCalibrated())
        {
            throw ERROR_INFO("cannot flip if not calibrated");
        }

        double origX = xAngle();
        double origY = yAngle();

        Debug.AddLine("FlipCalibration before: x=%.2f, y=%.2f", origX, origY);

        double newX = origX + M_PI;
        double newY = origY;

        Debug.AddLine("FlipCalibration pre-normalize: x=%.2f, y=%.2f", newX, newY);

        // normlize
        newX = atan2(sin(newX), cos(newX));
        newY = atan2(sin(newY), cos(newY));

        Debug.AddLine("FlipCalibration after: x=%.2f, y=%.2f", newX, newY);

        SetCalibration(newX, newY, xRate(), yRate(), m_calDeclination);

        pFrame->SetStatusText(wxString::Format(_("CAL: %.2f -> %.2f"), origX, newX), 0);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
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

wxString Mount::GetSettingsSummary() {
    // return a loggable summary of current mount settings
    wxString algorithms[] = {
        _T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
    };
    return wxString::Format("Mount %s, %s connected, guiding %s, %s\n",
        m_Name,
        IsConnected() ? "" : "not",
        m_guidingEnabled ? "enabled" : "disabled",
        IsCalibrated() ? wxString::Format("xAngle = %.3f, xRate = %.3f, yAngle = %.3f, yRate = %.3f",
                yAngle(), yRate(), xAngle(), xRate()) : "not calibrated"
    ) + wxString::Format("RA guide algorithm %s, %s",
        algorithms[GetXGuideAlgorithm()],
        m_pXGuideAlgorithm->GetSettingsSummary()
    ) + wxString::Format("DEC guide algorithm %s, %s",
        algorithms[GetYGuideAlgorithm()],
        m_pYGuideAlgorithm->GetSettingsSummary()
    );
}

static ConfigDialogPane *GetGuideAlgoDialogPane(GuideAlgorithm *algo, wxWindow *parent)
{
    // we need to force the guide alogorithm config pane to be large enough for
    // any of the guide algorithms
    ConfigDialogPane *pane = algo->GetConfigDialogPane(parent);
    pane->SetMinSize(-1, 110);
    return pane;
}

Mount::MountConfigDialogPane::MountConfigDialogPane(wxWindow *pParent, wxString title, Mount *pMount)
    : ConfigDialogPane(wxString::Format(_("%s Settings"),title), pParent)
{
    int width;
    m_pMount = pMount;

    m_pRecalibrate = new wxCheckBox(pParent ,wxID_ANY,_("Force calibration"),wxPoint(-1,-1),wxSize(75,-1));

    DoAdd(m_pRecalibrate, _("Check to clear any previous calibration and force PHD to recalibrate"));

    m_pEnableGuide = new wxCheckBox(pParent, wxID_ANY,_("Enable Guide Output"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pEnableGuide, _("Should mount guide commands be issued"));

    wxString xAlgorithms[] = {
        _("Identity"),_("Hysteresis"),_("Lowpass"),_("Lowpass2"), _("Resist Switch")
    };

    width = StringArrayWidth(xAlgorithms, WXSIZEOF(xAlgorithms));
    m_pXGuideAlgorithmChoice = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(xAlgorithms), xAlgorithms);
    DoAdd(_("RA Algorithm"), m_pXGuideAlgorithmChoice,
          _("Which Guide Algorithm to use for Right Ascension"));

    m_pParent->Connect(m_pXGuideAlgorithmChoice->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
        wxCommandEventHandler(Mount::MountConfigDialogPane::OnXAlgorithmSelected), 0, this);

    if (!m_pMount->m_pXGuideAlgorithm)
    {
        m_pXGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pXGuideAlgorithmConfigDialogPane = GetGuideAlgoDialogPane(m_pMount->m_pXGuideAlgorithm, pParent);
        DoAdd(m_pXGuideAlgorithmConfigDialogPane);
    }

    wxString yAlgorithms[] = {
        _("Identity"),_("Hysteresis"),_("Lowpass"),_("Lowpass2"), _("Resist Switch")
    };

    width = StringArrayWidth(yAlgorithms, WXSIZEOF(yAlgorithms));
    m_pYGuideAlgorithmChoice = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
                                    wxSize(width+35, -1), WXSIZEOF(yAlgorithms), yAlgorithms);
    DoAdd(_("Declination Algorithm"), m_pYGuideAlgorithmChoice,
          _("Which Guide Algorithm to use for Declination"));

    m_pParent->Connect(m_pYGuideAlgorithmChoice->GetId(), wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(Mount::MountConfigDialogPane::OnYAlgorithmSelected), 0, this);

    if (!pMount->m_pYGuideAlgorithm)
    {
        m_pYGuideAlgorithmConfigDialogPane  = NULL;
    }
    else
    {
        m_pYGuideAlgorithmConfigDialogPane  = GetGuideAlgoDialogPane(pMount->m_pYGuideAlgorithm, pParent);
        DoAdd(m_pYGuideAlgorithmConfigDialogPane);
    }
}

Mount::MountConfigDialogPane::~MountConfigDialogPane(void)
{
}

void Mount::MountConfigDialogPane::OnXAlgorithmSelected(wxCommandEvent& evt)
{
    ConfigDialogPane *oldpane = m_pXGuideAlgorithmConfigDialogPane;
    oldpane->Clear(true);
    m_pMount->SetXGuideAlgorithm(m_pXGuideAlgorithmChoice->GetSelection());
    ConfigDialogPane *newpane = GetGuideAlgoDialogPane(m_pMount->m_pXGuideAlgorithm, m_pParent);
    Replace(oldpane, newpane);
    m_pXGuideAlgorithmConfigDialogPane = newpane;
    m_pXGuideAlgorithmConfigDialogPane->LoadValues();
    m_pParent->Layout();
    m_pParent->Update();
    m_pParent->Refresh();
}

void Mount::MountConfigDialogPane::OnYAlgorithmSelected(wxCommandEvent& evt)
{
    ConfigDialogPane *oldpane = m_pYGuideAlgorithmConfigDialogPane;
    oldpane->Clear(true);
    m_pMount->SetYGuideAlgorithm(m_pYGuideAlgorithmChoice->GetSelection());
    ConfigDialogPane *newpane = GetGuideAlgoDialogPane(m_pMount->m_pYGuideAlgorithm, m_pParent);
    Replace(oldpane, newpane);
    m_pYGuideAlgorithmConfigDialogPane = newpane;
    m_pYGuideAlgorithmConfigDialogPane->LoadValues();
    m_pParent->Layout();
    m_pParent->Update();
    m_pParent->Refresh();
}

void Mount::MountConfigDialogPane::LoadValues(void)
{
    m_pRecalibrate->SetValue(!m_pMount->IsCalibrated());

    m_pXGuideAlgorithmChoice->SetSelection(m_pMount->GetXGuideAlgorithm());
    m_pYGuideAlgorithmChoice->SetSelection(m_pMount->GetYGuideAlgorithm());
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

    m_pMount->SetXGuideAlgorithm(m_pXGuideAlgorithmChoice->GetSelection());
    m_pMount->SetYGuideAlgorithm(m_pYGuideAlgorithmChoice->GetSelection());
}

GraphControlPane *Mount::GetXGuideAlgorithmControlPane(wxWindow *pParent)
{
    return m_pXGuideAlgorithm->GetGraphControlPane(pParent, _("RA:"));
}

GraphControlPane *Mount::GetYGuideAlgorithmControlPane(wxWindow *pParent)
{
    return m_pYGuideAlgorithm->GetGraphControlPane(pParent, _("DEC:"));
}
