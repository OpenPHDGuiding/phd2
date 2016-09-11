/*
 *  mount.cpp
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
#include "backlash_comp.h"
#include "guiding_assistant.h"

#include <wx/tokenzr.h>

inline static PierSide OppositeSide(PierSide p)
{
    switch (p) {
    case PIER_SIDE_EAST: return PIER_SIDE_WEST;
    case PIER_SIDE_WEST: return PIER_SIDE_EAST;
    default:             return PIER_SIDE_UNKNOWN;
    }
}

inline static bool IsOppositeSide(PierSide a, PierSide b)
{
    return (a == PIER_SIDE_EAST && b == PIER_SIDE_WEST) ||
        (a == PIER_SIDE_WEST && b == PIER_SIDE_EAST);
}

inline static wxString PierSideStr(PierSide p, const wxString& unknown = _("Unknown"))
{
    switch (p) {
    case PIER_SIDE_EAST: return _("East");
    case PIER_SIDE_WEST: return _("West");
    default:             return unknown;
    }
}

wxString Mount::PierSideStr(PierSide p)
{
    return ::PierSideStr(p);
}

inline static const char *ParityStr(GuideParity par)
{
    switch (par) {
    case GUIDE_PARITY_EVEN:      return "+";
    case GUIDE_PARITY_ODD:       return "-";
    case GUIDE_PARITY_UNKNOWN:   return "?";
    default: case GUIDE_PARITY_UNCHANGED: return "x";
    }
}

inline static GuideParity OppositeParity(GuideParity p)
{
    switch (p) {
    case GUIDE_PARITY_EVEN: return GUIDE_PARITY_ODD;
    case GUIDE_PARITY_ODD:  return GUIDE_PARITY_EVEN;
    default:                return p;
    }
}

wxString Mount::DeclinationStr(double dec, const wxString& numFormatStr)
{
    return dec == UNKNOWN_DECLINATION ? _("Unknown") : wxString::Format(numFormatStr, degrees(dec));
}

static ConfigDialogPane *GetGuideAlgoDialogPane(GuideAlgorithm *algo, wxWindow *parent)
{
    // we need to force the guide alogorithm config pane to be large enough for
    // any of the guide algorithms
    ConfigDialogPane *pane = algo->GetConfigDialogPane(parent);
    pane->SetMinSize(-1, 110);
    return pane;
}

Mount::MountConfigDialogPane::MountConfigDialogPane(wxWindow *pParent, const wxString& title, Mount *pMount)
    : ConfigDialogPane(title, pParent)
{
    // int width;
    m_pMount = pMount;
    m_pParent = pParent;
    m_pAlgoBox = NULL;
    m_pRABox = NULL;
    m_pDecBox = NULL;
}

// Lots of dynamic controls on this pane - keep the creation/management in ConfigDialogPane
void Mount::MountConfigDialogPane::LayoutControls(wxPanel *pParent, BrainCtrlIdMap& CtrlMap)
{
    int width;
    bool stepGuider = false;

    if (m_pMount)
    {
        stepGuider = m_pMount->IsStepGuider();
        m_pAlgoBox = new wxStaticBoxSizer(wxHORIZONTAL, m_pParent, wxEmptyString);
        m_pRABox = new wxStaticBoxSizer(wxVERTICAL, m_pParent, _("Right Ascension"));
        if (m_pDecBox)
            m_pDecBox->Clear(true);
        m_pDecBox = new wxStaticBoxSizer(wxVERTICAL, m_pParent, _("Declination"));
        wxSizerFlags def_flags = wxSizerFlags(0).Border(wxALL, 10).Expand();

        wxString xAlgorithms[] = 
        {
            _("None"), _("Hysteresis"), _("Lowpass"), _("Lowpass2"), _("Resist Switch"),
#if defined(MPIIS_GAUSSIAN_PROCESS_GUIDING_ENABLED__)
            _("Gaussian Process"),
#endif
        };

        width = StringArrayWidth(xAlgorithms, WXSIZEOF(xAlgorithms));
        m_pXGuideAlgorithmChoice = new wxChoice(m_pParent, wxID_ANY, wxPoint(-1, -1),
            wxSize(width + 35, -1), WXSIZEOF(xAlgorithms), xAlgorithms);
        m_pXGuideAlgorithmChoice->SetToolTip(_("Which Guide Algorithm to use for Right Ascension"));

        m_pParent->Connect(m_pXGuideAlgorithmChoice->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
            wxCommandEventHandler(Mount::MountConfigDialogPane::OnXAlgorithmSelected), 0, this);

        if (!m_pMount->m_pXGuideAlgorithm)
        {
            m_pXGuideAlgorithmConfigDialogPane = NULL;
        }
        else
        {
            m_pXGuideAlgorithmConfigDialogPane = GetGuideAlgoDialogPane(m_pMount->m_pXGuideAlgorithm, m_pParent);
        }
        m_pRABox->Add(m_pXGuideAlgorithmChoice, def_flags);
        m_pRABox->Add(m_pXGuideAlgorithmConfigDialogPane, def_flags);

        if (!stepGuider)
            m_pRABox->Add(GetSizerCtrl(CtrlMap, AD_szMaxRAAmt), wxSizerFlags(0).Border(wxTOP, 35).Center());

        // Parameter resets are applicable to either scope or AO "mounts"
        m_pResetRAParams = new wxButton(m_pParent, wxID_ANY, _("Reset"));
        m_pResetRAParams->SetToolTip(_("Causes an IMMEDIATE reset of the RA algorithm parameters to their default values"));
        m_pResetRAParams->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &Mount::MountConfigDialogPane::OnResetRAParams, this);
        if (!stepGuider)
            m_pRABox->Add(m_pResetRAParams, wxSizerFlags(0).Border(wxTOP, 55).Center());
        else
            m_pRABox->Add(m_pResetRAParams, wxSizerFlags(0).Border(wxTOP, 20).Center());

        wxString yAlgorithms[] = 
        {
            _("None"), _("Hysteresis"), _("Lowpass"), _("Lowpass2"), _("Resist Switch"),
#if defined(MPIIS_GAUSSIAN_PROCESS_GUIDING_ENABLED__)
            _("Gaussian Process"),
#endif
        };
        width = StringArrayWidth(yAlgorithms, WXSIZEOF(yAlgorithms));
        m_pYGuideAlgorithmChoice = new wxChoice(m_pParent, wxID_ANY, wxPoint(-1, -1),
            wxSize(width + 35, -1), WXSIZEOF(yAlgorithms), yAlgorithms);
        m_pYGuideAlgorithmChoice->SetToolTip(_("Which Guide Algorithm to use for Declination"));

        m_pParent->Connect(m_pYGuideAlgorithmChoice->GetId(), wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(Mount::MountConfigDialogPane::OnYAlgorithmSelected), 0, this);

        if (!m_pMount->m_pYGuideAlgorithm)
        {
            m_pYGuideAlgorithmConfigDialogPane = NULL;
        }
        else
        {
            m_pYGuideAlgorithmConfigDialogPane = GetGuideAlgoDialogPane(m_pMount->m_pYGuideAlgorithm, m_pParent);
        }
        m_pDecBox->Add(m_pYGuideAlgorithmChoice, def_flags);
        m_pDecBox->Add(m_pYGuideAlgorithmConfigDialogPane, def_flags);

        wxSizerFlags smaller_flags = wxSizerFlags(0).Border(wxLEFT | wxRIGHT, 10).Border(wxTOP, 5).Expand();
        if (!stepGuider)
        {
            wxBoxSizer *pSizer = new wxBoxSizer(wxHORIZONTAL);
            pSizer->Add(GetSingleCtrl(CtrlMap, AD_cbDecComp), wxSizerFlags(0).Border(wxTOP | wxLEFT, 5).Border(wxRIGHT, 20).Expand());
            pSizer->Add(GetSizerCtrl(CtrlMap, AD_szDecCompAmt), wxSizerFlags(0).Border(wxTOP | wxRIGHT, 5).Expand());
            m_pDecBox->Add(pSizer);
            m_pDecBox->Add(GetSizerCtrl(CtrlMap, AD_szMaxDecAmt), wxSizerFlags(0).Border(wxTOP, 10).Center());
            m_pDecBox->Add(GetSizerCtrl(CtrlMap, AD_szDecGuideMode), wxSizerFlags(0).Border(wxTOP, 10).Center());
        }

        m_pResetDecParams = new wxButton(m_pParent, wxID_ANY, _("Reset"));
        m_pResetDecParams->SetToolTip(_("Causes an IMMEDIATE reset of the DEC algorithm parameters to their default values"));
        m_pResetDecParams->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &Mount::MountConfigDialogPane::OnResetDecParams, this);
        m_pDecBox->Add(m_pResetDecParams, wxSizerFlags(0).Border(wxTOP, 20).Center());

        m_pAlgoBox->Add(m_pRABox, def_flags);
        m_pAlgoBox->Add(m_pDecBox, def_flags);
        m_pAlgoBox->Layout();
        this->Add(m_pAlgoBox, def_flags);

        Fit(m_pParent);
    }
}

Mount::MountConfigDialogPane::~MountConfigDialogPane(void)
{
}

void Mount::MountConfigDialogPane::ResetRAGuidingParams()
{
    // Re-initialize the algorithm params
    GuideAlgorithm* currRAAlgo = m_pMount->m_pXGuideAlgorithm;
    currRAAlgo->ResetParams();                  // Default is to remove the keys in the registry
    delete m_pMount->m_pXGuideAlgorithm;        // force creation of a new algo instance
    m_pMount->m_pXGuideAlgorithm = NULL;
    wxCommandEvent dummy;
    OnXAlgorithmSelected(dummy);                // Update the UI
    // Re-initialize any other RA guiding parameters not part of algos
    if (!m_pMount->IsStepGuider())
    {
        ScopeConfigDialogCtrlSet* scopeCtrlSet = (ScopeConfigDialogCtrlSet*) m_pMount->currConfigDialogCtrlSet;
        scopeCtrlSet->ResetRAParameterUI();
    }
}

void Mount::MountConfigDialogPane::OnResetRAParams(wxCommandEvent& evt)
{
    ResetRAGuidingParams();
}

void Mount::MountConfigDialogPane::ResetDecGuidingParams()
{
    // Re-initialize the algorithm params
    GuideAlgorithm* currDecAlgo = m_pMount->m_pYGuideAlgorithm;
    currDecAlgo->ResetParams();
    delete m_pMount->m_pYGuideAlgorithm;
    m_pMount->m_pYGuideAlgorithm = NULL;
    wxCommandEvent dummy;
    OnYAlgorithmSelected(dummy);
    // Re-initialize any other Dec guiding parameters not part of algos
    if (!m_pMount->IsStepGuider())
    {
        ScopeConfigDialogCtrlSet* scopeCtrlSet = (ScopeConfigDialogCtrlSet*)m_pMount->currConfigDialogCtrlSet;
        scopeCtrlSet->ResetDecParameterUI();
    }
}

void Mount::MountConfigDialogPane::OnResetDecParams(wxCommandEvent& evt)
{
    ResetDecGuidingParams();
}

void Mount::MountConfigDialogPane::OnXAlgorithmSelected(wxCommandEvent& evt)
{
    ConfigDialogPane *oldpane = m_pXGuideAlgorithmConfigDialogPane;
    oldpane->Clear(true);
    m_pMount->SetXGuideAlgorithm(m_pXGuideAlgorithmChoice->GetSelection());
    ConfigDialogPane *newpane = GetGuideAlgoDialogPane(m_pMount->m_pXGuideAlgorithm, m_pParent);
    m_pRABox->Replace(oldpane, newpane);
    m_pXGuideAlgorithmConfigDialogPane = newpane;
    m_pXGuideAlgorithmConfigDialogPane->LoadValues();
    m_pRABox->Layout();
    m_pAlgoBox->Layout();
    m_pParent->Layout();
    m_pParent->Update();
    m_pParent->Refresh();
}

void Mount::MountConfigDialogPane::OnYAlgorithmSelected(wxCommandEvent& evt)
{
    ConfigDialogPane *oldpane = m_pYGuideAlgorithmConfigDialogPane;
    oldpane->Clear(true);
    m_pMount->SetYGuideAlgorithm(m_pYGuideAlgorithmChoice->GetSelection());
    ConfigDialogPane *newpane = GetGuideAlgoDialogPane(m_pMount->m_pYGuideAlgorithm, m_pParent);
    m_pDecBox->Replace(oldpane, newpane);
    m_pYGuideAlgorithmConfigDialogPane = newpane;
    m_pYGuideAlgorithmConfigDialogPane->LoadValues();
    m_pDecBox->Layout();
    m_pAlgoBox->Layout();
    m_pParent->Layout();
    m_pParent->Update();
    m_pParent->Refresh();
}

void Mount::MountConfigDialogPane::LoadValues(void)
{
    m_initXGuideAlgorithmSelection = m_pMount->GetXGuideAlgorithmSelection();
    m_pXGuideAlgorithmChoice->SetSelection(m_initXGuideAlgorithmSelection);
    m_pXGuideAlgorithmChoice->Enable(!pFrame->CaptureActive);
    m_initYGuideAlgorithmSelection = m_pMount->GetYGuideAlgorithmSelection();
    m_pYGuideAlgorithmChoice->SetSelection(m_initYGuideAlgorithmSelection);
    m_pYGuideAlgorithmChoice->Enable(!pFrame->CaptureActive);

    if (m_pXGuideAlgorithmConfigDialogPane)
    {
        m_pXGuideAlgorithmConfigDialogPane->LoadValues();
    }

    if (m_pYGuideAlgorithmConfigDialogPane)
    {
        m_pYGuideAlgorithmConfigDialogPane->LoadValues();
    }
}

void Mount::MountConfigDialogPane::UnloadValues(void)
{

    // note these two have to be before the SetXxxAlgorithm calls, because if we
    // changed the algorithm, the current one will get freed, and if we make
    // these two calls after that, bad things happen
    if (m_pXGuideAlgorithmConfigDialogPane)
    {
        m_pXGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    if (m_pYGuideAlgorithmConfigDialogPane)
    {
        m_pYGuideAlgorithmConfigDialogPane->UnloadValues();
    }

    m_pMount->SetXGuideAlgorithm(m_pXGuideAlgorithmChoice->GetSelection());
    m_pMount->SetYGuideAlgorithm(m_pYGuideAlgorithmChoice->GetSelection());
}

// Restore the guide algorithms - all the UI controls will follow correctly if the actual algorithm choices are correct
void Mount::MountConfigDialogPane::Undo(void)
{
    if (pMount)
    {
        if (m_pXGuideAlgorithmConfigDialogPane)
        {
            m_pXGuideAlgorithmConfigDialogPane->Undo();
        }

        if (m_pYGuideAlgorithmConfigDialogPane)
        {
            m_pYGuideAlgorithmConfigDialogPane->Undo();
        }
        m_pMount->SetXGuideAlgorithm(m_initXGuideAlgorithmSelection);
        m_pXGuideAlgorithmChoice->SetSelection(m_initXGuideAlgorithmSelection);
        wxCommandEvent dummy;
        OnXAlgorithmSelected(dummy);
        m_pMount->SetYGuideAlgorithm(m_initYGuideAlgorithmSelection);
        m_pYGuideAlgorithmChoice->SetSelection(m_initYGuideAlgorithmSelection);
        OnYAlgorithmSelected(dummy);
    }
}

MountConfigDialogCtrlSet *Mount::GetConfigDialogCtrlSet(wxWindow *pParent, Mount *pMount, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap)
{
    return new MountConfigDialogCtrlSet(pParent, pMount, pAdvancedDialog, CtrlMap);
}

// These are only controls that are exported to other panes - all the other dynamically updated controls are handled in 
// ConfigDialogPane
MountConfigDialogCtrlSet::MountConfigDialogCtrlSet(wxWindow *pParent, Mount *pMount, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap) :
ConfigDialogCtrlSet(pParent, pAdvancedDialog, CtrlMap)
{
    bool enableCtrls = pMount != NULL;
    m_pMount = pMount;
    if (m_pMount)
    {
        if (!pMount->IsStepGuider())
        {
            m_pClearCalibration = new wxCheckBox(GetParentWindow(AD_cbClearCalibration), wxID_ANY, _("Clear mount calibration"));
            m_pClearCalibration->Enable(enableCtrls);
            AddCtrl(CtrlMap, AD_cbClearCalibration, m_pClearCalibration,
                _("Clear the current mount calibration data - calibration will be re-done when guiding is started"));
            m_pEnableGuide = new wxCheckBox(GetParentWindow(AD_cbEnableGuiding), wxID_ANY, _("Enable mount guide output"));
            AddCtrl(CtrlMap, AD_cbEnableGuiding, m_pEnableGuide,
                _("Keep this checked for guiding. Un-check to disable all mount guide commands and allow the mount to run un-guided"));
        }
    }
}

void MountConfigDialogCtrlSet::LoadValues()
{
    if (m_pMount && !m_pMount->IsStepGuider())
    {
        m_pClearCalibration->Enable(m_pMount->IsCalibrated());
        m_pClearCalibration->SetValue(false);
        m_pEnableGuide->SetValue(m_pMount->GetGuidingEnabled());
    }
}

void MountConfigDialogCtrlSet::UnloadValues()
{
    if (m_pMount && !m_pMount->IsStepGuider())
    {
        if (m_pClearCalibration->IsChecked())
        {
            m_pMount->ClearCalibration();
            Debug.Write(wxString::Format("User cleared %s calibration\n", m_pMount->IsStepGuider() ? "AO" : "Mount"));
        }

        m_pMount->SetGuidingEnabled(m_pEnableGuide->GetValue());
    }
}

GUIDE_ALGORITHM Mount::GetXGuideAlgorithmSelection(void)
{
    return GetGuideAlgorithm(m_pXGuideAlgorithm);
}

void Mount::SetXGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    if (!m_pXGuideAlgorithm || m_pXGuideAlgorithm->Algorithm() != guideAlgorithm)
    {
        delete m_pXGuideAlgorithm;

        if (CreateGuideAlgorithm(guideAlgorithm, this, GUIDE_X, &m_pXGuideAlgorithm))
        {
            CreateGuideAlgorithm(defaultAlgorithm, this, GUIDE_X, &m_pXGuideAlgorithm);
            guideAlgorithm = defaultAlgorithm;
        }

        pConfig->Profile.SetInt("/" + GetMountClassName() + "/XGuideAlgorithm", guideAlgorithm);
    }
}

GUIDE_ALGORITHM Mount::GetYGuideAlgorithmSelection(void)
{
    return GetGuideAlgorithm(m_pYGuideAlgorithm);
}

void Mount::SetYGuideAlgorithm(int guideAlgorithm, GUIDE_ALGORITHM defaultAlgorithm)
{
    if (!m_pYGuideAlgorithm || m_pYGuideAlgorithm->Algorithm() != guideAlgorithm)
    {
        delete m_pYGuideAlgorithm;

        if (CreateGuideAlgorithm(guideAlgorithm, this, GUIDE_Y, &m_pYGuideAlgorithm))
        {
            CreateGuideAlgorithm(defaultAlgorithm, this, GUIDE_Y, &m_pYGuideAlgorithm);
            guideAlgorithm = defaultAlgorithm;
        }

        pConfig->Profile.SetInt("/" + GetMountClassName() + "/YGuideAlgorithm", guideAlgorithm);
    }
}

void Mount::SetGuidingEnabled(bool guidingEnabled)
{
    if (guidingEnabled != m_guidingEnabled)
    {
        const char *s = IsStepGuider() ? "AOGuidingEnabled" : "MountGuidingEnabled";
        Debug.Write(wxString::Format("%s: %d\n", s, guidingEnabled));
        pFrame->NotifyGuidingParam(s, guidingEnabled);
        m_guidingEnabled = guidingEnabled;
    }
}

GUIDE_ALGORITHM Mount::GetGuideAlgorithm(GuideAlgorithm *pAlgorithm)
{
    return pAlgorithm ? pAlgorithm->Algorithm() : GUIDE_ALGORITHM_NONE;
}

bool Mount::CreateGuideAlgorithm(int guideAlgorithm, Mount *mount, GuideAxis axis, GuideAlgorithm** ppAlgorithm)
{
    bool bError = false;

    try
    {
        switch (guideAlgorithm)
        {
            case GUIDE_ALGORITHM_IDENTITY:
            case GUIDE_ALGORITHM_HYSTERESIS:
            case GUIDE_ALGORITHM_LOWPASS:
            case GUIDE_ALGORITHM_LOWPASS2:
            case GUIDE_ALGORITHM_RESIST_SWITCH:
#if defined(MPIIS_GAUSSIAN_PROCESS_GUIDING_ENABLED__)            
            case GUIDE_ALGORITHM_GAUSSIAN_PROCESS:
#endif
                break;
            case GUIDE_ALGORITHM_NONE:
            default:
                throw ERROR_INFO("invalid guideAlgorithm");
                break;
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        guideAlgorithm = GUIDE_ALGORITHM_IDENTITY;
    }

    switch (guideAlgorithm)
    {
        case GUIDE_ALGORITHM_IDENTITY:
            *ppAlgorithm = new GuideAlgorithmIdentity(mount, axis);
            break;
        case GUIDE_ALGORITHM_HYSTERESIS:
            *ppAlgorithm = new GuideAlgorithmHysteresis(mount, axis);
            break;
        case GUIDE_ALGORITHM_LOWPASS:
            *ppAlgorithm = new GuideAlgorithmLowpass(mount, axis);
            break;
        case GUIDE_ALGORITHM_LOWPASS2:
            *ppAlgorithm = new GuideAlgorithmLowpass2(mount, axis);
            break;
        case GUIDE_ALGORITHM_RESIST_SWITCH:
            *ppAlgorithm = new GuideAlgorithmResistSwitch(mount, axis);
            break;
            
#if defined(MPIIS_GAUSSIAN_PROCESS_GUIDING_ENABLED__)            
        case GUIDE_ALGORITHM_GAUSSIAN_PROCESS:
            *ppAlgorithm = new GuideGaussianProcess(mount, axis);
            break;
#endif

        case GUIDE_ALGORITHM_NONE:
        default:
            assert(false);
            break;
    }

    return bError;
}

#ifdef TEST_TRANSFORMS
/*
 * TestTransforms() is a routine to do some sanity checking on the transform routines, which
 * have been a source of a huge number of headaches involving getting the reverse transforms
 * to work
 *
 */
