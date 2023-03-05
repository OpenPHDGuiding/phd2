/*
*  calreview_dialog.cpp
*  PHD Guiding
*
*  Created by Bruce Waddington
*  Copyright (c) 2015 Bruce Waddington
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
*    Neither the name of Bret McKee, Dad Dog Development,
*     Craig Stark, Stark Labs nor the names of its
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
#include "calreview_dialog.h"
#include "scope.h"

// Event handling for base class - derived classes handle their own bindings
BEGIN_EVENT_TABLE( CalReviewDialog, wxDialog )

EVT_CLOSE(CalReviewDialog::OnCloseWindow)

END_EVENT_TABLE()

#define NA_STR _("N/A")
#define CALREVIEW_BITMAP_SIZE 250

CalReviewDialog::CalReviewDialog( )
{
}

CalReviewDialog::CalReviewDialog(wxFrame* parent, const wxString& caption)
{
    m_childDialog = false;
    Create(parent, caption);
}

CalReviewDialog::~CalReviewDialog()
{
    if (!m_childDialog)
        pFrame->pCalReviewDlg = NULL;
}

// Separated from constructor because derived classes may override functions used to populate the UI
bool CalReviewDialog::Create(wxWindow* parent, const wxString& caption, const wxWindowID& id, const wxPoint& pos,
    const wxSize& size, long style)
{

    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}

void CalReviewDialog::CreateControls()
{
    CalReviewDialog* parent = this;
    wxBoxSizer* topVSizer = new wxBoxSizer(wxVERTICAL);
    parent->SetSizer(topVSizer);

    wxNotebook* calibNotebook = new wxNotebook(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);

    wxPanel* panelMount = new wxPanel(calibNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxTAB_TRAVERSAL);
    CreatePanel(panelMount, false);
    panelMount->SetBackgroundColour("BLACK");

    calibNotebook->AddPage(panelMount, _("Mount"));

    // Build the AO panel only if an AO is configured
    if (pSecondaryMount)
    {
        wxPanel* panelAO = new wxPanel(calibNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxTAB_TRAVERSAL);
        CreatePanel(panelAO, true);
        panelAO->SetBackgroundColour("BLACK");
        calibNotebook->AddPage(panelAO, _("AO"));
    }

    topVSizer->Add(calibNotebook, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    AddButtons(this, topVSizer);                              // virtual function
}

// Base class version of buttons - subclasses can put their own buttons if needed. No buttons for the base class because it is non-modal -
// but the window close event is hooked in order to force a destroy() and null of the global pointer
void CalReviewDialog::AddButtons(CalReviewDialog* parentDialog, wxBoxSizer* parentVSizer)
{

}

// Populate one of the panels in the wxNotebook
void CalReviewDialog::CreatePanel(wxPanel *thisPanel, bool AO)
{
    wxBoxSizer *panelHSizer = new wxBoxSizer(wxHORIZONTAL);
    thisPanel->SetSizer(panelHSizer);

    // Put the graph and its legend on the left side
    wxBoxSizer *panelGraphVSizer = new wxBoxSizer(wxVERTICAL);
    panelHSizer->Add(panelGraphVSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    // Use a bitmap button so we don't have to fool with Paint events
    wxBitmap theGraph = CreateGraph(AO);
    wxStaticBitmap *graph = new wxStaticBitmap(thisPanel, wxID_ANY, theGraph, wxDefaultPosition, wxDefaultSize, 0);
    panelGraphVSizer->Add(graph, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxFIXED_MINSIZE, 5);

    wxBoxSizer *graphLegendGroup = new wxBoxSizer(wxHORIZONTAL);
    panelGraphVSizer->Add(graphLegendGroup, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    wxStaticText *labelRA = new wxStaticText(thisPanel, wxID_STATIC, _("Right Ascension"), wxDefaultPosition, wxDefaultSize, 0);
    labelRA->SetForegroundColour(pFrame->pGraphLog->GetRaOrDxColor());
    if (AO)
        labelRA->SetLabelText(_("X"));
    else
        labelRA->SetLabelText(_("Right Ascension"));
    graphLegendGroup->Add(labelRA, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    wxStaticText *labelDec = new wxStaticText(thisPanel, wxID_STATIC, _("Declination"), wxDefaultPosition, wxDefaultSize, 0);
    labelDec->SetForegroundColour(pFrame->pGraphLog->GetDecOrDyColor());
    if (AO)
        labelDec->SetLabelText(_("Y"));
    else
        labelDec->SetLabelText(_("Declination"));
    graphLegendGroup->Add(labelDec, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    // Done with left-hand side
    // Now put the data grid(s) on the right side
    thisPanel->SetForegroundColour("WHITE");
    CreateDataGrids(thisPanel, panelHSizer, AO);                  // Virtual function
}

// Base class version builds data grids showing last calibration details and calibration "context"
void CalReviewDialog::CreateDataGrids(wxPanel* parentPanel, wxSizer* parentHSizer, bool AO)
{
    const double siderealSecondPerSec = 0.9973;
    const double siderealRate = 15.0 * siderealSecondPerSec;

    Mount *mount;
    if (!pSecondaryMount || AO)
        // Normal case, no AO
        // or AO tab, use AO details
        mount = pMount;
    else
        // Mount tab, use mount details
        mount = pSecondaryMount;

    CalibrationDetails calDetails;
    mount->LoadCalibrationDetails(&calDetails);

    Calibration calBaseline;
    mount->GetLastCalibration(&calBaseline);

    wxBoxSizer* panelGridVSizer = new wxBoxSizer(wxVERTICAL);
    parentHSizer->Add(panelGridVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    int row = 0;
    int col = 0;
    bool validDetails = calDetails.IsValid(); // true for non-AO with pointing source info and "recent" calibration
    bool validBaselineDeclination = calBaseline.declination != UNKNOWN_DECLINATION;

    // Build the upper frame and grid for data from the last calibration
    wxStaticBox* staticBoxLastCal = new wxStaticBox(parentPanel, wxID_ANY, _("Last Mount Calibration"));
    if (AO)
        staticBoxLastCal->SetLabelText(_("Last AO Calibration"));
    wxStaticBoxSizer* calibFrame = new wxStaticBoxSizer(staticBoxLastCal, wxVERTICAL | wxEXPAND);
    panelGridVSizer->Add(calibFrame, 0, wxALIGN_LEFT | wxALL, 5);
    wxGrid* calGrid = new wxGrid(parentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxHSCROLL | wxVSCROLL);
    calGrid->SetColLabelSize(0);
    calGrid->SetRowLabelSize(0);
    if (!AO)
        calGrid->CreateGrid(6, 4);
    else
        calGrid->CreateGrid(5, 4);

    calGrid->EnableEditing(false);

    calGrid->SetCellValue(row, col++, _("RA steps:"));
    if (validDetails)
        calGrid->SetCellValue(row, col++, wxString::Format("%d", calDetails.raStepCount));
    else
        calGrid->SetCellValue(row, col++, NA_STR);
    calGrid->SetCellValue(row, col++, _("Dec steps:"));
    if (validDetails)
        calGrid->SetCellValue(row, col++, wxString::Format("%d", calDetails.decStepCount));
    else
        calGrid->SetCellValue(row, col++, NA_STR);
    row++;
    col = 0;
    calGrid->SetCellValue(row, col++, _("Camera angle:"));
    double cam_angle = degrees(norm_angle(calBaseline.xAngle));
    calGrid->SetCellValue(row, col++, wxString::Format("%.1f", cam_angle));
    calGrid->SetCellValue(row, col++, _("Orthogonality error:"));
    if (validDetails)
        calGrid->SetCellValue(row, col++, wxString::Format("%0.1f", calDetails.orthoError));
    else
        calGrid->SetCellValue(row, col++, NA_STR);

    row++;
    col = 0;

    double guideRaSiderealX = -1.0;
    double guideDecSiderealX = -1.0;

    if (validDetails && calDetails.raGuideSpeed > 0.0)
    {
        guideRaSiderealX = calDetails.raGuideSpeed * 3600.0 / siderealRate;  // Degrees/sec to Degrees/hour, 15 degrees/hour is roughly sidereal rate
        guideDecSiderealX = calDetails.decGuideSpeed * 3600.0 / siderealRate;  // Degrees/sec to Degrees/hour, 15 degrees/hour is roughly sidereal rate
    }

    wxString ARCSECPERSEC(_("a-s/sec"));
    wxString PXPERSEC(_("px/sec"));
    wxString ARCSECPERPX(_("a-s/px"));

    if (!AO)
    {
        calGrid->SetCellValue(row, col++, _("RA rate:"));
    }
    else
        calGrid->SetCellValue(row, col++, _("X rate:"));
    if (validDetails)
        calGrid->SetCellValue(row, col++, wxString::Format("%0.3f %s\n%0.3f %s", calBaseline.xRate * 1000 * calDetails.imageScale, ARCSECPERSEC,
            calBaseline.xRate * 1000, PXPERSEC));
    else
        calGrid->SetCellValue(row, col++, wxString::Format("%0.3f %s", calBaseline.xRate * 1000, PXPERSEC));      // just px/sec with no image scale data
    if (!AO)
        calGrid->SetCellValue(row, col++, _("Dec rate:"));
    else
        calGrid->SetCellValue(row, col++, _("Y rate:"));
    if (calBaseline.yRate != CALIBRATION_RATE_UNCALIBRATED)
    {
        if (validDetails)
            calGrid->SetCellValue(row, col++, wxString::Format("%0.3f %s\n%0.3f %s", calBaseline.yRate * 1000 * calDetails.imageScale, ARCSECPERSEC,
                calBaseline.yRate * 1000, PXPERSEC));
        else
            calGrid->SetCellValue(row, col++, wxString::Format("%0.3f %s", calBaseline.yRate * 1000, PXPERSEC));      // just px/sec with no image scale data
    }
    else
        calGrid->SetCellValue(row, col++, NA_STR);

    if (validDetails && calBaseline.yRate > 0)
    {
        row++;
        col = 0;
        calGrid->SetCellValue(row, col++, _("Expected RA rate:"));
        if (validBaselineDeclination && guideRaSiderealX != -1.0 && fabs(degrees(calBaseline.declination)) < 65.0)
        {
            // Dec speed setting corrected for pointing position and then for any difference in RA guide speed setting
            double expectedRaRate = siderealRate * cos(calBaseline.declination) * guideRaSiderealX;
            calGrid->SetCellValue(row, col++, wxString::Format("%0.1f %s", expectedRaRate, ARCSECPERSEC));
        }
        else
            calGrid->SetCellValue(row, col++, NA_STR);

        calGrid->SetCellValue(row, col++, _("Expected Dec rate:"));
        if (guideRaSiderealX != -1.0)
        {
            double expectedDecRate = siderealRate * guideDecSiderealX;
            calGrid->SetCellValue(row, col++, wxString::Format("%0.1f %s", expectedDecRate, ARCSECPERSEC));
        }
        else
            calGrid->SetCellValue(row, col++, NA_STR);
    }

    row++;
    col = 0;
    calGrid->SetCellValue(row, col++, _("Binning:"));
    calGrid->SetCellValue(row, col++, wxString::Format("%d", (int)calBaseline.binning));
    calGrid->SetCellValue(row, col++, _("Created:"));
    calGrid->SetCellValue(row, col++, validDetails ? calDetails.origTimestamp : _("Unknown"));

    if (validDetails && !AO)
    {
        row++;
        col = 0;
        calGrid->SetCellValue(row, col++, _("Side of pier:"));
        wxString side = calDetails.origPierSide  == PIER_SIDE_EAST ? _("East") :
            calDetails.origPierSide == PIER_SIDE_WEST ? _("West") : NA_STR;
        calGrid->SetCellValue(row, col++, side);
    }

    calGrid->AutoSize();
    calibFrame->Add(calGrid, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    if (!AO)                                        // Don't put this mount-related data on the AO panel
    {
        // Build the upper frame and grid for configuration data
        wxStaticBox* staticBoxMount = new wxStaticBox(parentPanel, wxID_ANY, _("Mount Configuration"));
        wxStaticBoxSizer* configFrame = new wxStaticBoxSizer(staticBoxMount, wxVERTICAL);
        panelGridVSizer->Add(configFrame, 0, wxALIGN_LEFT | wxALL, 5);

        wxGrid* cfgGrid = new wxGrid(parentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxHSCROLL | wxVSCROLL);
        row = 0;
        col = 0;
        cfgGrid->SetColLabelSize(0);
        cfgGrid->SetRowLabelSize(0);
        cfgGrid->CreateGrid(4, 4);
        cfgGrid->EnableEditing(false);

        cfgGrid->SetCellValue(row, col++, _("Modified:"));
        cfgGrid->SetCellValue(row, col++, calBaseline.timestamp);
        cfgGrid->SetCellValue(row, col++, _("Focal length:"));
        if (validDetails)
            cfgGrid->SetCellValue(row, col++, wxString::Format(_("%d mm"), calDetails.focalLength));
        else
            cfgGrid->SetCellValue(row, col++, NA_STR);
        row++;
        col = 0;
        cfgGrid->SetCellValue(row, col++, _("Image scale:"));
        if (validDetails)
        {
            wxString binning = wxString::Format(_("Binning: %d"), (int)calDetails.origBinning);      // Always binning used in actual calibration
            cfgGrid->SetCellValue(row, col++, wxString::Format("%0.2f %s\n%s", calDetails.imageScale, ARCSECPERPX, binning));
        }
        else
            cfgGrid->SetCellValue(row, col++, NA_STR);
        cfgGrid->SetCellValue(row, col++, _("Side-of-pier:"));
        wxString sPierSide = calBaseline.pierSide == PIER_SIDE_EAST ? _("East") :
            calBaseline.pierSide == PIER_SIDE_WEST ? _("West") : NA_STR;
        cfgGrid->SetCellValue(row, col++, _(sPierSide));

        row++;
        col = 0;

        cfgGrid->SetCellValue(row, col++, _("RA Guide speed:"));
        if (guideRaSiderealX != -1.0)                                       // Do the RA guide setting
        {
            cfgGrid->SetCellValue(row, col++, wxString::Format(_("%0.2fx"), guideRaSiderealX));
        }
        else
            cfgGrid->SetCellValue(row, col++, NA_STR);

        cfgGrid->SetCellValue(row, col++, _("Dec Guide speed:"));
        if (guideDecSiderealX != -1.0)                                      // Do the Dec guide setting
        {
            cfgGrid->SetCellValue(row, col++, wxString::Format(_("%0.2fx"), guideDecSiderealX));
        }
        else
            cfgGrid->SetCellValue(row, col++, NA_STR);

        row++;
        col = 0;

        // dec may be gotten from mount or imputed
        double dec = calBaseline.declination;
        bool decEstimated = false;

        if (!validBaselineDeclination)
        {
            if (fabs(calBaseline.yRate) > 0.00001 && fabs(calBaseline.xRate / calBaseline.yRate) <= 1.0)
            {
                dec = acos(calBaseline.xRate / calBaseline.yRate);        // RA_Rate = Dec_Rate * cos(dec)
                decEstimated = true;
            }
        }

        cfgGrid->SetCellValue(row, col++, _("Declination"));
        wxString decStr = decEstimated ?
            Mount::DeclinationStrTr(dec, "%0.1f (est)") :
            Mount::DeclinationStrTr(dec, "%0.1f");
        cfgGrid->SetCellValue(row, col++, decStr);

        cfgGrid->SetCellValue(row, col++, _("Rotator position:"));
        bool valid_rotator = fabs(calBaseline.rotatorAngle) < 360.0;
        if (valid_rotator)
            cfgGrid->SetCellValue(row, col++, wxString::Format("%0.1f", calBaseline.rotatorAngle));
        else
            cfgGrid->SetCellValue(row, col++, NA_STR);

        cfgGrid->AutoSize();
        configFrame->Add(cfgGrid, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    }
}

#define IntPoint(RealPoint, scaler) wxPoint(wxRound(RealPoint.x * scaler), wxRound(RealPoint.y * scaler))

// Build the calibration "step" graph which will appear on the lefthand side of the panels
wxBitmap CalReviewDialog::CreateGraph(bool AO)
{
    CalibrationDetails calDetails;

    Mount *mount;

    if (!pSecondaryMount || AO)
        mount = pMount;
    else
        mount = pSecondaryMount;                 // Mount tab, use mount details

    mount->LoadCalibrationDetails(&calDetails);

    // Find the max excursion from the origin in order to scale the points to fit the bitmap
    double biggestVal = -100.0;
    for (std::vector<wxRealPoint>::const_iterator it = calDetails.raSteps.begin(); it != calDetails.raSteps.end(); ++it)
    {
        biggestVal = wxMax(biggestVal, fabs(it->x));
        biggestVal = wxMax(biggestVal, fabs(it->y));
    }

    for (std::vector<wxRealPoint>::const_iterator it = calDetails.decSteps.begin(); it != calDetails.decSteps.end(); ++it)
    {
        biggestVal = wxMax(biggestVal, fabs(it->x));
        biggestVal = wxMax(biggestVal, fabs(it->y));
    }

    double scaleFactor;
    if (biggestVal > 0.0)
        scaleFactor = ((CALREVIEW_BITMAP_SIZE - 5) / 2) / biggestVal;           // Leave room for circular point
    else
        scaleFactor = 1.0;

    wxMemoryDC memDC;
    wxBitmap bmp(CALREVIEW_BITMAP_SIZE, CALREVIEW_BITMAP_SIZE, -1);
    memDC.SelectObject(bmp);
    memDC.SetBackground(*wxBLACK_BRUSH);
    memDC.Clear();
    wxPen axisPen("GREY", 3, wxPENSTYLE_CROSS_HATCH);
    memDC.SetPen(axisPen);
    // Draw the axes
    memDC.SetDeviceOrigin(wxCoord(CALREVIEW_BITMAP_SIZE / 2), wxCoord(CALREVIEW_BITMAP_SIZE / 2));
    memDC.DrawLine(-CALREVIEW_BITMAP_SIZE / 2, 0, CALREVIEW_BITMAP_SIZE / 2, 0);               // x
    memDC.DrawLine(0, -CALREVIEW_BITMAP_SIZE / 2, 0, CALREVIEW_BITMAP_SIZE / 2);               // y

    if (calDetails.raStepCount > 0)
    {
        const wxColour& raColor = pFrame->pGraphLog->GetRaOrDxColor();
        wxPen raPen(raColor, 3, wxPENSTYLE_SOLID);
        wxBrush raBrush(raColor, wxBRUSHSTYLE_SOLID);

        // Draw the RA data
        memDC.SetPen(raPen);
        memDC.SetBrush(raBrush);
        int ptRadius = 2;

        // Scale the points, then plot them individually
        for (int i = 0; i < (int) calDetails.raSteps.size(); i++)
        {
            if (i == calDetails.raStepCount + 2)        // Valid even for "single-step" calibration
            {
                memDC.SetPen(wxPen(raColor, 1));         // 1-pixel-thick outline
                memDC.SetBrush(wxNullBrush);           // Outline only for "return" data points
                ptRadius = 3;
            }
            memDC.DrawCircle(IntPoint(calDetails.raSteps.at(i), scaleFactor), ptRadius);
        }
        // Show the line PHD2 will use for the rate
        memDC.SetPen(raPen);
        if ((int)calDetails.raSteps.size() > calDetails.raStepCount)         // New calib, includes return values
            memDC.DrawLine(IntPoint(calDetails.raSteps.at(0), scaleFactor), IntPoint(calDetails.raSteps.at(calDetails.raStepCount), scaleFactor));
        else
            memDC.DrawLine(IntPoint(calDetails.raSteps.at(0), scaleFactor), IntPoint(calDetails.raSteps.at(calDetails.raStepCount - 1), scaleFactor));
    }

    // Handle the Dec data
    const wxColour& decColor = pFrame->pGraphLog->GetDecOrDyColor();
    wxPen decPen(decColor, 3, wxPENSTYLE_SOLID);
    wxBrush decBrush(decColor, wxBRUSHSTYLE_SOLID);
    memDC.SetPen(decPen);
    memDC.SetBrush(decBrush);
    int ptRadius = 2;
    if (calDetails.decStepCount > 0 && calDetails.decSteps.size() > 0)      // redundant, protection against old bug
    {
        for (int i = 0; i < (int) calDetails.decSteps.size(); i++)
        {
            if (i == calDetails.decStepCount + 2)
            {
                memDC.SetPen(wxPen(decColor, 1));         // 1-pixel-thick outline
                memDC.SetBrush(wxNullBrush);           // Outline only for "return" data points
                ptRadius = 3;
            }
            memDC.DrawCircle(IntPoint(calDetails.decSteps.at(i), scaleFactor), ptRadius);
        }
        // Show the line PHD2 will use for the rate
        memDC.SetPen(decPen);
        if ((int)calDetails.decSteps.size() > calDetails.decStepCount)         // New calib, includes return values
            memDC.DrawLine(IntPoint(calDetails.decSteps.at(0), scaleFactor), IntPoint(calDetails.decSteps.at(calDetails.decStepCount), scaleFactor));
        else
        memDC.DrawLine(IntPoint(calDetails.decSteps.at(0), scaleFactor), IntPoint(calDetails.decSteps.at(calDetails.decStepCount - 1), scaleFactor));
    }

    return bmp;
}

// Make this stuff deterministic and destroy the dialog right away
void CalReviewDialog::ShutDown()
{
    wxDialog::Destroy();
}

void CalReviewDialog::OnCancelClick(wxCommandEvent& event)
{
    ShutDown();
}

void CalReviewDialog::OnCloseWindow(wxCloseEvent& event)
{
    ShutDown();
    event.Skip();
}

// Derived classes from CalibrationRestoreDialog
// Restore dialog is basically the same as 'Review' except for option to actually restore the old calibration data - plus, it's modal
CalRestoreDialog::CalRestoreDialog(wxFrame* parent, const wxString& caption)
{
    m_childDialog = true;
    Create(parent, caption);
}

void CalRestoreDialog::AddButtons(CalReviewDialog* parentDialog, wxBoxSizer* parentVSizer)
{
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *pRestore = new wxButton(parentDialog, wxID_OK, _("Restore"));
    pRestore->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalRestoreDialog::OnRestore, this);
    pButtonSizer->Add(
        pRestore,
        wxSizerFlags(0).Align(0).Border(wxRIGHT | wxLEFT | wxBOTTOM, 10));
    pButtonSizer->Add(
        CreateButtonSizer(wxCANCEL),
        wxSizerFlags(0).Align(0).Border(wxRIGHT | wxLEFT | wxBOTTOM, 10));
    parentVSizer->Add(pButtonSizer, wxSizerFlags(0).Center());
}

void CalRestoreDialog::OnRestore(wxCommandEvent& event)
{
    Debug.AddLine("User-requested restore calibration");
    pFrame->LoadCalibration();
    pFrame->StatusMsg(_("Calibration restored"));
    EndModal(wxID_OK);
}

// CalSanity dialog may get launched as part of an 'alert' if the last calibration looked wonky - this one is non-modal
CalSanityDialog::CalSanityDialog(wxFrame *parent, const Calibration& oldParams, const CalibrationDetails& oldDetails,
    CalibrationIssueType issue)
{
    m_pScope = TheScope();
    m_pScope->GetLastCalibration(&m_newParams);
    pMount->LoadCalibrationDetails(&m_calDetails);
    m_oldParams = oldParams;
    m_oldDetails = oldDetails;
    m_issue = issue;
    m_childDialog = true;
    m_oldValid = oldParams.isValid;
    // All above data must be initialized before the UI can be built
    Create(parent, _("Calibration Sanity Check"));
}

CalSanityDialog::~CalSanityDialog()
{
    pFrame->pCalSanityCheckDlg = NULL;                                          // Clear the global pointer used to launch us
}

static const int MESSAGE_HEIGHT = 100;
static void HighlightCell(wxGrid *pGrid, int row, int col)
{
    pGrid->SetCellBackgroundColour(row, col, "DARK SLATE GREY");
    pGrid->SetCellTextColour(row, col, "white");
}

// Build the verbose message based on the type of issue
void CalSanityDialog::BuildMessage(wxStaticText *pText, CalibrationIssueType etype)
{
    wxString msg;

    switch (etype)
    {
    case CI_Steps:
        msg = _("The calibration was done with a very small number of steps, which can produce inaccurate results. "
            "Consider reducing the size of the calibration step parameter until you see at least 8 steps in each direction.  The 'calculator' "
            "feature in the 'Mount' configuration tab can help you with this.");
        break;
    case CI_Angle:
        msg = wxString::Format(_("The RA and Declination angles computed in the calibration are questionable.  Normally, "
            "these angles will be nearly perpendicular, having an 'orthogonality error' of less than 10 degrees.  In this calibration, your error was %s degrees, which "
            "is often caused by poor polar alignment, large Dec backlash, or a large periodic error in RA."), m_newAngleDelta);
        break;
    case CI_Different:
        msg = wxString::Format(_("The most recent calibration produced results that are %s%% different from the previous calibration.  If this is because "
            "you changed equipment configurations, you may want to use different profiles.  Doing so will allow you to switch back "
            "and forth between configurations and still retain earlier settings and calibration results."), m_oldNewDifference);
        break;
    case CI_Rates:
        msg = wxString::Format(_("The RA and Declination guiding rates differ by an unexpected amount.  For your declination of %0.0f degrees, "
            "the RA rate should be about %0.0f%% of the Dec rate.  But your RA rate is %0.0f%% of the Dec rate.  "
            "This usually means one of the axis calibrations is incorrect and may result in poor guiding."),
            degrees(m_newParams.declination), cos(m_newParams.declination) * 100.0, m_newParams.xRate / m_newParams.yRate * 100.0);
        break;
    default:
        msg = wxString::Format("Just testing");
        break;
    }
    pText->SetLabel(msg);
    pText->Wrap(420);
}

// Overridden method for building the data grids - these are substantially different from the CalReview base but the overall appearance and graph presence are the same
void CalSanityDialog::CreateDataGrids(wxPanel* parentPanel, wxSizer* parentHSizer, bool AO)
{
    wxString raSteps = wxString::Format("%d", m_calDetails.raStepCount);
    wxString decSteps = wxString::Format("%d", m_calDetails.decStepCount);
    wxString oldAngleDelta;
    double newRARate = m_newParams.xRate;
    double newDecRate = m_newParams.yRate;
    double imageScale = m_calDetails.imageScale;
    bool oldValid = m_oldParams.isValid;

    if (!AO)                // AO calibration never triggers sanity check alerts, so don't show that data
    {
        // Compute the orthogonality stuff
        m_newAngleDelta = wxString::Format("%0.1f", m_calDetails.orthoError);
        if (oldValid)
        {
            oldAngleDelta = wxString::Format("%0.1f", m_oldDetails.orthoError);
        }
        else
            oldAngleDelta = NA_STR;

        if (m_newParams.yRate != 0. && m_oldParams.yRate != 0.)
            m_oldNewDifference = wxString::Format("%0.1f", fabs(1.0 - m_newParams.yRate / m_oldParams.yRate) * 100.0);
        else
            m_oldNewDifference = "";

        // Lay out the controls
        wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
        wxStaticBoxSizer *pMsgGrp = new wxStaticBoxSizer(wxVERTICAL, parentPanel, _("Explanation"));

        // Explanation area
        wxStaticText *pMsgArea = new wxStaticText(parentPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(450, -1), wxALIGN_LEFT | wxST_NO_AUTORESIZE);
        BuildMessage(pMsgArea, m_issue);
        pMsgArea->SetSizeHints(wxSize(450, MESSAGE_HEIGHT));
        wxFont font = pMsgArea->GetFont();
        font.SetWeight(wxFONTWEIGHT_BOLD);
        pMsgArea->SetFont(font);
        pMsgGrp->Add(pMsgArea, wxSizerFlags().Border(wxALL, 5));
        pVSizer->Add(pMsgGrp, wxSizerFlags().Border(wxALL, 5));

        // Grid control for details
        wxStaticBoxSizer *pGridGrp = new wxStaticBoxSizer(wxVERTICAL, parentPanel, _("Details"));
        wxGrid *pGrid = new wxGrid(parentPanel, wxID_ANY);
        pGrid->CreateGrid(3, 4);
        pGrid->SetRowLabelSize(1);
        pGrid->SetColLabelSize(1);
        pGrid->EnableEditing(false);

        int col = 0;
        int row = 0;
        pGrid->SetCellValue(row, col++, _("Steps, RA:"));
        pGrid->SetCellValue(row, col++, raSteps);
        pGrid->SetCellValue(row, col++, _("Steps, Dec:"));
        pGrid->SetCellValue(row, col++, decSteps);
        if (m_issue == CI_Steps){
            if (raSteps <= decSteps)
                HighlightCell(pGrid, row, 1);
            else
                HighlightCell(pGrid, row, 3);
        }
        row++;
        col = 0;
        pGrid->SetCellValue(row, col++, _("Orthogonality error:"));
        pGrid->SetCellValue(row, col++, m_newAngleDelta);
        pGrid->SetCellValue(row, col++, _("Previous orthogonality error:"));
        pGrid->SetCellValue(row, col++, oldAngleDelta);
        if (m_issue == CI_Angle)
        {
            HighlightCell(pGrid, row, 1);
        }

        row++;
        col = 0;
        // Show either the new RA and Dec rates or the new and old Dec rates depending on the issue
        if (m_issue == CI_Different)
        {
            pGrid->SetCellValue(row, col++, _("This declination rate:"));
            if (newDecRate != CALIBRATION_RATE_UNCALIBRATED)
                pGrid->SetCellValue(row, col++, wxString::Format(_("%0.3f a-s/sec\n%0.3f px/sec"), newDecRate * 1000 * imageScale, newDecRate * 1000));
            else
                pGrid->SetCellValue(row, col++, NA_STR);
            pGrid->SetCellValue(row, col++, _("Previous declination rate:"));
            if (m_oldParams.yRate != CALIBRATION_RATE_UNCALIBRATED)
                pGrid->SetCellValue(row, col++, wxString::Format(_("\n%0.3f px/sec"), m_oldParams.yRate * 1000));
            else
                pGrid->SetCellValue(row, col++, NA_STR);
            HighlightCell(pGrid, row, 1);
            HighlightCell(pGrid, row, 3);
        }
        else
        {
            pGrid->SetCellValue(row, col++, _("RA rate:"));
            pGrid->SetCellValue(row, col++, wxString::Format(_("%0.3f a-s/sec\n%0.3f px/sec"), newRARate * 1000 * imageScale, newRARate * 1000));
            pGrid->SetCellValue(row, col++, _("Declination rate:"));
            if (newDecRate != CALIBRATION_RATE_UNCALIBRATED)
                pGrid->SetCellValue(row, col++, wxString::Format(_("%0.3f a-s/sec\n%0.3f px/sec"), newDecRate * 1000 *  imageScale, newDecRate * 1000));
            else
                pGrid->SetCellValue(row, col++, NA_STR);
            if (m_issue == CI_Rates)
            {
                HighlightCell(pGrid, row, 1);
                HighlightCell(pGrid, row, 3);
            }
        }

        pGrid->AutoSize();
        pGrid->ClearSelection();
        pGridGrp->Add(pGrid);
        pVSizer->Add(pGridGrp, wxSizerFlags(0).Border(wxALL, 5));

        // Checkboxes for being quiet
        m_pBlockThis = new wxCheckBox(parentPanel, wxID_ANY, _("Don't show calibration alerts of this type"));
        pVSizer->Add(m_pBlockThis, wxSizerFlags(0).Border(wxALL, 15));
        parentHSizer->Add(pVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    }

}

void CalSanityDialog::AddButtons(CalReviewDialog* parentDialog, wxBoxSizer* parentVSizer)
{
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *pIgnore = new wxButton(this, wxID_ANY, _("Accept calibration"));
    pIgnore->SetToolTip(_("Accept the calibration as being valid and continue guiding"));
    pIgnore->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSanityDialog::OnIgnore, this);
    wxButton *pRecal = new wxButton(this, wxID_ANY, _("Discard calibration"));
    pRecal->SetToolTip(_("Stop guiding and discard the most recent calibration.  Calibration will be re-done the next time you start guiding"));
    pRecal->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSanityDialog::OnRecal, this);
    wxButton *pRestore = new wxButton(this, wxID_ANY, _("Restore old calibration"));
    pRestore->SetToolTip(_("Stop guiding, discard the most recent calibration, then load the previous (good) calibration"));
    pRestore->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSanityDialog::OnRestore, this);
    pRestore->Enable(m_oldValid);

    pButtonSizer->Add(
        pIgnore,
        wxSizerFlags(0).Align(0).Border(wxRIGHT | wxLEFT | wxBOTTOM, 10));
    pButtonSizer->Add(
        pRecal,
        wxSizerFlags(0).Align(0).Border(wxRIGHT | wxLEFT | wxBOTTOM, 10));
    pButtonSizer->Add(
        pRestore,
        wxSizerFlags(0).Align(0).Border(wxRIGHT | wxLEFT | wxBOTTOM, 10));

    parentVSizer->Add(
        pButtonSizer,
        wxSizerFlags(0).Center());
}

void CalSanityDialog::OnIgnore(wxCommandEvent& evt)
{
    Debug.AddLine("Calibration sanity check: user chose to ignore alert");
    ShutDown();
}

void CalSanityDialog::OnRecal(wxCommandEvent& evt)
{
    if (pFrame->pGuider && pFrame->pGuider->IsCalibratingOrGuiding())
    {
        Debug.Write("CalSanityDialog::OnRecal stops capturing\n");
        pFrame->StopCapturing();
    }
    Debug.AddLine("Calibration sanity check: user discarded bad calibration");
    pMount->ClearCalibration();
    ShutDown();
}

void CalSanityDialog::OnRestore(wxCommandEvent& evt)
{
    if (pFrame->pGuider && pFrame->pGuider->IsCalibratingOrGuiding())
    {
        Debug.Write("CalSanityDialog::OnRestore stops capturing\n");
        pFrame->StopCapturing();
    }

    m_pScope->SetCalibration(m_oldParams);
    m_pScope->SetCalibrationDetails(m_oldDetails, m_oldParams.xAngle, m_oldParams.yAngle, m_oldDetails.origBinning);

    pFrame->LoadCalibration();
    pFrame->StatusMsg(_("Previous calibration restored"));
    Debug.AddLine("Calibration sanity check: user chose to restore old calibration");
    ShutDown();
}

// Force a destroy on the dialog right away
void CalSanityDialog::ShutDown()
{
    SaveBlockingOptions();
    wxDialog::Destroy();
}

void CalSanityDialog::SaveBlockingOptions()
{
    if (m_pBlockThis->IsChecked())
        m_pScope->SetCalibrationWarning(m_issue, false);
}
