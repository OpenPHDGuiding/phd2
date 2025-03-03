/*
 *  rotator.h
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

#ifndef ROTATOR_INCLUDED
#define ROTATOR_INCLUDED

class Rotator;

class RotatorConfigDialogCtrlSet : ConfigDialogCtrlSet
{
    Rotator *m_rotator;
    wxCheckBox *m_cbReverse;

public:
    RotatorConfigDialogCtrlSet(wxWindow *pParent, Rotator *pRotator, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~RotatorConfigDialogCtrlSet() {};
    virtual void LoadValues();
    virtual void UnloadValues();
};

class RotatorConfigDialogPane : public ConfigDialogPane
{
public:
    RotatorConfigDialogPane(wxWindow *parent);
    ~RotatorConfigDialogPane() {};

    void LoadValues() {};
    void UnloadValues() {};
    virtual void LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap);
};

class Rotator
{
    bool m_connected;
    bool m_isReversed;

public:
    static const float POSITION_UNKNOWN;
    static const float POSITION_ERROR;

    static wxArrayString RotatorList();
    static Rotator *Factory(const wxString& choice);

    static double RotatorPosition();

    Rotator();
    virtual ~Rotator();

    virtual bool Connect();
    virtual bool Disconnect();
    virtual bool IsConnected() const;

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    RotatorConfigDialogCtrlSet *GetConfigDlgCtrlSet(wxWindow *pParent, Rotator *pRotator, AdvancedDialog *pAdvancedDialog,
                                                    BrainCtrlIdMap& CtrlMap);
    virtual void ShowPropertyDialog();

    // get the display name of the rotator device
    virtual wxString Name() const = 0;

    // get the rotator position in degrees, or POSITION_ERROR in case of error
    virtual float Position() const = 0;

    bool IsReversed() const;
    void SetReversed(bool val);
};

extern Rotator *pRotator;

inline double Rotator::RotatorPosition()
{
    if (!pRotator || !pRotator->IsConnected())
        return POSITION_UNKNOWN;
    double pos = pRotator->Position();
    return pRotator->IsReversed() ? -pos : pos;
}

inline bool Rotator::IsReversed() const
{
    return m_isReversed;
}

#endif