void Mount::TestTransforms(void)
{
    static bool bTested = false;

    if (!bTested)
    {
        for(int inverted=0;inverted<2;inverted++)
        {
            // we test every 15 degrees, starting at -195 degrees and
            // ending at 375 degrees

            for(int i=-13;i<14;i++)
            {
                double xAngle = ((double)i) * M_PI/12.0;
                double yAngle;

                yAngle = xAngle+M_PI/2.0;

                if (inverted)
                {
                    yAngle -= M_PI;
                }

                // normalize the angles since real callers of setCalibration
                // will get the angle from atan2().

                xAngle = atan2(sin(xAngle), cos(xAngle));
                yAngle = atan2(sin(yAngle), cos(yAngle));

                Debug.Write(wxString::Format("xidx=%.2f, yIdx=%.2f\n", xAngle/M_PI*180.0/15, yAngle/M_PI*180.0/15));

                SetCalibration(xAngle, yAngle, 1.0, 1.0);

                for(int j=-13;j<14;j++)
                {
                    double cameraAngle = ((double)i) * M_PI/12.0;
                    cameraAngle = atan2(sin(cameraAngle), cos(cameraAngle));

                    PHD_Point p0(cos(cameraAngle), sin(cameraAngle));
                    assert(fabs((p0.X*p0.X + p0.Y*p0.Y) - 1.00) < 0.01);

                    double p0Angle = p0.Angle();
                    assert(fabs(cameraAngle - p0Angle) < 0.01);

                    PHD_Point p1, p2;

                    if (TransformCameraCoordinatesToMountCoordinates(p0, p1))
                    {
                        assert(false);
                    }

                    assert(fabs((p1.X*p1.X + p1.Y*p1.Y) - 1.00) < 0.01);

                    double adjustedCameraAngle = cameraAngle - xAngle;

                    double cosAdjustedCameraAngle = cos(adjustedCameraAngle);

                    // check that the X value is correct
                    assert(fabs(cosAdjustedCameraAngle - p1.X) < 0.01);

                    double sinAdjustedCameraAngle = sin(adjustedCameraAngle);

                    if (inverted)
                    {
                        sinAdjustedCameraAngle *= -1;
                    }
                    // and check that the Y value is correct;
                    assert(fabs(sinAdjustedCameraAngle - p1.Y) < 0.01);

                    if (TransformMountCoordinatesToCameraCoordinates(p1, p2))
                    {
                        assert(false);
                    }

                    double p2Angle = p2.Angle();
                    //assert(fabs(p0Angle - p2Angle) < 0.01);
                    assert(fabs((p2.X*p2.X + p2.Y*p2.Y) - 1.00) < 0.01);
                    assert(fabs(p0.X - p2.X) < 0.01);
                    assert(fabs(p0.Y - p2.Y) < 0.01);
                }
            }
        }

        bTested = true;
        ClearCalibration();
    }
}
#endif

