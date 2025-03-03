/*
 *  guide_algorithms.h
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

#ifndef GUIDE_ALGORITHMS_H_INCLUDED
#define GUIDE_ALGORITHMS_H_INCLUDED

enum GUIDE_ALGORITHM
{
    GUIDE_ALGORITHM_NONE = -1,
    GUIDE_ALGORITHM_IDENTITY,
    GUIDE_ALGORITHM_HYSTERESIS,
    GUIDE_ALGORITHM_LOWPASS,
    GUIDE_ALGORITHM_LOWPASS2,
    GUIDE_ALGORITHM_RESIST_SWITCH,
    GUIDE_ALGORITHM_GAUSSIAN_PROCESS,
    GUIDE_ALGORITHM_ZFILTER,
};

#include "guide_algorithm.h"
#include "guide_algorithm_identity.h"
#include "guide_algorithm_hysteresis.h"
#include "guide_algorithm_lowpass.h"
#include "guide_algorithm_lowpass2.h"
#include "guide_algorithm_resistswitch.h"
#include "guide_algorithm_gaussian_process.h"
#include "guide_algorithm_zfilter.h"

#endif /* GUIDE_ALGORITHMS_H_INCLUDED */
