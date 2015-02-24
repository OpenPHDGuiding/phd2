//
//  Imager101.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 15/01/2015.
//  Copyright (c) 2015 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_Imager101_h
#define ATIKOSXDrivers_Imager101_h

#include "Imager100.h"

class Imager101 : virtual public Imager100 {
public:
    virtual ~Imager101() {};
    
    // polling support for snapshot blocking=false
    // this is a fast return read of the current state and does not block
    virtual bool isSnapshotComplete() =0;

    
    
};


#endif