Mount::Mount(void)
{
    m_connected = false;
    m_requestCount = 0;
    m_errorCount = 0;

    m_pYGuideAlgorithm = NULL;
    m_pXGuideAlgorithm = NULL;
    m_guidingEnabled = true;

    m_backlashComp = NULL;
    m_lastStep.mount = this;
    m_lastStep.frameNumber = -1; // invalidate

    ClearCalibration();

#ifdef TEST_TRANSFORMS
    TestTransforms();
#endif
}

Mount::~Mount()
{
    delete m_pXGuideAlgorithm;
    delete m_pYGuideAlgorithm;
    delete m_backlashComp;
}

double Mount::xRate() const
{
    return m_xRate;
}

double Mount::yRate() const
{
    return m_cal.yRate;
}

double Mount::xAngle() const
{
    return m_cal.xAngle;
}

double Mount::yAngle() const
{
    return norm_angle(m_cal.xAngle - m_yAngleError + M_PI / 2.);
}

static wxString RotAngleStr(double rotAngle)
{
    if (rotAngle == Rotator::POSITION_UNKNOWN)
        return _("None");
    return wxString::Format("%.1f", rotAngle);
}

bool Mount::FlipCalibration(void)
{
    bool bError = false;

    try
    {
        if (!IsCalibrated())
        {
            throw ERROR_INFO("cannot flip if not calibrated");
        }

        double origX = xAngle();
        double origY = yAngle();

        bool decFlipRequired = CalibrationFlipRequiresDecFlip();

        Debug.Write(wxString::Format("FlipCalibration before: x=%.1f, y=%.1f decFlipRequired=%d sideOfPier=%s rotAngle=%s parity=%s/%s\n",
            degrees(origX), degrees(origY), decFlipRequired, ::PierSideStr(m_cal.pierSide), RotAngleStr(m_cal.rotatorAngle),
            ParityStr(m_cal.raGuideParity), ParityStr(m_cal.decGuideParity)));

        double newX = origX + M_PI;
        double newY = origY;

        if (decFlipRequired)
        {
            newY += M_PI;
        }

        Debug.Write(wxString::Format("FlipCalibration pre-normalize: x=%.1f, y=%.1f\n", degrees(newX), degrees(newY)));

        // normalize
        newX = norm_angle(newX);
        newY = norm_angle(newY);

        PierSide priorPierSide = m_cal.pierSide;
        PierSide newPierSide = OppositeSide(m_cal.pierSide);

        // Dec polarity changes when pier side changes, i.e. if Guide(NORTH) moves the star north on one side,
        // then Guide(NORTH) will move the star south on the other side of the pier.
        // For mounts with CalibrationFlipRequiresDecFlip, the parity does not change after the flip.
        GuideParity newDecParity = decFlipRequired ? m_cal.decGuideParity : OppositeParity(m_cal.decGuideParity);

        Debug.Write(wxString::Format("FlipCalibration after: x=%.1f y=%.1f sideOfPier=%s parity=%s/%s\n",
            degrees(newX), degrees(newY), ::PierSideStr(newPierSide), ParityStr(m_cal.raGuideParity), ParityStr(newDecParity)));

        Calibration cal(m_cal);
        cal.xAngle = newX;
        cal.yAngle = newY;
        cal.pierSide = newPierSide;
        cal.decGuideParity = newDecParity;

        SetCalibration(cal);

        pFrame->StatusMsg(wxString::Format(_("CAL: %s(%.f,%.f)->%s(%.f,%.f)"),
            ::PierSideStr(priorPierSide, wxEmptyString), degrees(origX), degrees(origY),
            ::PierSideStr(newPierSide, wxEmptyString), degrees(newX), degrees(newY)));
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Mount::LogGuideStepInfo()
{
    if (m_lastStep.frameNumber < 0)
        return;

    pFrame->UpdateGuiderInfo(m_lastStep);
    GuideLog.GuideStep(m_lastStep);
    EvtServer.NotifyGuideStep(m_lastStep);

    if (m_lastStep.moveType != MOVETYPE_DIRECT)
    {
        pFrame->pGraphLog->AppendData(m_lastStep);
        pFrame->pTarget->AppendData(m_lastStep);
        GuidingAssistant::NotifyGuideStep(m_lastStep);
    }

    m_lastStep.frameNumber = -1; // invalidate
}

Mount::MOVE_RESULT Mount::Move(const PHD_Point& cameraVectorEndpoint, MountMoveType moveType)
{
    MOVE_RESULT result = MOVE_OK;

    try
    {
        double xDistance, yDistance;
        PHD_Point mountVectorEndpoint;

        if (moveType == MOVETYPE_DEDUCED)
        {
            xDistance = m_pXGuideAlgorithm ? m_pXGuideAlgorithm->deduceResult() : 0.0;
            yDistance = m_pYGuideAlgorithm ? m_pYGuideAlgorithm->deduceResult() : 0.0;
            if (xDistance == 0.0 && yDistance == 0.0)
                return result;
            mountVectorEndpoint.X = xDistance;
            mountVectorEndpoint.Y = yDistance;

            Debug.Write(wxString::Format("Dead-reckoning move xDistance=%.2f yDistance=%.2f\n",
                xDistance, yDistance));
        }
        else
        {
            if (TransformCameraCoordinatesToMountCoordinates(cameraVectorEndpoint, mountVectorEndpoint))
            {
                throw ERROR_INFO("Unable to transform camera coordinates");
            }

            xDistance = mountVectorEndpoint.X;
            yDistance = mountVectorEndpoint.Y;

            Debug.Write(wxString::Format("Moving (%.2f, %.2f) raw xDistance=%.2f yDistance=%.2f\n",
                cameraVectorEndpoint.X, cameraVectorEndpoint.Y, xDistance, yDistance));

            if (moveType == MOVETYPE_ALGO)
            {
                // Feed the raw distances to the guide algorithms
                if (m_pXGuideAlgorithm)
                {
                    xDistance = m_pXGuideAlgorithm->result(xDistance);
                }

                // Let BLC track the raw offsets in Dec
                if (m_backlashComp)
                    m_backlashComp->TrackBLCResults(yDistance, m_pYGuideAlgorithm->GetMinMove(), m_cal.yRate);

                if (m_pYGuideAlgorithm)
                {
                    yDistance = m_pYGuideAlgorithm->result(yDistance);
                }
            }
            else
            {
                if (m_backlashComp)
                    m_backlashComp->ResetBaseline();
            }
        }

        // Figure out the guide directions based on the (possibly) updated distances
        GUIDE_DIRECTION xDirection = xDistance > 0.0 ? LEFT : RIGHT;
        GUIDE_DIRECTION yDirection = yDistance > 0.0 ? DOWN : UP;

        int requestedXAmount = (int) floor(fabs(xDistance / m_xRate) + 0.5);
        MoveResultInfo xMoveResult;
        result = Move(xDirection, requestedXAmount, moveType, &xMoveResult);

        MoveResultInfo yMoveResult;
        if (result == MOVE_OK || result == MOVE_ERROR)
        {
            int requestedYAmount = (int) floor(fabs(yDistance / m_cal.yRate) + 0.5);
            if (requestedYAmount > 0 && !IsStepGuider() && moveType != MOVETYPE_DIRECT && GetGuidingEnabled())
            {
                m_backlashComp->ApplyBacklashComp(yDirection, yDistance, &requestedYAmount);
            }
            result = Move(yDirection, requestedYAmount, moveType, &yMoveResult);
        }

        // Record the info about the guide step. The info will be picked up back in the main UI thread.
        // We don't want to do anything with the info here in the worker thread since UI operations are
        // not allowed outside the main UI thread.

        GuideStepInfo& info = m_lastStep;

        info.moveType = moveType;
        info.frameNumber = pFrame->m_frameCounter;
        info.time = pFrame->TimeSinceGuidingStarted();
        info.cameraOffset = cameraVectorEndpoint;
        info.mountOffset = mountVectorEndpoint;
        info.guideDistanceRA = xDistance;
        info.guideDistanceDec = yDistance;
        info.durationRA = xMoveResult.amountMoved;
        info.directionRA = xDirection;
        info.durationDec = yMoveResult.amountMoved;
        info.directionDec = yDirection;
        info.raLimited = xMoveResult.limited;
        info.decLimited = yMoveResult.limited;
        info.aoPos = GetAoPos();
        info.starMass = pFrame->pGuider->StarMass();
        info.starSNR = pFrame->pGuider->SNR();
        info.avgDist = pFrame->pGuider->CurrentError();
        info.starError = pFrame->pGuider->StarError();
    }
    catch (const wxString& errMsg)
    {
        POSSIBLY_UNUSED(errMsg);
        result = MOVE_ERROR;
    }

    return result;
}

/*
 * The transform code has proven really tricky to get right.  For future generations
 * (and for me the next time I try to work on it), I'm going to put some notes here.
 *
 * The goal of TransformCameraCoordinatesToMountCoordinates is to transform a camera
 * pixel coordinate into an x and y, and TransformMountCoordinatesToCameraCoordinates
 * does the reverse, converting a mount x and y into pixels coordinates.
 *
 * Note: If a mount's x and y axis are not perfectly perpendicular, the reverse transform
 * will not be able to accuratey reverse the forward transform. The amount of inaccuracy
 * depends upon the perpendicular error.
 *
 * The mount is calibrated but moving 1 axis, then the other, watching where the mount
 * started and stopped.  Looking at the coordinates of the start and stop point, we
 * can compute the angle and the speed.
 *
 * The original PHD code used cos() against both the angles, but that code had issues
 * with sign reversal for some of the reverse transforms.  I spent some time looking
 * at that code (OK, a lot of time) and I never managed to get it to work, so I
 * rewrote it.
 *
 * After calibration, we use the x axis angle, and compute the y axis error by
 * subtracting 90 degrees.  If the mount was perfectly perpendicular, the error
 * will either be 0 or 180, depending on whether the axis motion was reversed
 * during calibration.
 *
 * I put this quote in when I was trying to fix the cos/cos code.  Hopefully
 * since  completely rewrote that code, I underand the new code. But I'm going
 * to leave it hear as it still seems relevant, given the amount of trouble
 * I had getting this code to work.
 *
 * In the words of Stevie Wonder, noted Computer Scientist and singer/songwriter
 * of some repute:
 *
 * "When you believe in things that you don't understand
 * Then you suffer
 * Superstition ain't the way"
 *
 * As part of trying to get the transforms to work, I wrote a routine called TestTransforms()
 * which does a bunch of forward/reverse trasnform pairs, checking the results as it goes.
 *
 * If you ever have change the transform functions, it would be wise to #define TEST_TRANSFORMS
 * to make sure that you (at least) didn't break anything that TestTransforms() checks for.
 *
 */

bool Mount::TransformCameraCoordinatesToMountCoordinates(const PHD_Point& cameraVectorEndpoint,
                                                         PHD_Point& mountVectorEndpoint)
{
    bool bError = false;

    try
    {
        if (!cameraVectorEndpoint.IsValid())
        {
            throw ERROR_INFO("invalid cameraVectorEndPoint");
        }

        double hyp   = cameraVectorEndpoint.Distance();
        double cameraTheta = cameraVectorEndpoint.Angle();

        double xAngle = cameraTheta - m_cal.xAngle;
        double yAngle = cameraTheta - (m_cal.xAngle + m_yAngleError);

        // Convert theta and hyp into X and Y

        mountVectorEndpoint.SetXY(
            cos(xAngle) * hyp,
            sin(yAngle) * hyp
            );

        Debug.Write(wxString::Format("CameraToMount -- cameraTheta (%.2f) - m_xAngle (%.2f) = xAngle (%.2f = %.2f)\n",
                cameraTheta, m_cal.xAngle, xAngle, norm_angle(xAngle)));
        Debug.Write(wxString::Format("CameraToMount -- cameraTheta (%.2f) - (m_xAngle (%.2f) + m_yAngleError (%.2f)) = yAngle (%.2f = %.2f)\n",
                cameraTheta, m_cal.xAngle, m_yAngleError, yAngle, norm_angle(yAngle)));
        Debug.Write(wxString::Format("CameraToMount -- cameraX=%.2f cameraY=%.2f hyp=%.2f cameraTheta=%.2f mountX=%.2f mountY=%.2f, mountTheta=%.2f\n",
                cameraVectorEndpoint.X, cameraVectorEndpoint.Y, hyp, cameraTheta, mountVectorEndpoint.X, mountVectorEndpoint.Y,
                mountVectorEndpoint.Angle()));
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        mountVectorEndpoint.Invalidate();
    }

    return bError;
}

bool Mount::TransformMountCoordinatesToCameraCoordinates(const PHD_Point& mountVectorEndpoint,
                                                        PHD_Point& cameraVectorEndpoint)
{
    bool bError = false;

    try
    {
        if (!mountVectorEndpoint.IsValid())
        {
            throw ERROR_INFO("invalid mountVectorEndPoint");
        }

        double hyp = mountVectorEndpoint.Distance();
        double mountTheta = mountVectorEndpoint.Angle();

        if (fabs(m_yAngleError) > M_PI / 2.)
        {
            mountTheta = -mountTheta;
        }

        double xAngle = mountTheta + m_cal.xAngle;

        cameraVectorEndpoint.SetXY(
                cos(xAngle) * hyp,
                sin(xAngle) * hyp
                );

        Debug.Write(wxString::Format("MountToCamera -- mountTheta (%.2f) + m_xAngle (%.2f) = xAngle (%.2f = %.2f)\n",
                                     mountTheta, m_cal.xAngle, xAngle, norm_angle(xAngle)));
        Debug.Write(wxString::Format("MountToCamera -- mountX=%.2f mountY=%.2f hyp=%.2f mountTheta=%.2f cameraX=%.2f, cameraY=%.2f cameraTheta=%.2f\n",
                                     mountVectorEndpoint.X, mountVectorEndpoint.Y, hyp, mountTheta, cameraVectorEndpoint.X, cameraVectorEndpoint.Y,
                                     cameraVectorEndpoint.Angle()));
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        cameraVectorEndpoint.Invalidate();
    }

    return bError;
}

GraphControlPane *Mount::GetXGuideAlgorithmControlPane(wxWindow *pParent)
{
    return m_pXGuideAlgorithm->GetGraphControlPane(pParent, _("RA:"));
}

GraphControlPane *Mount::GetYGuideAlgorithmControlPane(wxWindow *pParent)
{
    return m_pYGuideAlgorithm->GetGraphControlPane(pParent, _("DEC:"));
}

GraphControlPane *Mount::GetGraphControlPane(wxWindow *pParent, const wxString& label)
{
    return NULL;
};

bool Mount::DecCompensationEnabled(void) const
{
    return false;
}

/*
 * Adjust the calibration data for the scope's current coordinates.
 *
 * This includes adjusting the xRate to compensate for changes in declination
 * relative to the declination where calibration was done, and possibly flipping
 * the calibration data if the mount is known to be on the other side of the
 * pier from where calibration was done.
 */
void Mount::AdjustCalibrationForScopePointing(void)
{
    double newDeclination = pPointingSource->GetDeclination();
    PierSide newPierSide = pPointingSource->SideOfPier();
    double newRotatorAngle = Rotator::RotatorPosition();
    unsigned short binning = pCamera->Binning;

    Debug.AddLine(wxString::Format("AdjustCalibrationForScopePointing (%s): current dec=%s pierSide=%d, cal dec=%s pierSide=%d rotAngle=%s bin=%hu",
        GetMountClassName(), DeclinationStr(newDeclination), newPierSide, DeclinationStr(m_cal.declination), m_cal.pierSide,
        RotAngleStr(newRotatorAngle), binning));

    // Compensate for binning change. At least one cam driver (ASCOM/Lodestar) can lie about the binning while changing
    // the reported pixel size
    if (fabs(pCamera->GetCameraPixelSize() - GuideCamera::GetProfilePixelSize()) >= 1.0)
    {
        // Punt on this, it's a cockpit error to be changing binning properties outside of the PHD2 UI
        pFrame->Alert(_("Camera pixel size has changed unexpectedly.  Re-calibrate to restore correct guiding."));
        Debug.Write(wxString::Format("Camera pixel size changed from %0.1f to %0.1f\n",
            GuideCamera::GetProfilePixelSize(), pCamera->GetCameraPixelSize()));
        // Update profile value to avoid repetitive alerts
        pCamera->SetCameraPixelSize(pCamera->GetCameraPixelSize());
    }

    if (binning != m_cal.binning)
    {
        Calibration cal(m_cal);

        double adj = (double) m_cal.binning / (double) binning;
        cal.xRate *= adj;
        cal.yRate *= adj;
        cal.binning = binning;

        Debug.Write(wxString::Format("Binning %hu -> %hu, rates (%.3f, %.3f) -> (%.3f, %.3f)\n",
            m_cal.binning, binning, m_cal.xRate * 1000., m_cal.yRate * 1000., cal.xRate * 1000., cal.yRate * 1000.));

        SetCalibration(cal);
        pFrame->HandleBinningChange();
    }

    // compensate RA guide rate for declination if the declination changed and we know both the
    // calibration declination and the current declination

    bool deccomp = false;

    if (newDeclination != m_cal.declination &&
        newDeclination != UNKNOWN_DECLINATION && m_cal.declination != UNKNOWN_DECLINATION)
    {
        // avoid division by zero and gross errors.  If the user didn't calibrate
        // somewhere near the celestial equator, we don't do this
        if (fabs(m_cal.declination) > Scope::DEC_COMP_LIMIT)
        {
            Debug.AddLine("skipping Dec comp: initial calibration too far from equator");
            pFrame->Alert(_("Calibration was too far from equator, recalibration is needed."));

        }
        else if (!DecCompensationEnabled())
        {
            Debug.AddLine("skipping Dec comp: Dec Comp not enabled");
        }
        else
        {
            m_xRate = (m_cal.xRate / cos(m_cal.declination)) * cos(newDeclination);
            deccomp = true;

            Debug.Write(wxString::Format("Dec comp: XRate %.3f -> %.3f for dec %.1f -> dec %.1f\n",
                                         m_cal.xRate * 1000.0, m_xRate * 1000.0, degrees(m_cal.declination), degrees(newDeclination)));
        }
    }
    if (!deccomp && m_xRate != m_cal.xRate)
    {
        Debug.Write(wxString::Format("No dec comp, using base xRate %.3f\n", m_cal.xRate * 1000.0));
        m_xRate  = m_cal.xRate;
    }

    if (IsOppositeSide(newPierSide, m_cal.pierSide))
    {
        Debug.AddLine(wxString::Format("Guiding starts on opposite side of pier: calibration data "
            "side is %s, current side is %s", ::PierSideStr(m_cal.pierSide), ::PierSideStr(newPierSide)));
        FlipCalibration();
    }

    if (newRotatorAngle != Rotator::POSITION_UNKNOWN)
    {
        if (m_cal.rotatorAngle == Rotator::POSITION_UNKNOWN)
        {
            // we do not know the rotator position at calibration time so cannot automatically adjust calibration
            pFrame->Alert(_("Rotator position has changed, recalibration is needed."));
            m_cal.rotatorAngle = newRotatorAngle;
        }
        else
        {
            double da = newRotatorAngle - m_cal.rotatorAngle;

            if (fabs(da) > 0.05)
            {
                Debug.Write(wxString::Format("New rotator position %.1f deg, prev = %.1f deg, delta = %.1f deg\n", newRotatorAngle, m_cal.rotatorAngle, da));

                da = radians(da);

                Calibration cal(m_cal);
                cal.xAngle = norm_angle(cal.xAngle - da);
                cal.yAngle = norm_angle(cal.yAngle - da);
                cal.rotatorAngle = newRotatorAngle;

                SetCalibration(cal);
            }
        }
    }
}

void Mount::IncrementRequestCount(void)
{
    m_requestCount++;

    // for the moment we never enqueue requests if the mount is busy, but we can
    // enqueue them two at a time.  There is no reason we can't, it's just that
    // right now we don't, and this might catch an error
    assert(m_requestCount <= 2);
}

void Mount::DecrementRequestCount(void)
{
    assert(m_requestCount > 0);
    m_requestCount--;
}

bool Mount::HasNonGuiMove(void)
{
    return false;
}

bool Mount::SynchronousOnly(void)
{
    return false;
}

bool Mount::HasSetupDialog(void) const
{
    return false;
}

void Mount::SetupDialog(void)
{
}

const wxString& Mount::Name(void) const
{
    return m_Name;
}

bool Mount::IsStepGuider(void) const
{
    return false;
}

wxPoint Mount::GetAoPos(void) const
{
    return wxPoint();
}

wxPoint Mount::GetAoMaxPos(void) const
{
    return wxPoint();
}

const char *Mount::DirectionStr(GUIDE_DIRECTION d)
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "None";
    case NORTH: return "North";
    case SOUTH: return "South";
    case EAST:  return "East";
    case WEST:  return "West";
    default:    return "?";
    }
}

