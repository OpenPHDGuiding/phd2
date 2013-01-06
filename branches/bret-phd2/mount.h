/*
 *  mount.h
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

#ifndef MOUNT_H_INCLUDED
#define MOUNT_H_INCLUDED

enum GUIDE_DIRECTION {
	NONE  = -1,
	NORTH = 0,	// Dec+
	SOUTH,		// Dec-
	EAST,		// RA-
	WEST		// RA+
};

class Mount
{
protected:
    bool m_bConnected;
    bool m_bGuiding;
    bool m_bCalibrated;

    double m_dDecAngle;
    double m_dRaAngle;
    double m_dDecRate;
    double m_dRaRate;

	wxString m_Name;
protected:
    class MountConfigDialogPane : public ConfigDialogPane
    {
        Mount *m_pMount;
        wxCheckBox *m_pRecalibrate;

        public:
        MountConfigDialogPane(wxWindow *pParent, Mount *pMount);
        ~MountConfigDialogPane(void);

        virtual void LoadValues(void);
        virtual void UnloadValues(void);
    };

public:
    Mount();
    virtual ~Mount();

    // these MUST be supplied by a subclass
    virtual bool BeginCalibration(const Point &currentPosition)=0;
    virtual bool UpdateCalibrationState(const Point &currentPosition)=0;

    // these CAN be supplied by a subclass
	virtual wxString &Name(void);
    virtual bool IsConnected(void);
    virtual bool IsCalibrated(void);
    virtual void ClearCalibration(void);
    virtual double DecAngle(void);
    virtual double RaAngle(void);
    virtual double DecRate(void);
    virtual double RaRate(void);
    virtual bool Connect(void);
	virtual bool Disconnect(void);
    virtual bool SetCalibration(double dRaAngle, double dDecAngle, double dRaRate, double dDecRate);
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
};

#endif /* MOUNT_H_INCLUDED */
