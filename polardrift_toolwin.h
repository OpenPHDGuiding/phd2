/*
 *  polardrift_toolwin.h
 *  PHD Guiding
 *
 *  Created by Ken Self
 *  Copyright (c) 2017 Ken Self
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
 *    Neither the name of openphdguiding.org nor the names of its
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
#ifndef POLARDRIFT_TOOLWIN_H
#define POLARDRIFT_TOOLWIN_H

#include "phd.h"

#include <wx/gbsizer.h>
#include <wx/valnum.h>
#include <wx/textwrapper.h>

//==================================
struct PolarDriftToolWin : public wxFrame
{
    PolarDriftToolWin();
    ~PolarDriftToolWin();
    /*
    Tool window controls
    */
    wxStaticText *w_instructions;
//    wxTextCtrl *w_camScale; // Text box for camera pixel scale
//    wxTextCtrl *w_camRot;   // Text box for camera rotation
//    wxCheckBox *w_manual;   // Checkbox for auto/manual slewing
    wxButton *w_start;      // Button to start or stop drift
    wxStaticText *w_notesLabel;
    wxTextCtrl *w_notes;
    wxButton *w_close;      // Close button
    wxStatusBar *w_statusBar;
//    wxChoice *w_refStarChoice;  // Listbox for reference stars
    wxChoice *w_hemiChoice;     // Listbox for manual hemisphere choice 
    bool m_savePrimaryMountEnabled;
    bool m_saveSecondaryMountEnabled;
    bool m_guideOutputDisabled;
//    wxBoxSizer *m_vSizer;

    /*
    Constants used in the tool window controls
    */
    wxString c_instr;

    enum PolarDriftCtrlIds
    {
        ID_HEMI = 10001,
        ID_START,
        ID_CLOSE,
    };

    bool g_canSlew;     // Mount can slew
    double g_pxScale;   // Camera pixel scale
    double g_camAngle;  // Camera angle to RA
    double g_camWidth;  // Camera width

    int a_hemi;         // Hemisphere of the observer

    bool s_drifting;        // Indicates that alignment points are being collected
    double t0;
    double sumt, sumt2, sumx, sumx2, sumy, sumy2, sumtx, sumty, sumxy;
    long num;
    double offset, alpha;
    PHD_Point current, target;

    double g_dispSz[2];     // Display size (dynamic)

    void FillPanel();

    void OnHemi(wxCommandEvent& evt);
    void OnStart(wxCommandEvent& evt);
    void OnCloseBtn(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);

    bool IsDrifting(){ return s_drifting; };
    bool WatchDrift();
    void PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale);

    DECLARE_EVENT_TABLE()
};

#endif

