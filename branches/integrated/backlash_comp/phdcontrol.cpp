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
    bool settlePriorFrameInRange;
    wxStopWatch *settleTimeout;
    wxStopWatch *settleInRange;
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

#define SETSTATE(newstate) do { \
    Debug.AddLine("PhdController: newstate " #newstate); \
    ctrl.state = newstate; \
} while (false)

static wxString ReentrancyError(const char *op)
{
    return wxString::Format("Cannot initiate %s while %s is in progress", op, ctrl.settleOp == OP_DITHER ? "dither" : "guide");
}

bool PhdController::Guide(bool recalibrate, const SettleParams& settle, wxString *error)
{
    if (ctrl.state != STATE_IDLE)
    {
        Debug.AddLine("PhdController::Guide reentrancy state = %d op = %d", ctrl.state, ctrl.settleOp);
        *error = ReentrancyError("guide");
        return false;
    }

    Debug.AddLine("PhdController::Guide begins");
    ctrl.forceCalibration = recalibrate;
    ctrl.settleOp = OP_GUIDE;
    ctrl.settle = settle;
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

bool PhdController::Dither(double pixels, bool raOnly, const SettleParams& settle, wxString *errMsg)
{
    if (ctrl.state != STATE_IDLE)
    {
        Debug.AddLine("PhdController::Dither reentrancy state = %d op = %d", ctrl.state, ctrl.settleOp);
        *errMsg = ReentrancyError("dither");
        return false;
    }

    Debug.AddLine("PhdController::Dither begins");

    bool error = pFrame->Dither(pixels, raOnly);
    if (error)
    {
        Debug.AddLine("PhdController::Dither pFrame->Dither failed");
        *errMsg = _T("Dither error");
        return false;
    }

    ctrl.settleOp = OP_DITHER;
    ctrl.settle = settle;
    SETSTATE(STATE_SETTLE_BEGIN);
    UpdateControllerState();

    return true;
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
        EvtServer.NotifySettleDone(wxEmptyString);
        GuideLog.NotifySettlingStateChange("Settling complete");
    }
    else
    {
        Debug.AddLine(wxString::Format("PHDController complete: fail: %s", ctrl.errorMsg));
        EvtServer.NotifySettleDone(ctrl.errorMsg);
        GuideLog.NotifySettlingStateChange("Settling failed");
    }
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
            SETSTATE(STATE_ATTEMPT_START);
            break;

        case STATE_ATTEMPT_START:

            if (!all_gear_connected())
            {
                do_fail(_T("all equipment must be connected first"));
            }
            else if (pFrame->pGuider->IsCalibratingOrGuiding())
            {
                GUIDER_STATE state = pFrame->pGuider->GetState();
                Debug.AddLine("PhdController: guider state = %d", state);
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

        case STATE_SELECT_STAR: {
            bool error = pFrame->pGuider->AutoSelect();
            if (error)
            {
                Debug.AddLine("auto find star failed, attempts remaining = %d", ctrl.autoFindAttemptsRemaining);
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
                Debug.AddLine("waiting for star selected, attempts remaining = %d", ctrl.waitSelectedRemaining);
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
            ctrl.settlePriorFrameInRange = false;
            ctrl.settleTimeout->Start();
            SETSTATE(STATE_SETTLE_WAIT);
            GuideLog.NotifySettlingStateChange("Settling started");
            done = true;
            break;

        case STATE_SETTLE_WAIT: {
            bool lockedOnStar = pFrame->pGuider->IsLocked();
            double currentError = pFrame->pGuider->CurrentError();
            bool inRange = lockedOnStar && currentError <= ctrl.settle.tolerancePx;
            bool aoBumpInProgress = IsAoBumpInProgress();
            long timeInRange = 0;

            Debug.AddLine("PhdController: settling, locked = %d, distance = %.2f (%.2f) aobump = %d", lockedOnStar, currentError,
                ctrl.settle.tolerancePx, aoBumpInProgress);

            if (inRange)
            {
                if (!ctrl.settlePriorFrameInRange)
                {
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
            EvtServer.NotifySettling(currentError, (double) timeInRange / 1000., ctrl.settle.settleTimeSec);
            ctrl.settlePriorFrameInRange = inRange;
            done = true;
            break;
        }

        case STATE_FINISH:
            do_notify();
            SETSTATE(STATE_IDLE);
            done = true;
            break;
        }
    }
}
