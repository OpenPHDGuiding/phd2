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

void MyFrame::OnGuide(wxCommandEvent& WXUNUSED(event)) {

    LOG Debug((char *) "guide", this->Menubar->IsChecked(MENU_DEBUG));

    try
    {
        double RA_dist, hyp, theta, RA_dur, last_guide, Dec_dur;
        ArrayOfDbl Dec_dist_list, Sorted_dec_list;
        double Dec_dist, Curr_Dec_dist, Curr_Dec_Side, Dec_History;
        bool Allow_Dec_Move = false;
        int frame_index = 1;
        int StarErrorCode = 0;
        wxString logline;
        float elapsed_time;
        wxColor DefaultColor = GetBackgroundColour();
        int i;
        static int run=0;
        wxStopWatch swatch;
        double ExpDur = RequestedExposureDuration();

        if (!pScope->IsConnected() || !GuideCameraConnected) { // must be connected to both
            wxMessageBox(_T("Both camera and mount must be connected before you attempt to guide"));
            throw ERROR_INFO("Both camera and mount must be connected before you attempt to guide");
        }

        if (!FoundStar) {
            wxMessageBox(_T("Please select a guide star before attempting to guide"));
            throw ERROR_INFO("Please select a guide star before attempting to guide");
        }

        if (canvas->State != STATE_SELECTED)  // must have a star selected and not doing something
            throw ERROR_INFO("canvas->State != STATE_SELECTED");

        if (CaptureActive) { // Looping an exposure already
            Abort = 2;
            throw ERROR_INFO("Already looping an exposure");
        }

        // no mount selected -- should never happen
        if (pScope == NULL) {
            throw ERROR_INFO("pScope == NULL");
        }

        // no cal or user wants to recal
        if (!pScope->IsCalibrated() && pScope->Calibrate()){
            throw ERROR_INFO("Unable to calibrate");
        }

        if (!ManualLock) { // Not in the manually-specified lock position -- resync lock pos
            LockX=StarX;
            LockY=StarY;
            dX = dY = 0;
        }

        wxDateTime now = wxDateTime::Now();
        Debug << _T("\n\nDebug PHD Guide ") << VERSION << _T(" ") <<  now.FormatDate() << _T(" ") <<  now.FormatTime() << _T("\n");
        Debug << _T("Machine: ") << wxGetOsDescription() << _T(" ") << wxGetUserName() << _T("\n");
        Debug << _T("Camera: ") << CurrentGuideCamera->Name << _T("\n");
        Debug << _T("Dur: ") << ExpDur << _T(" NR: ") << GuideCameraPrefs::NR_mode << _T(" Dark: ") << CurrentGuideCamera->HaveDark << _T("\n");
        Debug << _T("Guiding entered\n");

        CaptureActive = true;
        canvas->State = STATE_GUIDING_LOCKED;
 //       SetStatusText(_T("Guiding"));
        if (Log_Data) {
            if (LogFile->Exists()) LogFile->Open();
            else LogFile->Create();
            wxDateTime now = wxDateTime::Now();
            LogFile->AddLine(wxString::Format(_T("PHD Guide %s  -- "),VERSION) + now.FormatDate()  + _T(" ") + now.FormatTime());
            LogFile->AddLine(_T("Guiding begun"));
            LogFile->AddLine(wxString::Format(_T("lock %.1f %.1f, star %.1f %.1f, Min Motion %.2f"),LockX,LockY,StarX,StarY, MinMotion));
            LogFile->AddLine(wxString::Format(_T("Max RA dur %d, Max DEC dur %d, Star Mass delta thresh %.2f"),Max_RA_Dur, Max_Dec_Dur, StarMassChangeRejectThreshold));
            LogFile->AddLine(wxString::Format(_T("RA angle %.2f, rate %.4f, aggr %.2f, hyst=%0.2f"),pScope->RaAngle(), pScope->RaRate(), RA_aggr, RA_hysteresis));
            LogFile->AddLine(wxString::Format(_T("DEC angle %.2f, rate %.4f, Dec mode %d, Algo %d, slopewt = %.2f"),pScope->DecAngle(), pScope->DecRate(), Dec_guide, Dec_algo, Dec_slopeweight));
            LogFile->AddLine(_T("Frame,Time,dx,dy,Theta,RADuration,RADistance,DECDuration,DECDistance,StarMass,ErrorCode"));
            LogFile->Write();
        }
        last_guide = 0.0;
        CurrentGuideCamera->InitCapture();
        Loop_Button->Enable(false);
        Guide_Button->Enable(false);
        Cam_Button->Enable(false);
        Scope_Button->Enable(false);
        Brain_Button->Enable(false);

        Dec_dist = 0.0;
        RA_dist = 0.0;
        for (i=0; i<10; i++)  // set dec guide list to "no guide" by default
            Dec_dist_list.Add(0.0);
        Curr_Dec_Side = 0.0;  // not on either side of the gear

        swatch.Start(0);  // reset the clock
        while (!Abort) {
            run++;
            Debug.Flush();
            while (Paused) {
                wxMilliSleep(250);
                wxTheApp->Yield();
            }
            Debug << _T("Capturing - ");
            try {
                ExpDur = RequestedExposureDuration();
                CurrentFullFrame.InitDate();
                CurrentFullFrame.ImgExpDur = ExpDur;
                if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame)) {
                    Abort = 1;
                    break;
                }
            }
            catch (...) {
                wxMessageBox(_T("Exception thrown during image capture - bailing"));
                Debug << _T("Camera threw an exception during capture\n");
                Abort = 1;
                break;
            }

            if (!Abort) {
                Debug << _T("Done\n");
                if  (GuideCameraPrefs::NR_mode) {
                    Debug << _T("Calling NR - ");
                    if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
                        QuickLRecon(CurrentFullFrame);
                    else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
                        Median3(CurrentFullFrame);
                    Debug << _T("Done\n");
                }
                SetStatusText(_T(""),1);
                logline = _T("");
                Debug << _T("Finding star - ");
                StarErrorCode = FindStar(CurrentFullFrame); // track it
                Debug << _T("Done (") << FoundStar << _T(")\n");
                elapsed_time = (float) swatch.Time() / 1000.0;
                if ( ((fabs(dX) > SearchRegion) || (fabs(dY)>SearchRegion)) && !DisableGuideOutput && !ManualLock && Dec_guide) { // likely lost lock -- stay here
                    StarX = LockX;
                    StarY = LockY;
                    dX = 0.0;
                    dY = 0.0;
                    FoundStar = false;
                    StarErrorCode = STAR_LARGEMOTION;
                    // sound the alarm and wait here
                }
                if (!FoundStar) {
					SetStatusText(_T("Guiding") + wxString::Format(" ??,??"));
                    SetBackgroundColour(wxColour(64,0,0));
                    Refresh();
                    wxTheApp->Yield();
                    wxBell();
                    wxMilliSleep(100);
                    SetBackgroundColour(DefaultColor);
                    Refresh();
                }
				else 
					SetStatusText(_T("Guiding") + wxString::Format(" %.1f,%.1f",StarX,StarY));
   
                Debug << _T("Calculating - RA ");
                if (dX == 0.0) dX = 0.000001;
                if (dX > 0.0) theta = atan(dY/dX);		// theta = angle star is at
                else if (dY >= 0.0) theta = atan(dY/dX) + PI;
                else theta = atan(dY/dX) - PI;
                hyp = sqrt(dX*dX+dY*dY);	// dist b/n lock and star
                // Do RA guide
                RA_dist = cos(pScope->RaAngle() - theta)*hyp;	// dist in RA star needs to move
                RA_dist = (1.0 - RA_hysteresis) * RA_dist + RA_hysteresis * last_guide;	// add in hysteresis
                RA_dur = (fabs(RA_dist)/pScope->RaRate())*RA_aggr;	// duration of pulse
                if (Dec_guide)
                    CurrentError = hyp;
                else
                    CurrentError = fabs(RA_dist);
                if (RA_dur > (double) Max_RA_Dur) RA_dur = (double) Max_RA_Dur;  // cap pulse length
                Debug << _T("Frame: ") << frame_index << _T("\n");
                if ((fabs(RA_dist) > MinMotion) && FoundStar){ // not worth <0.25 pixel moves
                    Debug << _T("- Guiding RA ");
                    if (RA_dist > 0.0) {
                        SetStatusText(wxString::Format(_T("E dur=%.1f dist=%.2f"),RA_dur,RA_dist),1);
                        if (!DisableGuideOutput)
							pScope->Guide(EAST,(int) RA_dur);	// So, guide in the RA- direction;
                        if (Log_Data) {
                            logline = wxString::Format(_T("%d,%.3f,%.2f,%.2f,%.1f,%.1f,%.2f"),frame_index,elapsed_time,dX,dY,theta,RA_dur,RA_dist);
                        }
                    }
                    else {
                        SetStatusText(wxString::Format(_T("W dur=%.1f dist=%.2f"),RA_dur,RA_dist),1);
                        if (!DisableGuideOutput)
							pScope->Guide(WEST,(int) RA_dur);
                        if (Log_Data) {
                            logline = wxString::Format(_T("%d,%.3f,%.2f,%.2f,%.1f,%.1f,%.2f"),frame_index,elapsed_time,dX,dY,theta,RA_dur,RA_dist);
                        }
                    }
                }
                else {
                    if (Log_Data) {
                        logline = wxString::Format(_T("%d,%.3f,%.2f,%.2f,%.1f,0.0,%.2f"),frame_index,elapsed_time,dX,dY,theta,RA_dist);
                    }
                }
                last_guide  = RA_dist;
                Debug << _T("Done\n");
                if ((Dec_guide) && (Dec_algo == DEC_RESISTSWITCH) && FoundStar) { // Do Dec guide using the newer resist-switch algo
                    Debug << _T("Dec resist switch - \n");
                    Dec_dist = cos(pScope->DecAngle() - theta)*hyp;	// dist in Dec star needs to move
                    Dec_dur = fabs(Dec_dist)/pScope->DecRate();
    //				if (fabs(Dec_dist) < 0.5)   // if drift is small, assume noisy and don't include in history - set to 0
    //					Dec_dist_list.Add(0.0);
    //				else
                        Dec_dist_list.Add(Dec_dist);
    //					Dec_dist_list.Add(SIGN(Dec_dist));
                    Dec_dist_list.RemoveAt(0);
                    if (fabs(Dec_dist) < MinMotion)
                        Allow_Dec_Move = false;
                    else
                        Allow_Dec_Move = true; // so far, assume we'll allow the movement
                    Dec_History = 0.0;
                    for (i=0; i<10; i++) {
                        if (fabs(Dec_dist_list.Item(i)) > MinMotion) // only count decent-size errors
                            Dec_History += SIGN(Dec_dist_list.Item(i));
                    }
    //					Dec_History += Dec_dist_list.Item(i);
                    Debug << Curr_Dec_Side << _T(" ") << Dec_dist << _T(" ") << Dec_dur << _T(" ") << (int) Allow_Dec_Move << _T(" ") << Dec_History << _T("\n");
                    // see if on same side of Dec and if we have enough evidence to switch
                    if ( ((Curr_Dec_Side == 0) || (Curr_Dec_Side == (-1.0 * SIGN(Dec_History)))) &&
                        Allow_Dec_Move && (Dec_guide == DEC_AUTO)) { // think about switching
                        wxString HistString = _T("Thinking of switching - Hist: ");
                        for (i=0; i<10; i++)
                            HistString += wxString::Format(_T("%.2f "),Dec_dist_list.Item(i));
                        HistString += wxString::Format(_T("(%.2f)\n"),Dec_History);
                        Debug << HistString;
                        
                        if (fabs(Dec_History) < 3.0) { // not worth of switch
                            Allow_Dec_Move = false;
                            Debug << _T("..Not compelling enough\n");
                        }
                        else { // Think some more
                            if (fabs(Dec_dist_list.Item(0) + Dec_dist_list.Item(1) + Dec_dist_list.Item(2)) <
                                fabs(Dec_dist_list.Item(9) + Dec_dist_list.Item(8) + Dec_dist_list.Item(7))) {
                                Debug << _T(".. !!!! Getting worse - Switching ") << Curr_Dec_Side << _T(" to ") << SIGN(Dec_History) << _T("\n");
                                Curr_Dec_Side = SIGN(Dec_History);
                                Allow_Dec_Move = true;
                            }
                            else {
                                Allow_Dec_Move = false;
                                Debug << _T("..Current error less than prior error -- not switching\n");
                            }
                        }
                    }
                    if (Allow_Dec_Move && (Dec_guide == DEC_AUTO)) {
                        if (Curr_Dec_Side != SIGN(Dec_dist)) {
                            Allow_Dec_Move = false;
                            Debug << _T(".. Dec move VETO .. must have overshot\n");
                        }
                    }
                    if (Allow_Dec_Move) {
                        Debug << _T(" Dec move ") << Dec_dur << _T(" ") << Dec_dist;
                        if (Dec_dur > (float) Max_Dec_Dur) {
                            Dec_dur = (float) Max_Dec_Dur;
                            Debug << _T("... Dec move clipped to ") << Dec_dur << _T("\n");
                        }
                        if ((Dec_dist > 0.0) && ((Dec_guide == DEC_AUTO) || (Dec_guide == DEC_SOUTH))) {
                            SetStatusText(wxString::Format(_T("S dur=%.1f dist=%.2f"),Dec_dur,Dec_dist),1);
							if (!DisableGuideOutput)
								pScope->Guide(SOUTH,(int) Dec_dur);	// So, guide in the Dec- direction;
                            if (Log_Data) {
                                logline = logline + wxString::Format(_T(",%.1f,%.2f"),Dec_dur,Dec_dist);
                            }
                        }
                        else if ((Dec_dist < 0.0) && ((Dec_guide == DEC_AUTO) || (Dec_guide == DEC_NORTH))){
                            SetStatusText(wxString::Format(_T("N dur=%.1f dist=%.2f"),Dec_dur,Dec_dist),1);
							if (!DisableGuideOutput)
	                            pScope->Guide(NORTH,(int) Dec_dur);	// So, guide in the Dec- direction;
                            if (Log_Data) {
                                logline = logline + wxString::Format(_T(",%.1f,%.2f"),Dec_dur,Dec_dist);
                            }
                        }
                        else { // will hit this if in north or south only mode and the direction is the opposite
                            logline = logline + wxString::Format(_T(",0.0,%.2f"),Dec_dist);
                            Debug << _T("In N or S only mode and dir is opposite\n");
                        }
                    }
                    else { // not enough motion
                        logline = logline + wxString::Format(_T(",0.0,%.2f"),Dec_dist);
                        Debug << _T("not enough motion\n");
                    }
                    Debug << _T("Done\n");
                }
                else if (Dec_guide && FoundStar && ((Dec_algo == DEC_LOWPASS) || (Dec_algo == DEC_LOWPASS2))) {// version 1, lowpass filter
                    Debug << _T("Dec lowpass - ");
                    Dec_dist = cos(pScope->DecAngle() - theta)*hyp;	// dist in Dec star needs to move
                    Dec_dist_list.Add(Dec_dist);
                    if (1) { // enough elements to work with  (Dec_dist_list.GetCount() > 10
                        if (Dec_algo == DEC_LOWPASS) {
                            // Get the median of the current 11 values in there.
                            Sorted_dec_list = Dec_dist_list;
                            Sorted_dec_list.Sort(dbl_sort_func);
                            Curr_Dec_dist = Sorted_dec_list.Item(5); // this is now the median of the 11 items in there
                            float slope = CalcSlope(Dec_dist_list);
                            Curr_Dec_dist = Curr_Dec_dist + Dec_slopeweight * slope;
                            if (fabs(Curr_Dec_dist) > fabs(Dec_dist)) {
                                Debug << _T(" reset CDist (") << Curr_Dec_dist << _T(") to dist ") << Dec_dist << _T(" as model of error is larger than true");
                                Curr_Dec_dist=Dec_dist;
                            }
                            Dec_dist_list.RemoveAt(0);  // take the first one off the list, should be back to 10 now, ready for next
                            Dec_dur = (fabs(Curr_Dec_dist)/pScope->DecRate()) / 11.0;	// since the distance is cumulative, this is what should be applied for each time, on average
                        }
                        else { // LOWPASS2 - linear regression
                            float slope = CalcSlope(Dec_dist_list);  // Calc slope - change per frame - over last 11
                            Dec_dist_list.RemoveAt(0);
                            //Dec_dist = Curr_Dec_dist;
                            if (fabs(Dec_dist) < fabs(slope)) { // Error is less than the slope, don't overshoot
                                Dec_dur = fabs(Dec_dist) / pScope->DecRate();
                                Curr_Dec_dist = Dec_dist;
                                Debug << _T("Using Dec_dist\n");
                            }
                            else {
                                Dec_dur = fabs(slope) / pScope->DecRate();  // Send one frame's worth (delta per frame)
                                Curr_Dec_dist = slope;
                                Debug << _T("Using slope\n");
                            }
                            wxString HistString = _T("History: ");
                            for (i=0; i<10; i++)
                                HistString += wxString::Format(_T("%.2f "),Dec_dist_list.Item(i));
                            Debug << HistString;
                            Debug << _T("\n   Dist=") << Dec_dist << _T("Cdist= ") << Curr_Dec_dist << _T("  Dur=") << Dec_dur << _T(" Slope=") << slope << _T("\n");
                        }
                        if ((fabs(Curr_Dec_dist) > MinMotion) || (Dec_algo == DEC_LOWPASS2)) {  // enough motion to warrant a move
                            Debug << _T("Dec guide ") << Dec_dist;
                            if (Dec_dur > (float) Max_Dec_Dur) {
                                Dec_dur = (float) Max_Dec_Dur;
                                Debug << _T("... Dec move clipped to ") << Dec_dur << _T("\n");
                            }

                            if ((Curr_Dec_dist > 0.0) && ((Dec_guide == DEC_AUTO) || (Dec_guide == DEC_SOUTH))) {
                                SetStatusText(wxString::Format(_T("S dur=%.1f dist=%.2f cdist=%.2f"),Dec_dur,Dec_dist,Curr_Dec_dist),1);
								if (!DisableGuideOutput)
		                            pScope->Guide(SOUTH,(int) Dec_dur);	// So, guide in the Dec- direction;
                                if (Log_Data) {
                                    logline = logline + wxString::Format(_T(",%.1f,%.2f"),Dec_dur,Dec_dist);
                                }
                            }
                            else if ((Curr_Dec_dist < 0.0) && ((Dec_guide == DEC_AUTO) || (Dec_guide == DEC_NORTH))){
                                SetStatusText(wxString::Format(_T("N dur=%.1f dist=%.2f cdist=%.2f"),Dec_dur,Dec_dist,Curr_Dec_dist),1);
								if (!DisableGuideOutput)
		                            pScope->Guide(NORTH,(int) Dec_dur);	// So, guide in the Dec- direction;
                                if (Log_Data) {
                                    logline = logline + wxString::Format(_T(",%.1f,%.2f"),Dec_dur,Dec_dist);
                                }
                            }
                        }
                        else { // not enough motion
                            logline = logline + wxString::Format(_T(",0.0,%.2f"),Dec_dist);
                            Debug << _T("Not enough motion\n");
                        }
                    }
                    else // not enough elements yet for drift
                        logline = logline + wxString::Format(_T(",0.0,%.2f"),Dec_dist);
                    Debug << _T(" Done\n");

                }

                else // no DEC guide
                    logline = logline + _T(",0,0");
                if (Log_Data) {
                    logline = logline + wxString::Format(_T(",%.2f,%d"),StarMass,StarErrorCode);
                    LogFile->AddLine(logline);
                    LogFile->Write();
                }
                this->GraphLog->AppendData(dX,dY,RA_dist,Dec_dist);
                canvas->FullFrameToDisplay();

                wxTheApp->Yield();
                if (Time_lapse) wxMilliSleep(Time_lapse);
                frame_index++;
            }
        }
    //	wxStopTimer();
        Loop_Button->Enable(true);
        Guide_Button->Enable(true);
        Cam_Button->Enable(true);
        Scope_Button->Enable(true);
        Brain_Button->Enable(true);

        CaptureActive = false;
        Abort = 0;
        canvas->State = STATE_NONE;
        canvas->Refresh();
        SetStatusText(_T("Guiding stopped"));
        SetStatusText(_T(""),1);
        if (Log_Data) LogFile->Write();
        if (Log_Data) LogFile->Close();
        Debug << _T("Guiding finished\n");
    }
    catch (char *ErrorMsg)
    {
        Debug << "OnGuide caught an exception " << ErrorMsg << _T("\n");
    }

    return;
}
