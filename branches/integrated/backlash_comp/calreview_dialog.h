/*
*  calreview_dialog.h
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

#ifndef _CALREVIEW_DIALOG_H_
#define _CALREVIEW_DIALOG_H_

#include "wx/notebook.h"

class CalReviewDialog: public wxDialog
{
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CalReviewDialog( );
    CalReviewDialog(wxFrame* parent, const wxString& caption = _("Review Calibration"));
    ~CalReviewDialog();

protected:
    /// Creation
    bool Create(wxWindow* parent, const wxString& caption, const wxWindowID& id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(400, 300), long style = wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCLOSE_BOX);

    /// Builds the controls and sizers with virtual functions for derived dialogs
    virtual void CreateControls();
    virtual void AddButtons(CalReviewDialog* parentDialog, wxBoxSizer* parentVSizer);
    void CreatePanel(wxPanel* thisPanel, bool AO);
    virtual void CreateDataGrids(wxPanel* parentPanel, wxSizer* parentHSizer, bool AO);
    wxBitmap CreateGraph(bool AO);
    void ShutDown();
    void OnCloseWindow(wxCloseEvent& event);
    void OnCancelClick(wxCommandEvent& event);

    bool m_childDialog;
};

// Derived classes
// CalRetoreDialog is used to review and optionally restore the last calibration
class CalRestoreDialog : public CalReviewDialog
{

public:
    CalRestoreDialog(wxFrame* parent, const wxString& caption = _("Restore Calibration"));
private:
    virtual void AddButtons(CalReviewDialog* parentDialog, wxBoxSizer* parentVSizer);
    void OnRestore(wxCommandEvent& event);
};

// CalSanityDialog is used for display of sanity-checking results
class CalSanityDialog : public CalReviewDialog
{
public:
    CalSanityDialog(wxFrame* parent, const Calibration& oldParams, const CalibrationDetails& oldDetails,
        Calibration_Issues issue);

    ~CalSanityDialog();
private:
    virtual void AddButtons(CalReviewDialog* parentDialog, wxBoxSizer* parentVSizer);
    virtual void CreateDataGrids(wxPanel* parentPanel, wxSizer* parentHSizer, bool AO);

    void OnIgnore(wxCommandEvent& evt);
    void OnRecal(wxCommandEvent& evt);
    void OnRestore(wxCommandEvent& evt);
    void SaveBlockingOptions();
    void BuildMessage(wxStaticText* pText, Calibration_Issues etype);
    void ShutDown();
    wxCheckBox *m_pBlockThis;
    Calibration m_newParams;
    Calibration m_oldParams;
    Calibration_Issues m_issue;
    CalibrationDetails m_calDetails;
    CalibrationDetails m_oldDetails;
    wxString m_newAngleDelta;
    wxString m_oldNewDifference;
    bool m_oldValid;
    Scope *m_pScope;
};
#endif
    // _CALREVIEW_DIALOG_H_
