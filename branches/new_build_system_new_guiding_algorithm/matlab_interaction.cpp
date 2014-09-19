//
//  matlab_interaction.cpp
//  PHD
//
//  Created by Stephan Wenninger on 14/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#include "matlab_interaction.h"

#include "wx/string.h"
#include "wx/socket.h"


bool MatlabInteraction::sendToUDPPort(wxString host, wxString port, const void *buf, wxUint32 len)
{
    wxIPV4address server;
    wxIPV4address client;
    client.Hostname(_T("localhost"));               // configures client to local host...
    client.Service(_T("1708"));
    if (!server.Hostname(host))            // validates destination host using DNS
        return false;
    if (!server.Service(port))            // ensure port is valid
        return false;
    wxDatagramSocket udpSocket(client,wxSOCKET_BLOCK); // define the local port
    udpSocket.SendTo(server, buf, len);
    
    return !udpSocket.Error();
}

bool MatlabInteraction::receiveFromUDPPort(wxString port, void * buf)
{
    wxIPV4address client;
    client.AnyAddress();               // configures client to local host...
    client.Service(port);
    
    wxDatagramSocket  udpSocket(client,wxSOCKET_BLOCK); // define the local port
    udpSocket.RecvFrom(client,buf, 8);
    
    return !udpSocket.Error();
}