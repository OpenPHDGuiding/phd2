/*
 *  imagelogger.h
 *  PHD2 Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2017 openphdguiding.org
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
 *    Neither the name of OpenPHDGuiding.org nor the names of its
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

#ifndef IMAGELOGGER_INCLUDED
#define IMAGELOGGER_INCLUDED

struct ImageLoggerSettings
{
    bool loggingEnabled;
    bool logFramesOverThreshRel;
    bool logFramesOverThreshPx;
    bool logFramesDropped;
    bool logAutoSelectFrames;
    bool logNextNFrames;
    double guideErrorThreshRel; // relative error theshold
    double guideErrorThreshPx; // pixel error theshold
    unsigned int logNextNFramesCount;

    ImageLoggerSettings() :
        loggingEnabled(false), logFramesOverThreshRel(false), logFramesOverThreshPx(false),
        logFramesDropped(false), logAutoSelectFrames(false), logNextNFrames(false)
    { }
};

class ImageLogger
{
public:

    static void Init();
    static void Destroy();

    static void GetSettings(ImageLoggerSettings *settings);
    static void ApplySettings(const ImageLoggerSettings& settings);

    static void SaveImage(usImage *img);
    static void LogImage(const usImage *img, const FrameDroppedInfo& info);
    static void LogImage(const usImage *img, double distance);
    static void LogImageStarDeselected(const usImage *img);
    static void LogAutoSelectImage(const usImage *img, bool succeeded);
};

#endif // IMAGELOGGER_INCLUDED
