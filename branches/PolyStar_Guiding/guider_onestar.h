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

class GuiderOneStar : public Guider
{
private:
    Star m_star;
    MassChecker *m_massChecker;

    // parameters
    bool m_massChangeThresholdEnabled;
    double m_massChangeThreshold;
    int m_searchRegion; // how far u/d/l/r do we do the initial search for a star

protected:
    class GuiderOneStarConfigDialogPane : public GuiderConfigDialogPane
    {
        GuiderOneStar *m_pGuiderOneStar;
        wxSpinCtrl *m_pSearchRegion;
        wxCheckBox *m_pEnableStarMassChangeThresh;
        wxSpinCtrlDouble *m_pMassChangeThreshold;

        public:
        GuiderOneStarConfigDialogPane(wxWindow *pParent, GuiderOneStar *pGuider);
        ~GuiderOneStarConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);

        void OnStarMassEnableChecked(wxCommandEvent& event);
    };

    virtual bool GetMassChangeThresholdEnabled(void);
    virtual void SetMassChangeThresholdEnabled(bool enable);
    virtual double GetMassChangeThreshold(void);
    virtual bool SetMassChangeThreshold(double starMassChangeThreshold);
    virtual int GetSearchRegion(void);
    virtual bool SetSearchRegion(int searchRegion);

    friend class GuiderOneStarConfigDialogPane;

public:
    GuiderOneStar(wxWindow *parent);
    virtual ~GuiderOneStar(void);

    virtual void OnPaint(wxPaintEvent& evt);

    virtual bool IsLocked(void);
    virtual bool AutoSelect(void);
    virtual const PHD_Point& CurrentPosition(void);
    virtual wxRect GetBoundingBox(void);
    virtual int GetMaxMovePixels(void);
    virtual double StarMass(void);
    virtual double SNR(void);
    virtual int StarError(void);
    virtual wxString GetSettingsSummary();

    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);

    virtual void LoadProfileSettings(void);

private:
    virtual bool IsValidLockPosition(const PHD_Point& pt);
    virtual void InvalidateCurrentPosition(bool fullReset = false);
    virtual bool UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo);
    virtual bool SetCurrentPosition(usImage *pImage, const PHD_Point& position);

    void OnLClick(wxMouseEvent& evt);

    void SaveStarFITS();

    DECLARE_EVENT_TABLE()
};

#endif /* GUIDER_ONESTAR_H_INCLUDED */
