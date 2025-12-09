/*
 *  scope_onstepguider.h
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

#include "phd.h"

bool OnboardST4::ST4HasGuideOutput(void)
{
    return false;
}

bool OnboardST4::ST4HostConnected(void)
{
    // Onboard ST4 is never connected to an external host
    Debug.Write("OnboardST4: ST4HostConnected() called - onboard ST4 cannot connect to external hosts\n");
    return true;
}

bool OnboardST4::ST4HasNonGuiMove(void)
{
    // Onboard ST4 doesn't support non-GUI moves
    Debug.Write("OnboardST4: ST4HasNonGuiMove() called - onboard ST4 does not support non-GUI moves\n");
    return true;
}

bool OnboardST4::ST4SynchronousOnly(void)
{
    return true;
}

bool OnboardST4::ST4PulseGuideScope(int direction, int duration)
{
    // Onboard ST4 doesn't support pulse guide
    Debug.Write(wxString::Format("OnboardST4: ST4PulseGuideScope(dir=%d, dur=%d) - onboard ST4 does not support pulse guide\n", 
                                direction, duration));
    return true;
}
