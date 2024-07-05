/*
 *  phdupdate.h
 *  Open PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2017 Andy Galasso.
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
 *    Neither the name of openphdguiding.org nor the names of its
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

#ifndef PHDUPDATE_INCLUDED
#define PHDUPDATE_INCLUDED

enum UpdateSeries
{
    UPD_SERIES_MAIN,
    UPD_SERIES_DEV,
};

struct UpdaterSettings
{
    bool enabled;
    UpdateSeries series;
};

class PHD2Updater
{
public:

    // start the updater process at app init
    static void InitUpdater();

    // for Brain dialog UI
    static void GetSettings(UpdaterSettings *settings);
    static void SetSettings(const UpdaterSettings& settings);

    // explicitly check for updates now
    static void CheckNow();

    // stop the updater process at app exit
    static void StopUpdater();

    // MyFrame needs to call this when it receives thread messages from the updater
    static void OnUpdaterStateChanged();
};

#endif