const char *Mount::DirectionChar(GUIDE_DIRECTION d)
{
    // these are used internally in the guide log and event server and are not translated
    switch (d) {
    case NONE:  return "-";
    case NORTH: return "N";
    case SOUTH: return "S";
    case EAST:  return "E";
    case WEST:  return "W";
    default:    return "?";
    }
}

bool Mount::IsCalibrated()
{
    bool bReturn = false;

    if (IsConnected())
    {
        bReturn = m_calibrated;
    }

    return bReturn;
}

void Mount::ClearCalibration(void)
{
    m_calibrated = false;
    if (pFrame) pFrame->UpdateCalibrationStatus();
}

void Mount::SetCalibration(const Calibration& cal)
{
    Debug.Write(wxString::Format("Mount::SetCalibration (%s) -- xAngle=%.1f yAngle=%.1f xRate=%.3f yRate=%.3f bin=%hu dec=%s pierSide=%d par=%s/%s rotAng=%s\n",
        GetMountClassName(), degrees(cal.xAngle), degrees(cal.yAngle), cal.xRate * 1000.0, cal.yRate * 1000.0, cal.binning,
        DeclinationStr(cal.declination), cal.pierSide, ParityStr(cal.raGuideParity), ParityStr(cal.decGuideParity),
        RotAngleStr(cal.rotatorAngle)));

    // we do the rates first, since they just get stored
    m_cal.xRate = cal.xRate;
    m_cal.yRate = cal.yRate;
    m_cal.binning = cal.binning;
    m_cal.declination = cal.declination;
    m_cal.pierSide = cal.pierSide;
    if (cal.raGuideParity != GUIDE_PARITY_UNCHANGED)
        m_cal.raGuideParity = cal.raGuideParity;
    if (cal.decGuideParity != GUIDE_PARITY_UNCHANGED)
        m_cal.decGuideParity = cal.decGuideParity;
    m_cal.rotatorAngle = cal.rotatorAngle;
    m_cal.isValid = true;

    m_xRate  = cal.xRate;

    // the angles are more difficult because we have to turn yAngle into a yError.
    m_cal.xAngle = cal.xAngle;
    m_cal.yAngle = cal.yAngle;
    m_yAngleError = norm_angle(cal.xAngle - cal.yAngle + M_PI / 2.);

    Debug.AddLine(wxString::Format("Mount::SetCalibration (%s) -- sets m_xAngle=%.1f m_yAngleError=%.1f",
        GetMountClassName(), degrees(m_cal.xAngle), degrees(m_yAngleError)));

    m_calibrated = true;

    if (pFrame) pFrame->UpdateCalibrationStatus();

    // store calibration data
    wxString prefix = "/" + GetMountClassName() + "/calibration/";
    pConfig->Profile.SetString(prefix + "timestamp", wxDateTime::Now().Format());
    pConfig->Profile.SetDouble(prefix + "xAngle", m_cal.xAngle);
    pConfig->Profile.SetDouble(prefix + "yAngle", m_cal.yAngle);
    pConfig->Profile.SetDouble(prefix + "xRate", m_cal.xRate);
    pConfig->Profile.SetDouble(prefix + "yRate", m_cal.yRate);
    pConfig->Profile.SetInt(prefix + "binning", m_cal.binning);
    pConfig->Profile.SetDouble(prefix + "declination", m_cal.declination);
    pConfig->Profile.SetInt(prefix + "pierSide", m_cal.pierSide);
    pConfig->Profile.SetInt(prefix + "raGuideParity", m_cal.raGuideParity);
    pConfig->Profile.SetInt(prefix + "decGuideParity", m_cal.decGuideParity);
    pConfig->Profile.SetDouble(prefix + "rotatorAngle", m_cal.rotatorAngle);
}

