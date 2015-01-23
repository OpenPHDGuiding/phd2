/*
 *  calstep_dialog.h
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

#ifndef CalstepDialog_h_included
#define CalstepDialog_h_included

class CalstepDialog : public wxDialog
{
private:

    // wx UI controls
    wxDialog *m_pParent;
    wxBoxSizer *m_pVSizer;
    wxFlexGridSizer *m_pInputTableSizer;
    wxFlexGridSizer *m_pOutputTableSizer;
    wxStaticBoxSizer *m_pInputGroupBox;
    wxStaticBoxSizer *m_pOutputGroupBox;
    wxTextCtrl *m_pFocalLength;
    wxSpinCtrlDouble *m_pPixelSize;
    wxSpinCtrlDouble *m_pGuideSpeed;
    wxSpinCtrlDouble *m_pNumSteps;
    wxSpinCtrlDouble *m_pDeclination;
    wxStaticText *m_status;
    wxTextCtrl *m_pRslt;
    wxTextCtrl *m_pImageScale;
    // numeric values from fields, populated by validators
    double m_fPixelSize;
    wxString m_sPixelSize;
    int m_iFocalLength;
    double m_fGuideSpeed;
    int m_iNumSteps;
    double m_fImageScale;
    int m_iStepSize;
    bool m_bValidResult;
    double m_dDeclination;

public:

    enum { DEFAULT_STEPS = 12 };
    static const double DEFAULT_GUIDESPEED;

    CalstepDialog(wxWindow *parent, int focalLength, double pixelSize);
    ~CalstepDialog(void);
    bool GetResults(int *focalLength, double *pixelSize, int *stepSize);

    static void GetCalibrationStepSize(int focalLength, double pixelSize, double guideSpeed, int desiredSteps,
                       double declination, double *imageScale, int *stepSize);

private:
    void OnText(wxCommandEvent& evt);
    void OnSpinCtrlDouble(wxSpinDoubleEvent& evt);
    void AddTableEntry(wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl, const wxString& toolTip);
    void DoRecalc(void);
};

#endif
