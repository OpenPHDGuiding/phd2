/*
 *  staticpa_toolwin.h
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
#ifndef STATICPA_TOOLWIN_H
#define STATICPA_TOOLWIN_H

#include "phd.h"

#include <wx/gbsizer.h>
#include <wx/valnum.h>
#include <wx/textwrapper.h>

//==================================
struct StaticPaToolWin : public wxFrame
{
    StaticPaToolWin();
    ~StaticPaToolWin();
    /*
    Tool window controls
    */
    wxHtmlWindow *w_instructions;
    wxTextCtrl *w_camScale; // Text box for camera pixel scale
    wxTextCtrl *w_camRot;   // Text box for camera rotation
    wxSpinCtrlDouble *w_hourangle; // Spinner to manually set HA
    wxCheckBox *w_manual;   // Checkbox for auto/manual slewing
    wxCheckBox *w_flip;     // Checkbox to flip camera
    wxCheckBox *w_orbit;    // Checkbox t show/hide orbits
    wxButton *w_star1;      // Button for manual get of point 1
    wxButton *w_star2;      // Button for manual get of point 2
    wxButton *w_star3;      // Button for manual get of point 3
    wxStaticText *w_notesLabel;
    wxTextCtrl *w_notes;
    wxButton *w_clear;     // Button to clear display
    wxButton *w_close;     // Close button
    wxStatusBar *w_statusBar;
    wxChoice *w_refStarChoice;  // Listbox for reference stars
    wxChoice *w_hemiChoice;     // Listbox for manual hemisphere choice 

    class PolePanel : public wxPanel
    {
    public:
        wxPoint origPt, currPt;
        PolePanel(StaticPaToolWin* parent);
        StaticPaToolWin *paParent;
        void OnClick(wxMouseEvent &evt);
        void OnPaint(wxPaintEvent &evt);
        void Paint();
        DECLARE_EVENT_TABLE()
    };
    PolePanel *w_pole; // Panel for drawing of pole stars 

    /*
    Constants used in the tool window controls
    */
    wxString c_autoInstr;
    wxString c_manualInstr;

    class Star
    {
    public:
        std::string name;
        double ra2000, dec2000, mag, ra, dec;
        Star(const char* a, const double b, const double c, const double d) :name(a), ra2000(b), dec2000(c), mag(d), ra(-1), dec(-1) {};
    };
    std::vector<Star> c_SthStars, c_NthStars; // Stars around the poles
    std::vector<Star> *poleStars;

    enum StaticPaCtrlIds
    {
        ID_HEMI = 10001,
        ID_HA,
        ID_MANUAL,
        ID_FLIP,
        ID_ORBIT,
        ID_REFSTAR,
        ID_ROTATE,
        ID_STAR2,
        ID_STAR3,
        ID_CLEAR,
        ID_CLOSE,
    };

    bool g_canSlew;     // Mount can slew
    double g_pxScale;   // Camera pixel scale
    double g_camAngle;  // Camera angle to RA
    double g_camWidth;  // Camera width

    double a_devpx;     // Number of pixels deviation needed to detect an arc
    int a_refStar;      // Selected reference star
    bool a_auto;        // Auto slewing - must be manual if mount cannot slew
    int a_hemi;         // Hemisphere of the observer
    double a_ha;        // Manual hour angle
    bool a_drawOrbit;   // Draw the star orbits
    bool a_flip;        // Flip the camera angle

    bool s_aligning;        // Indicates that alignment points are being collected
    unsigned int s_state;   // state of the alignment process
    int s_numPos;           // Number of alignment points
    double s_reqRot;        // Amount of rotation required
    int s_reqStep;          // Number of steps needed
    double s_totRot;        // Rotation so far
    int s_nStep;            // Number of steps taken so far

    double r_raPos[3];      // RA readings at each point
    PHD_Point r_pxPos[3];   // Alignment points - in pixels
    PHD_Point r_pxCentre;   // Centre of Rotation in pixels
    double r_radius;        // Radius of centre of rotation to reference star

    double g_dispSz[2];     // Display size (dynamic)
    PHD_Point m_AzCorr, m_AltCorr; // Calculated Alt and Az corrections
    PHD_Point m_ConeCorr, m_DecCorr;  //Calculated Dec and Cone offsets

    void FillPanel();

    void OnHemi(wxCommandEvent& evt);
    void OnHa(wxSpinDoubleEvent& evt);
    void OnRefStar(wxCommandEvent& evt);
    void OnManual(wxCommandEvent& evt);
    void OnFlip(wxCommandEvent& evt);
    void OnOrbit(wxCommandEvent& evt);
    void OnNotes(wxCommandEvent& evt);
    void OnRotate(wxCommandEvent& evt);
    void OnStar2(wxCommandEvent& evt);
    void OnStar3(wxCommandEvent& evt);
    void OnClear(wxCommandEvent& evt);
    void OnCloseBtn(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);

    void CreateStarTemplate(wxDC &dc, wxPoint currPt);
    bool IsAligning(){ return s_aligning; };
    bool RotateMount();
    bool RotateFail(const wxString msg);
    bool SetParams(double newoffset);
    bool MoveWestBy(double thetadeg);
    bool SetStar(int idx);
    bool IsAligned(){ return a_auto ? ((s_state>>1) & 3) ==3 : ((s_state>>1) & 7)==7; }
    bool IsCalced(){ return HasState(0); }
    void CalcRotationCentre(void);
    void CalcAdjustments(void);
    bool HasState(int ipos) { return (s_state & (1 << ipos)) > 0;  }
    void SetState(int ipos) { s_state = s_state | (1 << ipos); }
    void UnsetState(int ipos) { s_state = s_state & ~(1 << ipos) & 15; }
    void ClearState() { s_state = 0; }
    void PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale);
    PHD_Point Radec2Px(PHD_Point radec);
    PHD_Point J2000Now(PHD_Point& radec);

    DECLARE_EVENT_TABLE()
};

#endif

