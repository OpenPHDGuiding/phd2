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
wxDialog(pFrame, wxID_ANY, _T("Manual Output"), wxPoint(-1,-1), wxSize(300,300)) {
	wxGridSizer *sizer = new wxGridSizer(3,3,0,0);

	NButton = new wxButton(this,MGUIDE_N,_T("North"),wxPoint(-1,-1),wxSize(-1,-1));
	SButton = new wxButton(this,MGUIDE_S,_T("South"),wxPoint(-1,-1),wxSize(-1,-1));
	EButton = new wxButton(this,MGUIDE_E,_T("East"),wxPoint(-1,-1),wxSize(-1,-1));
	WButton = new wxButton(this,MGUIDE_W,_T("West"),wxPoint(-1,-1),wxSize(-1,-1));
	sizer->AddStretchSpacer();
	sizer->Add(NButton,wxSizerFlags().Expand().Border(wxALL,6));
	sizer->AddStretchSpacer();
	sizer->Add(WButton,wxSizerFlags().Expand().Border(wxALL,6));
	sizer->AddStretchSpacer();
	sizer->Add(EButton,wxSizerFlags().Expand().Border(wxALL,6));
	sizer->AddStretchSpacer();
	sizer->Add(SButton,wxSizerFlags().Expand().Border(wxALL,6));

	SetSizer(sizer);
	sizer->SetSizeHints(this);
}

BEGIN_EVENT_TABLE(TestGuideDialog, wxDialog)
EVT_BUTTON(MGUIDE_N,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE_S,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE_E,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE_W,TestGuideDialog::OnButton)
END_EVENT_TABLE()


void TestGuideDialog::OnButton(wxCommandEvent &evt) {
//	if ((pFrame->pGuider->GetState() > STATE_SELECTED) || !(pScope->IsConnected())) return;
	if (!(pScope->IsConnected())) return;
	switch (evt.GetId()) {
		case MGUIDE_N:
			pScope->Guide(NORTH, pScope->GetCalibrationDuration());
			break;
		case MGUIDE_S:
			pScope->Guide(SOUTH, pScope->GetCalibrationDuration());
			break;
		case MGUIDE_E:
			pScope->Guide(EAST, pScope->GetCalibrationDuration());
			break;
		case MGUIDE_W:
			pScope->Guide(WEST, pScope->GetCalibrationDuration());
			break;
	}
}

