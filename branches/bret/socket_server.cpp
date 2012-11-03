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

wxSocketBase *ServerEndpoint;
wxLogWindow *SocketLog = NULL;

extern int FindStar(usImage& img);

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
    MSG_SETLOCKPOSITION,	//15
	MSG_FLIPRACAL,			//16
	MSG_GETSTATUS,			//17
	MSG_STOP,				//18
	MSG_LOOP,				//19
	MSG_STARTGUIDING    	//20
};

void MyFrame::OnServerMenu(wxCommandEvent &evt) {
	ServerMode = evt.IsChecked();
	StartServer(ServerMode);

}

bool MyFrame::StartServer(bool state) {
	wxIPV4address addr;
	addr.Service(4300);

	if (state) {
		if (!SocketLog) {
			SocketLog = new wxLogWindow(this,wxT("Server log"));
			SocketLog->SetVerbose(true);
			wxLog::SetActiveTarget(SocketLog);
		}
		// Create the socket
		SocketServer = new wxSocketServer(addr);

		// We use Ok() here to see if the server is really listening
		if (! SocketServer->Ok()) {
			wxLogStatus(_T("Server failed to start - Could not listen at the specified port"));
			return true;
		}
		SocketServer->SetEventHandler(*this, SERVER_ID);
		SocketServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
		SocketServer->Notify(true);
		SocketConnections = 0;
		SetStatusText(_T("Server started"));
		wxLogStatus(_T("Server started"));
		SocketLog->Show(this->Menubar->IsChecked(MENU_DEBUG));
	}
	else {
		wxLogStatus(_T("Server stopped"));
		wxLog::SetActiveTarget(NULL);
		delete SocketLog;
		SocketLog = NULL;
		delete SocketServer;
		SocketServer= NULL;
		SetStatusText(_T("Server stopped"));
	}

	return false;
}

void MyFrame::OnServerEvent(wxSocketEvent& event) {
//	wxSocketBase *sock;

	if (SocketServer == NULL) return;
	if (event.GetSocketEvent() != wxSOCKET_CONNECTION) {
		wxLogStatus(_T("WTF is this event?"));
		return;
	}

	ServerEndpoint = SocketServer->Accept(false);

	if (ServerEndpoint) {
        frame->SetStatusText("New connection");
		wxLogStatus(_T("New cnxn"));
 	}
	else {
		wxLogStatus(_T("Cnxn error"));
		return;
	}

	ServerEndpoint->SetEventHandler(*this, SOCKET_ID);
	ServerEndpoint->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
	ServerEndpoint->Notify(true);

	SocketConnections++;
}

