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

// Canvas area for image -- can take events
class GuiderOneStar: public Guider
{
protected:
    Star m_star;

    // parameters
    int m_maxDecDuration;
    int m_maxRaDuration;
    double m_raAggression;
    DEC_GUIDE_OPTION m_decGuideOption;
public:
	GuiderOneStar(wxWindow *parent);
    virtual ~GuiderOneStar(void);

    virtual bool SetState(GUIDER_STATE newState);

    virtual bool SetParms(int maxDecDuration, int maxRaDuration, DEC_GUIDE_OPTION decGuide);
	virtual void OnPaint(wxPaintEvent& evt);
    virtual bool UpdateGuideState(usImage *pImage, bool bUpdateStatus);
    virtual bool IsLocked(void);
    virtual Point &CurrentPosition(void);

protected:
    void OnLClick(wxMouseEvent& evt);
    bool DoGuide(void);

    void SaveStarFITS();

	DECLARE_EVENT_TABLE()
};

#endif /* GUIDER_ONESTAR_H_INCLUDED */