void Mount::SetCalibrationDetails(const CalibrationDetails& calDetails)
{
    wxString prefix = "/" + GetMountClassName() + "/calibration/";
    wxString stepStr = "";

    pConfig->Profile.SetInt(prefix + "focal_length", calDetails.focalLength);
    pConfig->Profile.SetDouble(prefix + "image_scale", calDetails.imageScale);
    pConfig->Profile.SetDouble(prefix + "ra_guide_rate", calDetails.raGuideSpeed);
    pConfig->Profile.SetDouble(prefix + "dec_guide_rate", calDetails.decGuideSpeed);
    pConfig->Profile.SetDouble(prefix + "ortho_error", calDetails.orthoError);
    pConfig->Profile.SetDouble(prefix + "orig_binning", calDetails.origBinning);
    pConfig->Profile.SetString(prefix + "orig_timestamp", calDetails.origTimestamp);

    for (std::vector<wxRealPoint>::const_iterator it = calDetails.raSteps.begin(); it != calDetails.raSteps.end(); ++it)
    {
        stepStr += wxString::Format("{%0.1f %0.1f}, ", it->x, it->y);
    }
    stepStr = stepStr.Left(stepStr.length() - 2);
    pConfig->Profile.SetString(prefix + "ra_steps", stepStr);

    stepStr = "";

    for (std::vector<wxRealPoint>::const_iterator it = calDetails.decSteps.begin(); it != calDetails.decSteps.end(); ++it)
    {
        stepStr += wxString::Format("{%0.1f %0.1f}, ", it->x, it->y);
    }
    stepStr = stepStr.Left(stepStr.length() - 2);
    pConfig->Profile.SetString(prefix + "dec_steps", stepStr);

    pConfig->Profile.SetInt(prefix + "ra_step_count", calDetails.raStepCount);
    pConfig->Profile.SetInt(prefix + "dec_step_count", calDetails.decStepCount);
    pConfig->Profile.SetInt(prefix + "last_issue", (int)calDetails.lastIssue);
}

