//
//  ImageFrame.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 18/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef ATIKOSXDrivers_ImageFrame_h
#define ATIKOSXDrivers_ImageFrame_h

#include <iostream>

class ImageFrame {
    void*       imageFrame;
    
    uint32_t     _binningX;
    uint32_t     _binningY;
    uint32_t    _width;
    uint32_t    _height;
    uint32_t      _cameraFrameOriginX;
    uint32_t      _cameraFrameOriginY;
    uint32_t      _cameraFrameSizeX;
    uint32_t      _cameraFrameSizeY;
    float       _duration;
    bool        _preview;
    bool        _subsampled;
    bool        _oversampled;
    
    uint32_t _imagebufferSize;
    uint8_t *_imagebuffer;

public:
    ImageFrame(uint32_t size, char *fileURL);
    ~ImageFrame();
    
    // read properties
    void        binning(uint32_t *binX, uint32_t *binY);
    uint32_t    width();
    uint32_t    height();
    void        cameraFrame(uint32_t *originX, uint32_t *originY,
                            uint32_t *sizeWidth, uint32_t *sizeHeight);
    float       duration();
    bool        isPreview();
    bool        isSubsampled();
    bool        isOversampled();
    
    uint32_t    imagebufferSize();
    uint8_t*    imagebuffer();
    
    // set properties
    void        setBinning(uint32_t binX, uint32_t binY);
    void        setWidth(uint32_t newWidth);
    void        setHeight(uint32_t newHeight);
    void        setCameraFrame(uint32_t originX, uint32_t originY,
                            uint32_t sizeWidth, uint32_t sizeHeight);
    void        setDuration(float durationInSeconds);
    void        setIsPreview(bool enabled);
    void        setIsSubsampled(bool enabled);
    void        setIsOversampled(bool enabled);
    
    // note that the self allocated memory buffer is released automatically
    // if this object is garbage collected.
    void selfAllocate();
    void explicitReleaseSelfAllocated(); // explicitly release self allocated memory, normally it can be left to delete.
    void setExternalBuffer(uint8_t* buffer); // or use this if you want to load directly into your own buffer (must be at least buffersize in size)
    
    // internal use
    void* obj();
    
};

#endif
