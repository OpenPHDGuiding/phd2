//
//  StateObserver.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 19/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_StateObserver_h
#define ATIKOSXDrivers_StateObserver_h
class StateObserver {
public:
    // called when the a state changes
    virtual void setState(const char* newState)=0;
    
};

#endif
