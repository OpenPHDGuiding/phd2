/*
 *  socket_server.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
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

wxSocketBase *ServerEndpoint;
wxLogWindow *SocketLog = NULL;

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
	MSG_MOVE5
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
				case MSG_PAUSE: Paused = true;
					wxLogStatus(_T("Paused"));
					break;
				case MSG_RESUME: Paused = false;
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
					rx = (float) (rand() % 1000) / 1000.0 * size - (size / 2.0);
					ry = (float) (rand() % 1000) / 1000.0 * size - (size / 2.0);
					if (DitherRAOnly) {
						if (fabs(tan(RA_angle)) > 1) 
							rx = ry / tan(RA_angle);
						else	
							ry = tan(RA_angle) * rx;
					}
					LockX = LockX + rx;
					LockY = LockY + ry;
					wxLogStatus(_T("Moving by %.2f,%.2f"),rx,ry);
					if (ExpDur < 1000)
						rval = 1;
					else
						rval = ExpDur / 1000;
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


wxSocketClient *VoyagerClient;

bool MyFrame::Voyager_Connect() {
	wxIPV4address addr;

	addr.Hostname(_T("localhost"));
	addr.Service(4030);

	VoyagerClient = new wxSocketClient();
	// Setup the event handler and subscribe to most events
	VoyagerClient->SetEventHandler(*this, VOYAGER_ID);
	VoyagerClient->SetNotify(wxSOCKET_CONNECTION_FLAG |
							wxSOCKET_INPUT_FLAG |
							wxSOCKET_LOST_FLAG);
	VoyagerClient->Notify(true);

	VoyagerClient->Connect(addr, false);
	VoyagerClient->WaitOnConnect(5);
	//	SocketClient->SetFlags(wxSOCKET_BLOCK);

	if (VoyagerClient->IsConnected()) {
		if (!SocketLog) {
			SocketLog = new wxLogWindow(this,wxT("Server log"));
			SocketLog->SetVerbose(true);
			wxLog::SetActiveTarget(SocketLog);
		}
		wxMessageBox(_T("Connection established"));
//		StatusText->SetLabel("Connection OK");
		wxLogStatus(_T("Connection to Voyager OK"));
		return false;
	}
	else {
		// Localhost failed, try asking for IP
		VoyagerClient->Close();
		wxString IPstr = wxGetTextFromUser(_T("Enter IP address"),_T("Voyager not found on localhost"));
		addr.Hostname(IPstr);
		addr.Service(4030);
		VoyagerClient->Connect(addr,false);
		VoyagerClient->WaitOnConnect(5);
		if (VoyagerClient->IsConnected()) {
			if (!SocketLog) {
				SocketLog = new wxLogWindow(this,wxT("Server log"));
				SocketLog->SetVerbose(true);
				wxLog::SetActiveTarget(SocketLog);
			}
			wxMessageBox(_T("Connection established"));
			//		StatusText->SetLabel("Connection OK");
			wxLogStatus(_T("Connection to Voyager OK"));
			return false;
		}
		else {
			VoyagerClient->Close();
			wxLogStatus(_T("Failed to connect to Voyager"));
			delete VoyagerClient;
			VoyagerClient = NULL;
	//		StatusText->SetLabel("Connection failed");
			wxMessageBox(_T("Can't connect to the specified host"), _T("Alert !"));
			return true;
		}
	}

	return false;
}

void MyFrame::OnVoyagerEvent(wxSocketEvent& event) {
	// may not need this since we never get direct input from Voyager -- here as a placeholder

	switch(event.GetSocketEvent())	{
		case wxSOCKET_INPUT      :
			VoyagerClient->SetNotify(wxSOCKET_LOST_FLAG);  // Disable input for now

/*			// Which command is coming in?
			unsigned char cmd, rval;
			SocketClient->Read(&cmd, 1);
			wxLogStatus(wxString::Format("Msg %d",(int) cmd));
			SocketLog->FlushActive();
			rval = 1; // assume error
			switch (cmd) {
				default:
					wxLogStatus("Unknown msg %d", (int) cmd);
					SocketClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
			}
*/
				// Enable input events again.
			VoyagerClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
			break;
		case wxSOCKET_LOST       :
			wxLogStatus(_T("Lost link"));
//			StatusText->SetLabel("No connection");
			break;
		case wxSOCKET_CONNECTION : wxLogStatus(_T("Connect evt")); break;
		default                  : wxLogStatus(_T("Unexpected even")); break;
	}

}

void Voyager_PulseGuideScope (int direction, int duration) {
	if (!VoyagerClient) return;
	if (!VoyagerClient->IsConnected())
		return;
	// Disable input notification
	VoyagerClient->SetNotify(wxSOCKET_LOST_FLAG);  // Disable input for now
	VoyagerClient->SetTimeout(2); // set to 2s timeout
	char msg[64], dir;
//	int bytes_xfered;
	sprintf(msg,"RATE 100\n\n");
	VoyagerClient->Write(msg,strlen(msg));
	wxLogStatus(wxString(msg, wxConvUTF8));
	VoyagerClient->Read(&msg,10);
	wxLogStatus(wxString(msg, wxConvUTF8));
	if (strstr(msg,"ERROR"))
		return;
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
	VoyagerClient->Write(msg,strlen(msg));
	wxLogStatus(wxString(msg, wxConvUTF8));
	VoyagerClient->Read(&msg,10);
	wxLogStatus(wxString(msg, wxConvUTF8));
	wxMilliSleep(duration);
	sprintf(msg,"STOP %c\n\n",dir);
	VoyagerClient->Write(msg,strlen(msg));
	wxLogStatus(wxString(msg, wxConvUTF8));
	VoyagerClient->Read(&msg,10);
	wxLogStatus(wxString(msg, wxConvUTF8));

	// Re-enable input
	VoyagerClient->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);

}


