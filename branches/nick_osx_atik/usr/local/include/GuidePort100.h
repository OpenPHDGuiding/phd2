//
//  GuidePort100.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 19/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_GuidePort100_h
#define ATIKOSXDrivers_GuidePort100_h

class GuidePort100 {
    
public:
    virtual ~GuidePort100() {;}
    
    virtual bool hasGuidePort()=0;
    virtual int stopGuiding()=0;
    virtual int guide(long direction)=0; // Direction: 0,1,2,3=N,S,E,W
    virtual int guidePort(int nibble)=0; // nibble: bit 1=N, bit 2=S, bit 3=E, bit 4=W
    virtual int pulseGuide(long direction, long duration)=0; // Direction: 0,1,2,3=N,S,E,W
};

#endif
