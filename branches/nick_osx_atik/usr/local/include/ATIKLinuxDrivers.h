//
//  ATIKLinuxDrivers.h
//  ATIKOSXDrivers
//
//  Created by Nick Kitchener on 17/12/2014.
//  Copyright (c) 2014 ATIK. All rights reserved.
//

#ifndef __ATIKOSXDrivers__ATIKLinuxDrivers__
#define __ATIKOSXDrivers__ATIKLinuxDrivers__

#include <iostream>
#include <map>

#include <libusb.h>

#include "DriverNotificationDelegate.h"
#include "Services.h"
#include "ATIKLinuxDrivers_DriverListManagement.h"
#include "BehaviourProxyFactory.h"

class ATIKLinuxDrivers : public virtual Services, public virtual BehaviourProxyFactory {
    libusb_context*                         libUsbContext;
    bool                                    _synchronousConnectOnly;
    bool                                    _useActiveProxies;
    DriverNotificationDelegate*             _notificationDelegate;
    ATIKLinuxDrivers_DriverListManagement*  _serviceList;
    std::map<uint32_t,int>                  _callbackMap;
    
public:
    // return driver version string
    const char*     version();
    const double    numericVersion();
    
    void setEnableDebug(bool enabled); // indirect Logger:setDebuggingEnabled() if you have a class called Logger like OpenPHD.
    
    // Initialise the object, however this will not automatically start USB support.
    ATIKLinuxDrivers();
    ~ATIKLinuxDrivers();
    
    // Set the delegate for driver notifications
    // optional to be called, note delegate entry may be on same or parallel seperate threads.
    void setNotificationDelegate(DriverNotificationDelegate* delegate);
    
    // set the driver to not use dispatch concurrent connect strategy
    void setSynchronousConnectOnly();
    
    // start driver support for existing and listening for new connects/disconnects
    // HID (ie filterwheel) will require execution of the runloop (due to OSX)
    // this adds ALL supported cameras - thus any supported ATIK camera attached or subsequently hotplugged will be initialsed and made available.
    // This is effectively full management by the driver.
    void startSupport(); // must occur and allow execution on a run loop (due to OSX HID management) before services are used.
    
    // Alternative to the fully automated startSupport
    // Where no camera detction is used, instead supportCamera commands need to be used - see below.
    void startSelectiveSupport(); // use this once to start selective support - then use the two operations below to add specific drivers
    
    // identifies the camera at the busAddress and if supported will start support for that single camera, returns false if camera not supported
    // this is NOT HOTPLUG so you will manage the connect/disconnect using the corresponding remove method.
    bool supportCameraIdentifiedByBusAddress(uint16_t busAddress);
    bool removeSupportCameraIdentifiedByBusAddress(uint16_t busAddress); // stops the driver support for a selective support device - also removes from services.

    // identifies the filterwheel at the usb address and adds if supported, returns false if not supported.
    // NOT HOTPLUG.
    bool supportFilterWheelIdentifiedByBusAddress(uint16_t usbBusAddress);
    bool removeSupportFilterWheelIdentifiedByBusAddress(uint16_t usbBusAddress); // stops driver support for bus address and removes from available services.

    // adds VID/PID support as a hot plug fashion
    // HOTPLUG - causing the detected cameras to automatically be initialised and notifications to be issued.
    // Note this may detect more than one camera.
    bool supportCameraIdentifiedByPID(uint16_t pid);
    
    // stops the driver listening for device connect/discconect events, also required for selective support.
    // all claimed services should be released before this call.
    // attempting to use services after this call is not supported and behaviour is undefined.
    void endSupport();
    
    // selective mode passive scanning
    // this uses passive matching scan against all the USB devices in the system using their VID/PID but not touching the camera
    // if the USBs match then they're returned in a map with the format:
    //      busAddress -> modelString
    // this allows applications that cannot work with the driver to manually poll the
    // bus looking for possible cameras and then use the supportCameraIdentifiedByBusAddress
    // to open the camera and make it available within the available services.
    // Use the kATIKModelModern<name> constants or
    // Use kATIKModelModernAnyCamera to return all connected modern cameras that are supported
    // or kATIKModelModernAnyFilterWheel to return all connected supported filterwheels.
    PotentialDeviceList* scanForPerspectiveServices(std::string modelMatch);

    
    // the following works for both automatic and selective modes.
    
    // return service management interface, on which you can claim/release devices.
    // Caller does not need to release the service management interface after calling.
    Services* serviceManagement();
    
    
    // will return if the camera drivers know the case senstive model string.
    // the driver does not need to be instantiated nor does the camera be connected.
    static bool isModelSupportedByDriver(std::string _modelString);
    
    // Services interface
    ServiceIdentityList* availableServices();
    ServiceInterface* claimService(std::string serviceIdentifier);
    void releaseService(ServiceInterface* serviceInstance);
    
    // add active proxy behaviour factory access (Just realised - english spelling)
    BehaviourProxyFactory*  behaviourFactory();
    
    // pseudo test devices - useful for application testing without a camera connected
    // Not currently available on C++ libraries
    void addTestProxyInstance(char* identity, char* type); // add a pseudo test device
    void removeTestProxyInstance(char* identity, char* type); // remove a pseudo test device

    
    
    
    
public: // internal (these may move in future)
    // hotplug callback
    void deviceConnected(struct libusb_context *ctx, struct libusb_device *dev);
    void deviceDisconnected(struct libusb_context *ctx, struct libusb_device *dev);
    
    // hid callback
    void hidAttach(libusb_device* device);
    void hidDetach(libusb_device* device);
    
    // don't use these directly - use behaviourFactory() to access as these defintions may move..
    void createActiveCameraProxy(Imager100* existingCameraService,Imager101** outputActiveCameraProxy);
    void createActiveCameraProxyWithEmbededFilterWheel(Imager100* existingCameraService, FilterWheel100* existingFilterWheelService, Imager101** outputActiveCameraProxy, FilterWheel100** outputActiveFilterWheelProxy);
    void createActiveFilterProxy(FilterWheel100* existingFilterWheelService, FilterWheel100** outputActiveFilterWheelProxy);
    void destroyActiveProxy(ServiceInterface* proxy);
    
private:
    uint16_t findBusAddress(struct libusb_device *dev);
    
    void setUpUSBHooks();
    void clearUSBHooks();
    void addDeviceHookForVendorProductBCD(uint16_t usbVendor, uint16_t usbProduct, uint16_t usbBcdNumber);
    void addDeviceHookForBusAddress(uint16_t usbBusAddress);
    void usbDeviceList();
    void* buildDriver(struct libusb_device *device);
    
    void setUpHIDHooks();
    void clearHIDHooks();
    void listHIDDevices();
    void addHIDHookForVendorProductBCD(uint16_t usbVendor, uint16_t usbProduct, uint16_t usbBcdNumber);
    void* buildHIDDriver(libusb_device* device);
    
};

#endif /* defined(__ATIKOSXDrivers__ATIKLinuxDrivers__) */
