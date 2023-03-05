/*
*  rotator.cpp
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

#include "gear_simulator.h"
#include "rotator_ascom.h"

const float Rotator::POSITION_ERROR = -999.f;
const float Rotator::POSITION_UNKNOWN = -888.f;

Rotator *pRotator;

wxArrayString Rotator::RotatorList()
{
    wxArrayString rotatorList;

    rotatorList.Add(_("None"));
#ifdef ROTATOR_ASCOM
    wxArrayString ascomRotators = RotatorAscom::EnumAscomRotators();
    for (unsigned int i = 0; i < ascomRotators.Count(); i++)
        rotatorList.Add(ascomRotators[i]);
#endif
#ifdef ROTATOR_SIMULATOR
    rotatorList.Add(_T("Simulator"));
#endif

    return rotatorList;
}

Rotator *Rotator::Factory(const wxString& choice)
{
    Rotator *rotator = 0;

    try
    {
        if (choice.IsEmpty())
        {
            throw ERROR_INFO("Rotator::Factory called with choice.IsEmpty()");
        }

        Debug.AddLine(wxString::Format("RotatorFactory(%s)", choice));

        if (false) // so else ifs can follow
        {
        }
#ifdef ROTATOR_ASCOM
        // do ascom first since it includes many choices, some of which match other choices below (like Simulator)
        else if (choice.Find(_T("ASCOM")) != wxNOT_FOUND) {
            rotator = new RotatorAscom(choice);
        }
#endif
        else if (choice.Find(_("None")) != wxNOT_FOUND) {
        }
#ifdef ROTATOR_SIMULATOR
        else if (choice.Find(_T("Simulator")) != wxNOT_FOUND) {
            rotator = GearSimulator::MakeRotatorSimulator();
        }
#endif
        else {
            throw ERROR_INFO("RotatorFactory: Unknown rotator choice");
        }
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
        if (rotator)
        {
            delete rotator;
            rotator = 0;
        }
    }

    return rotator;
}

Rotator::Rotator()
    : m_connected(false)
{
    m_isReversed = pConfig->Profile.GetBoolean("/rotator/isReversed", false);
    Debug.Write(wxString::Format("Rotator:ctor: isReversed = %d\n", m_isReversed));
}

Rotator::~Rotator()
{
}

bool Rotator::Connect()
{
    m_connected = true;
    return false;
}

bool Rotator::Disconnect()
{
    m_connected = false;
    return false;
}

bool Rotator::IsConnected() const
{
    return m_connected;
}

void Rotator::ShowPropertyDialog()
{
}

void Rotator::SetReversed(bool val)
{
    m_isReversed = val;
    pConfig->Profile.SetBoolean("/rotator/isReversed", val);
    Debug.Write(wxString::Format("Rotator:SetReversed: isReversed = %d\n", m_isReversed));
}

RotatorConfigDialogPane::RotatorConfigDialogPane(wxWindow *parent)
    : ConfigDialogPane(_("Rotator Settings"), parent)
{

}

void RotatorConfigDialogPane::LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap)
{
    this->Add(GetSingleCtrl(CtrlMap, AD_cbRotatorReverse), wxSizerFlags(0).Border(wxALL, 10));
    this->Layout();
    Fit(m_pParent);
}

ConfigDialogPane *Rotator::GetConfigDialogPane(wxWindow *parent)
{
    return new RotatorConfigDialogPane(parent);
}

RotatorConfigDialogCtrlSet *Rotator::GetConfigDlgCtrlSet(wxWindow *pParent, Rotator *pRotator, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap)
{
    return new RotatorConfigDialogCtrlSet(pParent, pRotator, pAdvancedDialog, CtrlMap);
}

RotatorConfigDialogCtrlSet::RotatorConfigDialogCtrlSet(wxWindow *pParent, Rotator *pRotator, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap)
    : ConfigDialogCtrlSet(pParent, pAdvancedDialog, CtrlMap)
{
    m_rotator = pRotator;
    m_cbReverse = new wxCheckBox(GetParentWindow(AD_cbRotatorReverse), wxID_ANY, _("Reverse sign of angle"));
    AddCtrl(CtrlMap, AD_cbRotatorReverse, m_cbReverse);
}

void RotatorConfigDialogCtrlSet::LoadValues()
{
    m_cbReverse->SetValue(m_rotator->IsReversed());
}

void RotatorConfigDialogCtrlSet::UnloadValues()
{
    m_rotator->SetReversed(m_cbReverse->GetValue());
}