inline static PierSide pier_side(int val)
{
    return val == PIER_SIDE_EAST ? PIER_SIDE_EAST : val == PIER_SIDE_WEST ? PIER_SIDE_WEST : PIER_SIDE_UNKNOWN;
}

inline static GuideParity guide_parity(int val)
{
    switch (val) {
    case GUIDE_PARITY_EVEN: return GUIDE_PARITY_EVEN;
    case GUIDE_PARITY_ODD: return GUIDE_PARITY_ODD;
    default: return GUIDE_PARITY_UNKNOWN;
    }
}

void Mount::NotifyGuidingStopped(void)
{
    Debug.Write("Mount: notify guiding stopped\n");

    if (m_pXGuideAlgorithm)
        m_pXGuideAlgorithm->GuidingStopped();

    if (m_pYGuideAlgorithm)
        m_pYGuideAlgorithm->GuidingStopped();

    if (m_backlashComp)
        m_backlashComp->ResetBaseline();
}

void Mount::NotifyGuidingPaused(void)
{
    Debug.Write("Mount: notify guiding paused\n");

    if (m_pXGuideAlgorithm)
        m_pXGuideAlgorithm->GuidingPaused();

    if (m_pYGuideAlgorithm)
        m_pYGuideAlgorithm->GuidingPaused();
}

