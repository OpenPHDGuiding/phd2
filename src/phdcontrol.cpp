/*
 *  phdcontrol.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2013 Andy Galasso
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

enum State
{
    STATE_IDLE = 0,
    STATE_SETUP,
    STATE_ATTEMPT_START,
    STATE_SELECT_STAR,
    STATE_WAIT_SELECTED,
    STATE_CALIBRATE,
    STATE_CALIBRATION_WAIT,
    STATE_GUIDE,
    STATE_SETTLE_BEGIN,
    STATE_SETTLE_WAIT,
    STATE_FINISH,
};

enum SettleOp
{
    OP_DITHER,
    OP_GUIDE,
};

enum { SETTLING_TIME_DISABLED = 9999 };

struct ControllerState
{
    State state;
    bool forceCalibration;
    bool haveSaveSticky;
    bool saveSticky;
    int autoFindAttemptsRemaining;
    int waitSelectedRemaining;
    SettleOp settleOp;
    SettleParams settle;
    wxRect roi;
    bool settlePriorFrameInRange;
    wxStopWatch *settleTimeout;
    wxStopWatch *settleInRange;
    DEC_GUIDE_MODE saveDecGuideMode;
    bool overrideDecGuideMode;
    int settleFrameCount;
    int droppedFrameCount;
    bool succeeded;
    wxString errorMsg;
};

static ControllerState ctrl;

void PhdController::OnAppInit()
{
    ctrl.settleTimeout = new wxStopWatch();
    ctrl.settleInRange = new wxStopWatch();
}

void PhdController::OnAppExit()
{
    delete ctrl.settleTimeout;
    ctrl.settleTimeout = NULL;
    delete ctrl.settleInRange;
    ctrl.settleInRange = NULL;
}

bool PhdController::IsSettling()
{
    return ctrl.state == STATE_SETTLE_BEGIN || ctrl.state == STATE_SETTLE_WAIT;
}

// Idle means controller is not in transitional states of starting/calibrating/stopping/settling
bool PhdController::IsIdle()
{
    return ctrl.state == STATE_IDLE;
}

#define SETSTATE(newstate) do { \
    Debug.AddLine("PhdController: newstate " #newstate); \
    ctrl.state = newstate; \
} while (false)

static wxString ReentrancyError(const char *op)
{
    return wxString::Format("Cannot initiate %s while %s is in progress", op, ctrl.settleOp == OP_DITHER ? "dither" : "guide");
}

bool PhdController::Guide(bool recalibrate, const SettleParams& settle, const wxRect& roi, wxString *error)
{
    if (ctrl.state != STATE_IDLE)
    {
        Debug.Write(wxString::Format("PhdController::Guide reentrancy state = %d op = %d\n", ctrl.state, ctrl.settleOp));
        *error = ReentrancyError("guide");
        return false;
    }

    Debug.AddLine("PhdController::Guide begins");
    ctrl.forceCalibration = recalibrate;
    ctrl.settleOp = OP_GUIDE;
    ctrl.settle = settle;
    ctrl.roi = roi;
    SETSTATE(STATE_SETUP);
    UpdateControllerState();
    return true;
}

static void do_fail(const wxString& msg)
{
    Debug.AddLine(wxString::Format("PhdController failed: %s", msg));
    ctrl.succeeded = false;
    ctrl.errorMsg = msg;
    SETSTATE(STATE_FINISH);
}

bool PhdController::Dither(double pixels, bool forceRaOnly, const SettleParams& settle, wxString *errMsg)

{
    if (ctrl.state != STATE_IDLE)
    {
        Debug.Write(wxString::Format("PhdController::Dither reentrancy state = %d op = %d\n", ctrl.state, ctrl.settleOp));
        *errMsg = ReentrancyError("dither");
        return false;
    }

    Debug.AddLine("PhdController::Dither begins");

    bool raOnly = pFrame->GetDitherRaOnly();
    if (forceRaOnly)
    {
        // event server client wants RA only
        raOnly = true;
    }

    bool overrideDecGuideMode = false;
    DEC_GUIDE_MODE dgm = DEC_NONE;

    if (pMount && !pMount->IsStepGuider() && !raOnly)
    {
        // Check to see if we need to force raOnly or temporarily adjust dec guide mode
        Scope *scope = static_cast<Scope *>(pMount);
        dgm = scope->GetDecGuideMode();

        if (dgm != DEC_AUTO)
        {
            if ((dgm == DEC_NORTH || dgm == DEC_SOUTH) && settle.settleTimeSec != SETTLING_TIME_DISABLED)
            {
                // Temporarily override dec guide mode because user has not set ra-only
                overrideDecGuideMode = true;
            }
            else
            {
                // DEC_NONE or no settling parameters

                Debug.Write(wxString::Format("PhdController: forcing dither RA-only since Dec guide mode is %s\n",
                    Scope::DecGuideModeStr(ctrl.saveDecGuideMode)));

                raOnly = true;
            }
        }
    }

    bool error = pFrame->Dither(pixels, raOnly);
    if (error)
    {
        Debug.AddLine("PhdController::Dither pFrame->Dither failed");
        *errMsg = _T("Dither error");
        return false;
    }

    ctrl.settleOp = OP_DITHER;
    ctrl.settle = settle;
    ctrl.overrideDecGuideMode = overrideDecGuideMode;
    ctrl.saveDecGuideMode = dgm;
    SETSTATE(STATE_SETTLE_BEGIN);
    UpdateControllerState();

    return true;
}

bool PhdController::Dither(double pixels, int settleFrames, wxString *errMsg)
{
    SettleParams settle;

    settle.tolerancePx = 99.;
    settle.settleTimeSec = SETTLING_TIME_DISABLED;
    settle.timeoutSec = SETTLING_TIME_DISABLED;
    settle.frames = settleFrames;

    return Dither(pixels, false, settle, errMsg);
}

bool PhdController::DitherCompat(double pixels, wxString *errMsg)
{
    AbortController("manual or phd1-style dither");

    enum { SETTLE_FRAMES = 10 };

    return Dither(pixels, SETTLE_FRAMES, errMsg);
}

void PhdController::AbortController(const wxString& reason)
{
    if (ctrl.state != STATE_IDLE)
    {
        do_fail(reason);
        UpdateControllerState();
    }
}

static bool all_gear_connected(void)
{
    return pCamera && pCamera->Connected &&
        (!pMount || pMount->IsConnected()) &&
        (!pSecondaryMount || pSecondaryMount->IsConnected());
}

static void do_notify(void)
{
    if (ctrl.succeeded)
    {
        Debug.AddLine("PhdController complete: success");
        EvtServer.NotifySettleDone(wxEmptyString, ctrl.settleFrameCount, ctrl.droppedFrameCount);
        GuideLog.NotifySettlingStateChange("Settling complete");
    }
    else
    {
        Debug.AddLine(wxString::Format("PhdController complete: fail: %s", ctrl.errorMsg));
        EvtServer.NotifySettleDone(ctrl.errorMsg, ctrl.settleFrameCount, ctrl.droppedFrameCount);
        GuideLog.NotifySettlingStateChange("Settling failed");
    }

    if (pMount)
        pMount->NotifyGuidingDitherSettleDone(ctrl.succeeded);
}

static bool start_capturing(void)
{
    if (!pCamera || !pCamera->Connected)
    {
        return false;
    }

    pFrame->pGuider->Reset(true); // invalidate current position, etc.
    pFrame->pGuider->ForceFullFrame(); // we need a full frame to auto-select a star
    pFrame->ResetAutoExposure();
    pFrame->StartCapturing();

    return true;
}

static bool start_guiding(void)
{
    bool error = pFrame->StartGuiding();
    return !error;
}

static bool IsAoBumpInProgress()
{
    return pMount && pMount->IsStepGuider() && static_cast<StepGuider *>(pMount)->IsBumpInProgress();
}

bool PhdController::CanGuide(wxString *error)
{
    if (!all_gear_connected())
    {
        *error = _T("all equipment must be connected first");
        return false;
    }
    return true;
}

void PhdController::UpdateControllerState(void)
{
    bool done = false;

    while (!done)
    {
        switch (ctrl.state) {
        case STATE_IDLE:
            done = true;
            break;

        case STATE_SETUP:
            Debug.AddLine("PhdController: setup");
            ctrl.haveSaveSticky = false;
            ctrl.autoFindAttemptsRemaining = 3;
            ctrl.overrideDecGuideMode = false;      // guide stop/start with no dithering
            SETSTATE(STATE_ATTEMPT_START);
            break;

        case STATE_ATTEMPT_START: {

            wxString err;

            if (!CanGuide(&err))
            {
                Debug.Write(wxString::Format("PhdController: not ready: %s\n", err));
                do_fail(err);
            }
            else if (pFrame->pGuider->IsCalibratingOrGuiding())
            {
                if (ctrl.forceCalibration)
                {
                    SETSTATE(STATE_CALIBRATE);
                }
                else
                {
                    GUIDER_STATE state = pFrame->pGuider->GetState();
                    Debug.Write(wxString::Format("PhdController: guider state = %d\n", state));
                    if (state == STATE_CALIBRATED || state == STATE_GUIDING)
                    {
                        SETSTATE(STATE_SETTLE_BEGIN);
                    }
                    else
                    {
                        SETSTATE(STATE_CALIBRATION_WAIT);
                        done = true;
                    }
                }
            }
            else if (!pFrame->CaptureActive)
            {
                Debug.AddLine("PhdController: start capturing");
                if (!start_capturing())
                {
                    do_fail(_T("unable to start capturing"));
                    break;
                }
                SETSTATE(STATE_SELECT_STAR);
                done = true;
            }
            else if (pFrame->pGuider->GetState() == STATE_SELECTED)
            {
                SETSTATE(STATE_CALIBRATE);
            }
            else
            {
                // capture is active, no star selected
                SETSTATE(STATE_SELECT_STAR);

                // if auto-exposure is enabled, reset to max exposure duration
                // and wait for the next camera frame
                if (pFrame->GetAutoExposureCfg().enabled)
                {
                    pFrame->ResetAutoExposure();
                    done = true;
                }
            }
            break;
        }

        case STATE_SELECT_STAR: {
            bool error = pFrame->AutoSelectStar(ctrl.roi);
            if (error)
            {
                Debug.Write(wxString::Format("auto find star failed, attempts remaining = %d\n", ctrl.autoFindAttemptsRemaining));
                if (--ctrl.autoFindAttemptsRemaining == 0)
                {
                    do_fail(_T("failed to find a suitable guide star"));
                }
                else
                {
                    pFrame->pGuider->Reset(true);
                    SETSTATE(STATE_ATTEMPT_START);
                    done = true;
                }
            }
            else
            {
                SETSTATE(STATE_WAIT_SELECTED);
                ctrl.waitSelectedRemaining = 3;
                done = true;
            }
            break;
        }

        case STATE_WAIT_SELECTED:
            if (pFrame->pGuider->GetState() == STATE_SELECTED)
            {
                SETSTATE(STATE_CALIBRATE);
            }
            else
            {
                Debug.Write(wxString::Format("waiting for star selected, attempts remaining = %d\n", ctrl.waitSelectedRemaining));
                if (--ctrl.waitSelectedRemaining == 0)
                {
                    SETSTATE(STATE_ATTEMPT_START);
                }
                done = true;
            }
            break;

        case STATE_CALIBRATE:
            if (ctrl.forceCalibration)
            {
                Debug.AddLine("PhdController: clearing calibration");
                if (pMount)
                    pMount->ClearCalibration();
                if (pSecondaryMount)
                    pSecondaryMount->ClearCalibration();
            }

            if ((pMount && !pMount->IsCalibrated()) ||
                (pSecondaryMount && !pSecondaryMount->IsCalibrated()))
            {
                Debug.AddLine("PhdController: start calibration");

                ctrl.saveSticky = pFrame->pGuider->LockPosIsSticky();
                ctrl.haveSaveSticky = true;
                pFrame->pGuider->SetLockPosIsSticky(true);

                if (!start_guiding())
                {
                    pFrame->pGuider->SetLockPosIsSticky(ctrl.saveSticky);
                    do_fail(_T("could not start calibration"));
                    break;
                }

                SETSTATE(STATE_CALIBRATION_WAIT);
                done = true;
            }
            else
            {
                SETSTATE(STATE_GUIDE);
            }
            break;

        case STATE_CALIBRATION_WAIT:
            if ((!pMount || pMount->IsCalibrated()) &&
                (!pSecondaryMount || pSecondaryMount->IsCalibrated()))
            {
                if (ctrl.haveSaveSticky)
                    pFrame->pGuider->SetLockPosIsSticky(ctrl.saveSticky);

                SETSTATE(STATE_SETTLE_BEGIN);
            }
            else
                done = true;
            break;

        case STATE_GUIDE:
            if (!start_guiding())
            {
                do_fail(_T("could not start guiding"));
                break;
            }
            SETSTATE(STATE_SETTLE_BEGIN);
            done = true;
            break;

        case STATE_SETTLE_BEGIN:
            EvtServer.NotifySettleBegin();
            GuideLog.NotifySettlingStateChange("Settling started");
            if (ctrl.overrideDecGuideMode)
            {
                Debug.Write(wxString::Format("PhdController: setting Dec guide mode to %s for dither settle\n",
                                             Scope::DecGuideModeStr(DEC_AUTO)));
                TheScope()->SetDecGuideMode(DEC_AUTO);
            }
            ctrl.settlePriorFrameInRange = false;
            ctrl.settleFrameCount = ctrl.droppedFrameCount = 0;
            ctrl.settleTimeout->Start();
            SETSTATE(STATE_SETTLE_WAIT);
            done = true;
            break;

        case STATE_SETTLE_WAIT: {
            bool lockedOnStar = pFrame->pGuider->IsLocked();
            double currentError = pFrame->CurrentGuideError();
            bool inRange = lockedOnStar && currentError <= ctrl.settle.tolerancePx;
            bool aoBumpInProgress = IsAoBumpInProgress();
            long timeInRange = 0;

            ++ctrl.settleFrameCount;

            if (!lockedOnStar)
                ++ctrl.droppedFrameCount;

            Debug.Write(wxString::Format("PhdController: settling, locked = %d, distance = %.2f (%.2f) aobump = %d frame = %d / %d\n",
                                         lockedOnStar, currentError, ctrl.settle.tolerancePx, aoBumpInProgress, ctrl.settleFrameCount,
                                         ctrl.settle.frames));

            if (ctrl.settleFrameCount >= ctrl.settle.frames)
            {
                ctrl.succeeded = true;
                SETSTATE(STATE_FINISH);
                break;
            }

            if (inRange)
            {
                if (!ctrl.settlePriorFrameInRange)
                {
                    // first frame
                    if (ctrl.settle.settleTimeSec <= 0)
                    {
                        ctrl.succeeded = true;
                        SETSTATE(STATE_FINISH);
                        break;
                    }
                    ctrl.settleInRange->Start();
                }
                else if (((timeInRange = ctrl.settleInRange->Time()) / 1000) >= ctrl.settle.settleTimeSec && !aoBumpInProgress)
                {
                    ctrl.succeeded = true;
                    SETSTATE(STATE_FINISH);
                    break;
                }
            }
            if ((ctrl.settleTimeout->Time() / 1000) >= ctrl.settle.timeoutSec)
            {
                do_fail(_T("timed-out waiting for guider to settle"));
                break;
            }
            EvtServer.NotifySettling(currentError, (double)timeInRange / 1000., ctrl.settle.settleTimeSec, lockedOnStar);
            ctrl.settlePriorFrameInRange = inRange;
            done = true;
            break;
        }

        case STATE_FINISH:
            if (ctrl.overrideDecGuideMode)
            {
                Debug.Write(wxString::Format("PhdController: restore Dec guide mode to %s after dither\n",
                                             Scope::DecGuideModeStr(ctrl.saveDecGuideMode)));
                TheScope()->SetDecGuideMode(ctrl.saveDecGuideMode);
                ctrl.overrideDecGuideMode = false;
            }
            do_notify();
            SETSTATE(STATE_IDLE);
            done = true;
            break;
        }
    }
}
