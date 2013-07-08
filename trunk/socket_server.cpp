/*
 *  socket_server.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2008-2010 Craig Stark.
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
#include <wx/log.h>
#include "socket_server.h"

static wxLogWindow *SocketLog = NULL;

enum {
    MSG_PAUSE = 1,
    MSG_RESUME,
    MSG_MOVE1,
    MSG_MOVE2,
    MSG_MOVE3,
    MSG_IMAGE,
    MSG_GUIDE,
    MSG_CAMCONNECT,
    MSG_CAMDISCONNECT,
    MSG_REQDIST,
    MSG_REQFRAME,
    MSG_MOVE4,
    MSG_MOVE5,
    MSG_AUTOFINDSTAR,
    MSG_SETLOCKPOSITION,    //15
    MSG_FLIPRACAL,          //16
    MSG_GETSTATUS,          //17
    MSG_STOP,               //18
    MSG_LOOP,               //19
    MSG_STARTGUIDING,       //20
    MSG_LOOPFRAMECOUNT,     //21
    MSG_CLEARCAL            //22
};

void MyFrame::OnServerMenu(wxCommandEvent &evt) {
    SetServerMode(evt.IsChecked());
    StartServer(GetServerMode());
}

bool MyFrame::StartServer(bool state)
{
    if (state) {
        if (!SocketLog) {
            SocketLog = new wxLogWindow(this,wxT("Server log"));
            SocketLog->SetVerbose(true);
            wxLog::SetActiveTarget(SocketLog);
        }

        // Create the SocketServer socket
        unsigned int port = 4300 + m_instanceNumber - 1;
        wxIPV4address sockServerAddr;
        sockServerAddr.Service(port);
        SocketServer = new wxSocketServer(sockServerAddr);

        // We use Ok() here to see if the server is really listening
        if (!SocketServer->Ok()) {
            wxLogStatus(wxString::Format("Socket server failed to start - Could not listen at port %u", port));
            wxLog::SetActiveTarget(NULL);
            delete SocketLog;
            SocketLog = NULL;
            delete SocketServer;
            SocketServer = NULL;
            return true;
        }
        SocketServer->SetEventHandler(*this, SOCK_SERVER_ID);
        SocketServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
        SocketServer->Notify(true);

        // start the event server
        if (EvtServer.EventServerStart(m_instanceNumber)) {
            wxLog::SetActiveTarget(NULL);
            delete SocketLog;
            SocketLog = NULL;
            delete SocketServer;
            SocketServer = NULL;
            return true;
        }

        SocketConnections = 0;
        SetStatusText(_("Server started"));
        wxLogStatus(wxString::Format(_("Server started, listening on port %u"), port));
        SocketLog->Show(this->Menubar->IsChecked(MENU_DEBUG));
    }
    else {
        wxLogStatus(_("Server stopped"));
        wxLog::SetActiveTarget(NULL);
        delete SocketLog;
        SocketLog = NULL;
        EvtServer.EventServerStop();
        delete SocketServer;
        SocketServer = NULL;
        SetStatusText(_("Server stopped"));
    }

    return false;
}

void MyFrame::OnSockServerEvent(wxSocketEvent& event)
{
    wxSocketServer *server = static_cast<wxSocketServer *>(event.GetSocket());

    if (server == NULL)
        return;

    if (event.GetSocketEvent() != wxSOCKET_CONNECTION) {
        wxLogStatus(_T("WTF is this event?"));
        return;
    }

    wxSocketBase *client = server->Accept(false);

    if (client) {
        pFrame->SetStatusText("New connection");
        wxLogStatus(_T("New cnxn"));
    }
    else {
        wxLogStatus(_T("Cnxn error"));
        return;
    }

    client->SetEventHandler(*this, SOCK_SERVER_CLIENT_ID);
    client->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
    client->Notify(true);

    SocketConnections++;
}

void MyFrame::HandleSockServerInput(wxSocketBase *sock)
{
    unsigned char rval = 0;

    try
    {
        // We disable input events, so that the test doesn't trigger
        // wxSocketEvent again.
        sock->SetNotify(wxSOCKET_LOST_FLAG);

        // Which command is coming in?
        unsigned char c;

        sock->Read(&c, 1);

        Debug.AddLine("read socket command %d", c);

        switch (c)
        {
            case MSG_PAUSE:
            case 'p':
                Debug.AddLine("processing socket request PAUSE");
                pGuider->SetPaused(true);
                wxLogStatus(_T("Paused"));
                GuideLog.ServerCommand(pGuider, "PAUSE");
                EvtServer.NotifyPaused();
                break;
            case MSG_RESUME:
            case 'r':
                Debug.AddLine("processing socket request RESUME");
                pGuider->SetPaused(false);
                wxLogStatus (_T("Resumed"));
                GuideLog.ServerCommand(pGuider, "RESUME");
                EvtServer.NotifyResumed();
                break;
            case MSG_MOVE1:  // +/- 0.5
            case MSG_MOVE2:  // +/- 1.0
            case MSG_MOVE3:  // +/- 2.0
            case MSG_MOVE4:  // +/- 3.0
            case MSG_MOVE5:  // +/- 5.0
            {
                Debug.AddLine("processing socket request MOVEn");

                double size = 1.0;

                if (pGuider->GetState() != STATE_GUIDING)
                {
                    throw ERROR_INFO("cannot dither if not guiding");
                }

                // note: size is twice the desired move amount
                switch(c)
                {
                    case MSG_MOVE1:  // +/- 0.5
                        size =  1.0;
                        break;
                    case MSG_MOVE2:  // +/- 1.0
                        size =  2.0;
                        break;
                    case MSG_MOVE3:  // +/- 2.0
                        size =  4.0;
                        break;
                    case MSG_MOVE4:  // +/- 3.0
                        size =  6.0;
                        break;
                    case MSG_MOVE5:  // +/- 5.0
                        size = 10.0;
                        break;
                }

                size = size * m_ditherScaleFactor;

                double dRa  =  (rand()/(double)RAND_MAX)*size - (size /2.0);
                double dDec =  (rand()/(double)RAND_MAX)*size - (size /2.0);

                if (m_ditherRaOnly)
                {
                    dDec = 0;
                }

                Debug.AddLine("dither: size=%.2f, dRA=%.2f dDec=%.2f", size, dRa, dDec);

                pGuider->MoveLockPosition(PHD_Point(dRa, dDec));

                wxLogStatus(_T("Moving by %.2f,%.2f"),dRa, dDec);
                GuideLog.ServerGuidingDithered(pGuider, dRa, dDec);
                EvtServer.NotifyGuidingDithered(dRa, dDec);

                rval = RequestedExposureDuration() / 1000;
                if (rval < 1)
                {
                    rval = 1;
                }
                break;
            }
            case MSG_REQDIST:
            {
                Debug.AddLine("processing socket request REQDIST");
                if (pGuider->GetState() != STATE_GUIDING)
                {
                    throw ERROR_INFO("cannot request distance if not guiding");
                }

                double currentError = pGuider->CurrentError();

                if (currentError > 2.55)
                {
                    rval = 255;
                }
                else
                {
                    rval = (unsigned char) (currentError * 100);
                }

                wxLogStatus(_T("Sending pixel error of %.2f"),(float) rval / 100.0);
                break;
            }
            case MSG_AUTOFINDSTAR:
                Debug.AddLine("processing socket request AUTOFINDSTAR");
                rval = pFrame->pGuider->AutoSelect();
                if (rval)
                {
                    wxCommandEvent *tmp_evt;
                    tmp_evt = new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, BUTTON_LOOP);
                    QueueEvent(tmp_evt);
                }
                GuideLog.ServerCommand(pGuider, "AUTO FIND STAR");
                break;
            case MSG_SETLOCKPOSITION:
            {
                // Sets LockX and LockY to be user-specified
                unsigned short x,y;
                sock->Read(&x, 2);
                sock->Read(&y, 2);
                sock->Discard();  // Clean out anything else


                if (pFrame->pGuider->SetLockPosition(PHD_Point(x,y), false))
                {
                    Debug.AddLine("processing socket request SETLOCKPOSITION for (%d, %d) succeeded", x, y);
                    wxLogStatus(wxString::Format("Lock set to %d,%d",x,y));
                    GuideLog.ServerSetLockPosition(pGuider);
                }
                else
                {
                    Debug.AddLine("processing socket request SETLOCKPOSITION for (%d, %d) failed", x, y);
                    wxLogStatus(wxString::Format("Lock set to %d,%d failed",x,y));
                }
                break;
            }
            case MSG_FLIPRACAL:
            {
                Debug.AddLine("processing socket request FLIPRACAL");
                bool wasPaused = pGuider->SetPaused(true);
                // return 1 for success, 0 for failure
                rval = 1;
                if (FlipRACal())
                {
                    rval = 0;
                }
                pGuider->SetPaused(wasPaused);
                GuideLog.ServerCommand(pGuider, "FLIP RA CAL");
                break;
            }
            case MSG_GETSTATUS:
                Debug.AddLine("processing socket request GETSTATUS");
                rval = Guider::GetExposedState();
                break;
            case MSG_LOOP:
            {
                Debug.AddLine("processing socket request LOOP");
                wxCommandEvent *tmp_evt;
                tmp_evt = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, BUTTON_LOOP);
                QueueEvent(tmp_evt);
                GuideLog.ServerCommand(pGuider, "LOOP");
                break;
            }
            case MSG_STOP:
            {
                Debug.AddLine("processing socket request STOP");
                wxCommandEvent *tmp_evt;
                tmp_evt = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, BUTTON_STOP);
                QueueEvent(tmp_evt);
                GuideLog.ServerCommand(pGuider, "STOP");
                break;
            }
            case MSG_STARTGUIDING:
            {
                Debug.AddLine("processing socket request STARTGUIDING");
                wxCommandEvent *tmp_evt;
                tmp_evt = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, BUTTON_GUIDE);
                QueueEvent(tmp_evt);
                GuideLog.ServerCommand(pGuider, "START GUIDING");
                break;
            }
            case MSG_LOOPFRAMECOUNT:
                Debug.AddLine("processing socket request LOOPFRAMECOUNT");
                if (m_frameCounter > UCHAR_MAX)
                {
                    rval = UCHAR_MAX;
                }
                else
                {
                    rval = m_frameCounter;
                }
                break;
            case MSG_CLEARCAL:
                Debug.AddLine("processing socket request CLEARCAL");

                if (!pMount)
                {
                    throw ERROR_INFO("cannot CLEARCAL if !pMount");
                }

                if (!pMount->IsConnected())
                {
                    throw ERROR_INFO("cannot CLEARCAL if !pMount->IsConnected");
                }

                if (!pMount->IsCalibrated())
                {
                    throw ERROR_INFO("cannot CLEARCAL if !pMount->IsCalibrated()");
                }

                pMount->ClearCalibration();
                GuideLog.ServerCommand(pGuider, "CLEAR CAL");
            default:
                wxLogStatus(_T("Unknown test id received from client: %d"),(int) c);
                rval = 1;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    Debug.AddLine("Sending socket respose %d (0x%x)", rval, rval);
    // Send response
    sock->Write(&rval,1);

    // Enable input events again.
    sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
}

void MyFrame::OnSockServerClientEvent(wxSocketEvent& event)
{
    try
    {
        wxSocketBase *sock = event.GetSocket();

        if (SocketServer == NULL)
        {
            throw ERROR_INFO("socket command when SocketServer == NULL");
        }

        // Now we process the event
        switch (event.GetSocketEvent())
        {
            case wxSOCKET_INPUT:
                HandleSockServerInput(sock);
                break;
            case wxSOCKET_LOST:
                SocketConnections--;
                wxLogStatus(_T("Deleting socket"));
                sock->Destroy();
                break;
            default:
                break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

#ifdef NEB_SBIG

// this code only works when there is a single socket connection from Nebulosity

static int SocketConnections;
static wxSocketBase *ServerEndpoint;

bool ServerSendGuideCommand (int direction, int duration) {
    // Sends a guide command to Nebulosity
    if (!pFrame->SocketServer || !SocketConnections)
        return true;

    unsigned char cmd = MSG_GUIDE;
    unsigned char rval = 0;
    wxLogStatus(_T("Sending guide: %d %d"), direction, duration);
//  cmd = 'Z';
    ServerEndpoint->Write(&cmd, 1);
    if (pFrame->SocketServer->Error())
        wxLogStatus(_T("Error sending Neb command"));
    else {
        wxLogStatus(_T("Cmd done - sending data"));
        ServerEndpoint->Write(&direction, sizeof(int));
        ServerEndpoint->Write(&duration, sizeof(int));
        ServerEndpoint->Read(&rval,1);
        wxLogStatus(_T("Sent guide command - returned %d"), (int) rval);
    }
    return false;
}

bool ServerSendCamConnect(int& xsize, int& ysize) {
    if (!pFrame->SocketServer || !SocketConnections)
        return true;
    wxLogStatus(_T("Sending cam connect request"));
    unsigned char cmd = MSG_CAMCONNECT;
    unsigned char rval = 0;

    ServerEndpoint->Write(&cmd, 1);
    if (pFrame->SocketServer->Error()) {
        wxLogStatus(_T("Error sending Neb command"));
        return true;
    }
    else {
//      unsigned char c;
        ServerEndpoint->Read(&rval, 1);
        wxLogStatus(_T("Cmd done - returned %d"), (int) rval);
    }
    if (rval)
        return true;
    else {  // cam connected OK
        // Should get x and y size back
        ServerEndpoint->Read(&xsize,sizeof(int));
        ServerEndpoint->Read(&ysize,sizeof(int));
        wxLogStatus(_T("Guide chip reported as %d x %d"),xsize,ysize);
        return false;
    }
}

bool ServerSendCamDisconnect() {
    if (!pFrame->SocketServer || !SocketConnections)
        return true;

    wxLogStatus(_T("Sending cam disconnect request"));
    unsigned char cmd = MSG_CAMDISCONNECT;
    unsigned char rval = 0;

    ServerEndpoint->Write(&cmd, 1);
    if (pFrame->SocketServer->Error()) {
        wxLogStatus(_T("Error sending Neb command"));
        return true;
    }
    else {
//      unsigned char c;
        ServerEndpoint->Read(&rval, 1);
        wxLogStatus(_T("Cmd done - returned %d"), (int) rval);
    }
    if (rval)
        return true;
    else
        return false;  // cam disconnected OK
}

bool ServerReqFrame(int duration, usImage& img) {
    if (!pFrame->SocketServer || !SocketConnections)
        return true;
    wxLogStatus(_T("Sending guide frame request"));
    unsigned char cmd = MSG_REQFRAME;
    unsigned char rval = 0;

    ServerEndpoint->Write(&cmd, 1);
    if (pFrame->SocketServer->Error()) {
        wxLogStatus(_T("Error sending Neb command"));
        return true;
    }
    else {
//      unsigned char c;
        ServerEndpoint->Read(&rval, 1);
        wxLogStatus(_T("Cmd done - returned %d"), (int) rval);
    }
    if (rval)
        return true;
    else { // grab frame data
        // Send duration request
        ServerEndpoint->Write(&duration,sizeof(int));
        wxLogStatus(_T("Starting %d ms frame"),duration);
        wxMilliSleep(duration); // might as well wait here nicely at least this long
        wxLogStatus(_T("Reading frame - looking for %d pixels (%d bytes)"),img.NPixels,img.NPixels*2);
        unsigned short *dataptr;
        dataptr = img.ImageData;
        unsigned short buffer[512];
        int i, xsize, ysize, pixels_left, packet_size;
        xsize = img.Size.GetWidth();
        ysize = img.Size.GetHeight();
        pixels_left = img.NPixels;
        packet_size = 256;  // # pixels to get
        int j=0;
        while (pixels_left > 0) {
            ServerEndpoint->Read(&buffer,packet_size * 2);
            pixels_left -= packet_size;
            if ((j%100) == 0)
                wxLogStatus(_T("%d left"),pixels_left);
            for (i=0; i<packet_size; i++, dataptr++)
                *dataptr = buffer[i];
            if (pixels_left < 256)
                packet_size = 256;
            ServerEndpoint->Write(&cmd,1);
            j++;
        }
        int min = 9999999;
        int max = -999999;
        dataptr = img.ImageData;
        for (i=0; i<(xsize * ysize); i++, dataptr++) {
            if (*dataptr > max) max = (int) *dataptr;
            else if (*dataptr < min) min = (int) *dataptr;
        }
        wxLogStatus(_T("Frame received min=%d max=%d"),min,max);

//      ServerEndpoint->ReadMsg(img.ImageData,(xsize * ysize * 2));
        wxLogStatus(_T("Frame read"));
    }

    return false;
}

#endif // NEB_SBIG
