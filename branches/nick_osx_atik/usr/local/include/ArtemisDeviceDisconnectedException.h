//
//  ArtemisDeviceDisconnectedException.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 09/04/2013.
//  Copyright (c) 2013 ATIK. All rights reserved.
//

#ifndef __ATIKOSXDrivers__ArtemisDeviceDisconnectedException__
#define __ATIKOSXDrivers__ArtemisDeviceDisconnectedException__
extern "C++" {
//#include <stdlib.h>
#include <iostream>
#include <string>

#include <stdexcept>
//using namespace std;

// : public std::runtime_error 
class ArtemisDeviceDisconnectedException {
public:
    ArtemisDeviceDisconnectedException(const char *what /*string& what*/, const int line);
};

};

#endif /* defined(__ATIKOSXDrivers__ArtemisDeviceDisconnectedException__) */
