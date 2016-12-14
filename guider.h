/*
 *  guider.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
 *  All rights reserved.
 *
 *  Based upon work by Craig Stark.
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

#ifndef GUIDER_H_INCLUDED
#define GUIDER_H_INCLUDED

enum GUIDER_STATE
{
    STATE_UNINITIALIZED = 0,
    STATE_SELECTING,
    STATE_SELECTED,
    STATE_CALIBRATING_PRIMARY,
    STATE_CALIBRATING_SECONDARY,
    STATE_CALIBRATED,
    STATE_GUIDING,
    STATE_STOP, // This is a pseudo state
};

enum EXPOSED_STATE
{
    EXPOSED_STATE_NONE = 0,
    EXPOSED_STATE_SELECTED,
    EXPOSED_STATE_CALIBRATING,
    EXPOSED_STATE_GUIDING_LOCKED,
    EXPOSED_STATE_GUIDING_LOST,

    EXPOSED_STATE_PAUSED = 100,
    EXPOSED_STATE_LOOPING,
};

enum DEC_GUIDE_MODE
{
    DEC_NONE = 0,
    DEC_AUTO,
    DEC_NORTH,
    DEC_SOUTH
};

enum DEC_GUDING_ALGORITHM
{
    DEC_LOWPASS = 0,
    DEC_RESISTSWITCH,
    DEC_LOWPASS2,
};

enum OVERLAY_MODE
{
    OVERLAY_NONE = 0,
    OVERLAY_BULLSEYE,
    OVERLAY_GRID_FINE,
    OVERLAY_GRID_COARSE,
    OVERLAY_RADEC,
    OVERLAY_SLIT,
};

struct OverlaySlitCoords
{
    wxPoint center;
    wxSize size;
    int angle;
    wxPoint corners[5];
};

enum PauseType
{
    PAUSE_NONE,     // not paused
    PAUSE_GUIDING,  // pause guide corrections but continue looping exposures
    PAUSE_FULL,     // pause guide corrections and pause looping exposures
};

struct LockPosShiftParams
{
    bool shiftEnabled;
    PHD_Point shiftRate;
    GRAPH_UNITS shiftUnits;
    bool shiftIsMountCoords;
};

class DefectMap;

/*
 * The Guider class is responsible for running the state machine
 * associated with the GUIDER_STATES enumerated type.
 *
 * It is also responsible for drawing and decorating the acquired
 * image in a way that makes sense for its type.
 *
 */

class GuiderConfigDialogCtrlSet : public ConfigDialogCtrlSet
{
    Guider *m_pGuider;
    wxCheckBox *m_pEnableFastRecenter;
    wxCheckBox *m_pScaleImage;

public:
    GuiderConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog* pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~GuiderConfigDialogCtrlSet() {};
    virtual void LoadValues(void);
    virtual void UnloadValues(void);

};

class Guider : public wxWindow
{
    // Private member data.

    wxImage *m_displayedImage;
    OVERLAY_MODE m_overlayMode;
    OverlaySlitCoords m_overlaySlitCoords;
    const DefectMap *m_defectMapPreview;
    double m_polarAlignCircleRadius;
    double m_polarAlignCircleCorrection;
    PHD_Point m_polarAlignCircleCenter;
    PauseType m_paused;
    ShiftPoint m_lockPosition;
    PHD_Point m_ditherRecenterStep;
    wxPoint m_ditherRecenterDir;
    PHD_Point m_ditherRecenterRemaining;
    time_t m_starFoundTimestamp;  // timestamp when star was last found
    double m_avgDistance;         // averaged distance for distance reporting
    bool m_avgDistanceNeedReset;
    GUIDER_STATE m_state;
    usImage *m_pCurrentImage;
    bool m_scaleImage;
    bool m_lockPosIsSticky;
    bool m_fastRecenterEnabled;
    LockPosShiftParams m_lockPosShift;
    bool m_measurementMode;

protected:
    int m_searchRegion; // how far u/d/l/r do we do the initial search for a star
    bool m_forceFullFrame;
    double m_scaleFactor;
    bool m_showBookmarks;
    std::vector<wxRealPoint> m_bookmarks;

    // Things related to the Advanced Config Dialog
public:
    class GuiderConfigDialogPane : public ConfigDialogPane
    {
        Guider *m_pGuider;
        wxCheckBox *m_pScaleImage;
        wxCheckBox *m_pEnableFastRecenter;

    public:
        GuiderConfigDialogPane(wxWindow *pParent, Guider *pGuider);
        ~GuiderConfigDialogPane(void) {};

        void LoadValues(void) {};
        void UnloadValues(void) {};
        virtual void LayoutControls(Guider *pGuider, BrainCtrlIdMap& CtrlMap);
    };


    OVERLAY_MODE GetOverlayMode(void);

public:
    virtual GuiderConfigDialogPane *GetConfigDialogPane(wxWindow *pParent)= 0;
    virtual GuiderConfigDialogCtrlSet *GetConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);

protected:
    Guider(wxWindow *parent, int xSize, int ySize);
    virtual ~Guider(void);

    bool PaintHelper(wxAutoBufferedPaintDCBase& dc, wxMemoryDC& memDC);
    void SetState(GUIDER_STATE newState);
    void UpdateCurrentDistance(double distance);

    void ToggleBookmark(const wxRealPoint& pt);

