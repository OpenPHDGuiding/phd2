/*
*  guiding_assistant.cpp
*  PHD Guiding
*
*  Created by Andy Galasso and Bruce Waddington
*  Copyright (c) 2015 Andy Galasso and Bruce Waddington
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
#include "guiding_assistant.h"
#include "backlash_comp.h"

struct Stats
{
    double alpha_lp;
    double alpha_hp;
    unsigned int n;
    double sum;
    double a;
    double q;
    double hpf;
    double lpf;
    double xprev;
    double peakRawDx;

    void InitStats(double hpfCutoffPeriod, double lpfCutoffPeriod, double samplePeriod)
    {
        alpha_hp = hpfCutoffPeriod / (hpfCutoffPeriod + wxMax(1.0, samplePeriod));
        alpha_lp = 1.0 - (lpfCutoffPeriod / (lpfCutoffPeriod + wxMax(1.0, samplePeriod)));
        Reset();
    }

    void Reset()
    {
        n = 0;
        sum = 0.0;
        a = 0.0;
        q = 0.0;
        peakRawDx = 0.0;
    }

    void AddSample(double x)
    {
        if (n == 0)
        {
            // first point
            hpf = lpf = x;
        }
        else
        {
            hpf = alpha_hp * (hpf + x - xprev);
            lpf += alpha_lp * (x - lpf);
        }

        if (n >= 1)
        {
            double const dx = fabs(x - xprev);
            if (dx > peakRawDx)
                peakRawDx = dx;
        }

        xprev = x;

        x = hpf;
        ++n;
        sum += x;
        double const k = (double) n;
        double const a0 = a;
        a += (x - a) / k;
        q += (x - a0) * (x - a);
    }

    void GetMeanAndStdev(double *mean, double *stdev)
    {
        if (n == 0)
        {
            *mean = *stdev = 0.0;
            return;
        }

        double const nn = (double) n;
        *mean = sum / nn;
        *stdev = sqrt(q / nn);
    }
};

inline static void StartRow(int& row, int& column)
{
    ++row;
    column = 0;
}



// Encapsulated struct for implementing the dialog box
struct GuidingAsstWin : public wxDialog
{
    enum DialogState
    {
        STATE_NO_STAR = 0,
        STATE_START_READY = 1,
        STATE_MEASURING = 2,
        STATE_STOPPED = 3
    };
    enum DlgConstants {MAX_BACKLASH_COMP = 2000};

    wxButton *m_start;
    wxButton *m_stop;
    wxTextCtrl *m_report;
    wxStaticText *m_instructions;
    wxGrid *m_statusgrid;
    wxGrid *m_displacementgrid;
    wxGrid *m_othergrid;
    wxFlexGridSizer *m_recommendgrid;
    wxBoxSizer *m_vSizer;
    wxBoxSizer *m_vResultsSizer;
    wxBoxSizer *m_hResultsSizer;
    wxStaticBoxSizer *m_recommend_group;
    wxCheckBox *m_backlashCB;
    wxStaticText *m_backlashInfo;
    wxButton *m_graphBtn;

    wxGridCellCoords m_timestamp_loc;
    wxGridCellCoords m_starmass_loc;
    wxGridCellCoords m_samplecount_loc;
    wxGridCellCoords m_snr_loc;
    wxGridCellCoords m_elapsedtime_loc;
    wxGridCellCoords m_exposuretime_loc;
    wxGridCellCoords m_hfcutoff_loc;
    wxGridCellCoords m_ra_rms_loc;
    wxGridCellCoords m_dec_rms_loc;
    wxGridCellCoords m_total_rms_loc;
    wxGridCellCoords m_ra_peak_loc;
    wxGridCellCoords m_dec_peak_loc;
    wxGridCellCoords m_ra_peakpeak_loc;
    wxGridCellCoords m_ra_drift_loc;
    wxGridCellCoords m_ra_drift_exp_loc;
    wxGridCellCoords m_dec_drift_loc;
    wxGridCellCoords m_pae_loc;
    wxGridCellCoords m_ra_peak_drift_loc;
    wxGridCellCoords m_backlash_loc;
    wxButton *m_raMinMoveButton;
    wxButton *m_decMinMoveButton;
    wxButton *m_decBacklashButton;
    wxStaticText *m_ra_msg;
    wxStaticText *m_dec_msg;
    wxStaticText *m_snr_msg;
    wxStaticText *m_pae_msg;
    wxStaticText *m_backlash_msg;
    wxStaticText *m_exposure_msg;
    wxStaticText *m_calibration_msg;
    double m_ra_val_rec;  // recommended value
    double m_dec_val_rec; // recommended value
    double m_min_exp_rec;
    double m_max_exp_rec;

    DialogState m_dlgState;
    bool m_measuring;
    wxLongLong_t m_startTime;
    PHD_Point m_startPos;
    wxString startStr;
    double m_freqThresh;
    Stats m_statsRA;
    Stats m_statsDec;
    double sumSNR;
    double sumMass;
    double minRA;
    double maxRA;
    double m_lastTime;
    double maxRateRA; // arc-sec per second
    double alignmentError; // arc-minutes

    bool m_guideOutputDisabled;
    bool m_savePrimaryMountEnabled;
    bool m_saveSecondaryMountEnabled;
    bool m_measurementsTaken;
    int  m_origSubFrames;
    bool m_suspectCalibration;

    bool m_measuringBacklash;

    BacklashTool *m_backlashTool;

    GuidingAsstWin();
    ~GuidingAsstWin();

    void OnClose(wxCloseEvent& event);
    void OnMouseMove(wxMouseEvent&);
    void OnAppStateNotify(wxCommandEvent& event);
    void OnStart(wxCommandEvent& event);
    void DoStop(const wxString& status = wxEmptyString);
    void OnStop(wxCommandEvent& event);
    void OnRAMinMove(wxCommandEvent& event);
    void OnDecMinMove(wxCommandEvent& event);
    void OnDecBacklash(wxCommandEvent& event);
    void OnGraph(wxCommandEvent& event);

    wxStaticText *AddRecommendationEntry(const wxString& msg, wxObjectEventFunction handler, wxButton **ppButton);
    wxStaticText *AddRecommendationEntry(const wxString& msg);
    void FillResultCell(wxGrid *pGrid, const wxGridCellCoords& loc, double pxVal, double asVal, const wxString& units1, const wxString& units2, const wxString& extraInfo = wxEmptyString);
    void UpdateInfo(const GuideStepInfo& info);
    void FillInstructions(DialogState eState);
    void MakeRecommendations();
    void LogResults();
    void BacklashStep(const PHD_Point& camLoc);
    void EndBacklashTest(bool normal);
    void BacklashError();
};

static void MakeBold(wxControl *ctrl)
{
    wxFont font = ctrl->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    ctrl->SetFont(font);
}

static void HighlightCell(wxGrid *pGrid, wxGridCellCoords where)
{
    pGrid->SetCellBackgroundColour(where.GetRow(), where.GetCol(), "DARK SLATE GREY");
    pGrid->SetCellTextColour(where.GetRow(), where.GetCol(), "white");
}

struct GridTooltipInfo : public wxObject
{
    wxGrid *grid;
    int gridNum;
    wxGridCellCoords prevCoords;
    GridTooltipInfo(wxGrid *g, int i) : grid(g), gridNum(i) { }
};

// Constructor
GuidingAsstWin::GuidingAsstWin()
    : wxDialog(pFrame, wxID_ANY, wxGetTranslation(_("Guiding Assistant"))),
      m_measuring(false),
      m_measurementsTaken(false),
      m_guideOutputDisabled(false),
      m_origSubFrames(-1)
{
    m_vSizer = new wxBoxSizer(wxVERTICAL);
    m_vResultsSizer = new wxBoxSizer(wxVERTICAL);
    m_hResultsSizer = new wxBoxSizer(wxHORIZONTAL);

    m_instructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(500, 70), wxALIGN_LEFT | wxST_NO_AUTORESIZE);
    MakeBold(m_instructions);
    m_vSizer->Add(m_instructions, wxSizerFlags(0).Border(wxALL, 8));

    // Grids have either 3 or 4 columns, so compute width of largest label as scaling term for column widths
    double minCol = StringWidth(this, 
        _(" -999.99 px/min (-999.99 arc-sec/min )")) + 6;
    // Start of status group
    wxStaticBoxSizer *status_group = new wxStaticBoxSizer(wxVERTICAL, this, _("Measurement Status"));
    m_statusgrid = new wxGrid(this, wxID_ANY);
    m_statusgrid->CreateGrid(3, 4);
    m_statusgrid->GetGridWindow()->Bind(wxEVT_MOTION, &GuidingAsstWin::OnMouseMove, this, wxID_ANY, wxID_ANY, new GridTooltipInfo(m_statusgrid, 1));
    m_statusgrid->SetRowLabelSize(1);
    m_statusgrid->SetColLabelSize(1);
    m_statusgrid->EnableEditing(false);
    m_statusgrid->SetDefaultColSize((round(2.0 * minCol / 4.0) + 0.5));

    int col = 0;
    int row = 0;
    m_statusgrid->SetCellValue(row, col++, _("Start time"));
    m_timestamp_loc.Set(row, col++);
    m_statusgrid->SetCellValue(row, col++, _("Exposure time"));
    m_exposuretime_loc.Set(row, col++);

    StartRow(row, col);
    m_statusgrid->SetCellValue(row, col++, _("SNR"));
    m_snr_loc.Set(row, col++);
    m_statusgrid->SetCellValue(row, col++, _("Star mass"));
    m_starmass_loc.Set(row, col++);

    StartRow(row, col);
    m_statusgrid->SetCellValue(row, col++, _("Elapsed time"));
    m_elapsedtime_loc.Set(row, col++);
    m_statusgrid->SetCellValue(row, col++, _("Sample count"));
    m_samplecount_loc.Set(row, col++);

    //StartRow(row, col);
    //m_statusgrid->SetCellValue(_("Frequency cut-off:"), row, col++);   // Leave out for now, probably not useful to users
    //m_hfcutoff_loc.Set(row, col++);

    status_group->Add(m_statusgrid);
    m_vSizer->Add(status_group, wxSizerFlags(0).Border(wxALL, 8));
    // End of status group

    // Start of star displacement group
    wxStaticBoxSizer *displacement_group = new wxStaticBoxSizer(wxVERTICAL, this, _("High-frequency Star Motion"));
    m_displacementgrid = new wxGrid(this, wxID_ANY);
    m_displacementgrid->CreateGrid(3, 2);
    m_displacementgrid->GetGridWindow()->Bind(wxEVT_MOTION, &GuidingAsstWin::OnMouseMove, this, wxID_ANY, wxID_ANY, new GridTooltipInfo(m_displacementgrid, 2));
    m_displacementgrid->SetRowLabelSize(1);
    m_displacementgrid->SetColLabelSize(1);
    m_displacementgrid->EnableEditing(false);
    m_displacementgrid->SetDefaultColSize(minCol);

    row = 0;
    col = 0;
    m_displacementgrid->SetCellValue(row, col++, _("Right ascension, RMS"));
    m_ra_rms_loc.Set(row, col++);

    StartRow(row, col);
    m_displacementgrid->SetCellValue(row, col++, _("Declination, RMS"));
    m_dec_rms_loc.Set(row, col++);

    StartRow(row, col);
    m_displacementgrid->SetCellValue(row, col++, _("Total, RMS"));
    m_total_rms_loc.Set(row, col++);

    displacement_group->Add(m_displacementgrid);
    m_vResultsSizer->Add(displacement_group, wxSizerFlags(0).Border(wxALL, 8));
    // End of displacement group

    // Start of "Other" (peak and drift) group
    wxStaticBoxSizer *other_group = new wxStaticBoxSizer(wxVERTICAL, this, _("Other Star Motion"));
    m_othergrid = new wxGrid(this, wxID_ANY);
    m_othergrid->CreateGrid(9, 2);
    m_othergrid->GetGridWindow()->Bind(wxEVT_MOTION, &GuidingAsstWin::OnMouseMove, this, wxID_ANY, wxID_ANY, new GridTooltipInfo(m_othergrid, 3));
    m_othergrid->SetRowLabelSize(1);
    m_othergrid->SetColLabelSize(1);
    m_othergrid->EnableEditing(false);
    m_othergrid->SetDefaultColSize(minCol);

    row = 0;
    col = 0;
    m_othergrid->SetCellValue(row, col++, _("Right ascension, Peak"));
    m_ra_peak_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Declination, Peak"));
    m_dec_peak_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Right ascension, Peak-Peak"));
    m_ra_peakpeak_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Right ascension Drift Rate"));
    m_ra_drift_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Right ascension Max Drift Rate"));
    m_ra_peak_drift_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Drift-limiting exposure"));
    m_ra_drift_exp_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Declination Drift Rate"));
    m_dec_drift_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Declination Backlash"));
    m_backlash_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(row, col++, _("Polar Alignment Error"));
    m_pae_loc.Set(row, col++);

    other_group->Add(m_othergrid);
    m_vResultsSizer->Add(other_group, wxSizerFlags(0).Border(wxALL, 8));
    // End of peak and drift group

    // Start of Recommendations group - just a place-holder for layout, populated in MakeRecommendations
    m_recommend_group = new wxStaticBoxSizer(wxVERTICAL, this, _("Recommendations"));
    m_recommendgrid = new wxFlexGridSizer(2, 0, 0);
    m_recommendgrid->AddGrowableCol(0);
    m_ra_msg = NULL;
    m_dec_msg = NULL;
    m_snr_msg = NULL;
    m_backlash_msg = NULL;
    m_pae_msg = NULL;
    m_exposure_msg = NULL;
    m_calibration_msg = NULL;

    m_recommend_group->Add(m_recommendgrid, wxSizerFlags(1).Expand());
    // Put the recommendation block at the bottom so it can be hidden/shown
    m_hResultsSizer->Add(m_vResultsSizer);
    m_hResultsSizer->Add(m_recommend_group, wxSizerFlags(0).Border(wxALL, 8));

    m_vSizer->Add(m_hResultsSizer);
    //m_vSizer->Add(m_recommend_group, wxSizerFlags(1).Border(wxALL, 8).Expand());
    m_recommend_group->Show(false);
    // End of recommendations

    wxStaticBoxSizer *bl_group = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Dec Backlash"));
    m_backlashCB = new wxCheckBox(this, wxID_ANY, _("Measure Declination Backlash"));
    m_backlashCB->SetToolTip(_("PHD2 will move the guide star a considerable distance north, then south to measure backlash. Be sure the selected star has "
        "plenty of room to move in the north direction.  If the guide star is lost, increase the size of the search region to at least 20 px"));
    if (!pMount->IsStepGuider())
    {
        m_backlashCB->SetValue(true);
        m_backlashCB->Enable(true);
    }
    else
    {
        m_backlashCB->SetValue(false);
        m_backlashCB->Enable(false);
    }
    m_graphBtn = new wxButton(this, wxID_ANY, _("Show Graph"));
    m_graphBtn->SetToolTip(_("Show graph of backlash measurement points"));
    bl_group->Add(m_backlashCB, wxSizerFlags(0).Border(wxALL, 8));
    bl_group->Add(m_graphBtn, wxSizerFlags(0).Border(wxLEFT, 30));
    m_graphBtn->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GuidingAsstWin::OnGraph), NULL, this);
    m_vSizer->Add(bl_group, wxSizerFlags(0).Border(wxALL, 8).Center());
    m_graphBtn->Enable(false);

    m_backlashInfo = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(500, 40), wxALIGN_CENTER);
    MakeBold(m_backlashInfo);
    m_vSizer->Add(m_backlashInfo, wxSizerFlags(0).Border(wxALL, 8).Center());
    m_backlashInfo->Show(false);

    wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(0, 0, 1, wxEXPAND, 5);

    m_start = new wxButton(this, wxID_ANY, _("Start"), wxDefaultPosition, wxDefaultSize, 0);
    m_start->SetToolTip(_("Start measuring (disables guiding)"));
    btnSizer->Add(m_start, 0, wxALL, 5);
    m_start->Enable(false);

    m_stop = new wxButton(this, wxID_ANY, _("Stop"), wxDefaultPosition, wxDefaultSize, 0);
    m_stop->SetToolTip(_("Stop measuring and re-enable guiding"));
    m_stop->Enable(false);

    btnSizer->Add(m_stop, 0, wxALL, 5);
    btnSizer->Add(0, 0, 1, wxEXPAND, 5);
    m_vSizer->Add(btnSizer, 0, wxEXPAND, 5);



    SetAutoLayout(true);
    SetSizerAndFit(m_vSizer);

    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(GuidingAsstWin::OnClose));
    Connect(APPSTATE_NOTIFY_EVENT, wxCommandEventHandler(GuidingAsstWin::OnAppStateNotify));
    m_start->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GuidingAsstWin::OnStart), NULL, this);
    m_stop->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GuidingAsstWin::OnStop), NULL, this);

    m_backlashTool = new BacklashTool();
    m_measuringBacklash = false;

    int xpos = pConfig->Global.GetInt("/GuidingAssistant/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/GuidingAssistant/pos.y", -1);
    MyFrame::PlaceWindowOnScreen(this, xpos, ypos);

    wxCommandEvent dummy;
    OnAppStateNotify(dummy); // init state-dependent controls

    if (pFrame->pGuider->IsGuiding())
    {
        OnStart(dummy);             // Auto-start if we're already guiding
    }
}

GuidingAsstWin::~GuidingAsstWin(void)
{
    pFrame->pGuidingAssistant = 0;
    delete m_backlashTool;
}

static bool GetGridToolTip(int gridNum, const wxGridCellCoords& coords, wxString *s)
{
    int col = coords.GetCol();

    if (gridNum > 1 && col != 0)
        return false;
    else
    if (col != 0 && col != 2)
        return false;

    switch (gridNum * 100 + coords.GetRow())
    {
        // status grid
        case 101:
        {
            if (col == 0)
                *s = _("Signal-to-noise ratio; a measure of how well PHD2 can isolate the star from the sky/noise background");
            else
                *s = _("Measure of overall star brightness. Consider using 'Auto-select Star' (Alt-S) to choose the star.");
            break;
        }

        // displacement grid
        case 200: *s = _("Measure of typical high-frequency right ascension star movements; guiding usually cannot correct for fluctuations this small."); break;
        case 201: *s = _("Measure of typical high-frequency declination star movements; guiding usually cannot correct for fluctuations this small."); break;

        // other grid
        case 300: *s = _("Maximum sample-sample deflection seen in right ascension."); break;
        case 301: *s = _("Maximum sample-sample deflection seen in declination."); break;
        case 302: *s = _("Maximum peak-peak deflection seen in right ascension during sampling period."); break;
        case 303: *s = _("Estimated overall drift rate in right ascension."); break;
        case 304: *s = _("Maximum drift rate in right ascension during sampling period."); break;
        case 305: *s = _("Exposure time to keep maximum RA drift below the recommended min-move level."); break;
        case 306: *s = _("Estimated overall drift rate in declination."); break;
        case 307: *s = _("Estimate of declination backlash if backlash testing was completed successfully"); break;
        case 308: *s = _("Estimate of polar alignment error. If the scope declination is unknown, the value displayed is a lower bound and the actual error may be larger."); break;

        default: return false;
    }

    return true;
}

void GuidingAsstWin::OnMouseMove(wxMouseEvent& ev)
{
    GridTooltipInfo *info = static_cast<GridTooltipInfo *>(ev.GetEventUserData());
    wxGridCellCoords coords(info->grid->XYToCell(info->grid->CalcUnscrolledPosition(ev.GetPosition())));
    if (coords != info->prevCoords)
    {
        info->prevCoords = coords;
        wxString s;
        if (GetGridToolTip(info->gridNum, coords, &s))
            info->grid->GetGridWindow()->SetToolTip(s);
        else
            info->grid->GetGridWindow()->UnsetToolTip();
    }
    ev.Skip();
}

void GuidingAsstWin::FillInstructions(DialogState eState)
{
    wxString instr;

    switch (eState)
    {
    case STATE_NO_STAR:
        instr = _("Choose a non-saturated star with a good SNR (>= 8) and begin guiding");
        break;
    case STATE_START_READY:
        if (!m_measurementsTaken)
            instr = _("Click Start to begin measurements.  Guiding will be disabled during this time, so the star will move around.");
        else
            instr = m_instructions->GetLabel();
        break;
    case STATE_MEASURING:
        instr = _("Guiding output is disabled and star movement is being measured.  Click Stop when the RMS values have stabilized (at least 1 minute).");
        break;
    case STATE_STOPPED:
        instr = _("Guiding has been resumed. Look at the recommendations and make any desired changes.  Click Start to repeat the measurements, or close the window to continue guiding.");
        break;
    }
    m_instructions->SetLabel(instr);
    m_instructions->Wrap(500);
    m_instructions->Layout();
}

void GuidingAsstWin::BacklashStep(const PHD_Point& camLoc)
{
    m_backlashTool->DecMeasurementStep(camLoc);
    wxString bl_msg = _("Measuring backlash: ") + m_backlashTool->GetLastStatus();
    m_backlashInfo->SetLabel(bl_msg);
    if (m_backlashTool->GetBltState() == BacklashTool::BLT_STATE_COMPLETED)
    {
        m_backlashTool->DecMeasurementStep(camLoc);
        wxString bl_msg = _("Measuring backlash: ") + m_backlashTool->GetLastStatus();
        m_backlashInfo->SetLabel(bl_msg);
        if (m_backlashTool->GetBltState() == BacklashTool::BLT_STATE_COMPLETED)
        {
            BacklashTool::MeasurementResults qual = m_backlashTool->GetMeasurementQuality();
            if (qual != BacklashTool::MEASUREMENT_INVALID)
            {
                wxString preamble = qual == BacklashTool::MEASUREMENT_IMPAIRED ? ">=" : "";
                m_othergrid->SetCellValue(m_backlash_loc, wxString::Format("%s% .1f %s ( %d %s)", 
                    preamble, m_backlashTool->GetBacklashResultPx(), _("px"), m_backlashTool->GetBacklashResultMs(), _("ms")));
                HighlightCell(m_othergrid, m_backlash_loc);
                m_graphBtn->Enable(true);
            }
            else
            {
                m_othergrid->SetCellValue(m_backlash_loc, "");
            }
            EndBacklashTest(true);
        }
    }
}

void GuidingAsstWin::BacklashError()
{
    EndBacklashTest(false);
}

// Event handlers for applying recommendations
void GuidingAsstWin::OnRAMinMove(wxCommandEvent& event)
{
    GuideAlgorithm *raAlgo = pMount->GetXGuideAlgorithm();

    if (!raAlgo)
        return;

    if (raAlgo->GetMinMove() >= 0.0)
    {
        if (!raAlgo->SetMinMove(m_ra_val_rec))
        {
            Debug.Write(wxString::Format("GuideAssistant changed RA_MinMove to %0.2f\n", m_ra_val_rec));
            pFrame->pGraphLog->UpdateControls();
            pFrame->NotifyGuidingParam("RA " + raAlgo->GetGuideAlgorithmClassName() + " MinMove ", m_ra_val_rec);
            m_raMinMoveButton->Enable(false);
        }
        else
            Debug.Write("GuideAssistant could not change RA_MinMove\n");
    }
    else
        Debug.Write("GuideAssistant logic flaw, RA algorithm has no MinMove property\n");
}

void GuidingAsstWin::OnDecMinMove(wxCommandEvent& event)
{
    GuideAlgorithm *decAlgo = pMount->GetYGuideAlgorithm();

    if (!decAlgo)
        return;

    if (decAlgo->GetMinMove() >= 0.0)
    {
        if (!decAlgo->SetMinMove(m_dec_val_rec))
        {
            Debug.Write(wxString::Format("GuideAssistant changed Dec_MinMove to %0.2f\n", m_dec_val_rec));
            pFrame->pGraphLog->UpdateControls();
            pFrame->NotifyGuidingParam("Declination " + decAlgo->GetGuideAlgorithmClassName() + " MinMove ", m_dec_val_rec);
            m_decMinMoveButton->Enable(false);
        }
        else
            Debug.Write("GuideAssistant could not change Dec_MinMove\n");
    }
    else
        Debug.Write("GuideAssistant logic flaw, Dec algorithm has no MinMove property\n");
}

void GuidingAsstWin::OnDecBacklash(wxCommandEvent& event)
{
    BacklashComp *pComp = TheScope()->GetBacklashComp();

    pComp->SetBacklashPulse(m_backlashTool->GetBacklashResultMs());
    pComp->EnableBacklashComp(!pMount->IsStepGuider());
    m_decBacklashButton->Enable(false);
}

void GuidingAsstWin::OnGraph(wxCommandEvent& event)
{
    m_backlashTool->ShowGraph(this);
}

// Adds a recommendation string and a button bound to the passed event handler
wxStaticText *GuidingAsstWin::AddRecommendationEntry(const wxString& msg, wxObjectEventFunction handler, wxButton **ppButton)
{
    wxStaticText *rec_label;

    rec_label = new wxStaticText(this, wxID_ANY, msg);
    rec_label->Wrap(250);
    m_recommendgrid->Add(rec_label, 1, wxALIGN_LEFT | wxALL, 5);
    if (handler)
    {
        int min_h;
        int min_w;
        this->GetTextExtent(_("Apply"), &min_w, &min_h);
        *ppButton = new wxButton(this, wxID_ANY, _("Apply"), wxDefaultPosition, wxSize(min_w + 8, min_h + 8), 0);
        m_recommendgrid->Add(*ppButton, 0, wxALIGN_RIGHT | wxALL, 5);
        (*ppButton)->Connect(wxEVT_COMMAND_BUTTON_CLICKED, handler, NULL, this);
    }
    else
    {
        wxStaticText *rec_tmp = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
        m_recommendgrid->Add(rec_tmp, 0, wxALL, 5);
    }
    return rec_label;
}

// Jacket for simple addition of a text-only recommendation
wxStaticText *GuidingAsstWin::AddRecommendationEntry(const wxString& msg)
{
    return AddRecommendationEntry(msg, NULL, NULL);
}

void GuidingAsstWin::LogResults()
{
    Debug.Write("Guiding Assistant results follow:\n");
    Debug.Write(wxString::Format("SNR=%s, Samples=%s, Elapsed Time=%s, RA RMS=%s, Dec RMS=%s, Total RMS=%s\n",
        m_statusgrid->GetCellValue(m_snr_loc), m_statusgrid->GetCellValue(m_samplecount_loc), m_statusgrid->GetCellValue(m_elapsedtime_loc),
        m_displacementgrid->GetCellValue(m_ra_rms_loc),
        m_displacementgrid->GetCellValue(m_dec_rms_loc), m_displacementgrid->GetCellValue(m_total_rms_loc)));
    Debug.Write(wxString::Format("RA Peak=%s, RA Peak-Peak %s, RA Drift Rate=%s, Max RA Drift Rate=%s, Drift-Limiting Exp=%s\n",
        m_othergrid->GetCellValue(m_ra_peak_loc),
        m_othergrid->GetCellValue(m_ra_peakpeak_loc), m_othergrid->GetCellValue(m_ra_drift_loc),
        m_othergrid->GetCellValue(m_ra_peak_drift_loc),
        m_othergrid->GetCellValue(m_ra_drift_exp_loc)
        )
        );
    Debug.Write(wxString::Format("Dec Drift Rate=%s, Dec Peak=%s, PA Error=%s\n",
        m_othergrid->GetCellValue(m_dec_drift_loc), m_othergrid->GetCellValue(m_dec_peak_loc),
        m_othergrid->GetCellValue(m_pae_loc)));

    if (m_backlashTool->GetBacklashResultPx() > 0)
    {
        Debug.Write(wxString::Format("Backlash measures: %0.2f px, %d ms\n", m_backlashTool->GetBacklashResultPx(), m_backlashTool->GetBacklashResultMs()));
    }
}

static wxString SizedMsg(wxString msg)
{
    wxString padding = "                                        ";
    if (msg.length() < 70)
        return msg += padding.SubString(0, 70 - msg.length());
    else
        return msg;
}

void GuidingAsstWin::MakeRecommendations()
{
    double rarms;
    double ramean;
    m_statsRA.GetMeanAndStdev(&ramean, &rarms);

    double decrms;
    double decmean;
    bool largeBL = false;
    m_statsDec.GetMeanAndStdev(&decmean, &decrms);

    double multiplier_ra  = 1.28;  // 80% prediction interval
    double multiplier_dec = 1.64;  // 90% prediction interval
    double min_rec_exposure;
    double max_rec_exposure;
    double min_rec_range = 2.0;
    // round up to next multiple of .05, but do not go below 0.10 pixel
    double const unit = 0.05;
    double rounded_rarms = std::max(round(rarms * multiplier_ra / unit + 0.5) * unit, 0.10);
    double rounded_decrms = std::max(round(decrms * multiplier_dec / unit + 0.5) * unit, 0.10);
    CalibrationDetails calDetails;
    TheScope()->GetCalibrationDetails(&calDetails);
    m_suspectCalibration = calDetails.lastIssue != CI_None || m_backlashTool->GetBacklashExempted();

    if (pFrame->GetCameraPixelScale() >= 2.0)
    {
        min_rec_exposure = 2.0;
        max_rec_exposure = 8.0;
    }
    else
    {
        min_rec_exposure = 2.0;
        max_rec_exposure = 4.0;
    }
    m_ra_val_rec = rounded_rarms;
    m_dec_val_rec = rounded_decrms;
    // Need to apply some constraints on the relative ratios because the ra_rms stat can be affected by large PE or drift
    m_ra_val_rec = wxMin(wxMax(m_ra_val_rec, 0.8 * m_dec_val_rec), 1.2 * m_dec_val_rec);        // within 20% of dec recommendation

    LogResults();               // Dump the raw statistics
  
    // Clump the no-button messages at the top

    double drift_exp = round(rarms * multiplier_ra / maxRateRA + 0.4);
    m_min_exp_rec = std::max(1.0, std::min(drift_exp, min_rec_exposure));
    if (drift_exp > m_min_exp_rec)
    {
        if (drift_exp < max_rec_exposure)
            m_max_exp_rec = std::max(drift_exp, m_min_exp_rec + min_rec_range);
        else
            m_max_exp_rec = max_rec_exposure;
    }
    else
        m_max_exp_rec = m_min_exp_rec + min_rec_range;

    // Always make a recommendation on exposure times
    wxString msg = SizedMsg(wxString::Format(_("Try to keep your exposure times in the range of %.1fs to %.1fs"), m_min_exp_rec, m_max_exp_rec));
    if (!m_exposure_msg)
        m_exposure_msg = AddRecommendationEntry(msg);
    else
        m_exposure_msg->SetLabel(msg);
    Debug.Write(wxString::Format("Recommendation: %s\n", m_exposure_msg->GetLabelText()));

    if (m_suspectCalibration)
    {
        wxString msg = _("Consider re-doing your calibration ");
        if (calDetails.lastIssue != CI_None)
            msg += _("(Prior alert)");
        else
            msg += _("(Backlash clearing)");
        if (!m_calibration_msg)
            m_calibration_msg = AddRecommendationEntry(SizedMsg(msg));
        else
            m_calibration_msg->SetLabel(SizedMsg(msg));
        Debug.Write(wxString::Format("Recommendation: %s\n", m_calibration_msg->GetLabelText()));
    }

    if ((sumSNR / (double)m_statsRA.n) < 5.0)
    {
        wxString msg(_("Consider using a brighter star for the test or increasing the exposure time"));
        if (!m_snr_msg)
            m_snr_msg = AddRecommendationEntry(SizedMsg(msg));
        else
            m_snr_msg->SetLabel(SizedMsg(msg));
        Debug.Write(wxString::Format("Recommendation: %s\n", m_snr_msg->GetLabelText()));
    }
    else
    {
        if (m_snr_msg)
            m_snr_msg->SetLabel(wxEmptyString);
    }

    if (alignmentError > 5.0)
    {
        wxString msg = alignmentError < 10.0 ?
            _("Polar alignment error > 5 arc-min; that could probably be improved.") :
            _("Polar alignment error > 10 arc-min; try using the Drift Align tool to improve alignment.");
        if (!m_pae_msg)
            m_pae_msg = AddRecommendationEntry(SizedMsg(msg));
        else
        {
            m_pae_msg->SetLabel(SizedMsg(msg));
            m_pae_msg->Wrap(400);
        }
        Debug.Write(wxString::Format("Recommendation: %s\n", msg));
    }
    else
    {
        if (m_pae_msg)
            m_pae_msg->SetLabel(wxEmptyString);
    }

    if (pMount->GetXGuideAlgorithm() && pMount->GetXGuideAlgorithm()->GetMinMove() >= 0.0)
    {
        if (!m_ra_msg)
        {
            m_ra_msg = AddRecommendationEntry(SizedMsg(wxString::Format(_("Try setting RA min-move to %0.2f"), m_ra_val_rec)),
                wxCommandEventHandler(GuidingAsstWin::OnRAMinMove), &m_raMinMoveButton);
        }
        else
        {
            m_ra_msg->SetLabel(SizedMsg(wxString::Format(_("Try setting RA min-move to %0.2f"), m_ra_val_rec)));
            m_raMinMoveButton->Enable(true);
        }
        Debug.Write(wxString::Format("Recommendation: %s\n", m_ra_msg->GetLabelText()));
    }

    if (pMount->GetYGuideAlgorithm() && pMount->GetYGuideAlgorithm()->GetMinMove() >= 0.0)
    {
        if (!m_dec_msg)
        {
            m_dec_msg = AddRecommendationEntry(SizedMsg(wxString::Format(_("Try setting Dec min-move to %0.2f"), m_dec_val_rec)),
                wxCommandEventHandler(GuidingAsstWin::OnDecMinMove), &m_decMinMoveButton);
        }
        else
        {
            m_dec_msg->SetLabel(SizedMsg(wxString::Format(_("Try setting Dec min-move to %0.2f"), m_dec_val_rec)));
            m_decMinMoveButton->Enable(true);
        }
        Debug.Write(wxString::Format("Recommendation: %s\n", m_dec_msg->GetLabelText()));
    }

    if (m_backlashTool->GetBacklashResultMs() >= 100)
    {
        bool largeBL = m_backlashTool->GetBacklashResultMs() > MAX_BACKLASH_COMP;
        wxString msg;
        if (!largeBL)
            msg = SizedMsg(wxString::Format(_("Try setting a Dec backlash value of %d ms"), m_backlashTool->GetBacklashResultMs()));
        else
            msg = SizedMsg(wxString::Format(_("Backlash is %0.1f px; you may need to guide in only one Dec direction"), m_backlashTool->GetBacklashResultPx()));
        if (!m_backlash_msg)
        {
            m_backlash_msg = AddRecommendationEntry(msg, wxCommandEventHandler(GuidingAsstWin::OnDecBacklash), &m_decBacklashButton);
            m_decBacklashButton->Enable(!largeBL);
        }
        else
        {
            m_backlash_msg->SetLabel(msg);
            m_decBacklashButton->Enable(!largeBL);
        }
        Debug.Write(wxString::Format("Recommendation: %s\n", m_backlash_msg->GetLabelText()));
    }
    else
    {
        if (m_backlash_msg)
            m_backlash_msg->SetLabel(wxEmptyString);
    }

    m_recommend_group->Show(true);

    m_statusgrid->Layout();
    Layout();
    GetSizer()->Fit(this);
    Debug.Write("End of Guiding Assistant output....\n");
}

void GuidingAsstWin::OnStart(wxCommandEvent& event)
{
    if (!pFrame->pGuider->IsGuiding())
        return;

    double exposure = (double) pFrame->RequestedExposureDuration() / 1000.0;
    double lp_cutoff = wxMax(6.0, 3.0 * exposure);
    double hp_cutoff = 1.0;
    m_freqThresh = 1.0 / hp_cutoff;
    m_statsRA.InitStats(hp_cutoff, lp_cutoff, exposure);
    m_statsDec.InitStats(hp_cutoff, lp_cutoff, exposure);

    sumSNR = sumMass = 0.0;

    m_start->Enable(false);
    m_stop->Enable(true);
    m_dlgState = STATE_MEASURING;
    FillInstructions(m_dlgState);
    m_recommend_group->Show(false);
    HighlightCell(m_displacementgrid, m_ra_rms_loc);
    HighlightCell(m_displacementgrid, m_dec_rms_loc);
    HighlightCell(m_displacementgrid, m_total_rms_loc);

    Debug.AddLine("GuidingAssistant: Disabling guide output");

    if (pMount)
    {
        m_savePrimaryMountEnabled = pMount->GetGuidingEnabled();
        pMount->SetGuidingEnabled(false);
    }
    if (pSecondaryMount)
    {
        m_saveSecondaryMountEnabled = pSecondaryMount->GetGuidingEnabled();
        pSecondaryMount->SetGuidingEnabled(false);
    }

    m_guideOutputDisabled = true;

    startStr = wxDateTime::Now().FormatISOCombined(' ');
    m_measuring = true;
    m_startTime = ::wxGetUTCTimeMillis().GetValue();
    SetSizerAndFit(m_vSizer);
}

void GuidingAsstWin::DoStop(const wxString& status)
{
    m_measuring = false;
    m_recommendgrid->Show(true);
    m_dlgState = STATE_STOPPED;
    m_measurementsTaken = true;

    FillInstructions(m_dlgState);

    if (m_guideOutputDisabled)
    {
        Debug.Write(wxString::Format("GuidingAssistant: Re-enabling guide output (%d, %d)\n", m_savePrimaryMountEnabled, m_saveSecondaryMountEnabled));

        if (pMount)
            pMount->SetGuidingEnabled(m_savePrimaryMountEnabled);
        if (pSecondaryMount)
            pSecondaryMount->SetGuidingEnabled(m_saveSecondaryMountEnabled);

        m_guideOutputDisabled = false;
    }

    m_start->Enable(pFrame->pGuider->IsGuiding());
    m_stop->Enable(false);

    if (m_origSubFrames != -1)
    {
        pCamera->UseSubframes = m_origSubFrames ? true : false;
        m_origSubFrames = -1;
    }
}

void GuidingAsstWin::EndBacklashTest(bool normal)
{
    if (!normal)
    {
        m_backlashTool->StopMeasurement();
        m_othergrid->SetCellValue(m_backlash_loc, _("Backlash test aborted..."));
        m_graphBtn->Enable(false);
    }

    m_measuringBacklash = false;
    m_backlashCB->Enable(true);
    m_backlashInfo->Show(false);
    Layout();
    GetSizer()->Fit(this);

    m_start->Enable(pFrame->pGuider->IsGuiding());
    m_stop->Enable(false);
    if (normal)
        MakeRecommendations();
    else
    {
        wxCommandEvent dummy;
        OnAppStateNotify(dummy);                    // Need to get the UI back in synch
    }
    DoStop();
}

void GuidingAsstWin::OnStop(wxCommandEvent& event)
{
    if (m_backlashCB->IsChecked())
    {
        if (!m_measuringBacklash)                               // Run the backlash test after the sampling was completed
        {
            m_measuringBacklash = true;

            if (m_origSubFrames == -1)
                m_origSubFrames = pCamera->UseSubframes ? 1 : 0;
            pCamera->UseSubframes = false;

            m_backlashInfo->SetLabelText(_("Measuring backlash... ") + m_backlashTool->GetLastStatus());
            m_backlashInfo->Show(true);
            Layout();
            GetSizer()->Fit(this);
            m_backlashCB->Enable(false);                        // Don't let user turn it off once we've started
            m_measuring = false;
            m_backlashTool->StartMeasurement();
            m_instructions->SetLabel(_("Measuring backlash... "));
        }
        else
        {
            MakeRecommendations();
            EndBacklashTest(false);
        }
    }
    else
    {
        MakeRecommendations();
        DoStop();
    }
}

void GuidingAsstWin::OnAppStateNotify(wxCommandEvent& WXUNUSED(event))
{
    if (m_measuring || m_measuringBacklash)
    {
        if (!pFrame->pGuider->IsGuiding())
        {
            // if guiding stopped, stop measuring
            DoStop(_("Guiding stopped"));
        }
    }
    else
    {
        bool can_start = pFrame->pGuider->IsGuiding();
        m_start->Enable(can_start);
        if (can_start)
            m_dlgState = STATE_START_READY;
        else
            m_dlgState = STATE_NO_STAR;
        FillInstructions(m_dlgState);
    }
}

void GuidingAsstWin::OnClose(wxCloseEvent& evt)
{
    DoStop();

    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/GuidingAssistant/pos.x", x);
    pConfig->Global.SetInt("/GuidingAssistant/pos.y", y);

    Destroy();
}

void GuidingAsstWin::FillResultCell(wxGrid *pGrid, const wxGridCellCoords& loc, double pxVal, double asVal, const wxString& units1, const wxString& units2,
    const wxString& extraInfo)
{
    pGrid->SetCellValue(loc, wxString::Format("%6.2f %s (%6.2f %s %s)", pxVal, units1, asVal, units2, extraInfo));
}

void GuidingAsstWin::UpdateInfo(const GuideStepInfo& info)
{
    double ra = info.mountOffset.X;
    double dec = info.mountOffset.Y;
    double prevRAlpf = m_statsRA.lpf;

    m_statsRA.AddSample(ra);
    m_statsDec.AddSample(dec);

    if (m_statsRA.n == 1)
    {
        minRA = maxRA = ra;
        m_startPos = info.mountOffset;
        maxRateRA = 0.0;
    }
    else
    {
        if (ra < minRA)
            minRA = ra;
        if (ra > maxRA)
            maxRA = ra;

        double dt = info.time - m_lastTime;
        if (dt > 0.0001)
        {
            double raRate = fabs(m_statsRA.lpf - prevRAlpf) / dt;
            if (raRate > maxRateRA)
                maxRateRA = raRate;
        }
    }
    double rangeRA = maxRA - minRA;
    double driftRA = ra - m_startPos.X;
    double driftDec = dec - m_startPos.Y;

    m_lastTime = info.time;
    sumSNR += info.starSNR;
    sumMass += info.starMass;

    double ramean, rarms;
    double decmean, decrms;
    double pxscale = pFrame->GetCameraPixelScale();

    m_statsRA.GetMeanAndStdev(&ramean, &rarms);
    m_statsDec.GetMeanAndStdev(&decmean, &decrms);

    double n = (double) m_statsRA.n;
    double combined = hypot(rarms, decrms);

    wxLongLong_t elapsedms = ::wxGetUTCTimeMillis().GetValue() - m_startTime;
    double elapsed = (double) elapsedms / 1000.0;

    double raDriftRate = driftRA / elapsed * 60.0;
    double decDriftRate = driftDec / elapsed * 60.0;
    double declination = pPointingSource->GetDeclination();
    double cosdec;
    if (declination == UNKNOWN_DECLINATION)
        cosdec = 1.0; // assume declination 0
    else
        cosdec = cos(declination);
    // polar alignment error from Barrett:
    // http://celestialwonders.com/articles/polaralignment/PolarAlignmentAccuracy.pdf
    alignmentError = 3.8197 * fabs(decDriftRate) * pxscale / cosdec;

    wxString SEC(_("s"));
    wxString PX(_("px"));
    wxString ARCSEC(_("arc-sec"));
    wxString ARCMIN(_("arc-min"));
    wxString PXPERMIN(_("px/min"));
    wxString PXPERSEC(_("px/sec"));
    wxString ARCSECPERMIN(_("arc-sec/min"));
    wxString ARCSECPERSEC(_("arc-sec/sec"));
    //wxString HZ(_("Hz"));

    m_statusgrid->SetCellValue(m_timestamp_loc, startStr);
    m_statusgrid->SetCellValue(m_exposuretime_loc, wxString::Format("%g%s", (double)pFrame->RequestedExposureDuration() / 1000.0, SEC));
    m_statusgrid->SetCellValue(m_snr_loc, wxString::Format("%.1f", sumSNR / n));
    m_statusgrid->SetCellValue(m_starmass_loc, wxString::Format("%.1f", sumMass / n));
    m_statusgrid->SetCellValue(m_elapsedtime_loc, wxString::Format("%u%s", (unsigned int)(elapsedms / 1000), SEC));
    m_statusgrid->SetCellValue(m_samplecount_loc, wxString::Format("%.0f", n));

    FillResultCell(m_displacementgrid, m_ra_rms_loc, rarms, rarms * pxscale, PX, ARCSEC);
    FillResultCell(m_displacementgrid, m_dec_rms_loc, decrms, decrms * pxscale, PX, ARCSEC);
    FillResultCell(m_displacementgrid, m_total_rms_loc, combined, combined * pxscale, PX, ARCSEC);

    FillResultCell(m_othergrid, m_ra_peak_loc, m_statsRA.peakRawDx, m_statsRA.peakRawDx * pxscale, PX, ARCSEC);
    FillResultCell(m_othergrid, m_dec_peak_loc, m_statsDec.peakRawDx, m_statsDec.peakRawDx * pxscale, PX, ARCSEC);
    FillResultCell(m_othergrid, m_ra_peakpeak_loc, rangeRA, rangeRA * pxscale, PX, ARCSEC);
    FillResultCell(m_othergrid, m_ra_drift_loc, raDriftRate, raDriftRate * pxscale, PXPERMIN, ARCSECPERMIN);
    FillResultCell(m_othergrid, m_ra_peak_drift_loc, maxRateRA, maxRateRA * pxscale, PXPERSEC, ARCSECPERSEC);
    m_othergrid->SetCellValue(m_ra_drift_exp_loc, maxRateRA <= 0.0 ? _(" ") :
        wxString::Format("%6.1f %s ",  1.3 * rarms / maxRateRA, SEC));
    FillResultCell(m_othergrid, m_dec_drift_loc, decDriftRate, decDriftRate * pxscale, PXPERMIN, ARCSECPERMIN);
    m_othergrid->SetCellValue(m_pae_loc, wxString::Format("%s %.1f %s", declination == UNKNOWN_DECLINATION ? "> " : "", alignmentError, ARCMIN));
}

wxWindow *GuidingAssistant::CreateDialogBox()
{
    return new GuidingAsstWin();
}

void GuidingAssistant::NotifyGuideStep(const GuideStepInfo& info)
{
    if (pFrame && pFrame->pGuidingAssistant)
    {
        GuidingAsstWin *win = static_cast<GuidingAsstWin *>(pFrame->pGuidingAssistant);
        if (win->m_measuring)
            win->UpdateInfo(info);
    }
}

void GuidingAssistant::NotifyFrameDropped(const FrameDroppedInfo& info)
{
    if (pFrame && pFrame->pGuidingAssistant)
    {
        // anything needed?
    }
}

void GuidingAssistant::NotifyBacklashStep(const PHD_Point& camLoc)
{
    if (pFrame && pFrame->pGuidingAssistant)
    {
        GuidingAsstWin *win = static_cast<GuidingAsstWin *>(pFrame->pGuidingAssistant);
        if (win->m_measuringBacklash)
            win->BacklashStep(camLoc);
    }
}

void GuidingAssistant::NotifyBacklashError()
{
    if (pFrame && pFrame->pGuidingAssistant)
    {
        GuidingAsstWin *win = static_cast<GuidingAsstWin *>(pFrame->pGuidingAssistant);
        if (win->m_measuringBacklash)
            win->BacklashError();
    }
}

void GuidingAssistant::UpdateUIControls()
{
    // notify GuidingAssistant window to update its controls
    if (pFrame && pFrame->pGuidingAssistant)
    {
        wxCommandEvent event(APPSTATE_NOTIFY_EVENT, pFrame->GetId());
        event.SetEventObject(pFrame);
        wxPostEvent(pFrame->pGuidingAssistant, event);
    }
}
