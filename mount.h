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
	NORTH = 0,	// Dec+
	SOUTH,		// Dec-
	EAST,		// RA-
	WEST		// RA+
};

class Mount
{
protected:
    bool m_bConnected;
    bool m_bCalibrated;
    bool m_bGuiding;

    double m_dDecAngle;
    double m_dRaAngle;
    double m_dDecRate;
    double m_dRaRate;

	wxString m_Name;

public:
    Mount()
    {
        m_bConnected = false;
        m_bCalibrated = false;
        m_bGuiding = false;
    }

    virtual ~Mount()
    {
    }

    // these MUST be supplied by a subclass
    virtual bool Calibrate(void) = 0;
    virtual bool Guide(const GUIDE_DIRECTION direction, const int durationMs) = 0;
    virtual bool IsGuiding() = 0;

    // These CAN be supplied by a subclass
	virtual wxString &Name(void)
    {
        return m_Name;
    }

    virtual bool IsConnected()
    {
        bool bReturn = m_bConnected;

        return bReturn;
    }

    virtual bool IsCalibrated()
    {
        bool bReturn = false;

        if (IsConnected())
        {
            bReturn = m_bCalibrated;
        }

        return bReturn;

    }

    virtual void ClearCalibration(void)
    {
        m_bCalibrated = false;
    }

    virtual double DecAngle()
    {
        double dReturn = 0;

        if (IsCalibrated())
        {
            dReturn = m_dDecAngle;
        }

        return dReturn;
    }

    virtual double RaAngle()
    {
        double dReturn = 0;

        if (IsCalibrated())
        {
            dReturn = m_dRaAngle;
        }

        return dReturn;
    }

    virtual double DecRate()
    {
        double dReturn = 0;

        if (IsCalibrated())
        {
            dReturn = m_dDecRate;
        }

        return dReturn;
    }

    virtual double RaRate()
    {
        double dReturn = 0;

        if (IsCalibrated())
        {
            dReturn = m_dRaRate;
        }

        return dReturn;
    }

    virtual bool Connect(void)
    {
        m_bConnected = true;

        return false;
    }

	virtual bool Disconnect(void)
    {
        m_bConnected = false;

        return false;
    }

    virtual bool SetCalibration(double dRaAngle, double dDecAngle, double dRaRate, double dDecRate)
    {
        m_dDecAngle = dDecAngle;
        m_dRaAngle  = dRaAngle;
        m_dDecRate  = dDecRate;
        m_dRaRate   = dRaRate;
        m_bCalibrated = true;

        return true;
    }

};

#endif /* MOUNT_H_INCLUDED */
