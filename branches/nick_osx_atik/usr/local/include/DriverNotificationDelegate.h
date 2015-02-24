//
//  DriverNotificationDelegate.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 18/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_DriverNotificationDelegate_h
#define ATIKOSXDrivers_DriverNotificationDelegate_h

#include <string>

class DriverNotificationDelegate {

public:
    // the given device has connected
    virtual void deviceConnected(std::string identity)=0;

    // the given device has disconnected
    virtual void deviceDisconnected(std::string identity) =0;

};

#endif