#ifdef __APPLE__
//#include <AppleEvents.h>
//#include <AEDataModel.h>

// Code originally from Darryl @ Equinox
AppleEvent E6Return;
AppleEvent E6Event;
SInt16 E6ReturnCode;

OSErr E6AESendRoutine(double ewCorrection, double nsCorrection) {
// correction values (+- seconds) to send to E6

	OSErr err;
	FourCharCode E6Sig = 'MPj6';  // the Equinox 6 creator signature
	FourCharCode phdSig = 'PhDG';  // ***** you need to fill in your app signature here ******
	AEAddressDesc addDesc;

	AEEventClass evClass = 'phdG';  // the phd guide class.
	AEEventID evID = 'evGD';  // the phd guide event.
	AEKeyword  keyObject;
	AESendMode mode = kAEWaitReply;  //  you want something back

	// create Apple Event with Equinox 6 signature

	err = AECreateDesc( typeApplSignature, (Ptr) &E6Sig, sizeof(FourCharCode), &addDesc );  // make a description
	if( err != noErr ) return err;

	err = AECreateAppleEvent( evClass, evID, &addDesc, kAutoGenerateReturnID, kAnyTransactionID, &E6Event );  // create the AE
	if( err != noErr ) {
		AEDisposeDesc( &addDesc );
		return err;
	}

	// create the return Apple Event with your signature (so I know where to send it)

	err = AECreateDesc( typeApplSignature, (Ptr) &phdSig, sizeof(FourCharCode), &addDesc );
	if( err != noErr ) {
		AEDisposeDesc( &E6Event );
		return err;
	}

	err = AECreateAppleEvent( evClass, evID, &addDesc, kAutoGenerateReturnID, kAnyTransactionID, &E6Return );
	if( err != noErr ) {
		AEDisposeDesc( &E6Event );
		AEDisposeDesc( &addDesc );
		return err;
	}

	// put the correction values into parameters - I have used doubles for a ew and ns seconds correction

	keyObject = 'prEW';  // EW correction AE parameter (+ = east, - = west)
	err = AEPutParamPtr( &E6Event, keyObject, typeIEEE64BitFloatingPoint,  &ewCorrection, sizeof(double) );
	if( err != noErr ) {
		AEDisposeDesc( &E6Event );
		AEDisposeDesc( &addDesc );
		return err;
	}

	keyObject = 'prNS';  // NS correction AE parameter (+ = north, - = south)
	err = AEPutParamPtr( &E6Event, keyObject, typeIEEE64BitFloatingPoint, &nsCorrection, sizeof(double) );
	if( err != noErr ) {
		AEDisposeDesc( &E6Event );
		AEDisposeDesc( &addDesc );
		return err;
	}

	// you now have the send AE, the return AE and the correction values in AE parameters - so send it!

	err = AESendMessage( &E6Event, &E6Return, mode, kAEDefaultTimeout );  // you can specify a wait time (in ticks)
	if( err != noErr ) {  // Note: an error of -600 means E6 is not currently running
		AEDisposeDesc( &E6Event );
		AEDisposeDesc( &addDesc );
		return err;
	}

	// at this point you have received the return AE from E6, so go read the return code (what do you want returned ??)
	// the return code could indicate that E6 got the AE, can do it, or can't for some reason. You do NOT want to wait
	// until the corrections have been applied - you should time that on your own.

	keyObject = 'prRC';  // get return code
	Size returnSize;
	DescType returnType;
	err = AEGetParamPtr( &E6Return, keyObject, typeSInt16, &returnType, &E6ReturnCode, sizeof(SInt16), &returnSize );

	AEDisposeDesc( &E6Event );
	AEDisposeDesc( &addDesc );
	return 0;
}

bool Equinox_Connect() {
	// Check the E6 connection by sending 0,0 to it and checking E6Return
	OSErr err = E6AESendRoutine(0.0,0.0);
	if (E6ReturnCode == -1) {
		wxMessageBox ("E6 responded it's not connected to a mount");
		return true;
	}
	else if (err == -600) {
		wxMessageBox ("E6 not running");
		return true;
	}


	return false;
}

void Equinox_PulseGuideScope (int direction, int duration) {

	double NSTime = 0.0;
	double EWTime = 0.0;

	switch (direction) {
		case NORTH:
			NSTime = (double) duration / 1000.0;
			break;
		case SOUTH:
			NSTime = (double) duration / -1000.0;
			break;
		case EAST:
			EWTime = (double) duration / 1000.0;
			break;
		case WEST:
			EWTime = (double) duration / -1000.0;
			break;
	}
	OSErr err = E6AESendRoutine(EWTime, NSTime);
	if (E6ReturnCode == -1) {
		wxMessageBox ("E6 responded it's not connected to a mount");
		return;
	}
	else if (err == -600) {
		wxMessageBox ("E6 not running");
		return;
	}
	wxMilliSleep(duration);
}

#endif

