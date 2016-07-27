/*
 *  guider_onestar.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Refactored by Bret McKee
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

#ifndef GUIDER_ONESTAR_H_INCLUDED
#define GUIDER_ONESTAR_H_INCLUDED

class MassChecker;
class GuiderOneStar;
class GuiderConfigDialogCtrlSet;

class GuiderOneStarConfigDialogCtrlSet : public GuiderConfigDialogCtrlSet
{

public:
    GuiderOneStarConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~GuiderOneStarConfigDialogCtrlSet();

    GuiderOneStar *m_pGuiderOneStar;
    wxSpinCtrl *m_pSearchRegion;
    wxCheckBox *m_pEnableStarMassChangeThresh;
    wxSpinCtrlDouble *m_pMassChangeThreshold;

    virtual void LoadValues(void);
    virtual void UnloadValues(void);
    void OnStarMassEnableChecked(wxCommandEvent& event);
};

class GuiderOneStar : public Guider
{
private:
    Star m_star;
    MassChecker *m_massChecker;

    // parameters
    bool m_massChangeThresholdEnabled;
    double m_massChangeThreshold;

public:
    class GuiderOneStarConfigDialogPane : public GuiderConfigDialogPane
    {
    protected:

        public:
        GuiderOneStarConfigDialogPane(wxWindow *pParent, GuiderOneStar *pGuider);
        ~GuiderOneStarConfigDialogPane(void) {};

        virtual void LoadValues(void) {};
        virtual void UnloadValues(void) {};
        void LayoutControls(Guider *pGuider, BrainCtrlIdMap& CtrlMap);
    };

    bool GetMassChangeThresholdEnabled(void);
    void SetMassChangeThresholdEnabled(bool enable);
    double GetMassChangeThreshold(void);
    bool SetMassChangeThreshold(double starMassChangeThreshold);
    bool SetSearchRegion(int searchRegion);

    friend class GuiderOneStarConfigDialogPane;
    friend class GuiderOneStarConfigDialogCtrlSet;

public:
    GuiderOneStar(wxWindow *parent);
    virtual ~GuiderOneStar(void);

    void OnPaint(wxPaintEvent& evt);

    bool IsLocked(void);
    bool AutoSelect(void);
    const PHD_Point& CurrentPosition(void);
    wxRect GetBoundingBox(void);
    int GetMaxMovePixels(void);
    double StarMass(void);
    unsigned int StarPeakADU(void);
    double SNR(void);
    double HFD(void);
    int StarError(void);
    wxString GetSettingsSummary();

    Guider::GuiderConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    GuiderConfigDialogCtrlSet *GetConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);

    void LoadProfileSettings(void);

private:
    bool IsValidLockPosition(const PHD_Point& pt);
    void InvalidateCurrentPosition(bool fullReset = false);
    bool UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo);
    bool SetCurrentPosition(usImage *pImage, const PHD_Point& position);

    void OnLClick(wxMouseEvent& evt);

    void SaveStarFITS();

    DECLARE_EVENT_TABLE()
};

#endif /* GUIDER_ONESTAR_H_INCLUDED */
