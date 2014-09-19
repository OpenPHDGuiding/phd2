/*
 * File: openssag_priv.h
 *
 * Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
 *
 */

#ifndef __OPENSSAG_PRIV_H_
#define __OPENSSAG_PRIV_H_ 

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <libusb.h>

#ifdef __WIN32__
#   include "windows.h"
#   define sleep(n) Sleep(1000 * n)
#endif

#include <cassert>

#if DEBUG
#   define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#   define DBG(...)
#endif

/* Opens a usb_dev_handle based on the vendor id and product id */
static inline int usb_open_device(libusb_device_handle **device, int vendorID, int productId, const char *serial)
{
    libusb_device **list = 0;
    
    libusb_device_handle *handle = 0;

    ssize_t cnt = libusb_get_device_list(NULL, &list);
    if (cnt < 0)
    {
        DBG("No USB device found.");
        return 0;
    }

    for (ssize_t i = 0; i < cnt; i++) 
    {
        assert(handle == 0);
        libusb_device *device = list[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0)
        {
            DBG("Device description querying failed for device %d.", i);
            continue;
        }
        
        if (desc.idVendor == vendorID && desc.idProduct == productId) 
        {
            r = libusb_open(device, &handle);
            if(r < 0 || handle == 0)
            {
                DBG("Device #%d opening error (serial %d).", i, desc.iSerialNumber);
                continue;
            }
            
            if (serial == NULL) 
            {
                goto havedevice;
            }

            char devserial[256];
            
            /* Copy serial */
            int len = libusb_get_string_descriptor_ascii(
                    handle, 
                    desc.iSerialNumber & 0xff, 
                    (unsigned char*)devserial, 
                    (int) sizeof(devserial));
            if (len < 0)
            {
                DBG("Device open failed: cannot get the serial from the handle.");
                libusb_close(handle);
                handle = 0;
                continue;
            }
            
            
            
            if ((len > 0) && (strcmp(devserial, serial) == 0)) {
                goto havedevice;
            }

            libusb_close(handle);
            handle = 0;
        }
    }
    
    libusb_free_device_list(list, 1);
    return 0;
    
    
    
havedevice:
    libusb_set_configuration(handle, 1);
    int r = libusb_claim_interface(handle, 0);
    if (r != 0) 
    {
      DBG("USB error in claiming the interface");
    }    
    
    
    *device = handle;
    return 1;

/*

    usb_find_busses();
    usb_find_devices();
    




    
    

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == vendorID &&
                    dev->descriptor.idProduct == productId) {
                handle = usb_open(dev);
                if (handle) 
                {
                    if (serial == NULL) {
                        goto havedevice;
                    }
                    else 
                    {
                        char devserial[256];
                        int len = 0;
                        if (dev->descriptor.iSerialNumber > 0) {
                            len = usb_get_string_simple(handle, dev->descriptor.iSerialNumber, devserial, sizeof(devserial));
                        }
                        if ((len > 0) && (strcmp(devserial, serial) == 0)) {
                            goto havedevice;
                        }
                    }
                }
            }
        }
    }
    return 0;
havedevice:
    usb_set_configuration(handle, 1);
    usb_claim_interface(handle, 0);
    
*/

/*** JB--- "Function not implemented" so don't call it?
     libusb Developers Guide says, "Implemented on Linux only."
     Why is #ifdef=TRUE, if it's not implemented (and where is it getting set true)?
     KWIQGuider will always fail (may crash) to connect with this uncommented (even though it may say it succeeded???)***/
/*
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
    if (usb_detach_kernel_driver_np(handle, 0) < 0) {
        fprintf(stderr, "Warning: Could not detach kernel driver: %s\nYou may need to run this as root or add yourself to the usb group\n", usb_strerror());
        return 0;
    }
#endif
*/

//    *device = handle;
//    return 1;
}


#endif /* __OPENSSAG_PRIV_H_ */
