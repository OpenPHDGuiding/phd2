//
//  Imager100.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 18/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_Imager100_h
#define ATIKOSXDrivers_Imager100_h

#include "ImageFrame.h"
#include "ServiceInterface.h"
#include "Cooling100.h"
#include "GuidePort100.h"
#include "StateObserver.h"


const std::string kATIKProtocolNameImagerAnyVersion = std::string("Imager");


// Provides useful camera imager properties.
class ImagerProperties100 {
public:
    virtual ~ImagerProperties100() {};
    
    virtual uint32_t xPixels()=0; // total number of X pixels
    virtual uint32_t yPixels()=0; // total number of Y pixels
    
    virtual uint32_t maxBinX()=0; // maximum binning in X supported
    virtual uint32_t maxBinY()=0; // maximum binning in Y supported
    
    virtual float xPixelSize()=0; // pixel X size in microns*100, ie 740 would be 7.4um.
    virtual float yPixelSize()=0; // pixel Y size in microns*100, ie 740 would be 7.4um.
    
    // properties to show if these are supported.
    virtual bool isBinningSupported()=0;
    virtual bool isSubframingSupported()=0;
    virtual bool isPreviewSupported()=0;
    
    virtual bool isFIFOProgrammable()=0;
};

class Imager100 : virtual public ServiceInterface, virtual public ImagerProperties100 {
public:
    virtual ~Imager100() {};
    
    // This allows the camera to peform some validation checking and buffer
    // size allocation with the image frame.
    // This should be called each time a frame is used.
    virtual void prepare(ImageFrame* image)=0;
    
    // This allows the camera to perform any post processing required.
    // It should be called each time a frame snapshot have been completed.
    virtual void postProcess(ImageFrame* image)=0;
    
    // This takes an image
    virtual void snapShot(ImageFrame* image, bool shouldBlock)=0;
    
    // abort the current exposure, the exposure timer is aborted but download still occurs.
    virtual void abortCapture()=0;
    
    // return the cooling interface for the device
    virtual Cooling100* cooling()=0;
    
    // return the guide port for this imager
    virtual GuidePort100* guidePort()=0;
    
    virtual void setExtension(const char* extension, const char* value)=0;
    
    virtual const std::string state()=0;
    
    virtual void setStateObserver(StateObserver* observer)=0;
    
};

#endif
