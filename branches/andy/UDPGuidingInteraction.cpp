//
//  matlab_interaction.cpp
//  PHD
//
//  Created by Stephan Wenninger
//  Copyright 2014, Max Planck Society.

/*
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
 */


#include "UDPGuidingInteraction.h"
#include "phd.h"


#include <iostream>



#include "wx/string.h"
#include "wx/socket.h"


UDPGuidingInteraction::UDPGuidingInteraction(wxString host,
                                             wxString sendPort,
                                             wxString rcvPort)
: host(host),
  sendPort(sendPort),
  rcvPort(rcvPort),
  server(),
  sendClient(),
  receiveClient() {
    // Configure Sending
    sendClient.Hostname("localhost");     // configures client to local host...
    sendClient.Service(0);

    /* 
     * If these don´t work, we get uninitialized sendSocket which will then 
     * crash when using SendToUDPPort 
     */
    if (server.Hostname(host)) {             // validates destination host using DNS
        if (server.Service(sendPort)) {      // ensure port is valid
            sendSocket = new wxDatagramSocket(sendClient,wxSOCKET_NONE); // define the local port
            if(!sendSocket->IsOk())
            {
              ERROR_INFO("[UDP] the send socket is not valid");
            }
            sendSocket->SetTimeout(1);
        }
    }

    // Configure Receiving
    receiveClient.AnyAddress();     // configures client to local host...
    receiveClient.Service(rcvPort);

    receiveSocket = new wxDatagramSocket(receiveClient,wxSOCKET_NONE); // define the local port

    if(!receiveSocket->IsOk())
    {
      ERROR_INFO("[UDP] the reception socket is not valid");
    }
    receiveSocket->SetTimeout(1);
}

UDPGuidingInteraction::~UDPGuidingInteraction() {
    sendSocket->Destroy();
    receiveSocket->Destroy();
}


bool UDPGuidingInteraction::SendToUDPPort(void *buf, wxUint32 len) {
    while (!sendSocket->WaitForWrite()) {
        LOG_INFO("Socket not ready to write!");
    }
    sendSocket->SendTo(server, buf, len);

    // Store the buffer in order to be able to resend it.
    // NOTE: This only works if the buffer is not deleted by the caller
    // after sending it.
    this->last_sent_buffer = buf;
    this->last_sent_buffer_length = len;
    
    return !sendSocket->Error();
}

bool UDPGuidingInteraction::ReceiveFromUDPPort(void * buf, wxUint32 len) {
    while (!receiveSocket->WaitForRead()) {
        LOG_INFO("Socket not ready to read from, resending last buffer");
        SendToUDPPort(this->last_sent_buffer, this->last_sent_buffer_length);
    }
    receiveSocket->RecvFrom(receiveClient, buf, len);
    return !receiveSocket->Error();
}
