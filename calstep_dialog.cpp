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

#define MIN_PIXELSIZE 0.1
#define MAX_PIXELSIZE 25.0
#define MIN_GUIDESPEED 0.10
#define MAX_GUIDESPEED 2.0
#define MIN_STEPS 6.0
#define MAX_STEPS 60.0
#define MIN_DECLINATION -60.0
#define MAX_DECLINATION 60.0

const double CalstepDialog::DEFAULT_GUIDESPEED = 1.0;       // 100% sidereal rate, help insure we don't end up with too few steps

static wxSpinCtrlDouble *NewSpinner(wxWindow *parent, int width, double val, double minval, double maxval, double inc)
{
    wxSpinCtrlDouble *pNewCtrl = new wxSpinCtrlDouble(parent, wxID_ANY, _T("foo2"), wxPoint(-1, -1),
        wxSize(width, -1), wxSP_ARROW_KEYS, minval, maxval, val, inc);
    pNewCtrl->SetDigits(2);
    return pNewCtrl;
}

CalstepDialog::CalstepDialog(wxWindow *parent, int focalLength, double pixelSize) :
    wxDialog(parent, wxID_ANY, _("Calibration Step Calculator"), wxDefaultPosition, wxSize(400, 500), wxCAPTION | wxCLOSE_BOX)
{
    double dGuideRateDec = 0.0; // initialize to suppress compiler warning
    double dGuideRateRA = 0.0; // initialize to suppress compiler warning
    const double dSiderealSecondPerSec = 0.9973;

    // Get squared away with initial parameter values - start with values from the profile
    m_iNumSteps = pConfig->Profile.GetInt("/CalStepCalc/NumSteps", DEFAULT_STEPS);
    m_dDeclination = pConfig->Profile.GetDouble ("/CalStepCalc/CalDeclination", 0.0);
    m_iFocalLength = focalLength;
    m_fPixelSize = pixelSize;
    m_fGuideSpeed = (float) pConfig->Profile.GetDouble ("/CalStepCalc/GuideSpeed", DEFAULT_GUIDESPEED);
    // Now improve on Dec and guide speed if mount/pointing info is available
    if (pPointingSource && pPointingSource->IsConnected())
    {
        if (!pPointingSource->GetGuideRates(&dGuideRateRA, &dGuideRateDec))
        {
            if (dGuideRateRA >= dGuideRateDec)
                m_fGuideSpeed = dGuideRateRA * 3600.0 / (15.0 * dSiderealSecondPerSec);  // Degrees/sec to Degrees/hour, 15 degrees/hour is roughly sidereal rate
            else
                m_fGuideSpeed = dGuideRateDec * 3600.0 / (15.0 * dSiderealSecondPerSec);

            if (m_fGuideSpeed < MIN_GUIDESPEED)
                m_fGuideSpeed = MIN_GUIDESPEED;
        }
        double ra_val, dec_val, st;
        if (!pPointingSource->GetCoordinates(&ra_val, &dec_val, &st))
            m_dDeclination = dec_val;
    }

    m_bValidResult = false;

    // Create the sizers we're going to need
    m_pVSizer = new wxBoxSizer(wxVERTICAL);
    m_pInputTableSizer = new wxFlexGridSizer (2, 2, 15, 15);
    m_pOutputTableSizer = new wxFlexGridSizer (2, 2, 15, 15);

    // Build the group of input fields
    m_pInputGroupBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Input Parameters"));

    // Note that "min" values in fp validators don't work right - so leave them out
    // Focal length - any positive int, same as global tab
    int width = StringWidth(this, "00000") + 10;
    wxIntegerValidator <int> valFocalLength (&m_iFocalLength, 0);
    valFocalLength.SetRange(0, wxINT32_MAX);
    m_pFocalLength = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1), 0, valFocalLength);
    m_pFocalLength->Enable(!pFrame->CaptureActive);
    m_pFocalLength->Bind(wxEVT_TEXT, &CalstepDialog::OnText, this);
    AddTableEntry (m_pInputTableSizer, _("Focal length, mm"), m_pFocalLength, _("Guide scope focal length"));

    // Pixel size
    m_pPixelSize = NewSpinner (this, 1.5*width, m_fPixelSize, MIN_PIXELSIZE, MAX_PIXELSIZE, 0.1);
    m_pPixelSize->Enable(!pFrame->CaptureActive);
    m_pPixelSize->Bind(wxEVT_SPINCTRLDOUBLE, &CalstepDialog::OnSpinCtrlDouble, this);
    AddTableEntry (m_pInputTableSizer, _("Pixel size, microns"), m_pPixelSize, _("Guide camera pixel size"));

    // Guide speed
    m_pGuideSpeed = NewSpinner (this, 1.5*width, m_fGuideSpeed, MIN_GUIDESPEED, MAX_GUIDESPEED, 0.25);
    m_pGuideSpeed->Bind(wxEVT_SPINCTRLDOUBLE, &CalstepDialog::OnSpinCtrlDouble, this);
    AddTableEntry (m_pInputTableSizer, _("Guide speed, n.nn x sidereal"), m_pGuideSpeed,
                   _("Guide speed, multiple of sidereal rate; if your mount's guide speed is 50% sidereal rate, enter 0.5"));

    // Number of steps
    m_pNumSteps = NewSpinner (this, 1.5*width, m_iNumSteps, MIN_STEPS, MAX_STEPS, 1);
    m_pNumSteps->SetDigits (0);
    m_pNumSteps->Bind(wxEVT_SPINCTRLDOUBLE, &CalstepDialog::OnSpinCtrlDouble, this);
    AddTableEntry (m_pInputTableSizer, _("Calibration steps"), m_pNumSteps,
                   wxString::Format(_("Targeted number of steps in each direction. The default value (%d) works fine for most setups."), (int) DEFAULT_STEPS));

    // Calibration declination
    m_pDeclination = NewSpinner(this, 1.5*width, m_dDeclination, MIN_DECLINATION, MAX_DECLINATION, 5.0);
    m_pDeclination->SetDigits(0);
    m_pDeclination->Bind(wxEVT_SPINCTRLDOUBLE, &CalstepDialog::OnSpinCtrlDouble, this);
    AddTableEntry (m_pInputTableSizer, _("Calibration declination, degrees"), m_pDeclination, _("Approximate declination where you will do calibration"));

    // Build the group of output fields
    m_pOutputGroupBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Computed Values"));

    m_status = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    m_pImageScale = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    m_pImageScale->Enable(false);
    AddTableEntry (m_pOutputTableSizer, _("Image scale, arc-sec/px"), m_pImageScale, "");
    m_pRslt = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width, -1));
    AddTableEntry (m_pOutputTableSizer, _("Calibration step, ms"), m_pRslt, "");

    // Add the tables to the panel, centered
    m_pInputGroupBox->Add (m_pInputTableSizer, 0, wxALL, 10);
    m_pOutputGroupBox->Add (m_pOutputTableSizer, 0, wxALL, 10);
    m_pVSizer->Add(m_status, 1, wxALL, 5);
    m_pVSizer->Add (m_pInputGroupBox, wxSizerFlags().Center().Border(wxALL, 10));
    m_pVSizer->Add (m_pOutputGroupBox, wxSizerFlags().Center().Border(wxRIGHT|wxLEFT|wxBOTTOM, 10));

    // ok/cancel buttons
    m_pVSizer->Add(
        CreateButtonSizer(wxOK | wxCANCEL),
        wxSizerFlags(0).Expand().Border(wxALL, 10) );

    SetSizerAndFit (m_pVSizer);
}

