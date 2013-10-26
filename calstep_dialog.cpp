/*
 *  calstep_dialog.cpp
 *  PHD Guiding
 *
 *  Created by Bruce Waddington
 *  Copyright (c) 2013 Bruce Waddington
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
#include "calstep_dialog.h"
#include "wx/valnum.h"

CalstepDialog::CalstepDialog(int focalLength, double pixelSize, const wxString& configPrefix) :
    wxDialog(pFrame, wxID_ANY, _("Calibration Step Calculator"), wxDefaultPosition, wxSize(400, 500), wxCAPTION | wxCLOSE_BOX)
{
    double dGuideRateDec = 0.0; // initialize to suppress compiler warning
    double dGuideRateRA = 0.0; // initialize to suppress compiler warning
    bool bRateError = true;
    const double dSiderealSecondPerSec = 0.9973;

    // Get squared away with initial parameter values
    m_iNumSteps = 12;
    if (focalLength > 0)
        m_iFocalLength = focalLength;
    else
        m_iFocalLength = 0;
    if (pixelSize > 0)
        m_fPixelSize = pixelSize;
    else
        m_fPixelSize = 0;
    // See if we can get the guide rates - if not, use our best default
    try
    {
        if (pSecondaryMount && pSecondaryMount->IsConnected())
            bRateError = pSecondaryMount->GetGuideRates(&dGuideRateRA, &dGuideRateDec);
        else if (pMount && pMount->IsConnected())
            bRateError = pMount->GetGuideRates(&dGuideRateRA, &dGuideRateDec);
    }
    catch (wxString sMsg)
    {
        bRateError = true;
        POSSIBLY_UNUSED(sMsg);
    }
    if (!bRateError)
    {
        if (dGuideRateRA >= dGuideRateDec)
            m_fGuideSpeed = dGuideRateRA * 3600.0/(15.0 * dSiderealSecondPerSec);                    // Degrees/sec to Degrees/hour, 15 degrees/hour is roughly sidereal rate
        else
            m_fGuideSpeed = dGuideRateDec/(15.0 * dSiderealSecondPerSec);
    }
    else
    {
        m_sConfigPrefix = configPrefix;             // Try getting it from the config file
        if (m_sConfigPrefix.Len() > 0)
            m_fGuideSpeed = (float) pConfig->Profile.GetDouble (m_sConfigPrefix + "/GuideSpeed", 0);
        else
            m_fGuideSpeed = 0;
    }
    if (m_fGuideSpeed <= 0)
        m_fGuideSpeed = 1.0;                // Give him an ok default
    m_bValidResult = false;

    // Create the sizers we're going to need
    m_pVSizer = new wxBoxSizer(wxVERTICAL);
    m_pInputTableSizer = new wxFlexGridSizer (2, 2, 15, 15);
    m_pOutputTableSizer = new wxFlexGridSizer (2, 2, 15, 15);

    // Build the group of input fields
    m_pInputGroupBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Input Parameters"));

    // Note that "min" values in fp validators don't work right - so leave them out
    // Focal length - int <= 4000
    int width = StringWidth(_T("00000")) + 10;
    wxIntegerValidator <int> valFocalLength (&m_iFocalLength, 0);
    valFocalLength.SetRange (0, 3500);
    m_pFocalLength = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1), 0, valFocalLength);
    AddTableEntry (m_pInputTableSizer, _("Focal length, mm"), m_pFocalLength, _("Guide scope focal length"));

    // Pixel size: float <= 25
    m_pPixelSize = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1), 0);
    m_pPixelSize->SetValue(wxString::Format("%.2f",m_fPixelSize));
    AddTableEntry (m_pInputTableSizer, _("Pixel size, microns"), m_pPixelSize, _("Guide camera pixel size"));

    // Guide speed multiplier: float <= 2.0
    m_pGuideSpeed = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1), 0);
    m_pGuideSpeed->SetValue(wxString::Format("%.2f", m_fGuideSpeed));
    AddTableEntry (m_pInputTableSizer, _("Guide speed, n.nn x sidereal"), m_pGuideSpeed, _("Guide speed, multiple of sidereal rate; to guide at ") +
        _("50% sidereal rate, enter 0.5"));

    // Number of steps: int < MAX_CALIBRATION_STEPS
    wxIntegerValidator <int> valNumSteps (&m_iNumSteps, 0);
    valNumSteps.SetRange (0, 60);
    m_pNumSteps = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1), 0, valNumSteps);
    AddTableEntry (m_pInputTableSizer, _("Calibration steps"), m_pNumSteps, _("Targeted # steps in each direction"));

    // Build the group of output fields
    m_pOutputGroupBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Computed Values"));

    m_pImageScale = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    AddTableEntry (m_pOutputTableSizer, _("Image scale, arc-sec/px"), m_pImageScale, "");
    m_pRslt = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    AddTableEntry (m_pOutputTableSizer, _("Calibration step, ms"), m_pRslt, "");

    // Add the tables to the panel, centered
    m_pInputGroupBox->Add (m_pInputTableSizer, 0, wxALL, 10);
    m_pOutputGroupBox->Add (m_pOutputTableSizer, 0, wxALL, 10);
    m_pVSizer->Add (m_pInputGroupBox, wxSizerFlags().Center().Border(10));
    m_pVSizer->Add (m_pOutputGroupBox, wxSizerFlags().Center().Border(10));

    // Now deal with the buttons
    wxBoxSizer *pButtonSizer = new wxBoxSizer( wxHORIZONTAL );

     //create three buttons that are horizontally unstretchable,
     // with an all-around border with a width of 10 and implicit top alignment
    wxButton *pRecalc = new wxButton( this, wxID_OK, _("Recalc") );
    pRecalc->Bind (wxEVT_COMMAND_BUTTON_CLICKED, &CalstepDialog::OnRecalc, this);
    pButtonSizer->Add(
        pRecalc,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
	pButtonSizer->Add(
		CreateButtonSizer(wxOK | wxCANCEL),
		wxSizerFlags(0).Align(0).Border(wxALL, 10));

     //position the buttons centered with no border
     m_pVSizer->Add(
        pButtonSizer,
        wxSizerFlags(0).Center() );
    SetSizerAndFit (m_pVSizer);
}

int CalstepDialog::StringWidth(const wxString& string)
{
    int width, height;

    GetTextExtent(string, &width, &height);

    return width;
}

// Utility function to add the <label, input> tuples to the grid including tool-tips
void CalstepDialog::AddTableEntry (wxFlexGridSizer *pTable, wxString label, wxWindow *pControl, wxString toolTip)
{
    wxStaticText *pLabel = new wxStaticText(this, wxID_ANY, label + _(": "),wxPoint(-1,-1),wxSize(-1,-1));
    pTable->Add (pLabel, 1, wxALL, 5);
    pTable->Add (pControl, 1, wxALL, 5);
    pControl->SetToolTip (toolTip);
}

 // Based on computed image scale, compute a calibration pulse direction that will result in DesiredSteps
//  for a "travel" distance of MAX_CALIBRATION_DISTANCE in each direction.  Note: this doesn't take into account any dec
//  compensation for RA times.  Result will be rounded up to the nearest 50 ms
bool CalstepDialog::CalcDefaultDuration (int FocalLength, double PixelSize, double GuideSpeed, int DesiredSteps, double& ImageScale, int& StepSize)
{
    int totalDistance;            // In units of arc-secs
    float totalDuration;
    int iTemp;

    try
    {
        // Interim variables to ease debugging
        ImageScale = 3438.0 * PixelSize/1000.0 * 60.0f / FocalLength;
        totalDistance = 25.0 * ImageScale;
        totalDuration = totalDistance / (15.0 * GuideSpeed);
        iTemp = floor (totalDuration/DesiredSteps * 1000.0);            // milliseconds
        StepSize = ((iTemp+49) / 50) * 50;                                // discourage "false precision" - round up to nearest 50 ms
        return (true);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        return (false);
    }

}
// Event handler for the 'Recalc' button
void CalstepDialog::OnRecalc (wxCommandEvent& evt)
{
    m_bValidResult = false;

    if (this->Validate() && this->TransferDataFromWindow())
    {
        m_pPixelSize->GetValue().ToDouble(&m_fPixelSize);
        m_pPixelSize->SetValue(wxString::Format("%.2f",m_fPixelSize));

        m_pGuideSpeed->GetValue().ToDouble(&m_fGuideSpeed);
        m_pGuideSpeed->SetValue(wxString::Format("%.2f",m_fGuideSpeed));

        // The validators make life easier by insuring there will be numeric values of some kind in the various properties
        if (m_iFocalLength >= 50 && m_iFocalLength <= 4000)
            if (m_fPixelSize >= 3.0 && m_fPixelSize <= 25)
                if (m_fGuideSpeed >= 0.2 && m_fGuideSpeed <= 2.0)
                    if (m_iNumSteps >= 6 && m_iNumSteps <= 60)
                    {
                        if (CalcDefaultDuration (m_iFocalLength, m_fPixelSize, m_fGuideSpeed, m_iNumSteps, m_fImageScale, m_iRslt))
                        {
                            m_pImageScale->SetValue (wxString::Format ("%.2f", m_fImageScale));
                            m_pRslt->SetValue (wxString::Format ("%3d", m_iRslt));
                            // Remember the guide speed chosen is just to help the user - purely a UI thing, no guiding implications
                            pConfig->Profile.SetDouble (m_sConfigPrefix + "/GuideSpeed", m_fGuideSpeed);
                            m_bValidResult = true;
                        }
                        else
                            wxMessageBox ("Could not compute step size", "Error", wxOK | wxICON_ERROR);
                    }
                    else
                        wxMessageBox (_("Num steps must be >= 6 and <= 60"),  _("Error"), wxOK | wxICON_ERROR);
                else
                    wxMessageBox (_("Guide speed must be >= 0.2 and <= 2.0"),  _("Error"), wxOK | wxICON_ERROR);
            else
                wxMessageBox (_("Pixel size must be >= 3.0 and <= 25.0"),  _("Error"), wxOK | wxICON_ERROR);
        else
            wxMessageBox (_("Focal length must be >= 50 and < 4000"),  _("Error"), wxOK | wxICON_ERROR);
    }
}
// Public function for client to get the calculated value for step-size
int CalstepDialog::GetResult ()
{
    if (m_bValidResult)
        return (m_iRslt);
    else
        return (0);
}

CalstepDialog::~CalstepDialog(void)
{
}
