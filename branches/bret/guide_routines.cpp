/*
 *  guide_routines.cpp
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
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

float SIGN(float x) {
	if (x > 0.0) return 1.0;
	else if (x < 0.0) return -1.0;
	else return 0.0;
}

void LogGuideEntry(int frame_index,
                   double elapsed_time,
                   double dX, double dY, double theta,
                   double RaDur, double RaDist,
                   double DecDur, double DecDist,
                   double mass,
                   int ErrorCode
        )
{
    if (Log_Data) {
        wxString logline = wxString::Format(_T("%d,%.3f,%.2f,%.2f,%.1f,%.1f,%.2f,%.1f,%.2f,%.2f,%d"),
                                            frame_index,
                                            elapsed_time,
                                            dX, dY, theta,
                                            RaDur, RaDist, 
                                            DecDur, DecDist, 
                                            mass, 
                                            ErrorCode);

        LogFile->AddLine(logline);
        LogFile->Write();
    }
}

void MyFrame::GuidestarLost(void)
{
    // signal lost star

    wxColor DefaultColor = GetBackgroundColour();

    SetBackgroundColour(wxColour(64,0,0));
    Refresh();
    wxTheApp->Yield();
    wxBell();
    wxMilliSleep(100);
    SetBackgroundColour(DefaultColor);
    Refresh();
}

bool MyFrame::GuideCapture(LOG &Debug)
{
    bool bError = false;

    Debug << _T("\nCapturing - ");

    try
    {
        if (Time_lapse)
            wxMilliSleep(Time_lapse);
        int ExpDur = RequestedExposureDuration();
        CurrentFullFrame.InitDate();
        CurrentFullFrame.ImgExpDur = ExpDur;
        bError = CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame);
    }
    catch (...) 
    {
        wxMessageBox(_T("Exception thrown during image capture - bailing"));
        Debug << _T("Camera threw an exception during capture\n");
        bError = true;
    }

    Debug << _T("Done\n");

    if (!bError)
    {
        if  (GuideCameraPrefs::NR_mode)
        {
            Debug << _T("Calling NR - ");
            if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
                QuickLRecon(CurrentFullFrame);
            else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
                Median3(CurrentFullFrame);
            Debug << _T("Done\n");
        }

        SetStatusText(_T(""),1);

        Debug << _T("Finding star - ");
        GuideStar.Find(CurrentFullFrame); // track it
        Debug << _T("Done (WasFound=") << GuideStar.WasFound() << _T(")\n");

        if (GuideStar.WasFound())
        {
            double dX = pLockPoint->dx(GuideStar.pCenter);
            double dY = pLockPoint->dy(GuideStar.pCenter);

            if ( ((fabs(dX) > SearchRegion) || (fabs(dY)>SearchRegion)) && 
                 !DisableGuideOutput &&
                 !ManualLock && Dec_guide)
            { 
                // likely lost lock -- stay here
                GuideStar.SetError(Star::STAR_LARGEMOTION);
            }
        }
    }

    return bError;
}

void MyFrame::StepGuide(LOG &Debug)
{
    try
    {
        wxStopWatch swatch;
        double PULSE_GUIDE_THRESHOLD = 0.80; // Todo: make a brain option
        double STEP_BUMP_SIZE = 0.60;        // Todo: make a brain option
        bool bMoveScope = false;

        if (!pStepGuider->IsCalibrated() && pStepGuider->Calibrate())
        {
            throw ERROR_INFO("Unable to calibrate step guider");
        }

        swatch.Start(0);  // start the clock

        for(int frame_index=1;!Abort;frame_index++)
        {
             while (Paused) 
             {
                wxMilliSleep(250);
                wxTheApp->Yield();
            }

            if (GuideCapture(Debug))
            {
                Abort = 1;
                break; // we are done if the capture failed
            }

            double elapsed_time = (float) swatch.Time() / 1000.0;

            if (!GuideStar.WasFound()) {
                GuidestarLost();
                continue; // don't do anything else since we don't have a star
            }

            // if we get here, we found the guide star
            double dX = pLockPoint->dx(GuideStar.pCenter);
            double dY = pLockPoint->dy(GuideStar.pCenter);

            // dist b/n lock and star
            double hyp = pLockPoint->Distance(GuideStar.pCenter);
            double theta = pLockPoint->Angle(GuideStar.pCenter);

            Debug << wxString::Format(_T("distances: dX=%.2f dY=%.2f hyp=%.2f theta=%.2f theta2=%.2f\n"), dX, dY, hyp, theta);

            double RaDist = cos(pStepGuider->RaAngle() - theta)*hyp;
            double DecDist = cos(pStepGuider->DecAngle() - theta)*hyp;

            // step guiders are quantized devices. Figure out how many steps
            // we need to move
            int RaSteps  = ROUND(RaDist/pStepGuider->RaRate());
            int DecSteps = ROUND(DecDist/pStepGuider->DecRate());

            GUIDE_DIRECTION RaDirection = RaSteps > 0.0 ? EAST : WEST;
            GUIDE_DIRECTION DecDirection = DecSteps > 0.0 ? SOUTH : NORTH;

            Debug << wxString::Format(_T("Stepping: RA=%.2f steps=%d Dec=%.2f steps=%d\n"), RaDist, RaSteps, DecDist, DecSteps);

            if (RaSteps > pStepGuider->MaxStep(RaDirection))
            {
                RaSteps = pStepGuider->MaxStep(RaDirection);
            }

            if (DecSteps > pStepGuider->MaxStep(DecDirection))
            {
                DecSteps = pStepGuider->MaxStep(DecDirection);
            }

            SetStatusText(wxString::Format(_T("Stepping: RA=%d Dec=%d"),RaSteps, DecSteps),1);

            if (!DisableGuideOutput)
            {
                Debug << _T("Stepping RA: Steps=") << RaSteps << _T("\n");
                pStepGuider->Step(RaDirection, RaSteps);

                Debug << _T("Stepping Dec: Steps=") << DecSteps << _T("\n");
                pStepGuider->Step(DecDirection, DecSteps);
            }

            LogGuideEntry(frame_index,elapsed_time,dX,dY,theta, RaSteps, RaDist, DecSteps, DecDist, GuideStar.Mass, GuideStar.Result);

            /*
             * See if we need to bump the scope because we are getting too
             * close to the edge of the step guider. 
             *
             * The only interesting issue is how far to move and in what sized
             * steps. We really want to move far enough to get back into the 
             * center of the step guider, but if we do it all in one move, we 
             * will cause misshaped stars
             *
             * Instead, once we get past PULSE_GUIDE_THREHOLD of the way through
             * the step guider travel, we start bumping back by STEP_BUMP_SIZE of 
             * one step of the step guider until we get within PULSE_GUIDE_THREHOLD/2
             *
             * Hopefully this will allow the step guider to move it back on the next
             * cycle to keep the stars round
             */

            if (pStepGuider->Position(RaDirection) > PULSE_GUIDE_THRESHOLD*pStepGuider->MaxStep(RaDirection) ||
                pStepGuider->Position(DecDirection) > PULSE_GUIDE_THRESHOLD*pStepGuider->MaxStep(DecDirection))
            {
                Debug << _T("setting bMoveScope to true\n");
                bMoveScope = true;
            }
            else if (bMoveScope &&
                     pStepGuider->Position(RaDirection) < PULSE_GUIDE_THRESHOLD/2*pStepGuider->MaxStep(RaDirection) &&
                     pStepGuider->Position(DecDirection) < PULSE_GUIDE_THRESHOLD/2*pStepGuider->MaxStep(DecDirection))
            {
                Debug << _T("setting bMoveScope to false\n");
                bMoveScope = false;
            }

            if (bMoveScope)
            {
                // At least one of the axies has gone farther than we like. Bump
                // the scope to move the star back towards the center of the step
                // guider

                // Todo: fix this calculation. It is wrong, because it needs to deal with 
                // the difference between the angles of the step guider and the scope 
                // guider, but for now it is just a placeholder

                double ScopeRaDist  = fabs(STEP_BUMP_SIZE*pStepGuider->RaRate())/pScope->RaRate();
                double ScopeDecDist = fabs(STEP_BUMP_SIZE*pStepGuider->DecRate())/pScope->RaRate();

                double ScopeRaDur = fabs(ScopeRaDist)/pScope->RaRate();
                double ScopeDecDur = fabs(ScopeDecDist)/pScope->RaRate();

                GUIDE_DIRECTION ScopeRaDirection  = RaDist > 0.0 ? EAST : WEST;
                GUIDE_DIRECTION ScopeDecDirection = DecDist > 0.0 ? SOUTH : NORTH;

                SetStatusText(wxString::Format(_T("Bumping: RA=%.1f Dec=%.1f"), ScopeRaDur, ScopeDecDur),1);

                if (!DisableGuideOutput)
                {
                    pScope->Guide(ScopeRaDirection,(int) ScopeRaDur);	// So, guide in the RA- direction;
                    pScope->Guide(ScopeDecDirection,(int) ScopeDecDur);	// So, guide in the RA- direction;
                }
            }
        }
    }
    catch (char *ErrorMsg)
    {
        Debug << "StepGuide caught an exception, ErrorMsg=" << ErrorMsg << _T("\n");
    }

    Debug << "StepGuide Exits\n";
}

