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

struct Stats
{
    double alpha;
    unsigned int n;
    double sum;
    double a;
    double q;
    double hpf;
    double lpf;
    double xprev;
    double peakRawDx;

    void InitStats(double hpfCutoffPeriod, double samplePeriod)
    {
        alpha = hpfCutoffPeriod / (hpfCutoffPeriod + samplePeriod);
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
            hpf = alpha * (hpf + x - xprev);
            lpf += (1.0 - alpha) * (x - xprev);
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

    wxButton *m_start;
    wxButton *m_stop;
    wxTextCtrl *m_report;
    wxStaticText *m_instructions;
    wxGrid *m_statusgrid;
    wxGrid *m_displacementgrid;
    wxGrid *m_othergrid;
    wxGridSizer *m_recommendgrid;
    wxBoxSizer* m_vSizer;
    wxStaticBoxSizer* m_recommend_group;

    wxGridCellCoords m_timestamp_loc;
    wxGridCellCoords m_starmass_loc;
    wxGridCellCoords m_samplecount_loc;
    wxGridCellCoords m_snr_loc;
    wxGridCellCoords m_elapsedtime_loc;
    wxGridCellCoords m_exposuretime_loc;
    wxGridCellCoords m_hfcutoff_loc;
    wxGridCellCoords m_ra_rms_px_loc;
    wxGridCellCoords m_ra_rms_as_loc;
    wxGridCellCoords m_dec_rms_px_loc;
    wxGridCellCoords m_dec_rms_as_loc;
    wxGridCellCoords m_total_rms_px_loc;
    wxGridCellCoords m_total_rms_as_loc;
    wxGridCellCoords m_ra_peak_px_loc;
    wxGridCellCoords m_ra_peak_as_loc;
    wxGridCellCoords m_dec_peak_px_loc;
    wxGridCellCoords m_dec_peak_as_loc;
    wxGridCellCoords m_ra_peakpeak_px_loc;
    wxGridCellCoords m_ra_peakpeak_as_loc;
    wxGridCellCoords m_ra_drift_px_loc;
    wxGridCellCoords m_ra_drift_as_loc;
    wxGridCellCoords m_dec_drift_px_loc;
    wxGridCellCoords m_dec_drift_as_loc;
    wxGridCellCoords m_ra_peak_drift_px_loc;
    wxGridCellCoords m_ra_peak_drift_as_loc;
    wxButton* m_raMinMoveButton;
    wxButton* m_decMinMoveButton;
    wxStaticText* m_ra_msg;
    wxStaticText* m_dec_msg;
    wxStaticText* m_snr_msg;

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

    bool m_savePrimaryMountEnabled;
    bool m_saveSecondaryMountEnabled;
    bool m_measurementsTaken;

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

    wxStaticText* AddRecommendationEntry(const wxString& msg, wxObjectEventFunction handler, wxButton** ppButton);
    wxStaticText* AddRecommendationEntry(const wxString& msg);
    void UpdateInfo(const GuideStepInfo& info);
    void FillInstructions(DialogState eState);
    void MakeRecommendations();
};

static void MakeBold(wxControl* ctrl)
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
: wxDialog(pFrame, wxID_ANY, wxGetTranslation(_("Guiding Assistant")), wxPoint(-1, -1), wxDefaultSize),
    m_measuring(false), m_measurementsTaken(false)
{
    m_vSizer = new wxBoxSizer(wxVERTICAL);

    m_instructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(500, 40), wxALIGN_LEFT | wxST_NO_AUTORESIZE);
    MakeBold(m_instructions);
    m_vSizer->Add(m_instructions, wxSizerFlags(0).Border(wxALL, 8).Center());