void Mount::NotifyGuidingResumed(void)
{
    Debug.Write("Mount: notify guiding resumed\n");

    if (m_pXGuideAlgorithm)
        m_pXGuideAlgorithm->GuidingResumed();

    if (m_pYGuideAlgorithm)
        m_pYGuideAlgorithm->GuidingResumed();
}

void Mount::NotifyGuidingDithered(double dx, double dy)
{
    Debug.Write(wxString::Format("Mount: notify guiding dithered (%.1f, %.1f)\n", dx, dy));

    if (m_pXGuideAlgorithm)
        m_pXGuideAlgorithm->GuidingDithered(dx);

    if (m_pYGuideAlgorithm)
        m_pYGuideAlgorithm->GuidingDithered(dy);
}

void Mount::NotifyGuidingDitherSettleDone(bool success)
{
    Debug.Write(wxString::Format("Mount: notify guiding dither settle done success=%d\n", success));
    if (m_pXGuideAlgorithm)
        m_pXGuideAlgorithm->GuidingDitherSettleDone(success);
    if (m_pYGuideAlgorithm)
        m_pYGuideAlgorithm->GuidingDitherSettleDone(success);
}

void Mount::GetLastCalibration(Calibration *cal)
{
    wxString prefix = "/" + GetMountClassName() + "/calibration/";
    wxString sTimestamp = pConfig->Profile.GetString(prefix + "timestamp", wxEmptyString);

    if (sTimestamp.Length() > 0)
    {
        cal->xRate = pConfig->Profile.GetDouble(prefix + "xRate", 1.0);
        cal->yRate = pConfig->Profile.GetDouble(prefix + "yRate", 1.0);
        cal->binning = (unsigned short) pConfig->Profile.GetInt(prefix + "binning", 1);
        cal->xAngle = pConfig->Profile.GetDouble(prefix + "xAngle", 0.0);
        cal->yAngle = pConfig->Profile.GetDouble(prefix + "yAngle", 0.0);
        cal->declination = pConfig->Profile.GetDouble(prefix + "declination", 0.0);
        cal->pierSide = pier_side(pConfig->Profile.GetInt(prefix + "pierSide", PIER_SIDE_UNKNOWN));
        cal->raGuideParity = guide_parity(pConfig->Profile.GetInt(prefix + "raGuideParity", GUIDE_PARITY_UNKNOWN));
        cal->decGuideParity = guide_parity(pConfig->Profile.GetInt(prefix + "decGuideParity", GUIDE_PARITY_UNKNOWN));
        cal->rotatorAngle = pConfig->Profile.GetDouble(prefix + "rotatorAngle", Rotator::POSITION_UNKNOWN);
        cal->timestamp = sTimestamp;
        cal->isValid = true;
    }
    else
    {
        cal->isValid = false;
    }
}

