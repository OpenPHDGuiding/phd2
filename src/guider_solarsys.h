/*
 *  guider_solarsys.h
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

#ifndef GUIDER_SOLARSYS_H_INCLUDED
#define GUIDER_SOLARSYS_H_INCLUDED

class GuiderSolarSys;
class GuiderConfigDialogCtrlSet;

class GuiderSolarSysConfigDialogCtrlSet : public GuiderConfigDialogCtrlSet
{
public:
    GuiderSolarSysConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog,
                                      BrainCtrlIdMap& CtrlMap);
    virtual ~GuiderSolarSysConfigDialogCtrlSet();

    GuiderSolarSys *m_pGuiderSolarSys;
    virtual void LoadValues();
    virtual void UnloadValues();
};

class GuiderSolarSys : public Guider
{
    Star m_primaryStar;
    bool m_lockPositionMoved;
    unsigned int m_starsUsed;
    wxWindow *m_ImgDisplayWindow;

public:
    class GuiderSolarSysConfigDialogPane : public GuiderConfigDialogPane
    {
    protected:
    public:
        GuiderSolarSysConfigDialogPane(wxWindow *pParent, GuiderSolarSys *pGuider);
        ~GuiderSolarSysConfigDialogPane() {};

        virtual void LoadValues() {};
        virtual void UnloadValues() {};
        void LayoutControls(Guider *pGuider, BrainCtrlIdMap& CtrlMap);
    };

    bool SetTolerateJumps(bool enable, double threshold);

    friend class GuiderSolarSysConfigDialogPane;
    friend class GuiderSolarSysConfigDialogCtrlSet;

public:
    GuiderSolarSys(wxWindow *parent);

    void OnPaint(wxPaintEvent& evt) override;
    void SetImageDisplayWindow(wxWindow *DispWin);
    bool PaintHelper(wxAutoBufferedPaintDCBase& dc, wxMemoryDC& memDC) override;

    virtual bool SetLockPosition(const PHD_Point& position) override;
    bool IsLocked() const override;
    bool AutoSelect(const wxRect& roi) override;
    const PHD_Point& CurrentPosition() const override;
    wxRect GetBoundingBox() const override;
    int GetMaxMovePixels() const override;
    const Star& PrimaryStar() const override;
    bool GetMultiStarMode() const override;
    wxString GetStarCount() const override;
    wxString GetSettingsSummary() const override;

    Guider::GuiderConfigDialogPane *GetConfigDialogPane(wxWindow *pParent) override;
    GuiderConfigDialogCtrlSet *GetConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog,
                                                      BrainCtrlIdMap& CtrlMap) override;
    void LoadProfileSettings() override;

private:
    bool IsValidLockPosition(const PHD_Point& pt) final;
    bool IsValidSecondaryStarPosition(const PHD_Point& pt) final;
    void InvalidateCurrentPosition(bool fullReset = false) final;
    bool UpdateCurrentPosition(const usImage *pImage, GuiderOffset *ofs, FrameDroppedInfo *errorInfo) final;
    bool SetCurrentPosition(const usImage *pImage, const PHD_Point& position) final;
    void OnLClick(wxMouseEvent& evt);

    void SaveStarFITS();

    wxDECLARE_EVENT_TABLE();
};

inline int GuiderSolarSys::GetMaxMovePixels() const
{
    return m_searchRegion;
}

inline const Star& GuiderSolarSys::PrimaryStar() const
{
    return m_primaryStar;
}

inline bool GuiderSolarSys::GetMultiStarMode() const
{
    return false;
}

inline bool GuiderSolarSys::IsLocked() const
{
    return m_primaryStar.WasFound();
}

inline const PHD_Point& GuiderSolarSys::CurrentPosition() const
{
    return m_primaryStar;
}

#endif /* GUIDER_SOLARSYS_H_INCLUDED */