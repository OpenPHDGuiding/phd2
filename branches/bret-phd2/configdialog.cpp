/*
 *  configdialog.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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

ConfigDialogPane::ConfigDialogPane(wxString heading, wxWindow *pParent)
    : wxStaticBoxSizer(new wxStaticBox(pParent, wxID_ANY, heading), wxVERTICAL)
{
    m_pParent = pParent;
}

ConfigDialogPane::~ConfigDialogPane(void)
{
}

void ConfigDialogPane::DoAdd(wxSizer *pSizer)
{
    this->Add(pSizer, wxSizerFlags().Expand().Border(wxALL,3));
}

void ConfigDialogPane::DoAdd(wxWindow *pWindow)
{
    this->Add(pWindow, wxSizerFlags().Expand().Border(wxALL,3));
}

void ConfigDialogPane::DoAdd(wxWindow *pWindow, wxString toolTip)
{
    pWindow->SetToolTip(toolTip);
    DoAdd(pWindow);
}

void ConfigDialogPane::DoAdd(wxWindow *pWindow1, wxWindow *pWindow2)
{
    wxBoxSizer *pSizer = new wxBoxSizer(wxHORIZONTAL);
    pSizer->Add(pWindow1);
    pSizer->Add(pWindow2);
    DoAdd(pSizer);
}

void ConfigDialogPane::DoAdd(wxString label, wxWindow *pControl, wxString toolTip)
{
    wxStaticText *pLabel = new wxStaticText(m_pParent, wxID_ANY, label + _T(": "),wxPoint(-1,-1),wxSize(-1,-1));
    pControl->SetToolTip(toolTip);

    DoAdd(pLabel, pControl);
}

int ConfigDialogPane::StringWidth(wxString string)
{
    int width, height;

    m_pParent->GetTextExtent(string, &width, &height);

    return width;
}

int ConfigDialogPane::StringArrayWidth(wxString string[], int nElements)
{
    int width = 0;

    for(int i=0;i<nElements;i++)
    {
        int thisWidth = StringWidth(string[i]);

        if (thisWidth > width)
        {
            width = thisWidth;
        }
    }

    return width;
}
