/*
 *  staticpa_toolwin.cpp
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

#include "phd.h"
#include "staticpa_tool.h"
#include "staticpa_toolwin.h"

#include <wx/gbsizer.h>
#include <wx/valnum.h>
#include <wx/textwrapper.h>

//==================================
BEGIN_EVENT_TABLE(StaticPaToolWin, wxFrame)
EVT_CHOICE(ID_HEMI, StaticPaToolWin::OnHemi)
EVT_SPINCTRLDOUBLE(ID_HA, StaticPaToolWin::OnHa)
EVT_CHECKBOX(ID_MANUAL, StaticPaToolWin::OnManual)
EVT_CHECKBOX(ID_FLIP, StaticPaToolWin::OnFlip)
EVT_CHECKBOX(ID_ORBIT, StaticPaToolWin::OnOrbit)
EVT_CHOICE(ID_REFSTAR, StaticPaToolWin::OnRefStar)
EVT_BUTTON(ID_ROTATE, StaticPaToolWin::OnRotate)
EVT_BUTTON(ID_STAR2, StaticPaToolWin::OnStar2)
EVT_BUTTON(ID_STAR3, StaticPaToolWin::OnStar3)
EVT_BUTTON(ID_GOTO, StaticPaToolWin::OnGoto)
EVT_BUTTON(ID_CLEAR, StaticPaToolWin::OnClear)
EVT_BUTTON(ID_CLOSE, StaticPaToolWin::OnCloseBtn)
EVT_CLOSE(StaticPaToolWin::OnClose)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(StaticPaToolWin::PolePanel, wxPanel)
EVT_PAINT(StaticPaToolWin::PolePanel::OnPaint)
EVT_LEFT_DCLICK(StaticPaToolWin::PolePanel::OnClick)
END_EVENT_TABLE()

StaticPaToolWin::PolePanel::PolePanel(StaticPaToolWin* parent):
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(320, 240), wxBU_AUTODRAW | wxBU_EXACTFIT),
    paParent(parent)
{
    origPt.x = 160;
    origPt.y = 120;
    currPt.x = 0;
    currPt.y = 0;
}

void StaticPaToolWin::PolePanel::OnPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    paParent->CreateStarTemplate(dc, currPt);
}
void StaticPaToolWin::PolePanel::Paint()
{
    wxClientDC dc(this);
    paParent->CreateStarTemplate(dc, currPt);
}
void StaticPaToolWin::PolePanel::OnClick(wxMouseEvent& evt)
{
    const wxPoint pt = wxGetMousePosition();
    const wxPoint mpt = GetScreenPosition();
    wxPoint mousePt = pt - mpt - origPt; // Distance fron centre
    currPt = currPt + mousePt; // Distance from origin
    paParent->FillPanel();
    Paint();
}

wxWindow *StaticPaTool::CreateStaticPaToolWindow()
{
    if (!pCamera || !pCamera->Connected)
    {
        wxMessageBox(_("Please connect a camera first."));
        return 0;
    }

    // confirm that image scale is specified
    if (pFrame->GetCameraPixelScale() == 1.0)
    {
        bool confirmed = ConfirmDialog::Confirm(_(
            "The Static Align tool is most effective when PHD2 knows your guide\n"
            "scope focal length and camera pixel size.\n"
            "\n"
            "Enter your guide scope focal length on the Global tab in the Brain.\n"
            "Enter your camera pixel size on the Camera tab in the Brain.\n"
            "\n"
            "Would you like to run the tool anyway?"),
            "/rotate_tool_without_pixscale");

        if (!confirmed)
        {
            return 0;
        }
    }
    if (pFrame->pGuider->IsCalibratingOrGuiding())
    {
        wxMessageBox(_("Please wait till Calibration is done and stop guiding"));
        return 0;
    }

    return new StaticPaToolWin();
}
bool StaticPaTool::IsAligning()
{
    StaticPaToolWin *win = static_cast<StaticPaToolWin *>(pFrame->pStaticPaTool);
    return win->IsAligning();
}
bool StaticPaTool::RotateMount()
{
    StaticPaToolWin *win = static_cast<StaticPaToolWin *>(pFrame->pStaticPaTool);
    return win->RotateMount();
}
void StaticPaTool::PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale)
{
    StaticPaToolWin *win = static_cast<StaticPaToolWin *>(pFrame->pStaticPaTool);
    return win->PaintHelper(dc, scale);
}

StaticPaToolWin::StaticPaToolWin()
: wxFrame(pFrame, wxID_ANY, _("Static Polar Alignment"), wxDefaultPosition, wxDefaultSize,
wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxTAB_TRAVERSAL | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR)
{
    s_numPos = 0;
    a_devpx = 5;
    ClearState();
    s_aligning = false;
    
    //Fairly convoluted way to get the camera size in pixels
    usImage *pCurrImg = pFrame->pGuider->CurrentImage();
    wxImage *pDispImg = pFrame->pGuider->DisplayedImage();
    double scalefactor = pFrame->pGuider->ScaleFactor();
    double xpx = pDispImg->GetWidth() / scalefactor;
    double ypx = pDispImg->GetHeight() / scalefactor;
    r_pxCentre.X = xpx/2;
    r_pxCentre.Y = ypx/2;
    g_pxScale = pFrame->GetCameraPixelScale();
    // Fullsize is easier but the camera simulator does not set this.
//    wxSize camsize = pCamera->FullSize;
    g_camWidth = pCamera->FullSize.GetWidth() == 0 ? xpx: pCamera->FullSize.GetWidth();

    g_camAngle = 0.0;
    double camAngle_rad = 0.0;
    a_flip = false;
    if (pMount && pMount->IsConnected() && pMount->IsCalibrated())
    {
        camAngle_rad = pMount->xAngle();
        Debug.AddLine(wxString::Format("StaticPA: Camera angle %.1f", degrees(camAngle_rad)));
        wxString prefix = "/" + pMount->GetMountClassName() + "/calibration/";
        int ipier = pConfig->Profile.GetInt(prefix + "pierSide", PIER_SIDE_UNKNOWN);
        PierSide calPierSide = ipier == PIER_SIDE_EAST ? PIER_SIDE_EAST : ipier == PIER_SIDE_WEST ? PIER_SIDE_WEST : PIER_SIDE_UNKNOWN;
        PierSide currPierSide = pPointingSource->SideOfPier();
        Debug.AddLine(wxString::Format("StaticPA: calPierSide %s; currPierSide %s", pMount->PierSideStr(calPierSide), pMount->PierSideStr(currPierSide)));
        if (currPierSide != calPierSide  && currPierSide != PIER_SIDE_UNKNOWN)
        {
            a_flip =  true;
            Debug.AddLine(wxString::Format("StaticPA: Flipped Camera angle"));
        }
        g_camAngle = degrees(camAngle_rad);
    }
    // RA and Dec in J2000.0
    c_SthStars = {
        Star("A: sigma Oct", 317.19908, -88.9564, 4.3),
        Star("B: HD99828", 165.91797, -89.2392, 7.5),
        Star("C: HD125371", 241.45949, -89.3087, 7.8),
        Star("D: HD92239", 142.27856, -89.3471, 8.0),
        Star("E: HD90105", 130.52896, -89.4606, 7.2),
        Star("F: BQ Oct", 218.86418, -89.7718, 6.8),
        Star("G: HD99685", 149.13626, -89.7824, 7.8),
        Star("H: HD98784", 134.64254, -89.8312, 8.9),
    };
    for (int is = 0; is < c_SthStars.size(); is++) {
        PHD_Point radec_now = J2000Now(PHD_Point(c_SthStars.at(is).ra2000, c_SthStars.at(is).dec2000));
        c_SthStars.at(is).ra = radec_now.X;
        c_SthStars.at(is).dec = radec_now.Y;
    }

    c_NthStars = {
        Star("A: HD5914", 23.48114, 89.0155, 6.45),
        Star("B: HD14369", 55.20640, 89.1048, 8.05),
        Star("C: Polaris", 37.96089, 89.2643, 1.95),
        Star("D: HD211455", 309.69879, 89.4065, 8.9),
        Star("E: TYC-4629-33-1", 75.97399, 89.4207, 9.25),
        Star("F: HD21070", 146.59109, 89.5695, 9.0),
        Star("G: HD1687", 9.92515, 89.4443, 8.1),
        Star("H: TYC-4629-37-1", 70.70722, 89.6301, 9.15),
    };
    for (int is = 0; is < c_NthStars.size(); is++) {
        PHD_Point radec_now = J2000Now(PHD_Point(c_NthStars.at(is).ra2000, c_NthStars.at(is).dec2000));
        c_NthStars.at(is).ra = radec_now.X;
        c_NthStars.at(is).dec = radec_now.Y;
    }


    // get site lat/long from scope to determine hemisphere.
    a_refStar = pConfig->Profile.GetInt("/StaticPaTool/RefStar", 0);
    a_hemi = pConfig->Profile.GetInt("/StaticPaTool/Hemisphere", 1);
    if (pPointingSource)
    {
        double lat, lon;
        if (!pPointingSource->GetSiteLatLong(&lat, &lon))
        {
            a_hemi = lat >= 0 ? 1 : -1;
        }
    }
    if (!pFrame->CaptureActive)
    {
        // loop exposures
        wxCommandEvent dummy;
        SetStatusText(_("Start Looping..."));
        pFrame->OnLoopExposure(dummy);
    }
    c_autoInstr = _(
        "Slew to near the Celestial Pole.<br/>"
        "Choose a Reference Star from the list.<br/>"
        "Select it as the guide star on the main display.<br/>"
        "Click Rotate to start the alignment.<br/>"
        "Wait for the adjustments to display.<br/>"
        "Adjust your mount's altitude and azimuth as displayed.<br/>"
        "Red=Altitude; Blue=Azimuth<br/>"
        );
    c_manualInstr = _(
        "Slew to near the Celestial Pole.<br/>"
        "Choose a Reference Star from the list.<br/>"
        "Select it as the guide star on the main display.<br/>"
        "Click Get first position.<br/>"
        "Slew at least 0h20m west in RA.<br/>"
        "Ensure the Reference Star is still selected.<br/>"
        "Click Get second position.<br/>"
        "Repeat for the third position.<br/>"
        "Wait for the adjustments to display.<br/>"
        "Adjust your mount's altitude and azimuth to place "
        "three reference stars on their orbits\n"
        );

    // can mount slew?
    a_auto = true;
    a_drawOrbit = true;
    g_canSlew = pPointingSource && pPointingSource->CanSlewAsync();
    a_ha = 0.0;
    if (!g_canSlew){
        a_auto = false;
        a_ha = 270.0;
    }

    // Start window definition
    SetBackgroundColour(wxColor(0xcccccc));
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    // a vertical sizer holding everything
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    // a horizontal box sizer for the bitmap and the instructions
    wxBoxSizer *instrSizer = new wxBoxSizer(wxHORIZONTAL);
    w_instructions = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(240, 240), wxHW_DEFAULT_STYLE);
    w_instructions->SetStandardFonts(8);
    /*
#ifdef __WXOSX__
    w_instructions->SetFont(*wxSMALL_FONT);
#endif
    */
    instrSizer->Add(w_instructions, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    w_pole = new PolePanel(this);
    instrSizer->Add(w_pole, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxFIXED_MINSIZE, 5);

    topSizer->Add(instrSizer);

    // static box sizer holding the scope pointing controls
    wxStaticBoxSizer *sbSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Alignment Parameters")), wxVERTICAL);

    // a grid box sizer for laying out scope pointing the controls
    wxGridBagSizer *gbSizer = new wxGridBagSizer(0, 0);
    gbSizer->SetFlexibleDirection(wxBOTH);
    gbSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    wxStaticText *txt;

    // First row of grid
    int gridRow = 0;
    // Hour angle
    txt = new wxStaticText(this, wxID_ANY, _("Hour Angle"));
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);

    txt = new wxStaticText(this, wxID_ANY, _("Hemisphere"));
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 1), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);

    // Alignment star specification
    txt = new wxStaticText(this, wxID_ANY, _("Reference Star"));
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);

    // Next row of grid
    gridRow++;
    w_hourangle = new wxSpinCtrlDouble(this, ID_HA, wxEmptyString, wxDefaultPosition, wxSize(10, -1), wxSP_ARROW_KEYS | wxSP_WRAP, 0.0, 24.0, a_ha/15.0, 0.1);
    w_hourangle->SetToolTip(_("Set your scope hour angle"));
    gbSizer->Add(w_hourangle, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxEXPAND | wxALL | wxFIXED_MINSIZE, 5);
    w_hourangle->SetDigits(1);

    wxArrayString hemi;
    hemi.Add(_("North"));
    hemi.Add(_("South"));
    w_hemiChoice = new wxChoice(this, ID_HEMI, wxDefaultPosition, wxDefaultSize, hemi);
    w_hemiChoice->SetToolTip(_("Select your hemisphere"));
    gbSizer->Add(w_hemiChoice, wxGBPosition(gridRow, 1), wxGBSpan(1, 1), wxALL, 5);

    wxBoxSizer *refSizer = new wxBoxSizer(wxHORIZONTAL);

    w_refStarChoice = new wxChoice(this, ID_REFSTAR, wxDefaultPosition, wxDefaultSize);
    w_refStarChoice->SetToolTip(_("Select the star used for checking alignment."));
    refSizer->Add(w_refStarChoice, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

    w_goto = new wxButton(this, ID_GOTO, _(">"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    refSizer->Add(w_goto, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 0);

    gbSizer->Add(refSizer, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxALL, 5);


    // Next row of grid
    gridRow++;
    txt = new wxStaticText(this, wxID_ANY, _("Camera Angle"));
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);

    txt = new wxStaticText(this, wxID_ANY, _("Arcsec/pixel"));
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 1), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);

    w_manual = new wxCheckBox(this, ID_MANUAL, _("Manual Slew"));
    gbSizer->Add(w_manual, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);
    w_manual->SetValue(false);
    w_manual->SetToolTip(_("Manually slew the mount to three alignment positions"));

    // Next row of grid
    gridRow++;

    w_camRot = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxSize(10, -1), wxTE_READONLY);
    w_camRot->SetMinSize(wxSize(10, -1));
    gbSizer->Add(w_camRot, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    w_camScale = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxSize(10, -1), wxTE_READONLY);
    w_camRot->SetMinSize(wxSize(10, -1));
    gbSizer->Add(w_camScale, wxGBPosition(gridRow, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    w_star1 = new wxButton(this, ID_ROTATE, _("Rotate"));
    gbSizer->Add(w_star1, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // Next row of grid
    gridRow++;

    w_flip = new wxCheckBox(this, ID_FLIP, _("Flip camera"));
    gbSizer->Add(w_flip, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);
    w_flip->SetValue(a_flip);
    w_flip->SetToolTip(_("Invert the camera angle"));

    w_star2 = new wxButton(this, ID_STAR2, _("Get second position"));
    gbSizer->Add(w_star2, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // Next row of grid
    gridRow++;

    w_orbit = new wxCheckBox(this, ID_ORBIT, _("Show Orbits"));
    gbSizer->Add(w_orbit, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);
    w_orbit->SetValue(a_drawOrbit);
    w_orbit->SetToolTip(_("Show or hide the star orbits"));

    w_star3 = new wxButton(this, ID_STAR3, _("Get third position"));
    gbSizer->Add(w_star3, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    w_clear = new wxButton(this, ID_CLEAR, _("Clear"), wxDefaultPosition, wxDefaultSize, 0);
    gbSizer->Add(w_clear, wxGBPosition(gridRow, 3), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // Next row of grid
    gridRow++;

    w_close = new wxButton(this, ID_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0);
    gbSizer->Add(w_close, wxGBPosition(gridRow, 3), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // add grid bag sizer to static sizer
    sbSizer->Add(gbSizer, 1, wxALIGN_CENTER, 5);

    // add static sizer to top-level sizer
    topSizer->Add(sbSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // add some padding below the static sizer
    topSizer->Add(0, 3, 0, wxEXPAND, 3);

    w_notesLabel = new wxStaticText(this, wxID_ANY, _("Adjustment notes"));
    w_notesLabel->Wrap(-1);
    topSizer->Add(w_notesLabel, 0, wxEXPAND | wxTOP | wxLEFT, 8);

    w_notes = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 54), wxTE_MULTILINE);
    pFrame->RegisterTextCtrl(w_notes);
    topSizer->Add(w_notes, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    w_notes->Bind(wxEVT_COMMAND_TEXT_UPDATED, &StaticPaToolWin::OnNotes, this);
    w_notes->SetValue(pConfig->Profile.GetString("/StaticPaTool/Notes", wxEmptyString));

    SetSizer(topSizer);

    w_statusBar = CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);

    Layout();
    topSizer->Fit(this);

    int xpos = pConfig->Global.GetInt("/StaticPaTool/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/StaticPaTool/pos.y", -1);
    MyFrame::PlaceWindowOnScreen(this, xpos, ypos);

    FillPanel();
}

StaticPaToolWin::~StaticPaToolWin()
{
    pFrame->pStaticPaTool = NULL;
}

void StaticPaToolWin::OnHemi(wxCommandEvent& evt)
{
    int i_hemi = w_hemiChoice->GetSelection() <= 0 ? 1 : -1;
    pConfig->Profile.SetInt("/StaticPaTool/Hemisphere", i_hemi);
    if (i_hemi != a_hemi)
    {
        a_refStar = 0;
        a_hemi = i_hemi;
    }
    FillPanel();
}
void StaticPaToolWin::OnHa(wxSpinDoubleEvent& evt)
{
    a_ha = evt.GetValue() * 15.0;
    w_pole->Paint();
}

void StaticPaToolWin::OnManual(wxCommandEvent& evt)
{
    a_auto = !w_manual->IsChecked();
    FillPanel();
}

void StaticPaToolWin::OnFlip(wxCommandEvent& evt)
{
    a_flip = w_flip->IsChecked();
    FillPanel();
}

void StaticPaToolWin::OnOrbit(wxCommandEvent& evt)
{
    a_drawOrbit = w_orbit->IsChecked();
    FillPanel();
}

void StaticPaToolWin::OnRefStar(wxCommandEvent& evt)
{
    int i_refStar = w_refStarChoice->GetSelection();
    pConfig->Profile.SetInt("/StaticPaTool/RefStar", w_refStarChoice->GetSelection());

    a_refStar = i_refStar;
}

void StaticPaToolWin::OnNotes(wxCommandEvent& evt)
{
    pConfig->Profile.SetString("/StaticPaTool/Notes", w_notes->GetValue());
}

void StaticPaToolWin::OnRotate(wxCommandEvent& evt)
{
    if (s_aligning && a_auto){ // STOP rotating
        pPointingSource->AbortSlew();
        s_aligning = false;
        s_numPos = 0;
        ClearState();
        SetStatusText(_("Static alignment stopped"));
        Debug.AddLine(wxString::Format("Static alignment stopped"));
        FillPanel();
        return;
    }
    if (pFrame->pGuider->IsCalibratingOrGuiding())
    {
        SetStatusText(_("Please wait till Calibration is done and/or stop guiding"));
        return;
    }
    if (!pFrame->pGuider->IsLocked())
    {
        SetStatusText(_("Please select a star"));
        return;
    }
    s_numPos = 1;
    if(a_auto) ClearState();
    s_aligning = true;
    FillPanel();
    return;
}

void StaticPaToolWin::OnStar2(wxCommandEvent& evt)
{
    s_numPos = 2;
    s_aligning = true;
}

void StaticPaToolWin::OnStar3(wxCommandEvent& evt)
{
    s_numPos = 3;
    s_aligning = true;
}

void StaticPaToolWin::OnGoto(wxCommandEvent& evt)
{
    // Get current star
    // Convert current star Ra Dec to pixels
    int is = a_refStar;
    double scale = 320.0 / g_camWidth;

    PHD_Point stardeg = PHD_Point(poleStars->at(is).ra, poleStars->at(is).dec);
    PHD_Point starpx = Radec2Px(stardeg);
    w_pole->currPt = wxPoint(starpx.X*scale, starpx.Y*scale);
    FillPanel();
    return;
}

void StaticPaToolWin::OnClear(wxCommandEvent& evt)
{
    if (IsCalced())
    {
        s_aligning = false;
        s_numPos = 0;
        ClearState();
        SetStatusText(_("Static Polar alignment display cleared"));
        Debug.AddLine(wxString::Format("Static PA display cleared"));
        FillPanel();
        return;
    }
}

void StaticPaToolWin::OnCloseBtn(wxCommandEvent& evt)
{
    Debug.AddLine("Close StaticPaTool");
    if (IsAligning())
    {
        s_aligning = false;
    }
    Destroy();
}

void StaticPaToolWin::OnClose(wxCloseEvent& evt)
{
    Debug.AddLine("Close StaticPaTool");
    Destroy();
}

void StaticPaToolWin::FillPanel()
{
    w_hourangle->Enable(true);

    if (!g_canSlew){
        w_manual->Hide();
    }
    w_manual->SetValue(!a_auto);

    wxString html = wxString::Format("<html><body style=\"background-color:#cccccc;\">%s</body></html>", a_auto ? c_autoInstr : c_manualInstr);
    w_instructions->SetPage(html);

    w_star1->SetLabel(_("Rotate"));
    if (s_aligning) {
        w_star1->SetLabel(_("Stop"));
    }
    w_star2->Hide();
    w_star3->Hide();
    w_hemiChoice->Enable(false);
    w_hemiChoice->SetSelection(a_hemi > 0 ? 0 : 1);
    if (!a_auto)
    {
        w_star1->SetLabel(_("Get first position"));
        w_star2->Show();
        w_star3->Show();
        w_hemiChoice->Enable(true);
    }
    poleStars = a_hemi >= 0 ? &c_NthStars : &c_SthStars;
    w_refStarChoice->Clear();
    std::string starname;
    for (int is = 0; is < poleStars->size(); is++) {
        w_refStarChoice->AppendString(poleStars->at(is).name);
    }
    w_refStarChoice->SetSelection(a_refStar);
    w_camScale->SetValue(wxString::Format("%.1f", g_pxScale));
    w_camRot->SetValue(wxString::Format("%.1f", g_camAngle));

    w_pole->Paint();
    Layout();
}

void StaticPaToolWin::CalcRotationCentre(void)
{
    double x1, y1, x2, y2, x3, y3;
    double cx, cy, cr;
    x1 = r_pxPos[0].X;
    y1 = r_pxPos[0].Y;
    x2 = r_pxPos[1].X;
    y2 = r_pxPos[1].Y;
    UnsetState(0);

    if (!a_auto)
    {
        double a, b, c, e, f, g, i, j, k;
        double m11, m12, m13, m14;
        x3 = r_pxPos[2].X;
        y3 = r_pxPos[2].Y;
        Debug.AddLine(wxString::Format("StaticPA: Manual CalcCoR: P1(%.1f,%.1f); P2(%.1f,%.1f); P3(%.1f,%.1f)", x1, y1, x2, y2, x3, y3));
        // |A| = aei + bfg + cdh -ceg -bdi -afh
        // a b c
        // d e f
        // g h i
        // a= x1^2+y1^2; b=x1; c=y1; d=1
        // e= x2^2+y2^2; f=x2; g=y2; h=1
        // i= x3^2+y3^2; j=x3; k=y3; l=1
        //
        // x0 = 1/2.|M12|/|M11|
        // y0 = -1/2.|M13|/|M11|
        // r = x0^2 + y0^2 + |M14| / |M11|
        //
        a = x1 * x1 + y1 * y1;
        b = x1;
        c = y1;
        e = x2 * x2 + y2 * y2;
        f = x2;
        g = y2;
        i = x3 * x3 + y3 * y3;
        j = x3;
        k = y3;
        m11 = b * g + c * j + f * k - g * j - c * f - b * k;
        m12 = a * g + c * i + e * k - g * i - c * e - a * k;
        m13 = a * f + b * i + e * j - f * i - b * e - a * j;
        m14 = a * f * k + b * g * i + c * e * j - c * f * i - b * e * k - a * g * j;
        cx = (1. / 2.) * m12 / m11;
        cy = (-1. / 2.) * m13 / m11;
        cr = sqrt(cx * cx + cy *cy + m14 / m11);
    }
    else
    {
        Debug.AddLine(wxString::Format("StaticPA Auto CalcCoR: P1(%.1f,%.1f); P2(%.1f,%.1f); RA: %.1f %.1f", x1, y1, x2, y2, r_raPos[0] * 15., r_raPos[1] * 15.));
        // Alternative algorithm based on two points and angle rotated
        double radiff, theta2;
        // Get RA change. For westward movement RA decreases.
        // Invert to get image rotation (mult by -1)
        // Convert to radians radians(mult by 15)
        // Convert to RH system (mult by a_hemi)
        // normalise to +/- PI
        radiff = norm_angle(radians((r_raPos[0] - r_raPos[1])*15.0*a_hemi));

        theta2 = radiff / 2.0; // Half the image rotation for midpoint of chord
        double lenchord = sqrt(pow((x1 - x2), 2.0) + pow((y1 - y2), 2.0));
        cr = fabs(lenchord / 2.0 / sin(theta2));
        double lenbase = fabs(cr*cos(theta2));
        // Calculate the slope of the chord in pixels
        // We know the image is moving clockwise in NH and anti-clockwise in SH
        // So subtract PI/2 in NH or add PI/2 in SH to get the slope to the CoR
        // Invert y values as pixels are +ve downwards
        double slopebase = atan2(y1 - y2, x2 - x1) - a_hemi* M_PI / 2.0;
        cx = (x1 + x2) / 2.0 + lenbase * cos(slopebase);
        cy = (y1 + y2) / 2.0 - lenbase * sin(slopebase); // subtract for pixels
        Debug.AddLine(wxString::Format("StaticPA CalcCoR: radiff(deg): %.1f; cr: %.1f; slopebase(deg) %.1f", degrees(radiff), cr, degrees(slopebase)));
    }
    r_pxCentre.X = cx;
    r_pxCentre.Y = cy;
    r_radius = cr;

    usImage *pCurrImg = pFrame->pGuider->CurrentImage();
    wxImage *pDispImg = pFrame->pGuider->DisplayedImage();
    double scalefactor = pFrame->pGuider->ScaleFactor();
    int xpx = pDispImg->GetWidth() / scalefactor;
    int ypx = pDispImg->GetHeight() / scalefactor;
    g_dispSz[0] = xpx;
    g_dispSz[1] = ypx;

    Debug.AddLine(wxString::Format("StaticPA CalcCoR: W:H:scale:angle %d: %d: %.1f %.1f", xpx, ypx, scalefactor, g_camAngle));

    // Distance and angle of CoR from centre of sensor
    double cor_r = sqrt(pow(xpx / 2 - cx, 2) + pow(ypx / 2 - cy, 2));
    double cor_a = degrees(atan2((ypx / 2 - cy), (xpx / 2 - cx)));
    double rarot = -g_camAngle;
    // Cone and Dec components of CoR vector
    double dec_r = cor_r * sin(radians(cor_a - rarot));
    m_DecCorr.X = -dec_r * sin(radians(rarot));
    m_DecCorr.Y = dec_r * cos(radians(rarot));
    double cone_r = cor_r * cos(radians(cor_a - rarot));
    m_ConeCorr.X = cone_r * cos(radians(rarot));
    m_ConeCorr.Y = cone_r * sin(radians(rarot));
    double decCorr_deg = dec_r * g_pxScale / 3600;
    SetState(0);
    FillPanel();
    return;
}

void StaticPaToolWin::CalcAdjustments(void)
{
    // Caclulate pixel values for the alignment stars relative to the CoR
    PHD_Point starpx, stardeg;
    stardeg = PHD_Point(poleStars->at(a_refStar).ra, poleStars->at(a_refStar).dec);
    starpx = Radec2Px(stardeg);
    // Convert to display pixel values by adding the CoR pixel coordinates
    double xt = starpx.X + r_pxCentre.X;
    double yt = starpx.Y + r_pxCentre.Y;

    int idx = a_auto ? 1 : 2;
    double xs = r_pxPos[idx].X;
    double ys = r_pxPos[idx].Y;

    // Calculate the camera rotation from the Azimuth axis
    // HA = LST - RA
    // In NH HA decreases clockwise; RA increases clockwise
    // "Up" is HA=0
    // Sensor "up" is 90deg counterclockwise from mount RA plus rotation
    // Star rotation is RAstar - RAmount

    // Alt angle aligns to HA=0, Azimuth (East) to HA = -90
    // In home position Az aligns with Dec
    // So at HA +/-90 (home pos) Alt rotation is 0 (HA+90)
    // At the meridian, HA=0 Alt aligns with dec so Rotation is +/-90
    // Let harot =  camera rotation from Alt axis
    // Alt axis is at HA+90
    // This is camera rotation from RA minus(?) LST angle 
    double    hcor_r = sqrt(pow(xt - xs, 2) + pow(yt - ys, 2)); //xt,yt: target, xs,ys: measured
    double    hcor_a = degrees(atan2((yt - ys), (xt - xs)));
    double ra_hrs, dec_deg, st_hrs, ha_deg;
    ha_deg = a_ha;
    if (pPointingSource && !pPointingSource->GetCoordinates(&ra_hrs, &dec_deg, &st_hrs))
    {
        ha_deg = norm((st_hrs - ra_hrs)*15.0 + a_ha, 0, 360);
    }
    double rarot = -g_camAngle;
    double harot = norm(rarot - (90 + ha_deg), 0, 360);
    double hrot = norm(hcor_a - harot, 0, 360);

    double az_r = hcor_r * sin(radians(hrot));
    double alt_r = hcor_r * cos(radians(hrot));

    m_AzCorr.X = -az_r * sin(radians(harot));
    m_AzCorr.Y = az_r * cos(radians(harot));
    m_AltCorr.X = alt_r * cos(radians(harot));
    m_AltCorr.Y = alt_r * sin(radians(harot));
    Debug.AddLine(wxString::Format("StaticPA CalcAdjust: Angles: rarot %.1f; ha_deg %.1f; a_ha %.1f; hcor_a %.1f; harot: %.1f", rarot, ha_deg, a_ha, hcor_a, harot));
    Debug.AddLine(wxString::Format("StaticPA CalcAdjust: Errors(px): alt %.1f; az %.1f; tot %.1f", alt_r, az_r, hcor_r));
    SetStatusText(wxString::Format("Polar Alignment Error (arcmin): Alt %.1f; Az %.1f Tot %.1f",
        fabs(alt_r)*g_pxScale/60, fabs(az_r)*g_pxScale/60, fabs(hcor_r)*g_pxScale/60));
}

PHD_Point StaticPaToolWin::Radec2Px(const PHD_Point& radec)
{
    // Convert dec to pixel radius
    double r = (90.0 - fabs(radec.Y)) * 3600 / g_pxScale;

    // Rotate by calibration angle and HA f object taking into account mount rotation (HA)
    double ra_hrs, dec_deg, ra_deg, st_hrs;
    ra_deg = 0.0;
    if (pPointingSource && !pPointingSource->GetCoordinates(&ra_hrs, &dec_deg, &st_hrs))
    {
        ra_deg = norm(ra_hrs * 15.0 + a_ha, 0, 360);
    }
    else
    {
        // If not a goto mount calculate ra_deg from LST assuming mount is in the home position (HA=18h)
        tm j2000_info;
        j2000_info.tm_year = 100;
        j2000_info.tm_mon = 0;  // January is month 0
        j2000_info.tm_mday = 1;
        j2000_info.tm_hour = 12;
        j2000_info.tm_min = 0;
        j2000_info.tm_sec = 0;
        j2000_info.tm_isdst = 0;
        time_t j2000 = mktime(&j2000_info);
        time_t nowutc = time(NULL);
        tm *nowinfo = localtime(&nowutc);
        time_t now = mktime(nowinfo);
        double since = difftime(now, j2000) / 86400.0;
        double hadeg = a_ha;
        ra_deg = norm((280.46061837 + 360.98564736629 * since - hadeg),0,360);
    }

    // Target hour angle - or rather the rotation needed to correct. 
    // HA = LST - RA
    // In NH HA decreases clockwise; RA increases clockwise
    // "Up" is HA=0
    // Sensor "up" is 90deg counterclockwise from mount RA plus rotation
    // Star rotation is RAstar - RAmount
    double a1 = radec.X - (ra_deg - 90.0);
    a1 = norm(a1, 0, 360);

    double l_camAngle;
    l_camAngle = norm((a_flip ? g_camAngle + 180.0 : g_camAngle), 0, 360);
    double a = l_camAngle - a1 * a_hemi;

    PHD_Point px(r * cos(radians(a)), -r * sin(radians(a)));
    return px;
}

PHD_Point StaticPaToolWin::J2000Now(const PHD_Point& radec)
{
    const double JD2000 = 2451545.0;    // Julian day for J2000

    tm j2000_info;
    j2000_info.tm_year = 100;
    j2000_info.tm_mon = 0;  // January is month 0
    j2000_info.tm_mday = 1;
    j2000_info.tm_hour = 12;
    j2000_info.tm_min = 0;
    j2000_info.tm_sec = 0;
    j2000_info.tm_isdst = 0;
    time_t j2000 = mktime(&j2000_info);
    time_t nowutc = time(NULL);
    double JDnow = difftime(nowutc, j2000) / 86400.0; 

    /*
    This code is adapted from paper
    Improvement of the IAU 2000 precession model
    N. Capitaine, P. T. Wallace, J. Chapront
    https://www.aanda.org/articles/aa/full/2005/10/aa1908/aa1908.html
    The order of polynomial to be used was found to be t^5 and the precision of the coefficients 0.1 uas. The following series with
    a 0.1 uas level of precision matches the canonical 4-rotation series to sub-microarcsecond accuracy over 4 centuries:
    zetaA = 2.5976176 + 2306.0809506 t + 0.3019015 t^2 + 0.0179663 t^3 - 0.0000327 t^4 - 0.0000002 t^5
    zedA = -2.5976176 + 2306.0803226 t + 1.0947790 t^2 + 0.0182273 t^3 + 0.0000470 t^4 - 0.0000003 t^5
    thetaA = 2004.1917476 t - 0.4269353 t^2 - 0.0418251 t^3 - 0.0000601 t^4 - 0.0000001 t^5

    In this implementation we use coefficients up to t^3
    */
    double tnow = JDnow / 36525;  // JDNow is days since J2000.0 so no need to subtract JD2000
    double t2 = pow(tnow, 2);
    double t3 = pow(tnow, 3);
    double zed, zeta, theta;            // arcseconds
    double zedrad, zetarad, thetarad;
    zeta = 2.5976176 + 2306.0809506*tnow + 0.3019015*t2 + 0.0179663*t3;
    zetarad = radians(zeta / 3600);
    zed = -2.5976176 + 2306.0803226*tnow + 1.0947790*t2 + 0.0182273*t3;
    zedrad = radians(zed / 3600);
    theta = 2004.1917476*tnow - 0.4269353*t2 - 0.0418251*t3;
    thetarad = radians(theta / 3600);

//  Build the transformation matrix
    double Xx, Xy, Xz, Yx, Yy, Yz, Zx, Zy, Zz;
    Xx = cos(zedrad)*cos(thetarad)*cos(zetarad) - sin(zedrad)*sin(zetarad);
    Yx = -cos(zedrad)*cos(thetarad)*sin(zetarad) - sin(zedrad)*cos(zetarad);
    Zx = -cos(zedrad)*sin(thetarad);
    Xy = sin(zedrad)*cos(thetarad)*cos(zetarad) + cos(zedrad)*sin(zetarad);
    Yy = -sin(zedrad)*cos(thetarad)*sin(zetarad) + cos(zedrad)*cos(zetarad);
    Zy = -sin(zedrad)*sin(thetarad);
    Xz = sin(thetarad)*cos(zetarad);
    Yz = -sin(thetarad)*sin(zetarad);
    Zz = cos(thetarad);

    // Transform coordinates;
    double x0, y0, z0;
    double x, y, z;
    x0 = cos(radians(radec.Y)) * cos(radians(radec.X));
    y0 = cos(radians(radec.Y)) * sin(radians(radec.X));
    z0 = sin(radians(radec.Y));
    x = Xx * x0 + Yx * y0 + Zx*z0;
    y = Xy * x0 + Yy * y0 + Zy*z0;
    z = Xz * x0 + Yz * y0 + Zz*z0;
    double radeg, decdeg;
    radeg = norm(degrees(atan2(y, x)), 0, 360);
    decdeg = degrees(atan2(z, sqrt(1 - z * z)));
    return PHD_Point(radeg, decdeg);
}

void StaticPaToolWin::PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale)
{
    double intens = 255;
    dc.SetPen(wxPen(wxColour(0, intens, intens), 1, wxSOLID));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    for (int i = 0; i < 3; i++)
    {
        if (HasState(i+1))
        {
            dc.DrawCircle(r_pxPos[i].X*scale, r_pxPos[i].Y*scale, 12 * scale);
        }
    }
    if (!IsCalced())
    {
        return;
    }
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColor(intens, 0, intens), 1, wxPENSTYLE_DOT));
    if (a_drawOrbit)
    {
        dc.DrawCircle(r_pxCentre.X*scale, r_pxCentre.Y*scale, r_radius*scale);
    }

    // draw the centre of the circle as a red cross
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
    int region = 10;
    dc.DrawLine((r_pxCentre.X - region)*scale, r_pxCentre.Y*scale, (r_pxCentre.X + region)*scale, r_pxCentre.Y*scale);
    dc.DrawLine(r_pxCentre.X*scale, (r_pxCentre.Y - region)*scale, r_pxCentre.X*scale, (r_pxCentre.Y + region)*scale);

    // Show the centre of the display wth a grey cross
    double xsc = g_dispSz[0] / 2;
    double ysc = g_dispSz[1] / 2;
    dc.SetPen(wxPen(wxColor(intens, intens, intens), 1, wxPENSTYLE_SOLID));
    dc.DrawLine((xsc - region)*scale, ysc*scale, (xsc + region)*scale, ysc*scale);
    dc.DrawLine(xsc*scale, (ysc - region)*scale, xsc*scale, (ysc + region)*scale);

    // Draw orbits for each reference star
    // Caclulate pixel values for the reference stars
    PHD_Point starpx, stardeg;
    double radpx;
    const std::string alpha = "ABCDEFGHIJKL";
    const wxFont& SmallFont =
#if defined(__WXOSX__)
        *wxSMALL_FONT;
#else
        *wxSWISS_FONT;
#endif
    dc.SetFont(SmallFont);
    for (int is = 0; is < poleStars->size(); is++)
    {
        stardeg = PHD_Point(poleStars->at(is).ra, poleStars->at(is).dec);
        starpx = Radec2Px(stardeg);
        radpx = sqrt(pow(starpx.X, 2) + pow(starpx.Y, 2));
        wxColor line_color = (is == a_refStar) ? wxColor(0, intens, 0) : wxColor(intens, intens, 0);
        dc.SetPen(wxPen(line_color, 1, wxPENSTYLE_DOT));
        if (a_drawOrbit)
        {
            dc.DrawCircle(r_pxCentre.X * scale, r_pxCentre.Y * scale, radpx * scale);
        }
        dc.SetPen(wxPen(line_color, 1, wxPENSTYLE_SOLID));
        dc.DrawCircle((r_pxCentre.X + starpx.X) * scale, (r_pxCentre.Y + starpx.Y) * scale, region*scale);
        dc.SetTextForeground(line_color);
        dc.DrawText(wxString::Format("%c", alpha[is]), (r_pxCentre.X + starpx.X + region) * scale, (r_pxCentre.Y + starpx.Y) * scale);
    }
    // Draw adjustment lines for centring the CoR on the display in blue (dec) and red (cone error)
    bool drawCone = false;
    if (drawCone){
        double xr = r_pxCentre.X * scale;
        double yr = r_pxCentre.Y * scale;
        dc.SetPen(wxPen(wxColor(intens, 0, 0), 1, wxPENSTYLE_SOLID));
        dc.DrawLine(xr, yr, xr + m_ConeCorr.X * scale, yr + m_ConeCorr.Y * scale);
        dc.SetPen(wxPen(wxColor(0, 0, intens), 1, wxPENSTYLE_SOLID));
        dc.DrawLine(xr + m_ConeCorr.X * scale, yr + m_ConeCorr.Y * scale,
            xr + m_DecCorr.X * scale + m_ConeCorr.X * scale,
            yr + m_DecCorr.Y * scale + m_ConeCorr.Y * scale);
        dc.SetPen(wxPen(wxColor(intens, intens, intens), 1, wxPENSTYLE_SOLID));
        dc.DrawLine(xr, yr, xr + m_DecCorr.X * scale + m_ConeCorr.X * scale,
            yr + m_DecCorr.Y * scale + m_ConeCorr.Y * scale);
    }
    // Draw adjustment lines for placing the guide star in its correct position relative to the CoR
    // Blue (azimuth) and Red (altitude)
    CalcAdjustments();
    bool drawCorr = true;
    if (drawCorr)
    {
        int idx;
        idx = a_auto ? 1 : 2;
        double xs = r_pxPos[idx].X * scale;
        double ys = r_pxPos[idx].Y * scale;
        dc.SetPen(wxPen(wxColor(intens, 0, 0), 1, wxPENSTYLE_DOT));
        dc.DrawLine(xs, ys, xs + m_AltCorr.X * scale, ys + m_AltCorr.Y * scale);
        dc.SetPen(wxPen(wxColor(0, 188.0*intens / 255.0, intens), 1, wxPENSTYLE_DOT));
        dc.DrawLine(xs + m_AltCorr.X * scale, ys + m_AltCorr.Y * scale,
            xs + m_AltCorr.X * scale + m_AzCorr.X * scale, ys + m_AzCorr.Y * scale + m_AltCorr.Y * scale);
        dc.SetPen(wxPen(wxColor(intens*2/3, intens*2/3, intens*2/3), 1, wxPENSTYLE_DOT));
        dc.DrawLine(xs, ys, xs + m_AltCorr.X * scale + m_AzCorr.X * scale,
            ys + m_AltCorr.Y * scale + m_AzCorr.Y * scale);
    }
}

bool StaticPaToolWin::RotateMount() 
{
    // Initially, assume an offset of 5.0 degrees of the camera from the CoR
    // Calculate how far to move in RA to get a detectable arc
    // Calculate the tangential ditance of that movement
    // Mark the starting position then rotate the mount
    SetStatusText(wxString::Format("Reading Star Position #%d", s_numPos));
    Debug.AddLine(wxString::Format("StaticPA: Reading Star Pos#%d", s_numPos));
    if (s_numPos == 1)
    {
        //   Initially offset is 5 degrees;
        if (!SetParams(5.0)) // s_reqRot, s_reqStep for assumed 5.0 degree PA error
        {
            Debug.AddLine(wxString::Format("StaticPA: Error from SetParams"));
            return RotateFail(wxString::Format("Error setting rotation parameters: Stopping"));
        }
        Debug.AddLine(wxString::Format("StaticPA: Pos#1 s_reqRot=%.1f s_reqStep=%d", s_reqRot, s_reqStep));
        if (!SetStar(s_numPos))
        {
            Debug.AddLine(wxString::Format("StaticPA: Error from SetStar %d", s_numPos));
            return RotateFail(wxString::Format("Error reading star position #%d: Stopping", s_numPos));
        }
        s_numPos++;
        if (!a_auto)
        {
            s_aligning = false;
            if (IsAligned())
            {
                CalcRotationCentre();
            }
        }
        s_totRot = 0.0;
        s_nStep = 0;
        return true;
    }
    if (s_numPos == 2)
    {
        double theta = s_reqRot - s_totRot;
        // Once the mount has rotated theta degrees (as indicated by prevtheta);
        if (!a_auto)
        {
            if (!SetStar(s_numPos))
            {
                Debug.AddLine(wxString::Format("StaticPA: Error from SetStar %d", s_numPos));
                return RotateFail(wxString::Format("Error reading star position #%d: Stopping", s_numPos));
            }
            s_numPos++;
            s_aligning = false;
            if (IsAligned()){
                CalcRotationCentre();
            }
            return true;
        }
        SetStatusText(wxString::Format("Star Pos#2 Step=%d / %d Rotated=%.1f / %.1f deg", s_nStep, s_reqStep, s_totRot, s_reqRot));
        Debug.AddLine(wxString::Format("StaticPA: Star Pos#2 s_nStep=%d / %d s_totRot=%.1f / %.1f deg", s_nStep, s_reqStep, s_totRot, s_reqRot));
        if (pPointingSource->Slewing())  // Wait till the mount has stopped
        {
            return true;
        }
        if (s_totRot < s_reqRot)
        {
            double newtheta = theta / (s_reqStep - s_nStep);
            if (!MoveWestBy(newtheta))
            {
                Debug.AddLine(wxString::Format("StaticPA: Error from MoveWestBy at step %d", s_nStep));
                return RotateFail(wxString::Format("Error moving west step %d: Stopping", s_nStep));
            }
            s_totRot += newtheta;
        }
        else
        {
            if (!SetStar(s_numPos))
            {
                Debug.AddLine(wxString::Format("StaticPA: Error from SetStar %d", s_numPos));
                return RotateFail(wxString::Format("Error reading star position #%d: Stopping", s_numPos));
            }

            // Calclate how far the mount moved compared to the expected movement;
            // This assumes that the mount was rotated through theta degrees;
            // So theta is the total rotation needed for the current offset;
            // And prevtheta is how we have already moved;
            // Recalculate the offset based on the actual movement;
            // CAUTION: This might end up in an endless loop. 
            double actpix = sqrt(pow(r_pxPos[1].X - r_pxPos[0].X, 2) + pow(r_pxPos[1].Y - r_pxPos[0].Y, 2));
            double actsec = actpix * g_pxScale;
            double actoffsetdeg = 90 - degrees(acos(actsec / 3600 / s_reqRot));
            Debug.AddLine(wxString::Format("StaticPA: Star Pos#2 actpix=%.1f actsec=%.1f g_pxScale=%.1f", actpix, actsec, g_pxScale));

            if (actoffsetdeg == 0)
            {
                Debug.AddLine(wxString::Format("StaticPA: Star Pos#2 Mount did not move actoffsetdeg=%.1f", actoffsetdeg));
                return RotateFail(wxString::Format("Star Pos#2 Mount did not move. Calculated polar offset=%.1f deg", actoffsetdeg));
            }
            double prev_rotdg = s_reqRot;
            int prev_nstep = s_reqStep;
            if (!SetParams(actoffsetdeg))
            {
                Debug.AddLine(wxString::Format("StaticPA: Error from SetParams"));
                return RotateFail(wxString::Format("Error setting rotation parameters: Stopping"));
            }
            if (s_reqRot <= prev_rotdg) // Moved far enough: show the adjustment chart
            {
                s_numPos++;
                s_nStep = 0;
                s_totRot = 0.0;
                s_aligning = false;
                CalcRotationCentre();
            }
            else if (s_reqRot > 45)
            {
                Debug.AddLine(wxString::Format("StaticPA: Pos#2 Too close to CoR actoffsetdeg=%.1f s_reqRot=%.1f", actoffsetdeg, s_reqRot));
                return RotateFail(wxString::Format("Star is too close to CoR (%.1f deg) - try another reference star", actoffsetdeg ));
            }
            else
            {
                s_nStep = int(s_reqStep * s_totRot/s_reqRot);
                Debug.AddLine(wxString::Format("StaticPA: Star Pos#2 s_nStep=%d / %d s_totRot=%.1f / %.1f", s_nStep, s_reqStep, s_totRot, s_reqRot));
            }
        }
        return true;
    }
    if (s_numPos == 3)
    {
        if (!a_auto)
        {
            if (!SetStar(s_numPos))
            {
                Debug.AddLine(wxString::Format("StaticPA: Error from SetStar %d", s_numPos));
                return RotateFail(wxString::Format("Error reading star position #%d: Stopping", s_numPos));
            }
            s_numPos++;
            s_aligning = false;
            if (IsAligned()){
                CalcRotationCentre();
            }
            return true;
        }
        s_numPos++;
        return true;
    }
    return true;
}

bool StaticPaToolWin::RotateFail(const wxString& msg)
{
    SetStatusText(msg);
    s_aligning = false;
    if (a_auto) // STOP rotating
    { 
        pPointingSource->AbortSlew();
        s_numPos = 0;
        ClearState();
        FillPanel();
    }
    return false;
}

bool StaticPaToolWin::SetStar(int npos)
{
    int idx = npos - 1;
    // Get X and Y coords from PHD2;
    // idx: 0=B, 1/2/3 = A[idx];
    double cur_dec, cur_st;
    UnsetState(npos);
    if (a_auto && pPointingSource->GetCoordinates(&r_raPos[idx], &cur_dec, &cur_st))
    {
        Debug.AddLine("StaticPA: SetStar failed to get scope coordinates");
        return false;
    }
    r_pxPos[idx].X = -1;
    r_pxPos[idx].Y = -1;
    PHD_Point star = pFrame->pGuider->CurrentPosition();
    if (!star.IsValid())
    {
        return false;
    }
    r_pxPos[idx] = star;
    SetState(npos);
    Debug.AddLine(wxString::Format("StaticPA: Setstar #%d %.0f, %.0f", npos, r_pxPos[idx].X, r_pxPos[idx].Y));
    SetStatusText(wxString::Format("Read Position #%d: %.0f, %.0f", npos, r_pxPos[idx].X, r_pxPos[idx].Y));
    FillPanel();
    return true;
}

bool StaticPaToolWin::SetParams(double newoffset)
{
    double offsetdeg = newoffset;
    double m_offsetpx = offsetdeg * 3600 / g_pxScale;
    Debug.AddLine(wxString::Format("StaticPA:SetParams(newoffset=%.1f) g_pxScale=%.1f m_offsetpx=%.1f a_devpx=%.1f", newoffset, g_pxScale, m_offsetpx, a_devpx));
    if (m_offsetpx < a_devpx)
    {
        Debug.AddLine(wxString::Format("StaticPA: SetParams() Too close to CoR: m_offsetpx=%.1f a_devpx=%.1f", m_offsetpx, a_devpx));
        return false;
    }
    s_reqRot = degrees(acos(1 - a_devpx / m_offsetpx));
    double m_rotpx = s_reqRot * 3600.0 / g_pxScale * sin(radians(offsetdeg));

    int region = pFrame->pGuider->GetSearchRegion();
    s_reqStep = 1;
    if (m_rotpx > region)
    {
        s_reqStep = int(ceil(m_rotpx / region));
    }
    Debug.AddLine(wxString::Format("StaticPA: SetParams() s_reqRot=%.1f m_rotpx=%.1f s_reqStep=%d region=%d", s_reqRot, m_rotpx, s_reqStep, region));
    return true;
}
bool StaticPaToolWin::MoveWestBy(double thetadeg)
{
    bool m_can_slew = pPointingSource && pPointingSource->CanSlew();
    double cur_ra, cur_dec, cur_st;
    if (pPointingSource->GetCoordinates(&cur_ra, &cur_dec, &cur_st))
    {
        Debug.AddLine("StaticPA: MoveWestBy failed to get scope coordinates");
        return false;
    }
    double slew_ra = cur_ra - thetadeg * 24.0 / 360.0;
    slew_ra = slew_ra - 24.0 * floor(slew_ra / 24.0);

    if (pPointingSource->SlewToCoordinatesAsync(slew_ra, cur_dec))
    {
        Debug.AddLine("StaticPA: MoveWestBy: async slew failed");
        return false;
    }

    s_nStep++;
    PHD_Point lockpos = pFrame->pGuider->CurrentPosition();
    if (pFrame->pGuider->SetLockPosToStarAtPosition(lockpos))
    {
        Debug.AddLine("StaticPA: MoveWestBy: Failed to lock star position");
        return false;
    }
    return true;
}

void StaticPaToolWin::CreateStarTemplate(wxDC& dc, const wxPoint& currPt)
{
    dc.SetBackground(*wxGREY_BRUSH);
    dc.Clear();

    double scale = 320.0 / g_camWidth;
    int region = 5;

    dc.SetTextForeground(*wxYELLOW);
    const wxFont& SmallFont =
#if defined(__WXOSX__)
        *wxSMALL_FONT;
#else
        *wxSWISS_FONT;
#endif
    dc.SetFont(SmallFont);

    const std::string alpha = "ABCDEFGHIJKL";
    PHD_Point starpx, stardeg;
    double starsz, starmag;
    // Draw position of each alignment star
    for (int is = 0; is < poleStars->size(); is++)
        {
        stardeg = PHD_Point(poleStars->at(is).ra, poleStars->at(is).dec);
        starmag = poleStars->at(is).mag;

        starsz = 356.0*exp(-0.3*starmag) / g_pxScale;
        starpx = Radec2Px(stardeg);
        dc.SetPen(*wxYELLOW_PEN);
        dc.SetBrush(*wxYELLOW_BRUSH);
        wxPoint starPt = wxPoint(starpx.X*scale, starpx.Y*scale) - currPt + wxPoint(160, 120);
        dc.DrawCircle(starPt.x, starPt.y, starsz*scale);
        dc.DrawText(wxString::Format("%c", alpha[is]), starPt.x + starsz*scale, starPt.y);
    }
    // draw the pole as a red cross
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
    dc.DrawLine(160- region*scale, 120, 160 + region*scale, 120);
    dc.DrawLine(160, 120 - region*scale, 160, 120 + region*scale);
    dc.DrawLine(160, 120, 160-currPt.x, 120-currPt.y);
    return;
}