void MyFrame::OnSocketEvent(wxSocketEvent& event) {
	wxSocketBase *sock = event.GetSocket();
	int ival;
	if (SocketServer == NULL) return;
//	sock = SocketServer;
	// First, print a message
/*	switch(event.GetSocketEvent()) {
		case wxSOCKET_INPUT : wxLogStatus("wxSOCKET_INPUT"); break;
		case wxSOCKET_LOST  : wxLogStatus("wxSOCKET_LOST"); break;
		default             : wxLogStatus("Unexpected event"); break;
	}
*/
	// Now we process the event
	switch(event.GetSocketEvent()) {
		case wxSOCKET_INPUT: {
			// We disable input events, so that the test doesn't trigger
			// wxSocketEvent again.
			sock->SetNotify(wxSOCKET_LOST_FLAG);

			// Which command is coming in?
			unsigned char c, rval;
			sock->Read(&c, 1);
//			wxLogStatus("Msg %d",(int) c);
			rval = 0;
			float rx, ry;
			float size = 1.0;
			switch (c) {
				case MSG_PAUSE: 
				case 'p':
					Paused = true;
					wxLogStatus(_T("Paused"));
					break;
				case MSG_RESUME:
				case 'r':
					Paused = false;
					wxLogStatus (_T("Resumed"));
					break;
				case MSG_MOVE1:  // +/- 0.5
				case MSG_MOVE2:  // +/- 1.0
				case MSG_MOVE3:  // +/- 2.0
				case MSG_MOVE4:  // +/- 3.0
				case MSG_MOVE5:  // +/- 5.0
					if (canvas->State != STATE_GUIDING_LOCKED) {
						break;
					}

					if (c == MSG_MOVE2)
						size = 2.0;
					else if (c == MSG_MOVE3)
						size = 4.0;
					else if (c==MSG_MOVE4)
						size = 4.0;
					else if (c==MSG_MOVE5)
						size = 5.0;
					size = size * DitherScaleFactor;
					rx = (float) (rand() % 1000) / 1000.0 * size - (size / 2.0);
					ry = (float) (rand() % 1000) / 1000.0 * size - (size / 2.0);
					if (DitherRAOnly) {
						if (fabs(tan(pScope->RaAngle())) > 1) 
							rx = ry / tan(pScope->RaAngle());
						else	
							ry = tan(pScope->RaAngle()) * rx;
					}
					LockX = LockX + rx;
					LockY = LockY + ry;
					wxLogStatus(_T("Moving by %.2f,%.2f"),rx,ry);
                    rval = RequestedExposureDuration() / 1000;
					if (rval < 1)
						rval = 1;
					break;
				case MSG_REQDIST:
					if ((canvas->State != STATE_GUIDING_LOCKED)  && (canvas->State != STATE_NONE)) {
						break;
					}
					if (CurrentError > 2.55)
						rval = 255;
					else
						rval = (unsigned char) (CurrentError * 100);
					if (canvas->State == STATE_NONE) rval = 0; // Idle -- let Neb free up
					wxLogStatus(_T("Sending pixel error of %.2f"),(float) rval / 100.0);
					break;
				case MSG_AUTOFINDSTAR:
//				case 'f':
					wxCommandEvent *tmp_evt;
					tmp_evt = new wxCommandEvent(0,wxID_EXECUTE);
					ival = canvas->State; // save state going in
					canvas->State = STATE_NONE;
					OnAutoStar(*tmp_evt);
					if (StarX + StarY)  // found a star, so reset the state
					{
						if( ival == STATE_NONE )
							canvas->State = STATE_SELECTED;
						else
							canvas->State = ival;
						rval = 1;
					}
					delete tmp_evt;
					break;
				case MSG_SETLOCKPOSITION:
                case 's':
                    // Sets LockX and LockY to be user-specified
                    unsigned short x,y;
                    Paused = true;
                    sock->Read(&x, 2);
                    sock->Read(&y, 2);
                    wxLogStatus(wxString::Format("Lock set to %d,%d",x,y));
                    sock->Discard();  // Clean out anything else
                    StarX=x;
                    StarY=y;
                    dX = dY = 0.0;
                    canvas->State=STATE_SELECTED;
                    FindStar(CurrentFullFrame);
                    LockX = StarX;
                    LockY = StarY;
                    Paused = false;
					break;
				case MSG_FLIPRACAL:
					{
						wxCommandEvent *tmp_evt;
						tmp_evt = new wxCommandEvent(0,wxID_EXECUTE);
						ival = canvas->State; // save state going in
						canvas->State = STATE_NONE;
						// return 1 for success, 0 for failure
						rval = FlipRACal(*tmp_evt) ? 1 : 0;
						canvas->State = ival;
						delete tmp_evt;
					}
					break;
				case MSG_GETSTATUS:
					if( Paused )
						rval = 100;
					else
						rval = canvas->State;
					break;
				case MSG_LOOP:
					{
						wxCommandEvent *tmp_evt;
						tmp_evt = new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, BUTTON_LOOP);
						QueueEvent(tmp_evt);
					}
					break;
				case MSG_STOP:
					Abort = 1;
					break;
				case MSG_STARTGUIDING:
					{
						wxCommandEvent *tmp_evt;
						tmp_evt = new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, BUTTON_GUIDE);
						QueueEvent(tmp_evt);
					}
					break;
				default:
					wxLogStatus(_T("Unknown test id received from client: %d"),(int) c);
					rval = 1;
			}
			sock->Write(&rval,1);
			// Enable input events again.
			sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
			break;
		}
		case wxSOCKET_LOST: {
			SocketConnections--;
			wxLogStatus(_T("Deleting socket"));
			sock->Destroy();
			break;
		}
		default: ;
	}

}

bool ServerSendGuideCommand (int direction, int duration) {
	// Sends a guide command to Nebulosity
	if (!SocketServer || !SocketConnections)
		return true;

	unsigned char cmd = MSG_GUIDE;
	unsigned char rval = 0;
	wxLogStatus(_T("Sending guide: %d %d"), direction, duration);
//	cmd = 'Z';
	ServerEndpoint->Write(&cmd, 1);
	if (SocketServer->Error())
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
	if (!SocketServer || !SocketConnections)
		return true;
	wxLogStatus(_T("Sending cam connect request"));
	unsigned char cmd = MSG_CAMCONNECT;
	unsigned char rval = 0;

	ServerEndpoint->Write(&cmd, 1);
	if (SocketServer->Error()) {
		wxLogStatus(_T("Error sending Neb command"));
		return true;
	}
	else {
//		unsigned char c;
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
	if (!SocketServer || !SocketConnections)
		return true;

	wxLogStatus(_T("Sending cam disconnect request"));
	unsigned char cmd = MSG_CAMDISCONNECT;
	unsigned char rval = 0;

	ServerEndpoint->Write(&cmd, 1);
	if (SocketServer->Error()) {
		wxLogStatus(_T("Error sending Neb command"));
		return true;
	}
	else {
//		unsigned char c;
		ServerEndpoint->Read(&rval, 1);
		wxLogStatus(_T("Cmd done - returned %d"), (int) rval);
	}
	if (rval)
		return true;
	else
		return false;  // cam disconnected OK
}

bool ServerReqFrame(int duration, usImage& img) {
	if (!SocketServer || !SocketConnections)
		return true;
	wxLogStatus(_T("Sending guide frame request"));
	unsigned char cmd = MSG_REQFRAME;
	unsigned char rval = 0;

	ServerEndpoint->Write(&cmd, 1);
	if (SocketServer->Error()) {
		wxLogStatus(_T("Error sending Neb command"));
		return true;
	}
	else {
//		unsigned char c;
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

//		ServerEndpoint->ReadMsg(img.ImageData,(xsize * ysize * 2));
		wxLogStatus(_T("Frame read"));
		return false;
	}

	return false;
}
