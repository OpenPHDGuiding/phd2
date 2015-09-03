/*
 *  configdialog.h
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

#ifndef CONFIG_DIALOG_H_INCLUDED
#define CONFIG_DIALOG_H_INCLUDED

enum BRAIN_CTRL_IDS : unsigned int;
struct BrainCtrlInfo;

class ConfigDialogPane : public wxStaticBoxSizer
{
protected:
    wxWindow *m_pParent;
public:
    ConfigDialogPane(const wxString& heading, wxWindow *pParent);
    virtual ~ConfigDialogPane(void) {};

    virtual void LoadValues(void) = 0;
    virtual void UnloadValues(void) = 0;
    virtual void Undo();
    wxWindow* GetSingleCtrl(std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS id);
    wxSizer*  GetSizerCtrl(std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS id);
    void CondAddCtrl(wxSizer* szr, std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS id, const wxSizerFlags& flags=0);

protected:
    wxSizer *MakeLabeledControl(const wxString& label, wxWindow *pControl, const wxString& toolTip, wxWindow *pControl2 = NULL);
    void DoAdd(wxSizer *pSizer);
    void DoAdd(wxWindow *pWindow);
    void DoAdd(wxWindow *pWindow, const wxString& toolTip);
    void DoAdd(const wxString& Label, wxWindow *pControl, const wxString& toolTip, wxWindow *pControl2 = NULL);

    int StringWidth(const wxString& string);
    int StringArrayWidth(wxString string[], int nElements);
};

class MyFrame;
class AdvancedDialog;

// ConfigDialogCtrlSet objects create and manage the UI controls and associated semantics - but they don't control tab locations or layout - those functions
// are done by the ConfigDialogPane objects
class ConfigDialogCtrlSet
{
protected:
    wxWindow* m_pParent;
    AdvancedDialog* m_pAdvDlg;

public:
    ConfigDialogCtrlSet(wxWindow* pParent, AdvancedDialog* pAdvancedDialog, std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap);
    virtual ~ConfigDialogCtrlSet(void) {};

    virtual void LoadValues(void) = 0;
    virtual void UnloadValues(void) = 0;

public:
    wxSizer *MakeLabeledControl(BRAIN_CTRL_IDS id, const wxString& label, wxWindow *pControl, const wxString& toolTip);
    void AddMapElement(std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS, wxObject *pElem);
    void AddGroup(std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS id, wxSizer *pSizer);      // Sizer
    void AddCtrl(std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS id, wxControl *pCtrl);      // Bare control
    void AddLabeledCtrl(std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS id, const wxString& Label, wxControl *pCtrl, const wxString& toolTip);
    void AddCtrl(std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> & CtrlMap, BRAIN_CTRL_IDS id, wxControl *pCtrl, const wxString& toolTip);     // Control with tooltip

    wxWindow* GetParentWindow(BRAIN_CTRL_IDS id);

    int StringWidth(const wxString& string);
    int StringArrayWidth(wxString string[], int nElements);
};
#endif // CONFIG_DIALOG_H_INCLUDED