    // Grids have either 3 or 4 columns, so compute width of largest label as scaling term for column widths
    double minCol = wxMax(160, StringWidth(this, _("Right ascension Max Drift Rate")) + 10);
    // Start of status group
    wxStaticBoxSizer* status_group = new wxStaticBoxSizer(wxVERTICAL, this, _("Measurement Status"));
    m_statusgrid = new wxGrid(this, wxID_ANY);
    m_statusgrid->CreateGrid(3, 4);
    m_statusgrid->GetGridWindow()->Bind(wxEVT_MOTION, &GuidingAsstWin::OnMouseMove, this, wxID_ANY, wxID_ANY, new GridTooltipInfo(m_statusgrid, 1));
    m_statusgrid->SetRowLabelSize(1);
    m_statusgrid->SetColLabelSize(1);
    m_statusgrid->EnableEditing(false);
    m_statusgrid->SetDefaultColSize((round(3.0 * minCol / 4.0) + 0.5));

    int col = 0;
    int row = 0;
    m_statusgrid->SetCellValue(_("Start time"), row, col++);
    m_timestamp_loc.Set(row, col++);
    m_statusgrid->SetCellValue(_("Exposure time"), row, col++);
    m_exposuretime_loc.Set(row, col++);

    StartRow(row, col);
    m_statusgrid->SetCellValue(_("SNR"), row, col++);
    m_snr_loc.Set(row, col++);
    m_statusgrid->SetCellValue(_("Star mass"), row, col++);
    m_starmass_loc.Set(row, col++);

    StartRow(row, col);
    m_statusgrid->SetCellValue(_("Elapsed time"), row, col++);
    m_elapsedtime_loc.Set(row, col++);
    m_statusgrid->SetCellValue(_("Sample count"), row, col++);
    m_samplecount_loc.Set(row, col++);

    //StartRow(row, col);
    //m_statusgrid->SetCellValue(_("Frequency cut-off:"), row, col++);   // Leave out for now, probably not useful to users
    //m_hfcutoff_loc.Set(row, col++);

    status_group->Add(m_statusgrid);
    m_vSizer->Add(status_group, wxSizerFlags(0).Border(wxALL, 8));
    // End of status group

    // Start of star displacement group
    wxStaticBoxSizer* displacement_group = new wxStaticBoxSizer(wxVERTICAL, this, _("High-frequency Star Motion"));
    m_displacementgrid = new wxGrid(this, wxID_ANY);
    m_displacementgrid->CreateGrid(3, 3);
    m_displacementgrid->GetGridWindow()->Bind(wxEVT_MOTION, &GuidingAsstWin::OnMouseMove, this, wxID_ANY, wxID_ANY, new GridTooltipInfo(m_displacementgrid, 2));
    m_displacementgrid->SetRowLabelSize(1);
    m_displacementgrid->SetColLabelSize(1);
    m_displacementgrid->EnableEditing(false);
    m_displacementgrid->SetDefaultColSize(minCol);

    row = 0;
    col = 0;
    m_displacementgrid->SetCellValue(_("Right ascension, RMS"), row, col++);
    m_ra_rms_px_loc.Set(row, col++);
    m_ra_rms_as_loc.Set(row, col++);

    StartRow(row, col);
    m_displacementgrid->SetCellValue(_("Declination, RMS"), row, col++);
    m_dec_rms_px_loc.Set(row, col++);
    m_dec_rms_as_loc.Set(row, col++);

    StartRow(row, col);
    m_displacementgrid->SetCellValue(_("Total, RMS"), row, col++);
    m_total_rms_px_loc.Set(row, col++);
    m_total_rms_as_loc.Set(row, col++);

    displacement_group->Add(m_displacementgrid);
    m_vSizer->Add(displacement_group, wxSizerFlags(0).Border(wxALL, 8));
    // End of displacement group

    // Start of "Other" (peak and drift) group
    wxStaticBoxSizer* other_group = new wxStaticBoxSizer(wxVERTICAL, this, _("Other Star Motion"));
    m_othergrid = new wxGrid(this, wxID_ANY);
    m_othergrid->CreateGrid(6, 3);
    m_othergrid->GetGridWindow()->Bind(wxEVT_MOTION, &GuidingAsstWin::OnMouseMove, this, wxID_ANY, wxID_ANY, new GridTooltipInfo(m_othergrid, 3));
    m_othergrid->SetRowLabelSize(1);
    m_othergrid->SetColLabelSize(1);
    m_othergrid->EnableEditing(false);
    m_othergrid->SetDefaultColSize(minCol);