void MyFrame::ScopeGuide(LOG &Debug)
{
    try
    {
        double RaDist, RaDur, last_guide, DecDur;
        ArrayOfDbl DecDist_list, Sorted_dec_list;
        double DecDist, Curr_DecDist, Curr_Dec_Sign, Dec_History;
        bool Allow_Dec_Move = false;
        int frame_index = 0;
        wxString logline;
        float elapsed_time;
        int i;
        static int run=0;
        wxStopWatch swatch;

        Debug << _T("ScopeGuide entered\n");

        if (Log_Data) {
            wxDateTime now = wxDateTime::Now();
            LogFile->AddLine(wxString::Format(_T("Max RA dur %d, Max DEC dur %d, Star Mass delta thresh %.2f"),ScopeMaxRaDur, ScopeMaxDecDur, StarMassChangeRejectThreshold));
            LogFile->AddLine(wxString::Format(_T("RA angle %.2f, rate %.4f, aggr %.2f, hyst=%0.2f"),pScope->RaAngle(), pScope->RaRate(), RA_aggr, RA_hysteresis));
            LogFile->AddLine(wxString::Format(_T("DEC angle %.2f, rate %.4f, Dec mode %d, Algo %d, slopewt = %.2f"),pScope->DecAngle(), pScope->DecRate(), Dec_guide, Dec_algo, Dec_slopeweight));
            LogFile->AddLine(_T("Frame,Time,dx,dy,Theta,RADuration,RADistance,DECDuration,DECDistance,StarMass,ErrorCode"));
            LogFile->Write();
        }

        last_guide = 0.0;

        DecDist = 0.0;
        RaDist = 0.0;
        for (i=0; i<10; i++)  // set dec guide list to "no guide" by default
            DecDist_list.Add(0.0);
        Curr_Dec_Sign = 0.0;  // not on either side of the gear

        swatch.Start(0);  // reset the clock
        while (!Abort) {
            run++;
            Debug.Flush();
            while (Paused) {
                wxMilliSleep(250);
                wxTheApp->Yield();
            }

            frame_index++;

            if (GuideCapture(Debug))
            {
                Abort = 1;
                break; // we are done if the capture failed
            }

            elapsed_time = (float) swatch.Time() / 1000.0;

            if (!GuideStar.WasFound()) {
                GuidestarLost();
                continue; // don't do anything else since we don't have a star
            }

            // if we get here, we found the guide star
            double dX = pLockPoint->dx(GuideStar.pCenter);
            double dY = pLockPoint->dy(GuideStar.pCenter);

            // dist b/n lock and star
            double hyp = pLockPoint->Distance(GuideStar.pCenter);
            double theta = pLockPoint->Angle(GuideStar.pCenter);

            Debug << wxString::Format(_T("distances: dX=%.2f dY=%.2f hyp=%.2f theta=%.2f theta2=%.2f\n"), dX, dY, hyp, theta);

            // Do RA guiding
            Debug << _T("RA - ");

            RaDist = cos(pScope->RaAngle() - theta)*hyp;	// dist in RA star needs to move
            RaDist = (1.0 - RA_hysteresis) * RaDist + RA_hysteresis * last_guide;	// add in hysteresis

            Debug << _T(" RaDist ") << RaDist << _T(" DecDist ") << DecDist << _T("\n");

            RaDur = (fabs(RaDist)/pScope->RaRate())*RA_aggr;	// duration of pulse

            if (RaDur > (double) ScopeMaxRaDur)
                RaDur = (double) ScopeMaxRaDur;  // cap pulse length

            Debug << _T("Frame: ") << frame_index;

            if (fabs(RaDist) <= MinMotion) // not worth doing really small moves
                RaDur = 0.0;

            Debug << _T(" RaDist ") << RaDist << _T(" RaDur ") << RaDur << _T("\n");

            if (RaDur > 0.0) {
                Debug << _T("Guiding RA...");
                GUIDE_DIRECTION thisDir = RaDist > 0.0 ? EAST : WEST;

                SetStatusText(wxString::Format(_T("%c dur=%.1f dist=%.2f"),thisDir == EAST?'E':'W', RaDur,RaDist),1);

                if (!DisableGuideOutput)
                    pScope->Guide(thisDir,(int) RaDur);	// So, guide in the RA- direction;
                Debug << _T("Done\n");
            }

            last_guide  = RaDist;

            // now for DEC

            Debug << _T("Dec -");

            DecDist = cos(pScope->DecAngle() - theta)*hyp;	// dist in Dec star needs to move
            Curr_DecDist = DecDist;

            // the "obvious" duration, which might get modified by a fancier algorithm
            DecDur = fabs(DecDist)/pScope->DecRate();

            Debug << _T(" DecDist ") << DecDist << _T(" DecDur ") << DecDur << _T("\n");

            if (!Dec_guide)
            {
                DecDur = 0.0;
            }
            else
            {
                if (Dec_algo == DEC_RESISTSWITCH) { // Do Dec guide using the newer resist-switch algo
                    Debug << _T("Dec resist switch - ");

                    Allow_Dec_Move = true;

                    if (fabs(DecDist) < MinMotion)
                        Allow_Dec_Move = false;

                    // update the history list
                    DecDist_list.RemoveAt(0);
                    DecDist_list.Add(DecDist);

                    // see what history tells us
                    Dec_History = 0.0;
                    for (i=0; i<10; i++) {
                        if (fabs(DecDist_list.Item(i)) > MinMotion) // only count decent-size errors
                            Dec_History += SIGN(DecDist_list.Item(i));
                    }
                    Debug << Curr_Dec_Sign << _T(" ") << DecDist << _T(" ") << DecDur << _T(" ") << (int) Allow_Dec_Move << _T(" ") << Dec_History << _T("\n");
                    // see if on same side of Dec and if we have enough evidence to switch
                    if (Curr_Dec_Sign != SIGN(Dec_History) && Allow_Dec_Move && Dec_guide == DEC_AUTO) {
                        // think about switching
                        wxString HistString = _T("Thinking of switching - Hist: ");
                        for (i=0; i<10; i++)
                            HistString += wxString::Format(_T("%.2f "),DecDist_list.Item(i));
                        HistString += wxString::Format(_T("(%.2f)\n"),Dec_History);
                        Debug << HistString;
                        
                        Allow_Dec_Move = false;

                        if (fabs(Dec_History) < 3.0) { // not worth of switch
                            Debug << _T("..Not compelling enough\n");
                        }
                        else { // Think some more
                            if (fabs(DecDist_list.Item(0) + DecDist_list.Item(1) + DecDist_list.Item(2)) <
                                fabs(DecDist_list.Item(9) + DecDist_list.Item(8) + DecDist_list.Item(7))) {
                                Debug << _T(".. !!!! Getting worse - Switching ") << Curr_Dec_Sign << _T(" to ") << SIGN(Dec_History) << _T("\n");
                                Curr_Dec_Sign = SIGN(Dec_History);
                                Allow_Dec_Move = true;
                            }
                            else {
                                Debug << _T("..Current error less than prior error -- not switching\n");
                            }
                        }
                    }

                    if (Allow_Dec_Move && (Dec_guide == DEC_AUTO)) {
                        if (Curr_Dec_Sign != SIGN(DecDist)) {
                            Allow_Dec_Move = false;
                            Debug << _T(".. Dec move VETO .. must have overshot\n");
                        }
                    }

                    if (!Allow_Dec_Move) {
                        DecDur = 0.0;
                        Debug << _T("not enough motion\n");
                    }

                    Debug << _T("Done\n");
                }
                else if ((Dec_algo == DEC_LOWPASS) || (Dec_algo == DEC_LOWPASS2)) {// version 1, lowpass filter
                    Debug << _T("Dec lowpass - ");
                    DecDist_list.Add(DecDist);
                    if (Dec_algo == DEC_LOWPASS) {
                        // Get the median of the current 11 values in there.
                        Sorted_dec_list = DecDist_list;
                        Sorted_dec_list.Sort(dbl_sort_func);
                        Curr_DecDist = Sorted_dec_list.Item(5); // this is now the median of the 11 items in there
                        float slope = CalcSlope(DecDist_list);
                        Curr_DecDist = Curr_DecDist + Dec_slopeweight * slope;
                        if (fabs(Curr_DecDist) > fabs(DecDist)) {
                            Debug << _T(" reset CDist (") << Curr_DecDist << _T(") to dist ") << DecDist << _T(" as model of error is larger than true");
                            Curr_DecDist=DecDist;
                        }
                        DecDist_list.RemoveAt(0);  // take the first one off the list, should be back to 10 now, ready for next
                        DecDur = (fabs(Curr_DecDist)/pScope->DecRate()) / 11.0;	// since the distance is cumulative, this is what should be applied for each time, on average
                    }
                    else { // LOWPASS2 - linear regression
                        float slope = CalcSlope(DecDist_list);  // Calc slope - change per frame - over last 11
                        DecDist_list.RemoveAt(0);
                        if (fabs(DecDist) < fabs(slope)) { // Error is less than the slope, don't overshoot
                            DecDur = fabs(DecDist) / pScope->DecRate();
                            Curr_DecDist = DecDist;
                            Debug << _T("Using DecDist\n");
                        }
                        else {
                            DecDur = fabs(slope) / pScope->DecRate();  // Send one frame's worth (delta per frame)
                            Curr_DecDist = slope;
                            Debug << _T("Using slope\n");
                        }
                        wxString HistString = _T("History: ");
                        for (i=0; i<10; i++)
                            HistString += wxString::Format(_T("%.2f "),DecDist_list.Item(i));
                        Debug << HistString;
                        Debug << _T("\n   Dist=") << DecDist << _T("Cdist= ") << Curr_DecDist << _T("  Dur=") << DecDur << _T(" Slope=") << slope << _T("\n");
                    }
                    Debug << _T(" Done\n");
                }
            }

            if (DecDur > 0.0)
            {
                Debug << _T("Dec guide: dist=") << DecDist << _T(" Dur=") << DecDur << _T("\n");

                if (fabs(Curr_DecDist) > MinMotion || Dec_algo == DEC_LOWPASS2) {  // enough motion to warrant a move
                    GUIDE_DIRECTION thisDir = DecDist > 0.0 ? SOUTH : NORTH;

                    if (DecDur > (float) ScopeMaxDecDur) {
                        DecDur = (float) ScopeMaxDecDur;
                        Debug << _T("Dec move clipped to ") << DecDur << _T("\n");
                    }

                    if (DecDist != Curr_DecDist)
                        SetStatusText(wxString::Format(_T("%c dur=%.1f dist=%.2f cdist=%.2f"),thisDir==SOUTH?'S':'N', DecDur,DecDist,Curr_DecDist),1);
                    else
                        SetStatusText(wxString::Format(_T("%c dur=%.1f dist=%.2f"),thisDir==SOUTH?'S':'N', DecDur,DecDist),1);

                    if (thisDir == SOUTH && ((Dec_guide == DEC_AUTO) || (Dec_guide == DEC_SOUTH))) {
                        if (!DisableGuideOutput)
                            pScope->Guide(SOUTH,(int) DecDur);	// So, guide in the Dec- direction;
                    }
                    else if (thisDir == NORTH && ((Dec_guide == DEC_AUTO) || (Dec_guide == DEC_NORTH))){
                        if (!DisableGuideOutput)
                            pScope->Guide(NORTH,(int) DecDur);	// So, guide in the Dec- direction;
                    }
                    else { // will hit this if in north or south only mode and the direction is the opposite
                        DecDur = 0.0;
                        Debug << _T("In N or S only mode and dir is opposite\n");
                    }
                }
                else { // not enough motion
                    DecDur = 0.0;
                    Debug << _T("Not enough motion\n");
                }
            }

            LogGuideEntry(frame_index,elapsed_time,dX,dY,theta,RaDur,RaDist, DecDur, DecDist, GuideStar.Mass, GuideStar.Result);
            this->GraphLog->AppendData(dX,dY,RaDist,DecDist);
            canvas->FullFrameToDisplay();

            wxTheApp->Yield();
        }
    }
    catch (char *ErrorMsg)
    {
        Debug << "ScopeGuide caught an exception, ErrorMsg=" << ErrorMsg << _T("\n");
    }

    Debug << "ScopeGuide Exits\n";
}

