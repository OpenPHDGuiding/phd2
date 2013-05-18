/*
 *  stepguider_sxAO.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2013 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
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

#if defined(STEPGUIDER_SXAO) && !defined(STEPGUIDER_SXAO_H_INCLUDED)
#define STEPGUIDER_SXAO_H_INCLUDED

#include "wx/msw/ole/automtn.h"

class StepGuiderSxAO : public StepGuider
{
    static const int MaxSteps       = 45;
    static const int DefaultTimeout =  1*1000;
    static const int CenterTimeout  = 45*1000;
    SerialPort *m_pSerialPort;
public:
    StepGuiderSxAO(void);
    virtual ~StepGuiderSxAO(void);

    virtual bool Connect(void);
    virtual bool Disconnect(void);

private:
    virtual bool Step(GUIDE_DIRECTION direction, int steps);
    virtual int MaxPosition(GUIDE_DIRECTION direction);
    virtual bool IsAtLimit(GUIDE_DIRECTION direction, bool& isAtLimit);

    bool SendThenReceive(unsigned char sendChar, unsigned char& receivedChar);
    bool SendThenReceive(unsigned char *pBuffer, unsigned bufferSize, unsigned char& recievedChar);

    bool SendShortCommand(unsigned char command, unsigned char& response);
    bool SendLongCommand(unsigned char command, unsigned char parameter, unsigned count, unsigned char& response);

    bool FirmwareVersion(unsigned& version);
    bool Unjam(void);
    bool Center(void);
    bool Center(unsigned char cmd);
};

#endif // if defined(STEPGUIDER_SXAO) && !defined(STEPGUIDER_SXAO_H_INCLUDED)