    row = 0;
    col = 0;
    m_othergrid->SetCellValue(_("Right ascension, Peak"), row, col++);
    m_ra_peak_px_loc.Set(row, col++);
    m_ra_peak_as_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(_("Declination, Peak"), row, col++);
    m_dec_peak_px_loc.Set(row, col++);
    m_dec_peak_as_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(_("Right ascension, Peak-Peak"), row, col++);
    m_ra_peakpeak_px_loc.Set(row, col++);
    m_ra_peakpeak_as_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(_("Right ascension Drift Rate"), row, col++);
    m_ra_drift_px_loc.Set(row, col++);
    m_ra_drift_as_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(_("Right ascension Max Drift Rate"), row, col++);
    m_ra_peak_drift_px_loc.Set(row, col++);
    m_ra_peak_drift_as_loc.Set(row, col++);

    StartRow(row, col);
    m_othergrid->SetCellValue(_("Declination Drift Rate"), row, col++);
    m_dec_drift_px_loc.Set(row, col++);
    m_dec_drift_as_loc.Set(row, col++);

    other_group->Add(m_othergrid);
    m_vSizer->Add(other_group, wxSizerFlags(0).Border(wxALL, 8));
    // End of peak and drift group

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

    // Start of Recommendations group - just a place-holder for layout, populated in MakeRecommendations
    m_recommend_group = new wxStaticBoxSizer(wxVERTICAL, this, _("Recommendations"));
    m_recommendgrid = new wxGridSizer(2, 0, 0);
    m_ra_msg = NULL;
    m_dec_msg = NULL;
    m_snr_msg = NULL;

    m_recommend_group->Add(m_recommendgrid);
    // Put the recommendation block at the bottom so it can be hidden/shown
    m_vSizer->Add(m_recommend_group, wxSizerFlags(0).Border(wxALL, 8));
    m_recommend_group->Show(false);
    // End of recommendations

    SetAutoLayout(true);
    SetSizerAndFit(m_vSizer);

    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(GuidingAsstWin::OnClose));
    Connect(APPSTATE_NOTIFY_EVENT, wxCommandEventHandler(GuidingAsstWin::OnAppStateNotify));
    m_start->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GuidingAsstWin::OnStart), NULL, this);
    m_stop->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GuidingAsstWin::OnStop), NULL, this);

    int xpos = pConfig->Global.GetInt("/GuidingAssistant/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/GuidingAssistant/pos.y", -1);
    if (xpos == -1 || ypos == -1)
        Centre(wxBOTH);
    else
        Move(xpos, ypos);

    wxCommandEvent dummy;
    OnAppStateNotify(dummy); // init state-dependent controls

    if (pFrame->pGuider->IsGuiding())
    {
        OnStart(dummy);             // Auto-start if we're already guiding
    }
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
            *s = _("Measure of overall star brightness. Consider using 'Auto-select Star' (Alt-s) to choose the star.");
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
        case 304: *s = _("Maximum drift rate in right ascension during sampling period; may be useful for setting exposure time."); break;
        case 305: *s = _("Estimated overall drift rate in declination."); break;

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
        instr = _("Choose a non-saturated star with a good SNR (>10) and begin guiding");
        break;
    case STATE_START_READY:
        if (!m_measurementsTaken)
            instr = _("Click 'Start' to begin measurements.  Guiding will be disabled during this time, so the star will move around.");
        else
            instr = m_instructions->GetLabel();
        break;
    case STATE_MEASURING:
        instr = _("Guiding output is disabled and star movement is being measured.  Click on 'Stop' when the RMS values have stabilized (at least 1 minute).");
        break;
    case STATE_STOPPED:
        instr = _("Guiding has been resumed. Look at the recommendations and make any desired changes.  Click 'Start' to repeat the measurements or close the window to continue guiding.");
        break;
    }
    m_instructions->SetLabel(instr);
}

