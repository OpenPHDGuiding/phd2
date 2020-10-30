/*
 *  guider_multistar.h
 *  PHD Guiding
 *
 *  Original guider_onestar Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  guider_onestar completely refactored by Bret McKee
 *  Copyright (c) 2012 Bret McKee
 *  All rights reserved.
 *
 *  guider_multistar extensions created by Bruce Waddington
 *  Copyright (c) 2020 Bruce Waddington
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

#ifndef GUIDER_MULTISTAR_H_INCLUDED
#define GUIDER_MULTISTAR_H_INCLUDED

class MassChecker;
class GuiderMultiStar;
class GuiderConfigDialogCtrlSet;

class GuiderMultiStarConfigDialogCtrlSet : public GuiderConfigDialogCtrlSet
{
public:
    GuiderMultiStarConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~GuiderMultiStarConfigDialogCtrlSet();

    GuiderMultiStar *m_pGuiderMultiStar;
    wxSpinCtrl *m_pSearchRegion;
    wxCheckBox *m_pEnableStarMassChangeThresh;
    wxSpinCtrlDouble *m_pMassChangeThreshold;
    wxSpinCtrlDouble *m_MinHFD;
    wxChoice *m_autoSelDownsample;
    wxCheckBox *m_pBeepForLostStarCtrl;
    wxCheckBox *m_pUseMultiStars;
    wxSpinCtrlDouble *m_MinSNR;

    virtual void LoadValues();
    virtual void UnloadValues();
    void OnStarMassEnableChecked(wxCommandEvent& event);
    void OnMultiStarChecked(wxCommandEvent& evt);
};

class GuiderMultiStar : public Guider
{
private:
    Star m_primaryStar;
    std::vector<GuideStar> m_guideStars;
    DescriptiveStats *m_primaryDistStats;
    MassChecker *m_massChecker;
    double m_lastPrimaryDistance;
    bool m_multiStarMode;
    bool m_stabilizing;
    bool m_lockPositionMoved;
    int m_starsUsed;

    // parameters
    bool m_massChangeThresholdEnabled;
    double m_massChangeThreshold;
    bool m_tolerateJumpsEnabled;
    double m_tolerateJumpsThreshold;
    int m_maxStars;
    double m_stabilitySigmaX;

public:
    class GuiderMultiStarConfigDialogPane : public GuiderConfigDialogPane
    {
    protected:

        public:
        GuiderMultiStarConfigDialogPane(wxWindow *pParent, GuiderMultiStar *pGuider);
        ~GuiderMultiStarConfigDialogPane() {};

        virtual void LoadValues() {};
        virtual void UnloadValues() {};
        void LayoutControls(Guider *pGuider, BrainCtrlIdMap& CtrlMap);
    };

    bool GetMassChangeThresholdEnabled() const;
    void SetMassChangeThresholdEnabled(bool enable);
    double GetMassChangeThreshold() const;
    bool SetMassChangeThreshold(double starMassChangeThreshold);
    bool SetTolerateJumps(bool enable, double threshold);
    bool SetSearchRegion(int searchRegion);
    void RefineOffset(const usImage *pImage, GuiderOffset* pOffset);

    friend class GuiderMultiStarConfigDialogPane;
    friend class GuiderMultiStarConfigDialogCtrlSet;

public:
    GuiderMultiStar(wxWindow *parent);
    virtual ~GuiderMultiStar();

    void OnPaint(wxPaintEvent& evt) override;

    virtual bool SetLockPosition(const PHD_Point& position) override;
    bool IsLocked() override;
    bool AutoSelect(const wxRect& roi) override;
    const PHD_Point& CurrentPosition() override;
    wxRect GetBoundingBox() override;
    int GetMaxMovePixels() override;
    double StarMass() override;
    unsigned int StarPeakADU() override;
    double SNR() override;
    double HFD() override;
    int StarError() override;
    bool GetMultiStarMode() override;
    wxString GetStarCount() override;
    void SetMultiStarMode(bool val) override;
    wxString GetSettingsSummary() const override;

    Guider::GuiderConfigDialogPane *GetConfigDialogPane(wxWindow *pParent) override;
    GuiderConfigDialogCtrlSet *GetConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap) override;

    void LoadProfileSettings() override;

private:
    bool IsValidLockPosition(const PHD_Point& pt) final;
    void InvalidateCurrentPosition(bool fullReset = false) final;
    bool UpdateCurrentPosition(const usImage *pImage, GuiderOffset *ofs, FrameDroppedInfo *errorInfo) final;
    bool SetCurrentPosition(const usImage *pImage, const PHD_Point& position) final;

    void OnLClick(wxMouseEvent& evt);

    void SaveStarFITS();

    DECLARE_EVENT_TABLE()
};

#endif /* GUIDER_MULTISTAR_H_INCLUDED */