// Utility function to add the <label, input> tuples to the grid including tool-tips
void CalstepDialog::AddTableEntry (wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl, const wxString& toolTip)
{
    wxStaticText *pLabel = new wxStaticText(this, wxID_ANY, label + _(": "),wxPoint(-1,-1),wxSize(-1,-1));
    pTable->Add (pLabel, 1, wxALL, 5);
    pTable->Add (pControl, 1, wxALL, 5);
    pControl->SetToolTip (toolTip);
}

// Based on computed image scale, compute an RA calibration pulse direction that will result in DesiredSteps
//  for a "travel" distance of MAX_CALIBRATION_DISTANCE in each direction, adjusted for declination.
//  Result will be rounded up to the nearest 50 ms and is constrained to insure Dec calibration is at least
//  MIN_STEPs
//
//  FocalLength = focal length in millimeters
//  PixelSize = pixel size in microns
//  GuideSpeed = guide rate as fraction of sidereal rate
//  DesiredSteps = desired number of calibration steps
//  Declination = declination in degrees
//  pImageScale = address of computed image scale (arc-sec/pixel)
//  pStepSize = address of computed calibration step size, milliseconds
//
void CalstepDialog::GetCalibrationStepSize(int FocalLength, double PixelSize, double GuideSpeed, int DesiredSteps,
    double Declination, double *pImageScale, int *pStepSize)
{
    enum { CALIBRATION_PIXELS = 25 };
    double ImageScale = MyFrame::GetPixelScale(PixelSize, FocalLength); // arc-sec per pixel
    double totalDistance = (double) CALIBRATION_PIXELS * ImageScale; // arc-seconds
    double totalDuration = totalDistance / (15.0 * GuideSpeed);      // 15 arc-sec/sec approx sidereal rate
    double Pulse = totalDuration / DesiredSteps * 1000.0;            // milliseconds at DEC=0
    double MaxPulse = totalDuration / MIN_STEPS * 1000.0;            // max pulse size to still get MIN steps
    Pulse = wxMin(MaxPulse, Pulse / cos(M_PI / 180.0 * Declination)); // UI forces abs(Dec) <= 60 degrees
    if (pImageScale)
        *pImageScale = ImageScale;
    *pStepSize = (int) ceil(Pulse / 50.0) * 50;                      // round up to nearest 50 ms
}