GuidingAsstWin::~GuidingAsstWin(void)
{
    pFrame->pGuidingAssistant = 0;
}

// Event handlers for applying recommendations
void GuidingAsstWin::OnRAMinMove(wxCommandEvent& event)
{
    double ramean;
    double rarms;
    GuideAlgorithm *raAlgo = pMount->GetXGuideAlgorithm();

    if (raAlgo)
    {
        m_statsRA.GetMeanAndStdev(&ramean, &rarms);
        rarms = (round((rarms * 100) / 5.0) * 5) / 100.0;
        if (raAlgo->GetMinMove() >= 0)
        {
            if (!raAlgo->SetMinMove(rarms))
            {
                Debug.Write(wxString::Format("GuideAssistant changed RA_MinMove to %0.2f\n", rarms));
                pFrame->pGraphLog->UpdateControls();
                GuideLog.SetGuidingParam("RA " + raAlgo->GetGuideAlgorithmClassName() + " MinMove ", rarms);
                m_raMinMoveButton->Enable(false);
            }
            else
                Debug.Write("GuideAssistant could not change RA_MinMove\n");
        }
        else
            Debug.Write("GuideAssistant logic flaw, RA algorithm has no MinMove property");
    }
}

void GuidingAsstWin::OnDecMinMove(wxCommandEvent& event)
{
    double decmean;
    double decrms;
    GuideAlgorithm *decAlgo = pMount->GetYGuideAlgorithm();

    if (decAlgo)
    {
        m_statsDec.GetMeanAndStdev(&decmean, &decrms);
        decrms = (round((decrms * 100) / 5.0) * 5) / 100.0;
        if (decAlgo->GetMinMove() >= 0)
        {
            if (!decAlgo->SetMinMove(decrms))
            {
                Debug.Write(wxString::Format("GuideAssistant changed Dec_MinMove to %0.2f\n", decrms));
                pFrame->pGraphLog->UpdateControls();
                GuideLog.SetGuidingParam("Declination " + decAlgo->GetGuideAlgorithmClassName() + " MinMove ", decrms);
                m_decMinMoveButton->Enable(false);
            }
            else
                Debug.Write("GuideAssistant could not change Dec_MinMove\n");
        }
        else
            Debug.Write("GuideAssistant logic flaw, Dec algorithm has no MinMove property");
    }
}


// Adds a recommendation string and a button bound to the passed event handler
wxStaticText* GuidingAsstWin::AddRecommendationEntry(const wxString& msg, wxObjectEventFunction handler, wxButton** ppButton)
{
    wxStaticText* rec_label = new wxStaticText(this, wxID_ANY, msg, wxPoint(-1, -1), wxSize(300, -1));
    m_recommendgrid->Add(rec_label, 0, wxALIGN_LEFT | wxALL, 5);
    if (handler != NULL)
    {
        *ppButton = new wxButton(this, wxID_ANY, _("Apply"), wxDefaultPosition, wxDefaultSize, 0);
        m_recommendgrid->Add(*ppButton, 0, wxALIGN_RIGHT | wxALL, 5);
        (*ppButton)->Connect(wxEVT_COMMAND_BUTTON_CLICKED, handler, NULL, this);
    }
    else
    {
        wxStaticText* rec_tmp = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(-1, -1), wxDefaultSize);
        m_recommendgrid->Add(rec_tmp, 0, wxALL, 5);
    }
    return rec_label;
}

// Jacket for simple addition of a text-only recommendation
wxStaticText* GuidingAsstWin::AddRecommendationEntry(const wxString& msg)
{
    return AddRecommendationEntry(msg, NULL, NULL);
}

