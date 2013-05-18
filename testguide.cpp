/*
 *  testguide.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

TestGuideDialog::TestGuideDialog():
wxDialog(pFrame, wxID_ANY, _("Manual Guide"), wxPoint(-1,-1), wxSize(300,300)) {
    wxBoxSizer *pOuterSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *pWrapperSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Primary Mount");
    wxGridSizer *sizer = new wxGridSizer(3,3,0,0);
    static wxString AoLabels[] = {_("Up"), _("Down"), _("Right"), _("Left") };
    static wxString ScopeLabels[] = {_("North"), _("South"), _("East"), _("West") };

    wxString *pLabels;

    if (pSecondaryMount && pSecondaryMount->IsConnected())
    {
        pLabels = AoLabels;
    }
    else
    {
        pLabels = ScopeLabels;
    }

    // Build the buttons for the primary mount

    NButton1 = new wxButton(this, MGUIDE1_UP, pLabels[0], wxPoint(-1,-1),wxSize(-1,-1));
    SButton1 = new wxButton(this, MGUIDE1_DOWN, pLabels[1], wxPoint(-1,-1),wxSize(-1,-1));
    EButton1 = new wxButton(this, MGUIDE1_RIGHT, pLabels[2], wxPoint(-1,-1),wxSize(-1,-1));
    WButton1 = new wxButton(this, MGUIDE1_LEFT, pLabels[3], wxPoint(-1,-1),wxSize(-1,-1));

    sizer->AddStretchSpacer();
    sizer->Add(NButton1,wxSizerFlags().Expand().Border(wxALL,6));
    sizer->AddStretchSpacer();
    sizer->Add(WButton1,wxSizerFlags().Expand().Border(wxALL,6));
    sizer->AddStretchSpacer();
    sizer->Add(EButton1,wxSizerFlags().Expand().Border(wxALL,6));
    sizer->AddStretchSpacer();
    sizer->Add(SButton1,wxSizerFlags().Expand().Border(wxALL,6));

    pWrapperSizer->Add(sizer);
    pOuterSizer->Add(pWrapperSizer);

    if (pSecondaryMount && pSecondaryMount->IsConnected())
    {
        pWrapperSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Secondary Mount");
        sizer = new wxGridSizer(3,3,0,0);

        pLabels = ScopeLabels;

        NButton2 = new wxButton(this, MGUIDE2_UP, pLabels[0], wxPoint(-1,-1),wxSize(-1,-1));
        SButton2 = new wxButton(this, MGUIDE2_DOWN, pLabels[1], wxPoint(-1,-1),wxSize(-1,-1));
        EButton2 = new wxButton(this, MGUIDE2_RIGHT, pLabels[2], wxPoint(-1,-1),wxSize(-1,-1));
        WButton2 = new wxButton(this, MGUIDE2_LEFT, pLabels[3], wxPoint(-1,-1),wxSize(-1,-1));

        sizer->AddStretchSpacer();
        sizer->Add(NButton2,wxSizerFlags().Expand().Border(wxALL,6));
        sizer->AddStretchSpacer();
        sizer->Add(WButton2,wxSizerFlags().Expand().Border(wxALL,6));
        sizer->AddStretchSpacer();
        sizer->Add(EButton2,wxSizerFlags().Expand().Border(wxALL,6));
        sizer->AddStretchSpacer();
        sizer->Add(SButton2,wxSizerFlags().Expand().Border(wxALL,6));

        pWrapperSizer->Add(sizer);
        pOuterSizer->Add(pWrapperSizer);
    }

    SetSizer(pOuterSizer);
    pOuterSizer->SetSizeHints(this);
}

BEGIN_EVENT_TABLE(TestGuideDialog, wxDialog)
EVT_BUTTON(MGUIDE1_UP,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE1_DOWN,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE1_RIGHT,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE1_LEFT,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE2_UP,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE2_DOWN,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE2_RIGHT,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE2_LEFT,TestGuideDialog::OnButton)
END_EVENT_TABLE()

void TestGuideDialog::OnButton(wxCommandEvent &evt) {
    switch (evt.GetId()) {
        case MGUIDE1_UP:
            if (pMount && pMount->IsConnected())
            {
                pMount->CalibrationMove(UP);
            }
            break;
        case MGUIDE1_DOWN:
            if (pMount && pMount->IsConnected())
            {
                pMount->CalibrationMove(DOWN);
            }
            break;
        case MGUIDE1_RIGHT:
            if (pMount && pMount->IsConnected())
            {
                pMount->CalibrationMove(RIGHT);
            }
            break;
        case MGUIDE1_LEFT:
            if (pMount && pMount->IsConnected())
            {
                pMount->CalibrationMove(LEFT);
            }
            break;
        case MGUIDE2_UP:
            if (pSecondaryMount && pSecondaryMount->IsConnected())
            {
                pSecondaryMount->CalibrationMove(UP);
            }
            break;
        case MGUIDE2_DOWN:
            if (pSecondaryMount && pSecondaryMount->IsConnected())
            {
                pSecondaryMount->CalibrationMove(DOWN);
            }
            break;
        case MGUIDE2_RIGHT:
            if (pSecondaryMount && pSecondaryMount->IsConnected())
            {
                pSecondaryMount->CalibrationMove(RIGHT);
            }
            break;
        case MGUIDE2_LEFT:
            if (pSecondaryMount && pSecondaryMount->IsConnected())
            {
                pSecondaryMount->CalibrationMove(LEFT);
            }
            break;
    }
}
