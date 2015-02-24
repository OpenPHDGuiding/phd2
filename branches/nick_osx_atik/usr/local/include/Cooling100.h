//
//  Cooling100.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 19/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_Cooling100_h
#define ATIKOSXDrivers_Cooling100_h

class Cooling100 {
public:
    virtual ~Cooling100() {};
    
    virtual bool    hasSetpointCooling() =0;
    
    // on/off of cooling.
    virtual void enableCooling(bool enabled)=0;
    
    // Sets a new temperature, returning the existing setting (note - setting, not actual temp).
    virtual float setTemperature(float setting)=0;
    
    // reads the current temperature of the setpoint cooling
    virtual float temperature()=0;
    
    // initialises the camera warmup cycle - note that setting temps will not work during startup.
    virtual void startWarmUpCycle()=0;
};

#endif
