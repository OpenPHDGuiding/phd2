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
    DEC_LOWPASS2
};

enum OVERLAY_MODE
{
    OVERLAY_NONE = 0,
    OVERLAY_BULLSEYE,
    OVERLAY_GRID_FINE,
    OVERLAY_GRID_COARSE,
    OVERLAY_RADEC
};

/*
 * The Guider class is responsible for running the state machine
 * associated with the GUIDER_STATES enumerated type.
 *
 * It is also responsible for drawing and decorating the acquired
 * image in a way that makes sense for its type.
 *
 */

class Guider: public wxWindow
{
    // Private member data.

    wxImage *m_displayedImage;
    OVERLAY_MODE m_overlayMode;
    int m_polarAlignCircleRadius;
    PHD_Point m_polarAlignCircleCenter;
    bool m_paused;
    PHD_Point m_lockPosition;
    GUIDER_STATE m_state;
    usImage *m_pCurrentImage;
    bool m_scaleImage;
    bool m_lockPosIsSticky;

protected:
    double  m_scaleFactor;

    // Things related to the Advanced Config Dialog
protected:
    class GuiderConfigDialogPane : public ConfigDialogPane
    {
        Guider *m_pGuider;
        wxCheckBox *m_pScaleImage;

    public:
        GuiderConfigDialogPane(wxWindow *pParent, Guider *pGuider);
        ~GuiderConfigDialogPane(void);

        void LoadValues(void);
        void UnloadValues(void);
    };

    OVERLAY_MODE GetOverlayMode(void);

public:
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent) = 0;

protected:
    Guider(wxWindow *parent, int xSize, int ySize);
    virtual ~Guider(void);

    bool PaintHelper(wxClientDC &dc, wxMemoryDC &memDC);
    void SetState(GUIDER_STATE newState);
    usImage *CurrentImage(void);

public:
    bool IsPaused(void);
    bool SetPaused(bool paused);
    double CurrentError(void);
    GUIDER_STATE GetState(void);
    static EXPOSED_STATE GetExposedState(void);
    void OnClose(wxCloseEvent& evt);
    void OnErase(wxEraseEvent& evt);
    void UpdateImageDisplay(usImage *pImage=NULL);
    bool DoGuide(void);

    bool MoveLockPosition(const PHD_Point& mountDelta);
    bool SetLockPosition(const PHD_Point& position, bool bExact=true);
    void SetLockPosIsSticky(bool isSticky) { m_lockPosIsSticky = isSticky; }
    const PHD_Point& LockPosition();

    bool SetOverlayMode(int newMode);
    void SetPolarAlignCircle(const PHD_Point& center, unsigned int radius);
    bool SaveCurrentImage(const wxString& fileName);

    void StartGuiding(void);
    void UpdateGuideState(usImage *pImage, bool bStopping=false);

    bool SetScaleImage(bool newScaleValue);
    bool GetScaleImage(void);

    void Reset(void);

    // virtual functions -- these CAN be overridden by a subclass, which should
    // consider whether they need to call the base class functions as part of
    // their operation
private:
    virtual void InvalidateLockPosition(void);
    virtual void UpdateLockPosition(void);

    // pure virutal functions -- these MUST be overridden by a subclass
private:
    virtual void InvalidateCurrentPosition(void) = 0;
    virtual bool UpdateCurrentPosition(usImage *pImage, wxString& statusMessage) = 0;
    virtual bool SetCurrentPosition(usImage *pImage, const PHD_Point& position)=0;

public:
    virtual void OnPaint(wxPaintEvent& evt) = 0;

    virtual bool IsLocked(void) = 0;
    virtual bool AutoSelect(usImage *pImage=NULL)=0;

    virtual const PHD_Point& CurrentPosition(void) = 0;
    virtual wxRect GetBoundingBox(void) = 0;
    virtual double StarMass(void) = 0;
    virtual double SNR(void) = 0;

    virtual wxImage *DisplayedImage(void);
    virtual double ScaleFactor(void);

    virtual wxString GetSettingsSummary();

    // wxWindows Event table
private:
    DECLARE_EVENT_TABLE()
};

#endif /* GUIDER_H_INCLUDED */
