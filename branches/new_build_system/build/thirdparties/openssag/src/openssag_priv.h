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
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
    if (usb_detach_kernel_driver_np(handle, 0) < 0) {
        fprintf(stderr, "Warning: Could not detach kernel driver: %s\nYou may need to run this as root or add yourself to the usb group\n", usb_strerror());
        return 0;
    }
#endif
    *device = handle;
    return 1;
}

#if DEBUG
#   define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#   define DBG(...)
#endif

#endif /* __OPENSSAG_PRIV_H_ */