void CalstepDialog::OnText(wxCommandEvent& evt)
{
    DoRecalc();
    evt.Skip();
}

void CalstepDialog::OnSpinCtrlDouble(wxSpinDoubleEvent& evt)
{
    DoRecalc();
    evt.Skip();
}

void CalstepDialog::DoRecalc(void)
{
    m_bValidResult = false;

    if (this->Validate() && this->TransferDataFromWindow())
    {
        m_fPixelSize = m_pPixelSize->GetValue();
        m_pPixelSize->SetValue(m_fPixelSize);           // For European locales, '.' -> ',' on output
        m_fGuideSpeed = m_pGuideSpeed->GetValue();
        m_pGuideSpeed->SetValue(m_fGuideSpeed);
        m_iNumSteps = m_pNumSteps->GetValue();
        m_dDeclination = abs(m_pDeclination->GetValue());

        if (m_iFocalLength < 50)
        {
            m_status->SetLabel(_("Please enter a focal length of at least 50"));
        }
        else if (m_fPixelSize <= 0.0)
        {
            m_status->SetLabel(_("Please enter a pixel size greater than zero."));
        }
        else
        {
            m_status->SetLabel(wxEmptyString);

            // Spin controls enforce numeric ranges
            GetCalibrationStepSize(m_iFocalLength, m_fPixelSize, m_fGuideSpeed, m_iNumSteps, m_dDeclination, &m_fImageScale, &m_iStepSize);
            m_bValidResult = true;
        }

        if (m_bValidResult)
        {
            m_pImageScale->SetValue (wxString::Format ("%.2f", m_fImageScale));
            m_pRslt->SetValue (wxString::Format ("%3d", m_iStepSize));
        }
        else
        {
            m_pImageScale->SetValue(wxEmptyString);
            m_pRslt->SetValue(wxEmptyString);
        }
    }
}
// Public function for client to get the computed step-size along with possibly modified values for focal length and pixel size
bool CalstepDialog::GetResults(int *focalLength, double *pixelSize, int *stepSize)
{
    if (m_bValidResult)
    {
        // Remember the guide speed chosen is just to help the user - purely a UI thing, no guiding implications
        pConfig->Profile.SetDouble("/CalStepCalc/GuideSpeed", m_fGuideSpeed);
        pConfig->Profile.SetDouble("/CalStepCalc/CalDeclination", m_dDeclination);
        pConfig->Profile.SetInt("/CalStepCalc/NumSteps", m_iNumSteps);

        *focalLength = m_iFocalLength;
        *pixelSize = m_fPixelSize;
        *stepSize = m_iStepSize;
        long lval;
        if (m_pRslt->GetValue().ToLong(&lval) && lval > 0)
            *stepSize = (int) lval;
        return true;
    }
    else
        return false;
}

CalstepDialog::~CalstepDialog(void)
{
}