void Mount::GetCalibrationDetails(CalibrationDetails *details)
{
    wxStringTokenizer tok;
    wxString prefix = "/" + GetMountClassName() + "/calibration/";
    wxString stepStr;
    bool err = false;

    details->focalLength = pConfig->Profile.GetInt(prefix + "focal_length", 0);
    details->imageScale = pConfig->Profile.GetDouble(prefix + "image_scale", 1.0);
    details->raGuideSpeed = pConfig->Profile.GetDouble(prefix + "ra_guide_rate", -1.0);
    details->decGuideSpeed = pConfig->Profile.GetDouble(prefix + "dec_guide_rate", -1.0);
    details->orthoError = pConfig->Profile.GetDouble(prefix + "ortho_error", 0.0);
    details->raStepCount = pConfig->Profile.GetInt(prefix + "ra_step_count", 0);
    details->decStepCount = pConfig->Profile.GetInt(prefix + "dec_step_count", 0);
    details->origBinning = pConfig->Profile.GetDouble(prefix + "orig_binning", 1.0);
    details->lastIssue = (CalibrationIssueType) pConfig->Profile.GetInt(prefix + "last_issue", 0);
    details->origTimestamp = pConfig->Profile.GetString(prefix + "orig_timestamp", "Unknown");
    // Populate raSteps
    stepStr = pConfig->Profile.GetString(prefix + "ra_steps", "");
    tok.SetString(stepStr, "},", wxTOKEN_STRTOK);
    details->raSteps.clear();
    while (tok.HasMoreTokens() && !err)
    {
        wxString tk = (tok.GetNextToken()).Trim(false);           // looks like {x y, left-trimmed
        int blankLoc = tk.find(" ");
        double x;
        double y;
        if (!tk.Mid(1, blankLoc-1).ToDouble(&x))
            err = true;
        tk = tk.Mid(blankLoc + 1);
        if (!tk.ToDouble(&y))
            err = true;
        if (!err)
            details->raSteps.push_back(wxRealPoint(x, y));
    }
    // Do the same for decSteps
    stepStr = pConfig->Profile.GetString(prefix + "dec_steps", "");
    tok.SetString(stepStr, "},", wxTOKEN_STRTOK);
    details->decSteps.clear();
    while (tok.HasMoreTokens() && !err)
    {
        wxString tk = (tok.GetNextToken()).Trim(false);
        int blankLoc = tk.find(" ");
        double x;
        double y;
        if (!tk.Mid(1, blankLoc - 1).ToDouble(&x))
            err = true;
        tk = tk.Mid(blankLoc + 1);
        if (!tk.ToDouble(&y))
            err = true;
        if (!err)
            details->decSteps.push_back(wxRealPoint(x, y));
    }
}

bool Mount::Connect(void)
{
    m_connected = true;
    ResetErrorCount();

    if (pFrame)
    {
        pFrame->UpdateCalibrationStatus();
    }

    return false;
}

bool Mount::Disconnect(void)
{
    m_connected = false;
    if (pFrame) pFrame->UpdateCalibrationStatus();

    return false;
}

wxString Mount::GetSettingsSummary()
{
    // return a loggable summary of current mount settings
    wxString algorithms[] = {
        _T("None"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
    };
    wxString auxMountStr = wxEmptyString;
    if (m_Name == _("On Camera") && pPointingSource && pPointingSource->IsConnected() && pPointingSource->CanReportPosition())
        auxMountStr = "AuxMount=" +  pPointingSource->Name();
    wxString s = wxString::Format("%s = %s,%s connected, guiding %s, %s, %s\n",
        IsStepGuider() ? "AO" : "Mount",
        m_Name,
        IsConnected() ? " " : " not",
        m_guidingEnabled ? "enabled" : "disabled",
        IsCalibrated() ?
            wxString::Format("xAngle = %.1f, xRate = %.3f, yAngle = %.1f, yRate = %.3f, parity = %s/%s",
                degrees(xAngle()), xRate() * 1000.0, degrees(yAngle()), yRate() * 1000.0,
                ParityStr(m_cal.raGuideParity), ParityStr(m_cal.decGuideParity)) :
            "not calibrated",
        auxMountStr
    ) + wxString::Format("X guide algorithm = %s, %s",
        algorithms[GetXGuideAlgorithmSelection()],
        m_pXGuideAlgorithm->GetSettingsSummary()
    ) + wxString::Format("Y guide algorithm = %s, %s",
        algorithms[GetYGuideAlgorithmSelection()],
        m_pYGuideAlgorithm->GetSettingsSummary()
    );

    if (m_backlashComp)
    {
        s += wxString::Format("Backlash comp = %s, pulse = %d ms\n",
            m_backlashComp->IsEnabled() ? "enabled" : "disabled",
            m_backlashComp->GetBacklashPulse());
    }

    return s;
}

bool Mount::CalibrationFlipRequiresDecFlip(void)
{
    return false;
}

void Mount::StartDecDrift(void)
{
}

void Mount::EndDecDrift(void)
{
}

bool Mount::IsDecDrifting(void) const
{
    return false;
}
