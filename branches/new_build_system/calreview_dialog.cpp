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

    calibNotebook->AddPage(panelMount, _("Mount"));

    // Build the AO panel only if an AO is configured
    if (pSecondaryMount)
    {
        wxPanel* panelAO = new wxPanel(calibNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxTAB_TRAVERSAL);
        CreatePanel(panelAO, true);
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
void CalReviewDialog::CreatePanel(wxPanel* thisPanel, bool AO)
{
    wxBoxSizer* panelHSizer = new wxBoxSizer(wxHORIZONTAL);
    thisPanel->SetSizer(panelHSizer);

    // Put the graph and its legend on the left side
    wxBoxSizer* panelGraphVSizer = new wxBoxSizer(wxVERTICAL);
    panelHSizer->Add(panelGraphVSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    // Use a bitmap button so we don't have to fool with Paint events
    wxBitmap theGraph = CreateGraph(AO);
    wxBitmapButton* graphButton = new wxBitmapButton(thisPanel, wxID_ANY, theGraph, wxDefaultPosition, wxSize(250, 250), wxBU_AUTODRAW | wxBU_EXACTFIT);
    panelGraphVSizer->Add(graphButton, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxFIXED_MINSIZE, 5);
    graphButton->SetBitmapDisabled(theGraph);
    graphButton->Enable(false);

    wxBoxSizer* graphLegendGroup = new wxBoxSizer(wxHORIZONTAL);
    panelGraphVSizer->Add(graphLegendGroup, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    wxStaticText* labelRA = new wxStaticText(thisPanel, wxID_STATIC, _("Right Ascension"), wxDefaultPosition, wxDefaultSize, 0);
    labelRA->SetForegroundColour("RED");
    if (AO)
        labelRA->SetLabelText(_("X"));
    else
        labelRA->SetLabelText(_("Right Ascension"));
    graphLegendGroup->Add(labelRA, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 5);

    wxStaticText* labelDec = new wxStaticText(thisPanel, wxID_STATIC, _("Declination"), wxDefaultPosition, wxDefaultSize, 0);
    labelDec->SetForegroundColour("BLUE");
    if (AO)
        labelDec->SetLabelText(_("Y"));
    else
        labelDec->SetLabelText(_("Declination"));
    graphLegendGroup->Add(labelDec, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 5);

    // Done with left-hand side
    // Now put the data grid(s) on the right side

    CreateDataGrids(thisPanel, panelHSizer, AO);                  // Virtual function
}

// Base class version builds data grids showing last calibration details and calibration "context"
void CalReviewDialog::CreateDataGrids(wxPanel* parentPanel, wxSizer* parentHSizer, bool AO)
{
    Calibration calBaseline;
    CalibrationDetails calDetails;
    const double dSiderealSecondPerSec = 0.9973;
    bool validDetails = false;
    bool validAscomInfo = false;
    double guideRaSiderealX = 0.0;
    double guideDecSiderealX = 0.0;

    if (!pSecondaryMount)
    {
        pMount->GetCalibrationDetails(&calDetails);                              // Normal case, no AO
        pMount->GetLastCalibrationParams(&calBaseline);
    }
    else
    {
        if (AO)
        {
            pMount->GetCalibrationDetails(&calDetails);                          // AO tab, use AO details
            pMount->GetLastCalibrationParams(&calBaseline);
        }
        else
        {
            pSecondaryMount->GetCalibrationDetails(&calDetails);                 // Mount tab, use mount details
            pSecondaryMount->GetLastCalibrationParams(&calBaseline);
        }
    }

    wxBoxSizer* panelGridVSizer = new wxBoxSizer(wxVERTICAL);
    parentHSizer->Add(panelGridVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    int row = 0;
    int col = 0;
    validDetails = calDetails.raStepCount > 0;                                             // true for non-AO with pointing source info and "recent" calibration
    validAscomInfo = calBaseline.declination != 0.0;

    // Build the upper frame and grid for data from the last calibration
    wxStaticBox* staticBoxLastCal = new wxStaticBox(parentPanel, wxID_ANY, _("Last Mount Calibration"));
    if (AO)
        staticBoxLastCal->SetLabelText(_("Last AO Calibration"));
    wxStaticBoxSizer* calibFrame = new wxStaticBoxSizer(staticBoxLastCal, wxVERTICAL | wxEXPAND);
    panelGridVSizer->Add(calibFrame, 0, wxALIGN_LEFT | wxALL, 5);
    wxGrid* calGrid = new wxGrid(parentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxHSCROLL | wxVSCROLL);
    calGrid->SetColLabelSize(0);
    calGrid->SetRowLabelSize(0);
    if (AO)
        calGrid->CreateGrid(3, 4);
    else
        calGrid->CreateGrid(4, 4);
    calGrid->EnableEditing(false);

    calGrid->SetCellValue(_("RA steps:"), row, col++);
    if (validDetails)
        calGrid->SetCellValue(wxString::Format("%d", calDetails.raStepCount), row, col++);
    else
        calGrid->SetCellValue(NA_STR, row, col++);
    calGrid->SetCellValue(_("Dec steps:"), row, col++);
    if (validDetails)
        calGrid->SetCellValue(wxString::Format("%d", calDetails.decStepCount), row, col++);
    else
        calGrid->SetCellValue(NA_STR, row, col++);
    row++;
    col = 0;
    calGrid->SetCellValue(_("Camera angle:"), row, col++);
    double cam_angle = degrees(norm_angle(calBaseline.xAngle));
    calGrid->SetCellValue(wxString::Format("%.1f", cam_angle), row, col++);
    calGrid->SetCellValue(_("Orthogonality error:"), row, col++);
    if (validDetails)
        calGrid->SetCellValue(wxString::Format("%0.1f", calDetails.orthoError), row, col++);
    else
        calGrid->SetCellValue(NA_STR, row, col++);

    row++;
    col = 0;

    if (validDetails)
    {
        guideRaSiderealX = calDetails.raGuideSpeed * 3600.0 / (15.0 * dSiderealSecondPerSec);  // Degrees/sec to Degrees/hour, 15 degrees/hour is roughly sidereal rate
        guideDecSiderealX = calDetails.decGuideSpeed * 3600.0 / (15.0 * dSiderealSecondPerSec);  // Degrees/sec to Degrees/hour, 15 degrees/hour is roughly sidereal rate
    }

    wxString ARCSECPERSEC(_("a-s/sec"));
    wxString PXPERSEC(_("px/sec"));
    wxString ARCSECPERPX(_("a-s/px"));

    if (!AO)
    {
        calGrid->SetCellValue(_("RA rate:"), row, col++);
    }
    else
        calGrid->SetCellValue(_("X rate:"), row, col++);
    if (validDetails)
        calGrid->SetCellValue(wxString::Format("%0.3f a-s/sec\n%0.3f px/sec", calBaseline.xRate * 1000 * calDetails.imageScale, calBaseline.xRate * 1000), row, col++);
    else
        calGrid->SetCellValue(wxString::Format("%0.3f px/sec", calBaseline.xRate * 1000), row, col++);      // just px/sec with no image scale data
    if (!AO)
        calGrid->SetCellValue(_("Dec rate:"), row, col++);
    else
        calGrid->SetCellValue(_("Y rate:"), row, col++);
    if (calBaseline.yRate != CALIBRATION_RATE_UNCALIBRATED)
    {
        if (validDetails)
            calGrid->SetCellValue(wxString::Format("%0.3f %s\n%0.3f %s", calBaseline.yRate * 1000 * calDetails.imageScale, ARCSECPERSEC, calBaseline.yRate * 1000, PXPERSEC), row, col++);
        else
            calGrid->SetCellValue(wxString::Format("%0.3f %s", calBaseline.yRate * 1000, PXPERSEC), row, col++);      // just px/sec with no image scale data
    }
    else
        calGrid->SetCellValue(NA_STR, row, col++);


    row++;
    col = 0;

    if (validDetails && calBaseline.yRate > 0)
    {
        calGrid->SetCellValue(_("Expected RA rate:"), row, col++);
        if (validAscomInfo && fabs(degrees(calBaseline.declination)) < 65.0)
        {
            // Dec speed setting corrected for pointing position and then for any difference in RA guide speed setting
            calGrid->SetCellValue(wxString::Format("%0.1f %s", guideDecSiderealX * 15.0 * dSiderealSecondPerSec * cos(calBaseline.declination) *
                guideRaSiderealX / guideDecSiderealX, ARCSECPERSEC), row, col++);
        }
        else
            calGrid->SetCellValue(NA_STR, row, col++);
        calGrid->SetCellValue(_("Expected Dec rate:"), row, col++);
        if (validAscomInfo)
            calGrid->SetCellValue(wxString::Format("%0.1f %s", guideDecSiderealX * 15.0 * dSiderealSecondPerSec, ARCSECPERSEC), row, col);
        else
            calGrid->SetCellValue(NA_STR, row, col++);
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

        cfgGrid->SetCellValue(_("Timestamp:"), row, col++);
        cfgGrid->SetCellValue(calBaseline.timestamp, row, col++);
        cfgGrid->SetCellValue(_("Focal length:"), row, col++);
        if (validDetails)
            cfgGrid->SetCellValue(wxString::Format("%d mm", calDetails.focalLength), row, col++);
        else
            cfgGrid->SetCellValue(NA_STR, row, col++);
        row++;
        col = 0;
        cfgGrid->SetCellValue(_("Image scale:"), row, col++);
        if (validDetails)
            cfgGrid->SetCellValue(wxString::Format("%0.2f %s", calDetails.imageScale, ARCSECPERPX), row, col++);
        else
            cfgGrid->SetCellValue(NA_STR, row, col++);
        cfgGrid->SetCellValue(_("Side-of-pier:"), row, col++);
        wxString sPierSide = calBaseline.pierSide == PIER_SIDE_EAST ? _("East") :
            calBaseline.pierSide == PIER_SIDE_WEST ? _("West") : NA_STR;
        cfgGrid->SetCellValue(_(sPierSide), row, col++);

        row++;
        col = 0;

        cfgGrid->SetCellValue(_("RA Guide speed:"), row, col++);
        if (validAscomInfo)                                                // Do the RA guide setting
        {
            cfgGrid->SetCellValue(wxString::Format("%0.2fx", guideRaSiderealX), row, col++);
        }
        else
            cfgGrid->SetCellValue(NA_STR, row, col++);
        cfgGrid->SetCellValue(_("Dec Guide speed:"), row, col++);
        if (validAscomInfo)                                                // Do the Dec guide setting
        {
            cfgGrid->SetCellValue(wxString::Format("%0.2fx", guideDecSiderealX), row, col++);
        }
        else
            cfgGrid->SetCellValue(NA_STR, row, col++);

        row++;
        col = 0;

        // dec may be gotten from mount or imputed
        double dec = calBaseline.declination;

        if (!validAscomInfo)
        {
            if (fabs(calBaseline.yRate) > 0.00001 && fabs(calBaseline.xRate / calBaseline.yRate) <= 1.0)
                dec = degrees(acos(calBaseline.xRate / calBaseline.yRate));        // RA_Rate = Dec_Rate * cos(dec)
        }
        else
        {
            dec = degrees(dec);
        }
        cfgGrid->SetCellValue(_("Declination"), row, col++);
        if (validAscomInfo)
            cfgGrid->SetCellValue(wxString::Format("%0.1f", dec), row, col++);
        else
            cfgGrid->SetCellValue(wxString::Format("%0.1f", dec) + _(" (est)"), row, col++);

        cfgGrid->SetCellValue(_("Rotator position:"), row, col++);
        bool valid_rotator = fabs(calBaseline.rotatorAngle) < 360.0;
        if (valid_rotator)
            cfgGrid->SetCellValue(wxString::Format("%0.1f", calBaseline.rotatorAngle), row, col++);
        else
            cfgGrid->SetCellValue(NA_STR, row, col++);

        cfgGrid->AutoSize();
        configFrame->Add(cfgGrid, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
    }
}

#define IntPoint(RealPoint, scaler) wxPoint(wxRound(RealPoint.x * scaler), wxRound(RealPoint.y * scaler))

// Build the calibration "step" graph which will appear on the lefthand side of the panels
wxBitmap CalReviewDialog::CreateGraph(bool AO)
{
    wxMemoryDC memDC;
    wxBitmap bmp(CALREVIEW_BITMAP_SIZE, CALREVIEW_BITMAP_SIZE, -1);
    wxPen axisPen("BLACK", 3, wxCROSS_HATCH);
    wxPen redPen("RED", 3, wxSOLID);
    wxPen bluePen("BLUE", 3, wxSOLID);
    wxBrush redBrush("RED", wxSOLID);
    wxBrush blueBrush("BLUE", wxSOLID);
    CalibrationDetails calDetails;
    double scaleFactor;
    int ptRadius;

    if (!pSecondaryMount)
    {
        pMount->GetCalibrationDetails(&calDetails);                              // Normal case, no AO
    }
    else
    {
        if (AO)
        {
            pMount->GetCalibrationDetails(&calDetails);                          // AO tab, use AO details
        }
        else
        {
            pSecondaryMount->GetCalibrationDetails(&calDetails);                 // Mount tab, use mount details
        }
    }

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
    if (biggestVal > 0)
        scaleFactor = ((CALREVIEW_BITMAP_SIZE - 5) / 2) / biggestVal;           // Leave room for circular point
    else
        scaleFactor = 1.0;

    memDC.SelectObject(bmp);
    memDC.SetBackground(*wxLIGHT_GREY_BRUSH);
    memDC.Clear();
    memDC.SetPen(axisPen);
    // Draw the axes
    memDC.SetDeviceOrigin(wxCoord(CALREVIEW_BITMAP_SIZE / 2), wxCoord(CALREVIEW_BITMAP_SIZE / 2));
    memDC.DrawLine(-CALREVIEW_BITMAP_SIZE / 2, 0, CALREVIEW_BITMAP_SIZE / 2, 0);               // x
    memDC.DrawLine(0, -CALREVIEW_BITMAP_SIZE / 2, 0, CALREVIEW_BITMAP_SIZE / 2);               // y

    if (calDetails.raStepCount > 0)
    {
        // Draw the RA data
        memDC.SetPen(redPen);
        memDC.SetBrush(redBrush);
        ptRadius = 2;

        // Scale the points, then plot them individually
        for (int i = 0; i < (int) calDetails.raSteps.size(); i++)
        {
            if (i == calDetails.raStepCount + 2)        // Valid even for "single-step" calibration
            {
                memDC.SetPen(wxPen("Red", 1));         // 1-pixel-thick red outline
                memDC.SetBrush(wxNullBrush);           // Outline only for "return" data points
                ptRadius = 3;
            }
            memDC.DrawCircle(IntPoint(calDetails.raSteps.at(i), scaleFactor), ptRadius);
        }
        // Show the line PHD2 will use for the rate
        memDC.SetPen(redPen);
        if ((int)calDetails.raSteps.size() > calDetails.raStepCount)         // New calib, includes return values
            memDC.DrawLine(IntPoint(calDetails.raSteps.at(0), scaleFactor), IntPoint(calDetails.raSteps.at(calDetails.raStepCount), scaleFactor));
        else
            memDC.DrawLine(IntPoint(calDetails.raSteps.at(0), scaleFactor), IntPoint(calDetails.raSteps.at(calDetails.raStepCount - 1), scaleFactor));
    }

    // Handle the Dec data
    memDC.SetPen(bluePen);
    memDC.SetBrush(blueBrush);
    ptRadius = 2;
    if (calDetails.decStepCount > 0)
    {
    for (int i = 0; i < (int) calDetails.decSteps.size(); i++)
        {
            if (i == calDetails.decStepCount + 2)
            {
                memDC.SetPen(wxPen("Blue", 1));         // 1-pixel-thick red outline
                memDC.SetBrush(wxNullBrush);           // Outline only for "return" data points
                ptRadius = 3;
            }
            memDC.DrawCircle(IntPoint(calDetails.decSteps.at(i), scaleFactor), ptRadius);
        }
        // Show the line PHD2 will use for the rate
        memDC.SetPen(bluePen);
        if ((int)calDetails.decSteps.size() > calDetails.decStepCount)         // New calib, includes return values
            memDC.DrawLine(IntPoint(calDetails.decSteps.at(0), scaleFactor), IntPoint(calDetails.decSteps.at(calDetails.decStepCount), scaleFactor));
        else
        memDC.DrawLine(IntPoint(calDetails.decSteps.at(0), scaleFactor), IntPoint(calDetails.decSteps.at(calDetails.decStepCount - 1), scaleFactor));
    }

    memDC.SelectObject(wxNullBitmap);
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
    pFrame->SetStatusText(_("Calibration restored"));
    EndModal(wxID_OK);

}

// CalSanity dialog may get launched as part of an 'alert' if the last calibration looked wonky - this one is non-modal
CalSanityDialog::CalSanityDialog(wxFrame* parent, const Calibration& oldParams, const CalibrationDetails& oldDetails,
    Calibration_Issues issue)
{
    m_pScope = (Scope*) pMount;
    m_pScope->GetLastCalibrationParams(&m_newParams);
    pMount->GetCalibrationDetails(&m_calDetails);
    m_oldParams = oldParams;
    m_oldDetails = oldDetails;
    m_issue = issue;
    m_childDialog = true;
    m_oldValid = (oldParams.declination < INVALID_DECLINATION);
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
void CalSanityDialog::BuildMessage(wxStaticText* pText, Calibration_Issues etype)
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
            "these angles will be nearly perpendicular, having an 'orthogonality error' of less than 10 degrees.  In this calibration, your error was %s degrees. This "
            "could mean the calibration is inaccurate, perhaps because of small or erratic star movement during the calibration."), m_newAngleDelta);
        break;
    case CI_Different:
        msg = wxString::Format(_("The most recent calibration produced results that are %s%% different from the previous calibration.  If this is because "
            "you changed equipment configurations, you may want to use different profiles.  Doing so will allow you to switch back "
            "and forth between configurations and still retain earlier settings and calibration results."), m_oldNewDifference);
        break;
    case CI_Rates:
        msg = wxString::Format(_("The RA and Declination guiding rates differ by an unexpected amount.  For your declination of %0.0f degrees, "
            "the RA rate should be about %0.0f%% of the Dec rate.  But your RA rate is %0.0f%% of the Dec rate.  "
            "This could mean the calibration is inaccurate, perhaps because of small or erratic star movement during the calibration."),
            degrees(m_newParams.declination), cos(m_newParams.declination) * 100.0, m_newParams.xRate / m_newParams.yRate * 100.0);
        break;
    default:
        msg = wxString::Format("Just testing");
        break;
    }
    pText->SetLabel(msg);
    pText->Wrap(380);
}

// Overridden method for building the data grids - these are substantially different from the CalReview base but the overall appearance and graph presence are the same
void CalSanityDialog::CreateDataGrids(wxPanel* parentPanel, wxSizer* parentHSizer, bool AO)
{
    wxString raSteps = wxString::Format("%d", m_calDetails.raStepCount);
    wxString decSteps = wxString::Format("%d", m_calDetails.decStepCount);
    wxString oldAngleDelta;
    double newRARate = m_newParams.xRate * 1000;                          // px per sec for UI purposes
    double newDecRate = m_newParams.yRate * 1000;
    double imageScale = m_calDetails.imageScale;
    bool oldValid = m_oldParams.declination < INVALID_DECLINATION;

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
        wxStaticText *pMsgArea = new wxStaticText(parentPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(400, -1), wxALIGN_LEFT | wxST_NO_AUTORESIZE);
        BuildMessage(pMsgArea, m_issue);
        pMsgArea->SetSizeHints(wxSize(-1, MESSAGE_HEIGHT));
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
        pGrid->SetCellValue(_("Steps, RA:"), row, col++);
        pGrid->SetCellValue(raSteps, row, col++);
        pGrid->SetCellValue(_("Steps, Dec:"), row, col++);
        pGrid->SetCellValue(decSteps, row, col++);
        if (m_issue == CI_Steps){
            if (raSteps <= decSteps)
                HighlightCell(pGrid, row, 1);
            else
                HighlightCell(pGrid, row, 3);
        }
        row++;
        col = 0;
        pGrid->SetCellValue(_("Orthogonality error:"), row, col++);
        pGrid->SetCellValue(m_newAngleDelta, row, col++);
        pGrid->SetCellValue(_("Previous orthogonality error:"), row, col++);
        pGrid->SetCellValue(oldAngleDelta, row, col++);
        if (m_issue == CI_Angle)
        {
            HighlightCell(pGrid, row, 1);
        }

        row++;
        col = 0;
        // Show either the new RA and Dec rates or the new and old Dec rates depending on the issue
        if (m_issue == CI_Different)
        {
            pGrid->SetCellValue(_("This declination rate:"), row, col++);
            if (newDecRate != CALIBRATION_RATE_UNCALIBRATED)
                pGrid->SetCellValue(wxString::Format("%0.3f ''/sec\n%0.3f px/sec", newDecRate * imageScale, newDecRate), row, col++);
            else
                pGrid->SetCellValue(NA_STR, row, col++);
            pGrid->SetCellValue(_("Previous declination rate:"), row, col++);
            if (m_oldParams.yRate != CALIBRATION_RATE_UNCALIBRATED)
                pGrid->SetCellValue(wxString::Format("\n%0.3f px/sec", m_oldParams.yRate * 1000), row, col++);
            else
                pGrid->SetCellValue(NA_STR, row, col++);
            HighlightCell(pGrid, row, 1);
            HighlightCell(pGrid, row, 3);
        }
        else
        {
            pGrid->SetCellValue(_("RA rate:"), row, col++);
            pGrid->SetCellValue(wxString::Format("%0.3f a-s/sec\n%0.3f px/sec", newRARate * imageScale, newRARate), row, col++);
            pGrid->SetCellValue(_("Declination rate:"), row, col++);
            if (newDecRate != CALIBRATION_RATE_UNCALIBRATED)
                pGrid->SetCellValue(wxString::Format("%0.3f a-s/sec\n%0.3f px/sec", newDecRate * imageScale, newDecRate), row, col++);
            else
                pGrid->SetCellValue(NA_STR, row, col++);
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
        parentHSizer->Add(pVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);             // parentHSizer->Add(panelGridVSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
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
        pFrame->StopCapturing();
    Debug.AddLine("Calibration sanity check: user discarded bad calibration");
    pMount->ClearCalibration();
    ShutDown();
}

void CalSanityDialog::OnRestore(wxCommandEvent& evt)
{
    if (pFrame->pGuider && pFrame->pGuider->IsCalibratingOrGuiding())
        pFrame->StopCapturing();

    m_pScope->SetCalibration(m_oldParams);
    m_pScope->SetCalibrationDetails(m_oldDetails, m_oldParams.xAngle, m_oldParams.yAngle);

    pFrame->LoadCalibration();
    pFrame->SetStatusText(_("Previous calibration restored"));
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



