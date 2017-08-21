/*
 *  stepguider_sxAO.h
 *  PHD Guiding
 *
 *  Created by Andy Galasso, based on work by Bret McKee
 *  Copyright (c) 2013 Andy Galasso
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

#ifndef STEPGUIDER_SIMULATOR_INCLUDED
#define STEPGUIDER_SIMULATOR_INCLUDED

#ifdef STEPGUIDER_SIMULATOR

class StepGuiderSimulator : public StepGuider
{
public:
    StepGuiderSimulator(void);
    virtual ~StepGuiderSimulator(void);

    bool Connect(void) override;
    bool Disconnect(void) override;

private:
    bool Center(void) override;
    STEP_RESULT Step(GUIDE_DIRECTION direction, int steps) override;
    int MaxPosition(GUIDE_DIRECTION direction) const override;
    bool SetMaxPosition(int steps) override;
    bool HasNonGuiMove(void) override;
};

#endif // STEPGUIDER_SIMULATOR
#endif // STEPGUIDER_SIMULATOR_INCLUDED
