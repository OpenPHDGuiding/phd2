/*
*  guidability_tool.cpp
*  PHD Guiding
*
*  Created by Andy Galasso
*  Copyright (c) 2015 Andy Galasso
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
#include "guidability_tool.h"

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
            lpf += alpha * (x - xprev);
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

struct GuidabilityToolWin : public wxFrame
{
    wxButton *m_start;
    wxButton *m_stop;
    wxTextCtrl *m_report;
    wxStatusBar *m_statusBar;

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

    bool m_savePrimaryMountEnabled;
    bool m_saveSecondaryMountEnabled;

    GuidabilityToolWin();
    ~GuidabilityToolWin();

    void OnClose(wxCloseEvent& event);
    void OnAppStateNotify(wxCommandEvent& event);
    void OnStart(wxCommandEvent& event);
    void DoStop(const wxString& status = wxEmptyString);
    void OnStop(wxCommandEvent& event);

    void UpdateInfo(const GuideStepInfo& info);
};

static wxString TITLE = wxTRANSLATE("Guidability Check");
static wxString TITLE_ACTIVE = wxTRANSLATE("Guidability Check - In Progress");

GuidabilityToolWin::GuidabilityToolWin()
    : wxFrame(pFrame, wxID_ANY, wxGetTranslation(TITLE), wxPoint(-1, -1), wxSize(500, 340)),
    m_measuring(false)
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));

    wxBoxSizer *sizer1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *sizer2 = new wxBoxSizer(wxHORIZONTAL);

    sizer2->Add(0, 0, 1, wxEXPAND, 5);

    m_start = new wxButton(this, wxID_ANY, _("Start"), wxDefaultPosition, wxDefaultSize, 0);
    m_start->SetToolTip(_("Start measuring (disables guiding)"));
    sizer2->Add(m_start, 0, wxALL, 5);
    m_start->Enable(false);

    m_stop = new wxButton(this, wxID_ANY, _("Stop"), wxDefaultPosition, wxDefaultSize, 0);
    m_stop->SetToolTip(_("Stop measuring and re-enable guiding"));
    m_stop->Enable(false);

    sizer2->Add(m_stop, 0, wxALL, 5);
    sizer2->Add(0, 0, 1, wxEXPAND, 5);
    sizer1->Add(sizer2, 0, wxEXPAND, 5);

    sizer1->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    wxStaticText *staticText1 = new wxStaticText(this, wxID_ANY, _("Report"), wxDefaultPosition, wxDefaultSize, 0);
    staticText1->Wrap(-1);
    sizer1->Add(staticText1, 0, wxLEFT | wxRIGHT | wxTOP, 5);

    m_report = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    m_report->SetFont(wxFont(9, 75, 90, 90, false, _("Courier")));
    m_report->SetForegroundColour(wxColour(192, 192, 192));
    m_report->SetBackgroundColour(wxColour(0, 0, 0));

    sizer1->Add(m_report, 1, wxALL | wxEXPAND, 5);

    SetSizer(sizer1);

    m_statusBar = CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);
    Layout();

    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(GuidabilityToolWin::OnClose));
    Connect(APPSTATE_NOTIFY_EVENT, wxCommandEventHandler(GuidabilityToolWin::OnAppStateNotify));
    m_start->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GuidabilityToolWin::OnStart), NULL, this);
    m_stop->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GuidabilityToolWin::OnStop), NULL, this);

    int xpos = pConfig->Global.GetInt("/GuidabilityTool/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/GuidabilityTool/pos.y", -1);
    if (xpos == -1 || ypos == -1)
        Centre(wxBOTH);
    else
        Move(xpos, ypos);

    wxCommandEvent dummy;
    OnAppStateNotify(dummy); // init controls

    if (pFrame->pGuider->IsGuiding())
    {
        OnStart(dummy);
    }
}

GuidabilityToolWin::~GuidabilityToolWin(void)
{
    pFrame->pGuidabilityTool = 0;
}

void GuidabilityToolWin::OnStart(wxCommandEvent& event)
{
    if (!pFrame->pGuider->IsGuiding())
        return;

    double exposure = (double) pFrame->RequestedExposureDuration() / 1000.0;
    double cutoff = wxMax(3.0, 3.0 * exposure);
    m_freqThresh = 1.0 / cutoff;
    m_statsRA.InitStats(cutoff, exposure);
    m_statsDec.InitStats(cutoff, exposure);

    sumSNR = sumMass = 0.0;

    m_start->Enable(false);
    m_stop->Enable(true);

    SetTitle(wxGetTranslation(TITLE_ACTIVE));

    Debug.AddLine("GuidabilityTool: Disabling guide output");

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

    m_statusBar->SetStatusText(_("Measurement in progress, guide output is disabled"));

    startStr = wxDateTime::Now().FormatISOCombined(' ');
    m_measuring = true;
    m_startTime = ::wxGetUTCTimeMillis().GetValue();
}

void GuidabilityToolWin::DoStop(const wxString& status)
{
    m_measuring = false;

    m_statusBar->SetStatusText(status);

    Debug.AddLine("GuidabilityTool: Re-enabling guide output");

    if (pMount)
        pMount->SetGuidingEnabled(m_savePrimaryMountEnabled);
    if (pSecondaryMount)
        pSecondaryMount->SetGuidingEnabled(m_saveSecondaryMountEnabled);

    m_start->Enable(pFrame->pGuider->IsGuiding());
    m_stop->Enable(false);

    SetTitle(wxGetTranslation(TITLE));
}

void GuidabilityToolWin::OnStop(wxCommandEvent& event)
{
    DoStop();
}

void GuidabilityToolWin::OnAppStateNotify(wxCommandEvent& WXUNUSED(event))
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
            m_statusBar->SetStatusText(_("Click Start to begin measurement."));
        else
            m_statusBar->SetStatusText(_("Select a guide star and start guiding, then click Start to begin measurement."));
    }
}

void GuidabilityToolWin::OnClose(wxCloseEvent& evt)
{
    DoStop();

    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/GuidabilityTool/pos.x", x);
    pConfig->Global.SetInt("/GuidabilityTool/pos.y", y);

    Destroy();
}

void GuidabilityToolWin::UpdateInfo(const GuideStepInfo& info)
{
    double ra = info.mountOffset->X;
    double dec = info.mountOffset->Y;

    m_statsRA.AddSample(ra);
    m_statsDec.AddSample(dec);

    if (m_statsRA.n == 1)
    {
        minRA = maxRA = ra;
        m_startPos = *info.mountOffset;
    }
    else
    {
        if (ra < minRA)
            minRA = ra;
        if (ra > maxRA)
            maxRA = ra;
    }
    double rangeRA = maxRA - minRA;
    double driftRA = ra - m_startPos.X;
    double driftDec = dec - m_startPos.Y;

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

    m_report->SetValue(wxString::Format(
        "Guidability check %s\n"
        "Samples: %u  Elapsed Time: %us\n"
        "Star Mass: %.f  SNR: %.1f  Exposure: %gs\n"
        "\n"
        "Star centroid fluctuation (high frequency, > %.2f Hz):\n"
        "     RA  %6.2f px  %6.2f\"\n"
        "    Dec  %6.2f px  %6.2f\"\n"
        "  Total  %6.2f px  %6.2f\"\n"
        "\n"
        "Peak deflections, RA: %.1f px, %.1f\", Dec: %.1f px, %.1f\"\n"
        "\n"
        "RA error, peak-to-peak: %.1f px, %.1f\"\n"
        "RA drift rate: %.1f px/min, %.1f\"/min\n"
        "Dec drift rate: %.1f px/min, %.1f\"/min\n",
        startStr,
        m_statsRA.n, (unsigned int)(elapsedms / 1000),
        sumMass / n, sumSNR / n, (double) pFrame->RequestedExposureDuration() / 1000.0,
        m_freqThresh,
        rarms, rarms * pxscale,
        decrms, decrms * pxscale,
        combined, combined * pxscale,
        m_statsRA.peakRawDx, m_statsRA.peakRawDx * pxscale, m_statsDec.peakRawDx, m_statsDec.peakRawDx * pxscale,
        rangeRA, rangeRA * pxscale,
        raDriftRate, raDriftRate * pxscale,
        decDriftRate, decDriftRate * pxscale
    ));
}

wxWindow *GuidabilityTool::CreateGuidabilityToolWindow()
{
    return new GuidabilityToolWin();
}

void GuidabilityTool::NotifyGuideStep(const GuideStepInfo& info)
{
    if (pFrame && pFrame->pGuidabilityTool)
    {
        GuidabilityToolWin *win = static_cast<GuidabilityToolWin *>(pFrame->pGuidabilityTool);
        if (win->m_measuring)
            win->UpdateInfo(info);
    }
}

void GuidabilityTool::NotifyFrameDropped(const FrameDroppedInfo& info)
{
    if (pFrame && pFrame->pGuidabilityTool)
    {
        // anything needed?
    }
}

void GuidabilityTool::UpdateGuidabilityToolControls()
{
    // notify guidability tool window to update its controls
    if (pFrame && pFrame->pGuidabilityTool)
    {
        wxCommandEvent event(APPSTATE_NOTIFY_EVENT, pFrame->GetId());
        event.SetEventObject(pFrame);
        wxPostEvent(pFrame->pGuidabilityTool, event);
    }
}
