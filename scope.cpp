/*
 *  scope.cpp
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
#include "phd.h"

#include "image_math.h"
#include "wx/textfile.h"
#include "socket_server.h"
#include "calstep_dialog.h"
#include "calsanity_dialog.h"

#define CALIBRATION_RATE_UNCALIBRATED 123e4

static const int DefaultCalibrationDuration = 750;
static const int DefaultMaxDecDuration = 2500;
static const int DefaultMaxRaDuration = 2500;

static const DEC_GUIDE_MODE DefaultDecGuideMode = DEC_AUTO;
static const GUIDE_ALGORITHM DefaultRaGuideAlgorithm = GUIDE_ALGORITHM_HYSTERESIS;
static const GUIDE_ALGORITHM DefaultDecGuideAlgorithm = GUIDE_ALGORITHM_RESIST_SWITCH;

static const double DEC_BACKLASH_DISTANCE = 3.0;
static const int MAX_CALIBRATION_STEPS = 60;
static const double MAX_CALIBRATION_DISTANCE = 25.0;
static const int CAL_ALERT_MINSTEPS = 4;
static const double CAL_ALERT_ORTHOGONALITY_TOLERANCE = 10.;                // Degrees
static const double CAL_ALERT_DECRATE_DIFFERENCE = 0.20;                    // Ratio tolerance
static const double CAL_ALERT_AXISRATES_TOLERANCE = 0.20;                   // Ratio tolerance
static const bool SANITY_CHECKING_ACTIVE = false;                           // Disable calibration sanity checks for now

static int LIMIT_REACHED_WARN_COUNT = 5;
static int MAX_NUDGES = 3;
static double NUDGE_TOLERANCE = 2.0;

Scope::Scope(void)
    : m_raLimitReachedDirection(NONE),
      m_raLimitReachedCount(0),
      m_decLimitReachedDirection(NONE),
      m_decLimitReachedCount(0)
{
    m_calibrationSteps = 0;
    m_graphControlPane = NULL;

    wxString prefix = "/" + GetMountClassName();
    int calibrationDuration = pConfig->Profile.GetInt(prefix + "/CalibrationDuration", DefaultCalibrationDuration);
    SetCalibrationDuration(calibrationDuration);

    int maxRaDuration  = pConfig->Profile.GetInt(prefix + "/MaxRaDuration", DefaultMaxRaDuration);
    SetMaxRaDuration(maxRaDuration);

    int maxDecDuration = pConfig->Profile.GetInt(prefix + "/MaxDecDuration", DefaultMaxDecDuration);
    SetMaxDecDuration(maxDecDuration);

    int decGuideMode = pConfig->Profile.GetInt(prefix + "/DecGuideMode", DefaultDecGuideMode);
    SetDecGuideMode(decGuideMode);

    int raGuideAlgorithm = pConfig->Profile.GetInt(prefix + "/XGuideAlgorithm", DefaultRaGuideAlgorithm);
    SetXGuideAlgorithm(raGuideAlgorithm);

    int decGuideAlgorithm = pConfig->Profile.GetInt(prefix + "/YGuideAlgorithm", DefaultDecGuideAlgorithm);
    SetYGuideAlgorithm(decGuideAlgorithm);

    bool val = pConfig->Profile.GetBoolean(prefix + "/CalFlipRequiresDecFlip", false);
    SetCalibrationFlipRequiresDecFlip(val);

    val = pConfig->Profile.GetBoolean(prefix + "/AssumeOrthogonal", false);
    SetAssumeOrthogonal(val);
    // GetLastCalibrationParams(&m_prevCalibrationParams);
}

Scope::~Scope(void)
{
    if (m_graphControlPane)
    {
        m_graphControlPane->m_pScope = NULL;
    }
}

int Scope::GetCalibrationDuration(void)
{
    return m_calibrationDuration;
}

bool Scope::SetCalibrationDuration(int calibrationDuration)
{
    bool bError = false;

    try
    {
        if (calibrationDuration <= 0)
        {
            throw ERROR_INFO("invalid calibrationDuration");
        }

        m_calibrationDuration = calibrationDuration;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_calibrationDuration = DefaultCalibrationDuration;
    }

    pConfig->Profile.SetInt("/scope/CalibrationDuration", m_calibrationDuration);

    return bError;
}

int Scope::GetMaxDecDuration(void)
{
    return m_maxDecDuration;
}

bool Scope::SetMaxDecDuration(int maxDecDuration)
{
    bool bError = false;

    try
    {
        if (maxDecDuration < 0)
        {
            throw ERROR_INFO("maxDecDuration < 0");
        }

        m_maxDecDuration = maxDecDuration;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_maxDecDuration = DefaultMaxDecDuration;
    }

    pConfig->Profile.SetInt("/scope/MaxDecDuration", m_maxDecDuration);

    return bError;
}

int Scope::GetMaxRaDuration(void)
{
    return m_maxRaDuration;
}

bool Scope::SetMaxRaDuration(double maxRaDuration)
{
    bool bError = false;

    try
    {
        if (maxRaDuration < 0)
        {
            throw ERROR_INFO("maxRaDuration < 0");
        }

        m_maxRaDuration =  maxRaDuration;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_maxRaDuration = DefaultMaxRaDuration;
    }

    pConfig->Profile.SetInt("/scope/MaxRaDuration", m_maxRaDuration);

    return bError;
}

DEC_GUIDE_MODE Scope::GetDecGuideMode(void)
{
    return m_decGuideMode;
}

bool Scope::SetDecGuideMode(int decGuideMode)
{
    bool bError = false;

    try
    {
        switch (decGuideMode)
        {
            case DEC_NONE:
            case DEC_AUTO:
            case DEC_NORTH:
            case DEC_SOUTH:
                break;
            default:
                throw ERROR_INFO("invalid decGuideMode");
                break;
        }

        m_decGuideMode = (DEC_GUIDE_MODE)decGuideMode;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_decGuideMode = (DEC_GUIDE_MODE)DefaultDecGuideMode;
    }

    pConfig->Profile.SetInt("/scope/DecGuideMode", m_decGuideMode);
    if (pFrame)
        pFrame->UpdateCalibrationStatus();

    return bError;
}

static int CompareNoCase(const wxString& first, const wxString& second)
{
    return first.CmpNoCase(second);
}

wxArrayString Scope::List(void)
{
    wxArrayString ScopeList;

    ScopeList.Add(_("None"));
#ifdef GUIDE_ASCOM
    wxArrayString ascomScopes = ScopeASCOM::EnumAscomScopes();
    for (unsigned int i = 0; i < ascomScopes.Count(); i++)
        ScopeList.Add(ascomScopes[i]);
#endif
#ifdef GUIDE_ONCAMERA
    ScopeList.Add(_T("On-camera"));
#endif
#ifdef GUIDE_ONSTEPGUIDER
    ScopeList.Add(_T("On-AO"));
#endif
#ifdef GUIDE_GPUSB
    ScopeList.Add(_T("GPUSB"));
#endif
#ifdef GUIDE_GPINT
    ScopeList.Add(_T("GPINT 3BC"));
    ScopeList.Add(_T("GPINT 378"));
    ScopeList.Add(_T("GPINT 278"));
#endif
#ifdef GUIDE_VOYAGER
    ScopeList.Add(_T("Voyager"));
#endif
#ifdef GUIDE_EQUINOX
    ScopeList.Add(_T("Equinox 6"));
    ScopeList.Add(_T("EQMAC"));
#endif
#ifdef GUIDE_GCUSBST4
    ScopeList.Add(_T("GC USB ST4"));
#endif
#ifdef GUIDE_INDI
    ScopeList.Add(_T("INDI Mount"));
#endif

    ScopeList.Sort(&CompareNoCase);

    return ScopeList;
}

wxArrayString Scope::AuxMountList()
{
    wxArrayString scopeList;

    scopeList.Add(_("None"));      // Keep this at the top of the list

#ifdef GUIDE_ASCOM
    wxArrayString positionAwareScopes = ScopeASCOM::EnumAscomScopes();
    positionAwareScopes.Sort(&CompareNoCase);
    for (unsigned int i = 0; i < positionAwareScopes.Count(); i++)
        scopeList.Add(positionAwareScopes[i]);
#endif

#ifdef GUIDE_INDI
    scopeList.Add(_T("INDI Mount"));
#endif

    return scopeList;
}

Scope *Scope::Factory(const wxString& choice)
{
    Scope *pReturn = NULL;

    try
    {
        if (choice.IsEmpty())
        {
            throw ERROR_INFO("ScopeFactory called with choice.IsEmpty()");
        }

        Debug.AddLine(wxString::Format("ScopeFactory(%s)", choice));

        if (false) // so else ifs can follow
        {
        }
#ifdef GUIDE_ASCOM
        // do ASCOM first since it includes choices that could match stings belop like Simulator
        else if (choice.Find(_T("ASCOM")) != wxNOT_FOUND) {
            pReturn = new ScopeASCOM(choice);
        }
#endif
        else if (choice.Find(_("None")) + 1) {
        }
#ifdef GUIDE_ONCAMERA
        else if (choice.Find(_T("On-camera")) + 1) {
            pReturn = new ScopeOnCamera();
        }
#endif
#ifdef GUIDE_ONSTEPGUIDER
        else if (choice.Find(_T("On-AO")) + 1) {
            pReturn = new ScopeOnStepGuider();
        }
#endif
#ifdef GUIDE_GPUSB
        else if (choice.Find(_T("GPUSB")) + 1) {
            pReturn = new ScopeGpUsb();
        }
#endif
#ifdef GUIDE_GPINT
        else if (choice.Find(_T("GPINT 3BC")) + 1) {
            pReturn = new ScopeGpInt((short) 0x3BC);
        }
        else if (choice.Find(_T("GPINT 378")) + 1) {
            pReturn = new ScopeGpInt((short) 0x378);
        }
        else if (choice.Find(_T("GPINT 278")) + 1) {
            pReturn = new ScopeGpInt((short) 0x278);
        }
#endif
#ifdef GUIDE_VOYAGER
        else if (choice.Find(_T("Voyager")) + 1) {
            This needs work.  We have to move the setting of the IP address
                into the connect routine
            ScopeVoyager *pVoyager = new ScopeVoyager();
        }
#endif
#ifdef GUIDE_EQUINOX
        else if (choice.Find(_T("Equinox 6")) + 1) {
            pReturn = new ScopeEquinox();
        }
#endif
#ifdef GUIDE_EQMAC
        else if (choice.Find(_T("EQMAC")) + 1) {
            pReturn = new ScopeEQMac();
        }
#endif
#ifdef GUIDE_GCUSBST4
        else if (choice.Find(_T("GC USB ST4")) + 1) {
            pReturn = new ScopeGCUSBST4();
        }
#endif
#ifdef GUIDE_INDI
        else if (choice.Find(_T("INDI")) + 1) {
            pReturn = new ScopeINDI();
        }
#endif
        else {
            throw ERROR_INFO("ScopeFactory: Unknown Scope choice");
        }

        if (pReturn)
        {
            // virtual function call means we cannot do this in the Scope constructor
            pReturn->EnableStopGuidingWhenSlewing(pConfig->Profile.GetBoolean("/scope/StopGuidingWhenSlewing",
                pReturn->CanCheckSlewing()));
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (pReturn)
        {
            delete pReturn;
            pReturn = NULL;
        }
    }

    return pReturn;
}

bool Scope::GuidingCeases(void)
{
    // for scopes, we have nothing special to do when guiding stops
    return false;
}

bool Scope::RequiresCamera(void)
{
    return false;
}

bool Scope::RequiresStepGuider(void)
{
    return false;
}

bool Scope::CalibrationFlipRequiresDecFlip(void)
{
    return m_calibrationFlipRequiresDecFlip;
}

void Scope::SetCalibrationFlipRequiresDecFlip(bool val)
{
    m_calibrationFlipRequiresDecFlip = val;
    pConfig->Profile.SetBoolean("/scope/CalFlipRequiresDecFlip", val);
}

void Scope::SetAssumeOrthogonal(bool val)
{
    m_assumeOrthogonal = val;
    pConfig->Profile.SetBoolean("/scope/AssumeOrthogonal", val);
}

void Scope::EnableStopGuidingWhenSlewing(bool enable)
{
    if (enable)
        Debug.AddLine("Scope: enabling slew check, guiding will stop when slew is detected");
    else
        Debug.AddLine("Scope: slew check disabled");

    pConfig->Profile.SetBoolean("/scope/StopGuidingWhenSlewing", enable);
    m_stopGuidingWhenSlewing = enable;
}

void Scope::StartDecDrift(void)
{
    m_saveDecGuideMode = m_decGuideMode;
    m_decGuideMode = DEC_NONE;
    if (m_graphControlPane)
    {
        m_graphControlPane->m_pDecMode->SetSelection(DEC_NONE);
        m_graphControlPane->m_pDecMode->Enable(false);
    }
}

void Scope::EndDecDrift(void)
{
    m_decGuideMode = m_saveDecGuideMode;
    if (m_graphControlPane)
    {
        m_graphControlPane->m_pDecMode->SetSelection(m_decGuideMode);
        m_graphControlPane->m_pDecMode->Enable(true);
    }
}

bool Scope::IsDecDrifting(void) const
{
    return m_decGuideMode == DEC_NONE;
}

// Useful utility functions
#define DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))

inline static double norm(double val, double start, double end)
{
    double const range = end - start;
    double const ofs = val - start;
    return val - floor(ofs / range) * range;
}

inline static double norm_angle(double val)
{
    return norm(val, -M_PI, M_PI);
}

inline static double degrees(double radians)
{
    return radians * 180. / M_PI;
}

Mount::MOVE_RESULT Scope::CalibrationMove(GUIDE_DIRECTION direction, int duration)
{
    MOVE_RESULT result = MOVE_OK;

    Debug.AddLine(wxString::Format("scope calibration move dir= %d dur= %d", direction, duration));

    try
    {
        MoveResultInfo move;
        result = Move(direction, duration, false, &move);

        if (result != MOVE_OK)
        {
            throw THROW_INFO("Move failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    return result;
}

int Scope::CalibrationMoveSize(void)
{
    return m_calibrationDuration;
}

static wxString LimitReachedWarningKey(long axis)
{
    // we want the key to be under "/Confirm" so ConfirmDialog::ResetAllDontAskAgain() resets it, but we also want the setting to be per-profile
    return wxString::Format("/Confirm/%d/Max%sLimitWarningEnabled", pConfig->GetCurrentProfileId(), axis == GUIDE_RA ? "RA" : "Dec");
}

static void SuppressLimitReachedWarning(long axis)
{
    pConfig->Global.SetBoolean(LimitReachedWarningKey(axis), false);
}

static void AlertLimitReached(GuideAxis axis)
{
    if (pConfig->Global.GetBoolean(LimitReachedWarningKey(axis), true))
    {
        static time_t s_lastLogged;
        time_t now = time(0);
        if (s_lastLogged == 0 || now - s_lastLogged > 30)
        {
            s_lastLogged = now;
            wxString s = axis == GUIDE_RA ? _("Max RA Duration setting") : _("Max Dec Duration setting");
            pFrame->Alert(wxString::Format(_("Your %s is preventing PHD from making adequate corrections to keep the guide star locked. "
                "Increasing the %s will allow PHD to make the needed corrections."), s, s),
                _("Don't show\nthis again"), SuppressLimitReachedWarning, axis);
        }
    }
}

Mount::MOVE_RESULT Scope::Move(GUIDE_DIRECTION direction, int duration, bool normalMove, MoveResultInfo *moveResult)
{
    MOVE_RESULT result = MOVE_OK;
    bool limitReached = false;

    try
    {
        Debug.AddLine("Move(%d, %d, %d)", direction, duration, normalMove);

        if (!m_guidingEnabled)
        {
            throw THROW_INFO("Guiding disabled");
        }

        // Compute the actual guide durations

        switch (direction)
        {
            case NORTH:
            case SOUTH:

                // Enforce dec guiding mode and max dec duration for normal moves
                if (normalMove)
                {
                    if ((m_decGuideMode == DEC_NONE) ||
                        (direction == SOUTH && m_decGuideMode == DEC_NORTH) ||
                        (direction == NORTH && m_decGuideMode == DEC_SOUTH))
                    {
                        duration = 0;
                        Debug.AddLine("duration set to 0 by GuideMode");
                    }

                    if (duration > m_maxDecDuration)
                    {
                        duration = m_maxDecDuration;
                        Debug.AddLine("duration set to %d by maxDecDuration", duration);
                        limitReached = true;
                    }

                    if (limitReached && direction == m_decLimitReachedDirection)
                    {
                        if (++m_decLimitReachedCount >= LIMIT_REACHED_WARN_COUNT)
                            AlertLimitReached(GUIDE_DEC);
                    }
                    else
                        m_decLimitReachedCount = 0;

                    if (limitReached)
                        m_decLimitReachedDirection = direction;
                    else
                        m_decLimitReachedDirection = NONE;
                }
                break;
            case EAST:
            case WEST:

                if (normalMove)
                {
                    // enforce max RA duration for normal moves
                    if (duration > m_maxRaDuration)
                    {
                        duration = m_maxRaDuration;
                        Debug.AddLine("duration set to %d by maxRaDuration", duration);
                        limitReached = true;
                    }

                    if (limitReached && direction == m_raLimitReachedDirection)
                    {
                        if (++m_raLimitReachedCount >= LIMIT_REACHED_WARN_COUNT)
                            AlertLimitReached(GUIDE_RA);
                    }
                    else
                        m_raLimitReachedCount = 0;

                    if (limitReached)
                        m_raLimitReachedDirection = direction;
                    else
                        m_raLimitReachedDirection = NONE;
                }
                break;

            case NONE:
                break;
        }

        // Actually do the guide
        assert(duration >= 0);
        if (duration > 0)
        {
            result = Guide(direction, duration);
            if (result != MOVE_OK)
            {
                throw ERROR_INFO("guide failed");
            }
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        if (result == MOVE_OK)
            result = MOVE_ERROR;
        duration = 0;
    }

    Debug.AddLine(wxString::Format("Move returns status %d, amount %d", result, duration));

    if (moveResult)
    {
        moveResult->amountMoved = duration;
        moveResult->limited = limitReached;
    }

    return result;
}

bool Scope::GetLastCalibrationParams(Calibration_Params* params)
{
    wxString prefix = "/" + GetMountClassName() +"/calibration/";
    wxString sTimestamp = pConfig->Profile.GetString("/scope/calibration/timestamp", wxEmptyString);
    params->TimeStamp = sTimestamp;
    if (sTimestamp.Length() > 0)
    {
        params->XRate = pConfig->Profile.GetDouble(prefix + "xRate", 1.0); 
        params->YRate = pConfig->Profile.GetDouble(prefix + "yRate", 1.0);
        params->XAngle = pConfig->Profile.GetDouble(prefix + "xAngle", 0.0);
        params->YAngle = pConfig->Profile.GetDouble(prefix + "yAngle", 0.0);
        params->Declination = pConfig->Profile.GetDouble(prefix + "declination", 0.0);
        params->PierSide = pConfig->Profile.GetInt(prefix + "pierSide", PIER_SIDE_UNKNOWN);
        return true;
    }
    else
        return false;
}

static wxString CalibrationWarningKey(Calibration_Issues etype)
{
    wxString qual;
    switch (etype)
    {
    case CI_Angle:
        qual = "Angle";
        break;
    case CI_Different:
        qual = "Diff";
        break;
    case CI_Steps:
        qual = "Steps";
        break;
    case CI_Rates:
        qual = "Rates";
        break;
    case CI_None:
        qual = "Bogus";
        break;

    }
    wxString rtn = wxString::Format("/Confirm/%d/CalWarning_%s", pConfig->GetCurrentProfileId(), qual);
    return rtn;
}
void Scope::SetCalibrationWarning(Calibration_Issues etype, bool val)
{
    pConfig->Global.SetBoolean(CalibrationWarningKey(etype), val);
}

// Generic hook for "details" button in calibration sanity check alert
static void ShowCalibrationIssues(long scopeptr)
{
    Scope *pscope = (Scope*)scopeptr;
    pscope->HandleSanityCheckDialog();
    
}
// Handle the "details" dialog for the calibration sanity check
void Scope::HandleSanityCheckDialog()
{
    Calibration_Params newParams;
    
    if (pFrame->pCalSanityCheckDlg)
        pFrame->pCalSanityCheckDlg->Destroy();

    GetLastCalibrationParams(&newParams);
    pFrame->pCalSanityCheckDlg = new CalSanityDialog(m_prevCalibrationParams, newParams, m_raSteps, m_decSteps, m_lastCalibrationIssue, this);
    pFrame->pCalSanityCheckDlg->Show();
}

// Do some basic sanity checking on the just-completed calibration, looking for things that are fishy.  Do the checking in the order of 
// importance/confidence, since we only alert about a single condition
void Scope::SanityCheckCalibration(const Calibration_Params *pOld, const Calibration_Params *pNew, int xSteps, int ySteps)
{
    wxString detailInfo;
    m_lastCalibrationIssue = CI_None;

    // Too few steps
    if (xSteps < CAL_ALERT_MINSTEPS || ySteps < CAL_ALERT_MINSTEPS)
    {
        m_lastCalibrationIssue = CI_Steps;
        detailInfo = wxString::Format("Actual RA calibration steps = %d, Dec calibration steps = %d", xSteps, ySteps);
    }
    else
    {
        // Non-orthogonal RA/Dec axes
        double nonOrtho = fabs(fabs(norm_angle(pNew->XAngle - pNew->YAngle)) - M_PI/2.) * 180.0/ M_PI;         // Delta from the nearest multiple of 90 degrees
        if (nonOrtho >  CAL_ALERT_ORTHOGONALITY_TOLERANCE)
        {
            m_lastCalibrationIssue = CI_Angle;
            detailInfo = wxString::Format("Non-orthogonality = %0.3f", nonOrtho);
        }
        else
            // RA/Dec rates should be related by cos(dec) but don't check if Dec is too high or Dec guiding is disabled
            if (pNew->Declination != 0.0 && pNew->YRate != 0 && fabs(pNew->Declination) <= (M_PI / 2.0 * 2.0 / 3.0))
                {
                    double expectedRatio = cos(pNew->Declination);
                    double actualRatio = pNew->XRate / pNew->YRate;
                    if (fabs(expectedRatio - actualRatio) > CAL_ALERT_AXISRATES_TOLERANCE)
                    {
                        m_lastCalibrationIssue = CI_Rates;
                        detailInfo = wxString::Format("Expected ratio at dec=&0.1f is %0.3f, actual is %0.3f", pNew->Declination * 180.0/M_PI, actualRatio);
                    }
                }

        // Finally check for a dec rate that is more than nn% different than the last accepted calibration
        if (m_lastCalibrationIssue == CI_None && pOld->TimeStamp.Length() > 0)
        {
            double newDecRate = pNew->YRate;
            if (newDecRate != 0)
            {
                if (fabs(1.0 - (pOld->YRate / newDecRate)) > CAL_ALERT_DECRATE_DIFFERENCE)
                {
                    m_lastCalibrationIssue = CI_Different;
                    detailInfo = wxString::Format("Current/previous Dec rate ratio is %0.3f", pOld->YRate / newDecRate);
                }
            }
            else 
                if (pOld->YRate != 0)                   // Might have had Dec guiding disabled
                    m_lastCalibrationIssue = CI_Different;
        }
    }

    if (m_lastCalibrationIssue != CI_None)
    {
        wxString alertMsg;

        switch (m_lastCalibrationIssue)
        {
        case CI_Steps:
            alertMsg = _("Calibration is based on very few steps, so accuracy is questionable");
            break;
        case CI_Angle:
            alertMsg = _("Calibration computed RA/Dec axis angles that are questionable");
            break;
        case CI_Different:
            alertMsg = _("This calibration is substantially different from the previous one - have you changed configurations?");
            break;
        case CI_Rates:
            alertMsg = _("The RA and Dec rates vary by an unexpected amount");
        default:
            break;
        }
        if (pConfig->Global.GetBoolean(CalibrationWarningKey(m_lastCalibrationIssue), true))        // User hasn't disabled this type of alert
        {
            pFrame->Alert(alertMsg,
                _("Details..."), ShowCalibrationIssues, (long)this);
        }
        else
            Debug.AddLine(wxString::Format("Alert detected in scope calibration but not shown to user - suppressed message was: %s", alertMsg));
        Debug.AddLine(wxString::Format("Calibration alert details: %s", detailInfo));
    }
}

void Scope::ClearCalibration(void)
{
    Mount::ClearCalibration();

    m_calibrationState = CALIBRATION_STATE_CLEARED;
}

bool Scope::BeginCalibration(const PHD_Point& currentLocation)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Not connected");
        }

        if (!currentLocation.IsValid())
        {
            throw ERROR_INFO("Must have a valid lock position");
        }

        ClearCalibration();
        m_calibrationSteps = 0;
        m_calibrationInitialLocation = currentLocation;
        m_calibrationStartingLocation.Invalidate();
        m_calibrationState = CALIBRATION_STATE_GO_WEST;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Scope::SetCalibration(double xAngle, double yAngle, double xRate, double yRate, double declination, PierSide pierSide)
{
    m_calibrationXAngle = xAngle;
    m_calibrationXRate = xRate;
    m_calibrationYAngle = yAngle;
    m_calibrationYRate = yRate;
    Mount::SetCalibration(xAngle, yAngle, xRate, yRate, declination, pierSide);
}

bool Scope::IsCalibrated(void)
{
    if (!Mount::IsCalibrated())
        return false;

    switch (m_decGuideMode)
    {
    case DEC_NONE:
        return true;
    case DEC_AUTO:
    case DEC_NORTH:
    case DEC_SOUTH:
        {
            bool have_ns_calibration = m_calibrationYRate != CALIBRATION_RATE_UNCALIBRATED;
            return have_ns_calibration;
        }
    default:
        assert(false);
        return true;
    }
}

static double CalibrationDistance(void)
{
    return wxMin(pCamera->FullSize.GetHeight() * 0.05, MAX_CALIBRATION_DISTANCE);
}

int Scope::CalibrationTotDistance(void)
{
    return (int) ceil(CalibrationDistance());
}



// Convert camera coords to mount coords
static PHD_Point MountCoords(const PHD_Point& cameraVector, double xCalibAngle, double yCalibAngle)
{
    double hyp = cameraVector.Distance();
    double cameraTheta = cameraVector.Angle();
    double yAngleError = (xCalibAngle - yCalibAngle) + M_PI / 2;
    yAngleError = atan2(sin(yAngleError), cos(yAngleError));
    double xAngle = cameraTheta - xCalibAngle;
    double yAngle = cameraTheta - (xCalibAngle + yAngleError);
    return PHD_Point(hyp * cos(xAngle), hyp * sin(yAngle));
}

bool Scope::UpdateCalibrationState(const PHD_Point& currentLocation)
{
    bool bError = false;

    try
    {
        wxString status0, status1;

        if (!m_calibrationStartingLocation.IsValid())
        {
            m_calibrationStartingLocation = currentLocation;
            Debug.AddLine(wxString::Format("Scope::UpdateCalibrationstate: starting location = %.2f,%.2f", currentLocation.X, currentLocation.Y));
        }

        double dX = m_calibrationStartingLocation.dX(currentLocation);
        double dY = m_calibrationStartingLocation.dY(currentLocation);
        double dist = m_calibrationStartingLocation.Distance(currentLocation);
        double dist_crit = CalibrationDistance();

        switch (m_calibrationState)
        {
            case CALIBRATION_STATE_CLEARED:
                assert(false);
                break;

            case CALIBRATION_STATE_GO_WEST:

                // step number in the log is the step that just finished
                GuideLog.CalibrationStep(this, "West", m_calibrationSteps, dX, dY, currentLocation, dist);

                if (dist < dist_crit)
                {
                    if (m_calibrationSteps++ > MAX_CALIBRATION_STEPS)
                    {
                        wxString msg(wxTRANSLATE("RA Calibration Failed: star did not move enough"));
                        const wxString& translated(wxGetTranslation(msg));
                        pFrame->Alert(translated);
                        GuideLog.CalibrationFailed(this, msg);
                        EvtServer.NotifyCalibrationFailed(this, msg);
                        throw ERROR_INFO("RA calibration failed");
                    }
                    status0.Printf(_("West step %3d"), m_calibrationSteps);
                    pFrame->ScheduleCalibrationMove(this, WEST, m_calibrationDuration);
                    break;
                }

                m_calibrationXAngle = m_calibrationStartingLocation.Angle(currentLocation);
                m_calibrationXRate = dist / (m_calibrationSteps * m_calibrationDuration);

                Debug.AddLine(wxString::Format("WEST calibration completes with steps=%d angle=%.1f rate=%.4f", m_calibrationSteps, m_calibrationXAngle * 180. / M_PI, m_calibrationXRate));
                status1.Printf(_("angle=%.1f rate=%.4f"), m_calibrationXAngle * 180. / M_PI, m_calibrationXRate);
                m_raSteps = m_calibrationSteps;
                GuideLog.CalibrationDirectComplete(this, "West", m_calibrationXAngle, m_calibrationXRate);

                // for GO_EAST m_recenterRemaining contains the total remaining duration.
                // Choose the largest pulse size that will not lose the guide star or exceed
                // the user-specified max pulse

                m_recenterRemaining = m_calibrationSteps * m_calibrationDuration;

                if (pFrame->pGuider->IsFastRecenterEnabled())
                {
                    m_recenterDuration = (int) floor((double) pFrame->pGuider->GetMaxMovePixels() / m_calibrationXRate);
                    if (m_recenterDuration > m_maxRaDuration)
                        m_recenterDuration = m_maxRaDuration;
                    if (m_recenterDuration < m_calibrationDuration)
                        m_recenterDuration = m_calibrationDuration;
                }
                else
                    m_recenterDuration = m_calibrationDuration;

                m_calibrationSteps = DIV_ROUND_UP(m_recenterRemaining, m_recenterDuration);
                m_calibrationState = CALIBRATION_STATE_GO_EAST;

                // fall through
                Debug.AddLine("Falling Through to state GO_EAST");

            case CALIBRATION_STATE_GO_EAST:

                GuideLog.CalibrationStep(this, "East", m_calibrationSteps, dX, dY, currentLocation, dist);

                if (m_recenterRemaining > 0)
                {
                    int duration = m_recenterDuration;
                    if (duration > m_recenterRemaining)
                        duration = m_recenterRemaining;

                    status0.Printf(_("East step %3d"), m_calibrationSteps);

                    m_recenterRemaining -= duration;
                    --m_calibrationSteps;
                    m_lastLocation = currentLocation;

                    pFrame->ScheduleCalibrationMove(this, EAST, duration);
                    break;
                }

                // setup for clear backlash

                m_calibrationSteps = 0;
                dist = dX = dY = 0.0;
                m_calibrationStartingLocation = currentLocation;

                if (m_decGuideMode == DEC_NONE)
                {
                    m_calibrationState = CALIBRATION_STATE_COMPLETE;
                    m_calibrationYAngle = norm_angle(m_calibrationXAngle + M_PI / 2.); // choose an arbitrary angle perpendicular to xAngle
                    // indicate lack of Dec calibration data, see Scope::IsCalibrated.
                    m_calibrationYRate = CALIBRATION_RATE_UNCALIBRATED;
                    break;
                }

                m_calibrationState = CALIBRATION_STATE_CLEAR_BACKLASH;

                // fall through
                Debug.AddLine("Falling Through to state CLEAR_BACKLASH");

            case CALIBRATION_STATE_CLEAR_BACKLASH:

                GuideLog.CalibrationStep(this, "Backlash", m_calibrationSteps, dX, dY, currentLocation, dist);

                if (dist < DEC_BACKLASH_DISTANCE)
                {
                    if (m_calibrationSteps++ > MAX_CALIBRATION_STEPS)
                    {
                        wxString msg(wxTRANSLATE("Backlash Clearing Failed: star did not move enough"));
                        const wxString& translated(wxGetTranslation(msg));
                        pFrame->Alert(translated);
                        GuideLog.CalibrationFailed(this, msg);
                        EvtServer.NotifyCalibrationFailed(this, msg);
                        throw ERROR_INFO("Clear backlash failed");
                    }
                    status0.Printf(_("Clear backlash step %3d"), m_calibrationSteps);
                    pFrame->ScheduleCalibrationMove(this, NORTH, m_calibrationDuration);
                    break;
                }

                m_calibrationSteps = 0;
                dist = dX = dY = 0.0;
                m_calibrationStartingLocation = currentLocation;
                m_calibrationState = CALIBRATION_STATE_GO_NORTH;

                // fall through
                Debug.AddLine("Falling Through to state GO_NORTH");

            case CALIBRATION_STATE_GO_NORTH:

                GuideLog.CalibrationStep(this, "North", m_calibrationSteps, dX, dY, currentLocation, dist);

                if (dist < dist_crit)
                {
                    if (m_calibrationSteps++ > MAX_CALIBRATION_STEPS)
                    {
                        wxString msg(wxTRANSLATE("DEC Calibration Failed: star did not move enough"));
                        const wxString& translated(wxGetTranslation(msg));
                        pFrame->Alert(translated);
                        GuideLog.CalibrationFailed(this, msg);
                        EvtServer.NotifyCalibrationFailed(this, msg);
                        throw ERROR_INFO("Dec calibration failed");
                    }
                    status0.Printf(_("North step %3d"), m_calibrationSteps);
                    pFrame->ScheduleCalibrationMove(this, NORTH, m_calibrationDuration);
                    break;
                }

                // note: this calculation is reversed from the ra calculation, because
                // that one was calibrating WEST, but the angle is really relative
                // to EAST
                if (m_assumeOrthogonal)
                {
                    double a1 = norm_angle(m_calibrationXAngle + M_PI / 2.);
                    double a2 = norm_angle(m_calibrationXAngle - M_PI / 2.);
                    double yAngle = currentLocation.Angle(m_calibrationStartingLocation);
                    m_calibrationYAngle = fabs(norm_angle(a1 - yAngle)) < fabs(norm_angle(a2 - yAngle)) ? a1 : a2;
                    double dec_dist = dist * cos(yAngle - m_calibrationYAngle);
                    m_calibrationYRate = dec_dist / (m_calibrationSteps * m_calibrationDuration);

                    Debug.AddLine("Assuming orthogonal axes: measured Y angle = %.1f, X angle = %.1f, orthogonal = %.1f, %.1f, best = %.1f, dist = %.2f, dec_dist = %.2f",
                        degrees(yAngle), degrees(m_calibrationXAngle), degrees(a1), degrees(a2), degrees(m_calibrationYAngle), dist, dec_dist);
                }
                else
                {
                    m_calibrationYAngle = currentLocation.Angle(m_calibrationStartingLocation);
                    m_calibrationYRate = dist / (m_calibrationSteps * m_calibrationDuration);
                }

                Debug.AddLine(wxString::Format("NORTH calibration completes with angle=%.1f rate=%.4f", m_calibrationYAngle * 180. / M_PI, m_calibrationYRate));
                m_decSteps = m_calibrationSteps;
                status1.Printf(_("angle=%.1f rate=%.4f"), m_calibrationYAngle * 180. / M_PI, m_calibrationYRate);
                GuideLog.CalibrationDirectComplete(this, "North", m_calibrationYAngle, m_calibrationYRate);

                // for GO_SOUTH m_recenterRemaining contains the total remaining duration.
                // Choose the largest pulse size that will not lose the guide star or exceed
                // the user-specified max pulse
                m_recenterRemaining = m_calibrationSteps * m_calibrationDuration;

                if (pFrame->pGuider->IsFastRecenterEnabled())
                {
                    m_recenterDuration = (int) floor((double) pFrame->pGuider->GetMaxMovePixels() / m_calibrationYRate);
                    if (m_recenterDuration > m_maxDecDuration)
                        m_recenterDuration = m_maxDecDuration;
                    if (m_recenterDuration < m_calibrationDuration)
                        m_recenterDuration = m_calibrationDuration;
                }
                else
                    m_recenterDuration = m_calibrationDuration;

                m_calibrationSteps = DIV_ROUND_UP(m_recenterRemaining, m_recenterDuration);
                m_calibrationState = CALIBRATION_STATE_GO_SOUTH;

                // fall through
                Debug.AddLine("Falling Through to state GO_SOUTH");

            case CALIBRATION_STATE_GO_SOUTH:

                GuideLog.CalibrationStep(this, "South", m_calibrationSteps, dX, dY, currentLocation, dist);

                if (m_recenterRemaining > 0)
                {
                    int duration = m_recenterDuration;
                    if (duration > m_recenterRemaining)
                        duration = m_recenterRemaining;

                    status0.Printf(_("South step %3d"), m_calibrationSteps);

                    m_recenterRemaining -= duration;
                    --m_calibrationSteps;
                    m_lastLocation = currentLocation;

                    pFrame->ScheduleCalibrationMove(this, SOUTH, duration);
                    break;
                }

                // Nudge further South on Dec, get within 2 px North/South of starting point, don't try more than 3 times.
                if (m_calibrationSteps <= MAX_NUDGES && currentLocation.Distance(m_calibrationInitialLocation) > NUDGE_TOLERANCE)
                {
                    double firstDecAmt = MountCoords(m_lastLocation - m_calibrationInitialLocation, m_calibrationXAngle, m_calibrationYAngle).Y;
                    double decAmt = MountCoords(currentLocation - m_calibrationInitialLocation, m_calibrationXAngle, m_calibrationYAngle).Y;
                    Debug.AddLine(wxString::Format("Final nudging, decAmt = %.3f, lastSouthDistance = %.3f", decAmt, firstDecAmt));
                    if (decAmt * firstDecAmt > 0.0)           // still in the same direction
                    {
                        decAmt = fabs(decAmt);           // Sign doesn't matter now, we're always moving south
                        decAmt = wxMin(decAmt, (double) pFrame->pGuider->GetMaxMovePixels());
                        int pulseAmt = (int) floor(decAmt / m_calibrationYRate);
                        Debug.AddLine(wxString::Format("Sending NudgeSouth pulse of duration %d ms", pulseAmt));
                        ++m_calibrationSteps;
                        status0.Printf(_("Final Nudge South %3d"), m_calibrationSteps);
                        pFrame->ScheduleCalibrationMove(this, SOUTH, pulseAmt);
                        break;
                    }
                }

                Debug.AddLine(wxString::Format("Final nudging status: Current loc = {%.3f,%.3f}, targeting {%.3f,%.3f}", currentLocation.X, currentLocation.Y,
                    m_calibrationInitialLocation.X, m_calibrationInitialLocation.Y));

                m_calibrationState = CALIBRATION_STATE_COMPLETE;
                // fall through
                Debug.AddLine("Falling Through to state CALIBRATION_COMPLETE");

            case CALIBRATION_STATE_COMPLETE:
                GetLastCalibrationParams(&m_prevCalibrationParams);
                SetCalibration(m_calibrationXAngle, m_calibrationYAngle,
                               m_calibrationXRate,  m_calibrationYRate,
                               pPointingSource->GetGuidingDeclination(), pPointingSource->SideOfPier());
                Calibration_Params newParams;
                GetLastCalibrationParams(&newParams);
                if (SANITY_CHECKING_ACTIVE)
                    SanityCheckCalibration(&m_prevCalibrationParams, &newParams, m_raSteps, m_decSteps);
                pFrame->SetStatusText(_("calibration complete"),1);
                GuideLog.CalibrationComplete(this);
                EvtServer.NotifyCalibrationComplete(this);
                Debug.AddLine("Calibration Complete");
                break;
        }

        if (m_calibrationState != CALIBRATION_STATE_COMPLETE)
        {
            if (status1.IsEmpty())
            {
                double dX = m_calibrationStartingLocation.dX(currentLocation);
                double dY = m_calibrationStartingLocation.dY(currentLocation);
                double dist = m_calibrationStartingLocation.Distance(currentLocation);

                status1.Printf(_("dx=%4.1f dy=%4.1f dist=%4.1f"), dX, dY, dist);
            }
        }

        if (!status0.IsEmpty())
        {
            pFrame->SetStatusText(status0, 0);
        }

        if (!status1.IsEmpty())
        {
            pFrame->SetStatusText(status1, 1);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);

        ClearCalibration();

        bError = true;
    }

    return bError;
}

wxString Scope::GetSettingsSummary()
{
    // return a loggable summary of current mount settings
    return Mount::GetSettingsSummary() +
        wxString::Format("Calibration step = phdlab_placeholder, Max RA duration = %d, Max DEC duration = %d, DEC guide mode = %s\n",
            GetMaxRaDuration(),
            GetMaxDecDuration(),
            GetDecGuideMode() == DEC_NONE ? "Off" : GetDecGuideMode() == DEC_AUTO ? "Auto" :
            GetDecGuideMode() == DEC_NORTH ? "North" : "South"
        );
}

wxString Scope::CalibrationSettingsSummary()
{
    return wxString::Format("Calibration Step = %d ms, Assume orthogonal axes = %s", GetCalibrationDuration(),
        IsAssumeOrthogonal() ? "yes" : "no");
}

wxString Scope::GetMountClassName() const
{
    return wxString("scope");
}

ConfigDialogPane *Scope::GetConfigDialogPane(wxWindow *pParent)
{
    return new ScopeConfigDialogPane(pParent, this);
}

Scope::ScopeConfigDialogPane::ScopeConfigDialogPane(wxWindow *pParent, Scope *pScope)
    : MountConfigDialogPane(pParent, "Mount", pScope)
{
    int width;

    m_pScope = pScope;

    width = StringWidth(_T("00000"));
    m_pCalibrationDuration = new wxSpinCtrl(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 10000, 1000,_T("Cal_Dur"));

    // add the 'auto' button and bind it to the associated event-handler
    wxButton *pAutoDuration = new wxButton(pParent, wxID_OK, _("Calculate...") );
    pAutoDuration->SetToolTip(_("Click to open the Calibration Step Calculator to help find a good calibration step size"));
    pAutoDuration->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &Scope::ScopeConfigDialogPane::OnCalcCalibrationStep, this);

    DoAdd(_("Calibration step (ms)"), m_pCalibrationDuration,
        _("How long a guide pulse should be used during calibration? Click \"Calculate\" to compute a suitable value."), pAutoDuration);

    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText *lbl = new wxStaticText(pParent, wxID_ANY, "Max Duration");
    sizer->Add(lbl, wxSizerFlags().Expand().Border(wxALL, 3).Align(wxALIGN_CENTER_VERTICAL));

    width = StringWidth(_T("00000"));
    m_pMaxRaDuration = new wxSpinCtrl(pParent,wxID_ANY,_T("foo"),wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0, 2000, 150, _T("MaxDec_Dur"));
    wxSizer *sizer1 = MakeLabeledControl(_("RA"),  m_pMaxRaDuration,
          _("Longest length of pulse to send in RA\nDefault = 1000 ms."));
    sizer->Add(sizer1, wxSizerFlags().Expand().Border(wxALL,3));

    width = StringWidth(_T("00000"));
    m_pMaxDecDuration = new wxSpinCtrl(pParent,wxID_ANY,_T("foo"),wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS,0,2000,150,_T("MaxDec_Dur"));
    wxSizer *sizer2 = MakeLabeledControl(_("Dec"),  m_pMaxDecDuration,
          _("Longest length of pulse to send in declination\nDefault = 1000 ms.  Increase if drift is fast."));
    sizer->Add(sizer2, wxSizerFlags().Expand().Border(wxALL,3));

    DoAdd(sizer);

    m_pNeedFlipDec = new wxCheckBox(pParent, wxID_ANY, _("Reverse Dec output after meridian flip"));
    DoAdd(m_pNeedFlipDec, _("Check if your mount needs Dec output reversed after doing Flip Calibration Data"));

    if (pScope->CanCheckSlewing())
    {
        m_pStopGuidingWhenSlewing = new wxCheckBox(pParent, wxID_ANY, _("Stop guiding when mount slews"));
        DoAdd(m_pStopGuidingWhenSlewing, _("When checked, PHD will stop guiding if the mount starts slewing"));
    }
    else
        m_pStopGuidingWhenSlewing = 0;

    m_assumeOrthogonal = new wxCheckBox(pParent, wxID_ANY,
        _("Assume Dec orthogonal to RA"));
    DoAdd(m_assumeOrthogonal, _("Assume Dec axis is perpendicular to RA axis, regardless of calibration. Prevents RA periodic error from affecting Dec calibration. Option takes effect when calibrating DEC."));

    wxString dec_choices[] = {
        _("Off"),_("Auto"),_("North"),_("South")
    };
    width = StringArrayWidth(dec_choices, WXSIZEOF(dec_choices));
    m_pDecMode = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1),
            wxSize(width+35, -1), WXSIZEOF(dec_choices), dec_choices);
    DoAdd(_("Dec guide mode"), m_pDecMode,
          _("Guide in declination as well?"));
}

void Scope::ScopeConfigDialogPane::OnCalcCalibrationStep(wxCommandEvent& evt)
{
    int focalLength = 0;
    double pixelSize = 0;
    wxString configPrefix;
    AdvancedDialog *pAdvancedDlg = pFrame->pAdvancedDialog;

    if (pAdvancedDlg)
    {
        pixelSize = pAdvancedDlg->GetPixelSize();
        focalLength = pAdvancedDlg->GetFocalLength();
    }

    CalstepDialog calc(m_pParent, focalLength, pixelSize);
    if (calc.ShowModal() == wxID_OK)
    {
        int calibrationStep;
        if (calc.GetResults(&focalLength, &pixelSize, &calibrationStep))
        {
            // Following sets values in the UI controls of the various dialog tabs - not underlying data values
            pAdvancedDlg->SetFocalLength(focalLength);
            pAdvancedDlg->SetPixelSize(pixelSize);
            m_pCalibrationDuration->SetValue(calibrationStep);
        }
    }
}

Scope::ScopeConfigDialogPane::~ScopeConfigDialogPane(void)
{
}

void Scope::ScopeConfigDialogPane::LoadValues(void)
{
    MountConfigDialogPane::LoadValues();
    m_pCalibrationDuration->SetValue(m_pScope->GetCalibrationDuration());
    m_pMaxRaDuration->SetValue(m_pScope->GetMaxRaDuration());
    m_pMaxDecDuration->SetValue(m_pScope->GetMaxDecDuration());
    m_pDecMode->SetSelection(m_pScope->GetDecGuideMode());
    m_pNeedFlipDec->SetValue(m_pScope->CalibrationFlipRequiresDecFlip());
    if (m_pStopGuidingWhenSlewing)
        m_pStopGuidingWhenSlewing->SetValue(m_pScope->IsStopGuidingWhenSlewingEnabled());
    m_assumeOrthogonal->SetValue(m_pScope->IsAssumeOrthogonal());
}

void Scope::ScopeConfigDialogPane::UnloadValues(void)
{
    m_pScope->SetCalibrationDuration(m_pCalibrationDuration->GetValue());
    m_pScope->SetMaxRaDuration(m_pMaxRaDuration->GetValue());
    m_pScope->SetMaxDecDuration(m_pMaxDecDuration->GetValue());
    m_pScope->SetDecGuideMode(m_pDecMode->GetSelection());
    m_pScope->SetCalibrationFlipRequiresDecFlip(m_pNeedFlipDec->GetValue());
    if (m_pStopGuidingWhenSlewing)
        m_pScope->EnableStopGuidingWhenSlewing(m_pStopGuidingWhenSlewing->GetValue());
    m_pScope->SetAssumeOrthogonal(m_assumeOrthogonal->GetValue());

    MountConfigDialogPane::UnloadValues();
}

GraphControlPane *Scope::GetGraphControlPane(wxWindow *pParent, const wxString& label)
{
    return new ScopeGraphControlPane(pParent, this, label);
}

Scope::ScopeGraphControlPane::ScopeGraphControlPane(wxWindow *pParent, Scope *pScope, const wxString& label)
    : GraphControlPane(pParent, label)
{
    int width;
    m_pScope = pScope;
    pScope->m_graphControlPane = this;

    width = StringWidth(_T("0000"));
    m_pMaxRaDuration = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width+30, -1),
        wxSP_ARROW_KEYS, 0,2000,0);
    m_pMaxRaDuration->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &Scope::ScopeGraphControlPane::OnMaxRaDurationSpinCtrl, this);
    DoAdd(m_pMaxRaDuration, _("Mx RA"));

    width = StringWidth(_T("0000"));
    m_pMaxDecDuration = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width+30, -1),
        wxSP_ARROW_KEYS, 0,2000,0);
    m_pMaxDecDuration->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &Scope::ScopeGraphControlPane::OnMaxDecDurationSpinCtrl, this);
    DoAdd(m_pMaxDecDuration, _("Mx DEC"));

    wxString dec_choices[] = { _("Off"),_("Auto"),_("North"),_("South") };
    m_pDecMode = new wxChoice(this, wxID_ANY,
        wxDefaultPosition,wxDefaultSize, WXSIZEOF(dec_choices), dec_choices );
    m_pDecMode->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &Scope::ScopeGraphControlPane::OnDecModeChoice, this);
    m_pControlSizer->Add(m_pDecMode);

    m_pMaxRaDuration->SetValue(m_pScope->GetMaxRaDuration());
    m_pMaxDecDuration->SetValue(m_pScope->GetMaxDecDuration());
    m_pDecMode->SetSelection(m_pScope->GetDecGuideMode());
}

Scope::ScopeGraphControlPane::~ScopeGraphControlPane()
{
    if (m_pScope)
    {
        m_pScope->m_graphControlPane = NULL;
    }
}

void Scope::ScopeGraphControlPane::OnMaxRaDurationSpinCtrl(wxSpinEvent& WXUNUSED(evt))
{
    m_pScope->SetMaxRaDuration(m_pMaxRaDuration->GetValue());
    GuideLog.SetGuidingParam("Max RA duration", m_pMaxRaDuration->GetValue());
}

void Scope::ScopeGraphControlPane::OnMaxDecDurationSpinCtrl(wxSpinEvent& WXUNUSED(evt))
{
    m_pScope->SetMaxDecDuration(m_pMaxDecDuration->GetValue());
    GuideLog.SetGuidingParam("Max DEC duration", m_pMaxDecDuration->GetValue());
}

void Scope::ScopeGraphControlPane::OnDecModeChoice(wxCommandEvent& WXUNUSED(evt))
{
    m_pScope->SetDecGuideMode(m_pDecMode->GetSelection());
    wxString dec_choices[] = { _("Off"),_("Auto"),_("North"),_("South") };
    GuideLog.SetGuidingParam("DEC guide mode", dec_choices[m_pDecMode->GetSelection()]);
}