void MyFrame::OnGuide(wxCommandEvent& WXUNUSED(event)) 
{
    LOG Debug((char *) "debug-guide", this->Menubar->IsChecked(MENU_DEBUG));

    try
    {
        // no mount selected -- should never happen
        if (pScope == NULL) {
            throw ERROR_INFO("pScope == NULL");
        }

        if (!pScope->IsConnected() || !GuideCameraConnected) { // must be connected to both
            wxMessageBox(_T("Both camera and mount must be connected before you attempt to guide"));
            throw ERROR_INFO("Both camera and mount must be connected before you attempt to guide");
        }

        if (canvas->State != STATE_SELECTED)  // must have a star selected and not doing something
            throw ERROR_INFO("canvas->State != STATE_SELECTED");

        if (CaptureActive) { // Looping an exposure already
            Abort = 2;
            throw ERROR_INFO("Already looping an exposure");
        }

        // no cal or user wants to recal
        if (!pScope->IsCalibrated() && pScope->Calibrate()){
            throw ERROR_INFO("Unable to calibrate scope");
        }

        if (!GuideStar.WasFound()) {
            wxMessageBox(_T("Please select a guide star before attempting to guide"));
            throw ERROR_INFO("Please select a guide star before attempting to guide");
        }

        if (!ManualLock) { // Not in the manually-specified lock position -- resync lock pos
            UpdateLockPoint(GuideStar.pCenter);
        }

        wxDateTime now = wxDateTime::Now();
        Debug << _T("\n\nDebug PHD Guide ") << VERSION << _T(" ") <<  now.FormatDate() << _T(" ") <<  now.FormatTime() << _T("\n");
        Debug << _T("Machine: ") << wxGetOsDescription() << _T(" ") << wxGetUserName() << _T("\n");
        Debug << _T("Camera: ") << CurrentGuideCamera->Name << _T("\n");
        Debug << _T("Dur: ") << RequestedExposureDuration() << _T(" NR: ") << GuideCameraPrefs::NR_mode << _T(" Dark: ") << CurrentGuideCamera->HaveDark << _T("\n");

        if (Log_Data)
        {
            if (LogFile->Exists())
                LogFile->Open();
            else
                LogFile->Create();
            wxDateTime now = wxDateTime::Now();
            LogFile->AddLine(wxString::Format(_T("PHD Guide %s  -- "),VERSION) + now.FormatDate()  + _T(" ") + now.FormatTime());
            LogFile->AddLine(_T("Guiding begun"));
            LogFile->AddLine(wxString::Format(_T("lock %.1f %.1f, star %.1f %.1f, Min Motion %.2f"),pLockPoint->X,pLockPoint->Y,GuideStar.pCenter->X,GuideStar.pCenter->Y, MinMotion));
            LogFile->Write();
        }

        canvas->State = STATE_GUIDING_LOCKED;
        SetStatusText(_T("Guiding"));

        CurrentGuideCamera->InitCapture();
        Loop_Button->Enable(false);
        Guide_Button->Enable(false);
        Cam_Button->Enable(false);
        Scope_Button->Enable(false);
        Brain_Button->Enable(false);

        CaptureActive = true;

        if (pStepGuider)
        {
            StepGuide(Debug);
        }
        else
        {
            ScopeGuide(Debug);
        }

        CaptureActive = false;

        Loop_Button->Enable(true);
        Guide_Button->Enable(true);
        Cam_Button->Enable(true);
        Scope_Button->Enable(true);
        Brain_Button->Enable(true);

        Abort = 0;
        canvas->State = STATE_NONE;
        canvas->Refresh();
        SetStatusText(_T("Guiding stopped"));
        SetStatusText(_T(""),1);

        if (Log_Data)
        {
            LogFile->Write();
            LogFile->Close();
        }

        Debug << _T("Guiding finished\n");
    }
    catch (char *ErrorMsg)
    {
        Debug << _T("OnGuide caught an exception, ErrorMsg=") << ErrorMsg << _T("\n");
    }
}
