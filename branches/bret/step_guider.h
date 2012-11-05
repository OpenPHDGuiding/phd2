/*
 *  step_guider.h
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

#ifndef STEPGUIDER_H_INCLUDED
#define STEPGUIDER_H_INCLUDED

class StepGuider:public Mount
{
    int m_RaPos;
    int m_DecPos;

public:
    StepGuider(void)
    {
        Connect();
        Center();
    }

    ~StepGuider(void)
    {
        Disconnect();
    }

    virtual bool Step(const GUIDE_DIRECTION direction, const int steps)
    {
        switch (direction)
        {
            case EAST:
                m_RaPos += steps;
                break;
            case WEST:
                m_RaPos -= steps;
                break;
            case NORTH:
                m_DecPos += steps;
                break;
            case SOUTH:
                m_DecPos -= steps;
                break;
        }
    }

    virtual int Position(const GUIDE_DIRECTION direction)
    {
        int ret = 0;

        switch (direction)
        {
            case EAST:
                ret = m_RaPos;
                break;
            case WEST:
                ret = -m_RaPos;
                break;
            case NORTH:
                ret = -m_DecPos;
                break;
            case SOUTH:
                ret = m_DecPos;
                break;
        }

        return ret;
    }

    virtual unsigned MaxStep(const GUIDE_DIRECTION direction)
    {
        return StepLimit(direction) - Position(direction);
    }

    virtual bool Center(void)
    {
        m_RaPos = 0;
        m_DecPos = 0;
    }

    virtual bool IsStepping()
    {
        // default StepGuider is synchronous, so we are never
        // stepping
        return false;
    }

    virtual bool Calibrate(void)=0;
    virtual unsigned StepLimit(const GUIDE_DIRECTION direction)=0;
};

#endif /* STEPGUIDER_H_INCLUDED */