void GuidingAsstWin::MakeRecommendations()
{
    double rarms;
    double ramean;
    double decrms;
    double decmean;
    double rounded_rarms;
    double rounded_decrms;

    m_statsRA.GetMeanAndStdev(&ramean, &rarms);
    m_statsDec.GetMeanAndStdev(&decmean, &decrms);
    // Don't over-state the accuracy here - set things to the nearest .05
    rounded_rarms = (round((rarms * 100) / 5.0) * 5) / 100.0;
    rounded_decrms = (round((decrms * 100) / 5.0) * 5) / 100.0;

    m_recommend_group->Show(true);

    if (pMount->GetXGuideAlgorithm() && pMount->GetXGuideAlgorithm()->GetMinMove() >= 0)
    {
        if (m_ra_msg == NULL)
        {
            m_ra_msg = AddRecommendationEntry(wxString::Format(_("Try setting RA min-move to %0.2f"), rounded_rarms),
                wxCommandEventHandler(GuidingAsstWin::OnRAMinMove), &m_raMinMoveButton);
        }
        else
        {
            m_ra_msg->SetLabel(wxString::Format(_("Try setting RA min-move to %0.2f"), rounded_rarms));
            m_raMinMoveButton->Enable(true);
        }
    }

    if (pMount->GetYGuideAlgorithm() && pMount->GetYGuideAlgorithm()->GetMinMove() >= 0)
    {
        if (m_dec_msg == NULL)
        {
            m_dec_msg = AddRecommendationEntry(wxString::Format(_("Try setting Dec min-move to %0.2f"), rounded_decrms),
                wxCommandEventHandler(GuidingAsstWin::OnDecMinMove), &m_decMinMoveButton);
        }
        else
        {
            m_dec_msg->SetLabel(wxString::Format(_("Try setting Dec min-move to %0.2f"), rounded_decrms));
            m_decMinMoveButton->Enable(true);
        }
    }

    if ((sumSNR / (double)m_statsRA.n) < 10)
    {
        if (m_snr_msg == NULL)
            m_snr_msg = AddRecommendationEntry(_("Consider using a brighter star or increasing the exposure time"));
        else
            m_snr_msg->SetLabel(_("Consider using a brighter star or increasing the exposure time"));
    }
    else
        if (m_snr_msg != NULL)
            m_snr_msg->SetLabel(wxEmptyString);

    Layout();
    GetSizer()->Fit(this);
}

void GuidingAsstWin::OnStart(wxCommandEvent& event)
{
    if (!pFrame->pGuider->IsGuiding())
        return;

    double exposure = (double) pFrame->RequestedExposureDuration() / 1000.0;
    double cutoff = wxMax(6.0, 3.0 * exposure);
    m_freqThresh = 1.0 / cutoff;
    m_statsRA.InitStats(cutoff, exposure);
    m_statsDec.InitStats(cutoff, exposure);

    sumSNR = sumMass = 0.0;

    m_start->Enable(false);
    m_stop->Enable(true);
    m_dlgState = STATE_MEASURING;
    FillInstructions(m_dlgState);
    m_recommend_group->Show(false);
    HighlightCell(m_displacementgrid, m_ra_rms_px_loc);
    HighlightCell(m_displacementgrid, m_dec_rms_px_loc);
    HighlightCell(m_displacementgrid, m_total_rms_px_loc);

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

    Debug.AddLine("GuidingAssistant: Re-enabling guide output");

    if (pMount)
        pMount->SetGuidingEnabled(m_savePrimaryMountEnabled);
    if (pSecondaryMount)
        pSecondaryMount->SetGuidingEnabled(m_saveSecondaryMountEnabled);

    m_start->Enable(pFrame->pGuider->IsGuiding());
    m_stop->Enable(false);
}

void GuidingAsstWin::OnStop(wxCommandEvent& event)
{
    MakeRecommendations();
    DoStop();
}

