//
//  BehaviourProxyFactory.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 28/01/2015.
//  Copyright (c) 2015 ATIK. All rights reserved.
//
//  Factory access for object decorations to change behavour for applications.
//

#ifndef __ATIKOSXDrivers__BehaviourProxyFactory__
#define __ATIKOSXDrivers__BehaviourProxyFactory__

#include "Imager100.h"
#include "Imager101.h"
#include "FilterWheel100.h"

class BehaviourProxyFactory {
public:    
    //
    // Takes an already claimed service and adds a threaded active proxy
    // Use destroyActiveProxy to destroy the active proxy before releasing the service back to the driver.
    
    // creates active proxy for cameras
    virtual void createActiveCameraProxy(Imager100* existingCameraService,
                                 Imager101** outputActiveCameraProxy)=0;
    
    // creates a camera active proxy for cameras with embedded filterwheels.
    virtual void createActiveCameraProxyWithEmbededFilterWheel(Imager100*       existingCameraService,
                                                       FilterWheel100*  existingFilterWheelService,
                                                       Imager101**       outputActiveCameraProxy,
                                                       FilterWheel100**  outputActiveFilterWheelProxy)=0;
    
    //
    virtual void createActiveFilterProxy(FilterWheel100* existingFilterWheelService,
                                         FilterWheel100** outputActiveFilterWheelProxy)=0;
    
    // This shutsdown and destroys the active proxy but does not destroy the underlying service.
    virtual void destroyActiveProxy(ServiceInterface* proxy)=0;
    
};

#endif /* defined(__ATIKOSXDrivers__BehaviourProxyFactory__) */
