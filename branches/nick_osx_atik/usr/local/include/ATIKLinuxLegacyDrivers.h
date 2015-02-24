//
//  ATIKLinuxLegacyDrivers.h
//  ATIKOSXLegacyDrivers
//
//  Created by Nick Kitchener on 18/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef __ATIKOSXLegacyDrivers__ATIKLinuxLegacyDrivers__
#define __ATIKOSXLegacyDrivers__ATIKLinuxLegacyDrivers__

#include <iostream>
#include <map>

extern "C" {
#include <libusb.h>
}

#include "DriverNotificationDelegate.h"
#include "Services.h"
#include "ATIKLinuxDrivers_DriverListManagement.h"


class ATIKLinuxLegacyDrivers : public virtual Services {
    libusb_context*                         libUsbContext;
    bool                                    _synchronousConnectOnly;
    DriverNotificationDelegate*             _notificationDelegate;
    ATIKLinuxDrivers_DriverListManagement*  _serviceList;
    std::map<uint32_t,int>                  _callbackMap;
    std::map<std::string, uint32_t>              ftdiChipIds; // serial number, chipid
    
public:
    // Initialise the object, however this will not automatically start USB support.
    ATIKLinuxLegacyDrivers();
    ~ATIKLinuxLegacyDrivers();

    // return driver version string
    const char*     version();
    const double    numericVersion();
  
    // To a serial number to camera FTDIChipId mapping for the driver to use
    void registerFTDIChipId(uint32_t chipId, char* usbSerialNumber);
    
    // Set the delegate for driver notifications
    // optional to be called, note delegate entry may be on same or parallel seperate threads.
    void setNotificationDelegate(DriverNotificationDelegate* delegate);
    
    // start driver support for existing and listening for new connects/disconnects
    // HID (ie filterwheel) will require execution of the runloop (due to OSX)
    void startSupport(); // must occur and allow execution on a run loop (due to OSX HID management) before services are used.
    
    // Alternative of the fully automated startSupport where no camera detction is used, instead supportCamera defines specific cameras to support.
    void startSelectiveSupport(); // use this once to start selective support - then use the two operations below to add specific drivers
    bool supportCameraIdentifiedByBusAddress(uint16_t busAddress); // identifies the camera at the busAddress and if supported will start support for that single camera
    bool supportCameraIdentifiedByPID(uint16_t pid); // may return more than one camera if the same type is attached
    
    bool removeSupportCameraIdentifiedByBusAddress(uint16_t busAddress);
    
    // stops the driver listening for device connect/discconect events, also required for selective support.
    // the existing driver service instances will continue to function.
    void endSupport();
    
    
    // selective mode passive scanning
    // this uses passive matching scan against all the USB devices in the system
    // if the USBs match then they're returned in a map with the format:
    //      busAddress -> modelString
    // this allows applications that cannot work with the driver to manually poll the
    // bus looking for possible cameras and then use the supportCameraIdentifiedByBusAddress
    // to open the camera and make it available within the available services.
    // Use the kATIKModelLegacy<name> constants or
    // Use kATIKModelLegacyAnyCamera to return all connected legacy camera that are supported
    PotentialDeviceList* scanForPerspectiveServices(std::string modelMatch);
    
    // the following works for both automatic and selective modes.
    
    // return service management interface, on which you can claim/release devices.
    // Caller does not need to release the service management interface after calling.
    Services* serviceManagement();
    
    
    // will return if the camera drivers know the case senstive model string.
    // the driver does not need to be instantiated nor does the camera be connected.
    static bool isModelSupportedByDriver(std::string _modelString);


    
    // internal use only from this point - these will be removed later.
    
    // Services interface
    ServiceIdentityList* availableServices();
    ServiceInterface* claimService(std::string serviceIdentifier);
    void releaseService(ServiceInterface* serviceInstance);
    
    // hotplug callback
    void deviceConnected(struct libusb_context *ctx, struct libusb_device *dev);
    void deviceDisconnected(struct libusb_context *ctx, struct libusb_device *dev);
        
private:
    
    void setUpUSBHooks();
    void clearUSBHooks();
    void addDeviceHookForVendorProductBCD(uint16_t usbVendor, uint16_t usbProduct, uint16_t usbBcdNumber);
    void addDeviceHookForBusAddress(uint16_t usbBusAddress);
    void usbDeviceList();
    
    
    void* buildDriver(struct libusb_device *device);
    uint16_t findBusAddress(struct libusb_device *dev);

};

#endif /* defined(__ATIKOSXLegacyDrivers__ATIKLinuxLegacyDrivers__) */