void GuidingAsstWin::OnAppStateNotify(wxCommandEvent& WXUNUSED(event))
{
    if (m_measuring)
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

void GuidingAsstWin::UpdateInfo(const GuideStepInfo& info)
{
    double ra = info.mountOffset->X;
    double dec = info.mountOffset->Y;
    double prevRAlpf = m_statsRA.lpf;

    m_statsRA.AddSample(ra);
    m_statsDec.AddSample(dec);

    if (m_statsRA.n == 1)
    {
        minRA = maxRA = ra;
        m_startPos = *info.mountOffset;
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



    m_statusgrid->SetCellValue(m_timestamp_loc, startStr);
    m_statusgrid->SetCellValue(m_exposuretime_loc, wxString::Format("%gs", (double)pFrame->RequestedExposureDuration() / 1000.0));
    m_statusgrid->SetCellValue(m_snr_loc, wxString::Format("%.1f", sumSNR / n));
    m_statusgrid->SetCellValue(m_starmass_loc, wxString::Format("%.1f", sumMass / n));
    m_statusgrid->SetCellValue(m_elapsedtime_loc, wxString::Format("%us", (unsigned int)(elapsedms / 1000)));
    m_statusgrid->SetCellValue(m_samplecount_loc, wxString::Format("%.0f", n));
    //m_statusgrid->SetCellValue(m_hfcutoff_loc, wxString::Format("%.2f Hz", m_freqThresh));

    m_displacementgrid->SetCellValue(m_ra_rms_px_loc, wxString::Format("%6.2f px", rarms));
    m_displacementgrid->SetCellValue(m_ra_rms_as_loc, wxString::Format("%6.2f arc-sec", rarms * pxscale));
    m_displacementgrid->SetCellValue(m_dec_rms_px_loc, wxString::Format("%6.2f px", decrms));
    m_displacementgrid->SetCellValue(m_dec_rms_as_loc, wxString::Format("%6.2f arc-sec", decrms * pxscale));
    m_displacementgrid->SetCellValue(m_total_rms_px_loc, wxString::Format("%6.2f px", combined));
    m_displacementgrid->SetCellValue(m_total_rms_as_loc, wxString::Format("%6.2f arc-sec", combined * pxscale));

    m_othergrid->SetCellValue(m_ra_peak_px_loc, wxString::Format("%.1f px", m_statsRA.peakRawDx));
    m_othergrid->SetCellValue(m_ra_peak_as_loc, wxString::Format("%.1f arc-sec", m_statsRA.peakRawDx * pxscale));
    m_othergrid->SetCellValue(m_dec_peak_px_loc, wxString::Format("%.1f px", m_statsDec.peakRawDx));
    m_othergrid->SetCellValue(m_dec_peak_as_loc, wxString::Format("%.1f arc-sec", m_statsRA.peakRawDx * pxscale));
    m_othergrid->SetCellValue(m_ra_peakpeak_px_loc, wxString::Format("%.1f px", rangeRA));
    m_othergrid->SetCellValue(m_ra_peakpeak_as_loc, wxString::Format("%.1f arc-sec", rangeRA * pxscale));
    m_othergrid->SetCellValue(m_ra_drift_px_loc, wxString::Format("%.1f px/min", raDriftRate));
    m_othergrid->SetCellValue(m_ra_drift_as_loc, wxString::Format("%.1f arc-sec/min", raDriftRate * pxscale));
    m_othergrid->SetCellValue(m_ra_peak_drift_px_loc, wxString::Format("%0.1f px/sec", maxRateRA));
    m_othergrid->SetCellValue(m_ra_peak_drift_as_loc, wxString::Format("%0.1f arc-sec/sec (MaxExp: %0.1f)", maxRateRA * pxscale,
        maxRateRA > 0 ? rarms/maxRateRA : 0));
    m_othergrid->SetCellValue(m_dec_drift_px_loc, wxString::Format("%.1f px/min", decDriftRate));
    m_othergrid->SetCellValue(m_dec_drift_as_loc, wxString::Format("%.1f arc-sec/min", decDriftRate * pxscale));
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
