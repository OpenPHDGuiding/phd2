//
//  ServiceInterface.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 18/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef __ATIKOSXDrivers__ServiceInterface__
#define __ATIKOSXDrivers__ServiceInterface__

#include <iostream>
#include <string>

class ServiceInterface {
public:
    virtual ~ServiceInterface() {};
    
    // returns the protocol of the service
    // in the case of C++ that's a string protocol name that allows correct casting.
    // for objective-C++ that's a Protocol that allows detection of the protocol.
    // This is the latest protocol "Imager101" for example
    virtual std::string& serviceInterface()=0; // "Imager101"
    virtual std::string& serviceInterfaceSupportedProtocol()=0; // ie "Imager" without any numbers
    virtual uint16_t serviceInterfaceSupportedVersion()=0; // ie. 101
    
    // returns the unique idenitity of the service
    // This should be human readable.
    virtual std::string& uniqueIdentity()=0;
    
    // The model or type of device.
    virtual std::string& model()=0;
};

#endif /* defined(__ATIKOSXDrivers__ServiceInterface__) */
