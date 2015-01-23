/*
 *  ShoeString.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
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

#ifdef GUIDE_VOYAGER

bool ScopeVoyager::Connect(const wxString& hostname)
{
    bool bError = true;
    wxIPV4address addr;
    addr.Hostname(hostname);
    addr.Service(4030);

    VoyagerClient.Close(); // just in case it was already open

    VoyagerClient.Connect(addr, false);
    VoyagerClient.WaitOnConnect(5);

    if (VoyagerClient.IsConnected()) {
        wxMessageBox(_("Connection established"));
        bError = false;
    }

    if (!bError)
    {
        Scope::Connect();
    }

    return bError;
}

bool ScopeVoyager::Connect(void)
{
    return Connect(_T("localhost"));
}

bool ScopeVoyager::Disconnect(void)
{
    VoyagerClient.Close();
    Scope::DisConnect();

    return false;
}

Mount::MOVE_RESULT ScopeVoyager::Guide(GUIDE_DIRECTION direction, int duration)
{
    MOVE_RESULT result = MOVE_OK;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Voyager Scope: not connected");
        }

        // Disable input notification
        VoyagerClient.SetNotify(wxSOCKET_LOST_FLAG);  // Disable input for now
        VoyagerClient.SetTimeout(2); // set to 2s timeout
        char msg[64], dir;

        sprintf(msg,"RATE 100\n\n");
        VoyagerClient.Write(msg,strlen(msg));
        VoyagerClient.Read(&msg,10);

        if (strstr(msg,"ERROR"))
        {
            throw ERROR_INFO("Voyager Scope: error setting rate");
        }

        switch (direction) {
            case NORTH:
                dir = 'N';
                break;
            case SOUTH:
                dir = 'S';
                break;
            case EAST:
                dir = 'E';
                break;
            case WEST:
                dir = 'W';
                break;
        }
        sprintf(msg,"MOVE %c\n\n",dir);
        VoyagerClient.Write(msg,strlen(msg));
        VoyagerClient.Read(&msg,10);
        WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY);
        sprintf(msg,"STOP %c\n\n",dir);
        VoyagerClient.Write(msg,strlen(msg));
        VoyagerClient.Read(&msg,10);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        result = MOVE_ERROR;
    }

    return result;
}

#endif /* GUIDE_VOYAGER */
