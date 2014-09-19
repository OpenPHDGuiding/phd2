//
//  matlab_interaction.h
//  PHD
//
//  Created by Stephan Wenninger on 14/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#ifndef __PHD__matlab_interaction__
#define __PHD__matlab_interaction__

#include "wx/defs.h"
class wxString;


class MatlabInteraction {
    
public:
    static bool sendToUDPPort(wxString host, wxString port, const void * buf, wxUint32 len);
	static bool receiveFromUDPPort(wxString port, void * buf);
};

#endif /* defined(__PHD__matlab_interaction__) */
