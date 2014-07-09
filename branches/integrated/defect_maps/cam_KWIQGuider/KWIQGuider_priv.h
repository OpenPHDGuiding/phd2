/*********************************************************************************************

 Copyright/Use header?

 *********************************************************************************************/

// Enable 'DBG' macro
//#define DEBUG 1

#ifndef __KWIQGUIDER_PRIV_H_
#define __KWIQGUIDER_PRIV_H_

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <usb.h>

#ifdef __WIN32__
#   include "windows.h"
#   define sleep(n) Sleep(1000 * n)
#endif

/* Opens a usb_dev_handle based on the vendor id and product id */
static inline int usb_open_device(usb_dev_handle **device, int vendorID, int productId, const char *serial)
{
    struct usb_bus *bus;
    struct usb_device *dev;
    struct usb_dev_handle *handle = NULL;

    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == vendorID &&
                    dev->descriptor.idProduct == productId) {
                handle = usb_open(dev);
                if (handle) {
                    if (serial == NULL) {
                        goto havedevice;
                    }
                    else {
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
    *device = handle;
    return 1;
}

#if DEBUG
#   define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#   define DBG(...)
#endif

#endif /* __KWIQGUIDER_PRIV_H_ */
