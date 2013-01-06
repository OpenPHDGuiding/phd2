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

#include <wx/bitmap.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

enum E_GUIDER_STATES
{
	STATE_UNINITIALIZED = 0,
	STATE_SELECTING,
	STATE_SELECTED,
	STATE_CALIBRATING,
	STATE_CALIBRATED,
	STATE_GUIDING_LOCKED,
	STATE_GUIDING_LOST, 
	// these aren't actual canvas states below
	// mainly used for getting the status on the server
	STATE_PAUSED = 100,
	STATE_LOOPING,
	STATE_LOOPING_SELECTED 
};

/*
 * The Guider class is responsible for running the state machine
 * associated with the E_GUIDER_STATES enumerated type.
 *
 * It is also responsible for drawing and decorating the acquired
 * image in a way that makes sense for its type.
 *
 */

class Guider: public wxWindow
{
protected:
    E_GUIDER_STATES m_state;
	double	m_scaleFactor;
	wxImage	*m_displayedImage;

public:
    // functions with a implemenation in Guider
    virtual E_GUIDER_STATES GetState(void);
    virtual bool SetState(E_GUIDER_STATES newState);
	virtual void OnClose(wxCloseEvent& evt);
	virtual void OnErase(wxEraseEvent& evt);
    virtual void UpdateImageDisplay(usImage *pImage);

    // pure virutal functions
	virtual void OnPaint(wxPaintEvent& evt) = 0;
    virtual void UpdateGuideState(usImage *pImage, bool bUpdateStatus) = 0;
    virtual Point &LockPosition() = 0;
    virtual Point &CurrentPosition() = 0;

protected:
    virtual bool PaintHelper(wxAutoBufferedPaintDC &dc, wxMemoryDC &memDC);

	Guider(wxWindow *parent, int xSize, int ySize);
    virtual ~Guider(void);

    // Event table
	DECLARE_EVENT_TABLE()
};

#endif /* GUIDER_H_INCLUDED */