public:
    bool IsPaused(void) const;
    PauseType GetPauseType(void) const;
    PauseType SetPaused(PauseType pause);
    GUIDER_STATE GetState(void);
    static EXPOSED_STATE GetExposedState(void);
    bool IsCalibratingOrGuiding(void);
    bool IsGuiding(void) const;
    void OnClose(wxCloseEvent& evt);
    void OnErase(wxEraseEvent& evt);
    void UpdateImageDisplay(usImage *pImage=NULL);

    bool MoveLockPosition(const PHD_Point& mountDelta);
    bool SetLockPosition(const PHD_Point& position);
    bool SetLockPosToStarAtPosition(const PHD_Point& starPositionHint);
    bool ShiftLockPosition(void);
    void EnableLockPosShift(bool enable);
    void SetLockPosShiftRate(const PHD_Point& rate, GRAPH_UNITS units, bool isMountCoords);
    bool LockPosShiftEnabled(void) const { return m_lockPosShift.shiftEnabled; }
    void SetLockPosIsSticky(bool isSticky) { m_lockPosIsSticky = isSticky; }
    bool LockPosIsSticky(void) const { return m_lockPosIsSticky; }
    const PHD_Point& LockPosition();
    const LockPosShiftParams& GetLockPosShiftParams(void) const { return m_lockPosShift; }
    void ForceFullFrame(void);

    bool SetOverlayMode(int newMode);
    void GetOverlaySlitCoords(wxPoint *center, wxSize *size, int *angle);
    void SetOverlaySlitCoords(const wxPoint& center, const wxSize& size, int angle);
    void SetDefectMapPreview(const DefectMap *preview);
    void SetPolarAlignCircle(const PHD_Point& center, double radius);
    void SetPolarAlignCircleCorrection(double val);
    double GetPolarAlignCircleCorrection(void);
    bool SaveCurrentImage(const wxString& fileName);

    void StartGuiding(void);
    void StopGuiding(void);
    void UpdateGuideState(usImage *pImage, bool bStopping=false);

    bool SetScaleImage(bool newScaleValue);
    bool GetScaleImage(void);

    int GetSearchRegion(void) const;
    double CurrentError(void);

    bool GetBookmarksShown(void);
    void SetBookmarksShown(bool show);
    void ToggleShowBookmarks(void);
    void DeleteAllBookmarks(void);
    void BookmarkLockPosition();
    void BookmarkCurPosition();

    void Reset(bool fullReset);
    void EnableMeasurementMode(bool enabled);

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of
    // their operation
private:
    virtual void InvalidateLockPosition(void);
public:
    virtual void LoadProfileSettings(void);

    // pure virtual functions -- these MUST be overridden by a subclass
public:
    virtual bool IsValidLockPosition(const PHD_Point& pt) = 0;
private:
    virtual void InvalidateCurrentPosition(bool fullReset = false) = 0;
    virtual bool UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo) = 0;
    virtual bool SetCurrentPosition(usImage *pImage, const PHD_Point& position) = 0;

public:
    virtual void OnPaint(wxPaintEvent& evt) = 0;

    virtual bool IsLocked(void) = 0;
    virtual bool AutoSelect(void) = 0;

    virtual const PHD_Point& CurrentPosition(void) = 0;
    virtual wxRect GetBoundingBox(void) = 0;
    virtual int GetMaxMovePixels(void) = 0;
    virtual double StarMass(void) = 0;
    virtual unsigned int StarPeakADU(void) = 0;
    virtual double SNR(void) = 0;
    virtual double HFD(void) = 0;
    virtual int StarError(void) = 0;

    usImage *CurrentImage(void);
    virtual wxImage *DisplayedImage(void);
    virtual double ScaleFactor(void);

    virtual wxString GetSettingsSummary();

    bool IsFastRecenterEnabled(void);
    void EnableFastRecenter(bool enable);

private:
    void UpdateLockPosShiftCameraCoords(void);
    DECLARE_EVENT_TABLE()
};

inline bool Guider::IsPaused(void) const
{
    return m_paused != PAUSE_NONE;
}

inline PauseType Guider::GetPauseType(void) const
{
    return m_paused;
}

inline bool Guider::GetScaleImage(void)
{
    return m_scaleImage;
}

inline const PHD_Point& Guider::LockPosition()
{
    return m_lockPosition;
}

inline GUIDER_STATE Guider::GetState(void)
{
    return m_state;
}

inline bool Guider::IsGuiding(void) const
{
    return m_state == STATE_GUIDING;
}

inline bool Guider::IsCalibratingOrGuiding(void)
{
    return m_state >= STATE_CALIBRATING_PRIMARY && m_state <= STATE_GUIDING;
}

inline int Guider::GetSearchRegion(void) const
{
    return m_searchRegion;
}

inline bool Guider::IsFastRecenterEnabled(void)
{
    return m_fastRecenterEnabled;
}

inline double Guider::GetPolarAlignCircleCorrection(void)
{
    return m_polarAlignCircleCorrection;
}

inline void Guider::SetPolarAlignCircleCorrection(double val)
{
    m_polarAlignCircleCorrection = val;
}

inline usImage *Guider::CurrentImage(void)
{
    return m_pCurrentImage;
}

inline wxImage *Guider::DisplayedImage(void)
{
    return m_displayedImage;
}

inline double Guider::ScaleFactor(void)
{
    return m_scaleFactor;
}

inline bool Guider::GetBookmarksShown(void)
{
    return m_showBookmarks;
}

#endif /* GUIDER_H_INCLUDED */
