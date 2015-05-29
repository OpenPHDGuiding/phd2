/*
 * File: openssag.cpp
 *
 * Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
 *
 */

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include <time.h>

#include "openssag.h"
#include "openssag_priv.h"

#ifdef __APPLE__
  #include <unistd.h>
#endif




/*
 * MT9M001 Pixel Array
 * 
 * |-----------------1312 Pixels------------------|
 *
 *    |--------------1289 Pixels---------------|
 *
 *       |-----------1280 Pixels------------|
 *
 * +----------------------------------------------+     -
 * |  Black Rows          8                       |     |
 * |  +----------------------------------------+  |     |               -
 * |  |  Padding          4                    |  |     |               |
 * |  |  +----------------------------------+  |  |     |               |               -
 * |  |  | SXGA                             |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * | 7| 5|                                  |4 |16|     | 1048 Pixels   | 1033 Pixels   | 1024 Pixels
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  |                                  |  |  |     |               |               |
 * |  |  +----------------------------------+  |  |     |               |               -
 * |  |                   5                    |  |     |               |
 * |  +----------------------------------------+  |     |               -
 * |                      7                       |     |
 * +----------------------------------------------+     -
 *
 *
 */

/* USB commands to control the camera */
enum USB_REQUEST {
    USB_RQ_GUIDE = 16, /* 0x10 */
    USB_RQ_EXPOSE = 18, /* 0x12 */
    USB_RQ_SET_INIT_PACKET = 19, /* 0x13 */
    USB_RQ_PRE_EXPOSE = 20, /* 0x14 */
    USB_RQ_SET_BUFFER_MODE = 85, /* 0x55 */

    /* These aren't tested yet */
    USB_RQ_CANCEL_GUIDE = 24, /* 0x18 */
    USB_RQ_CANCEL_GUIDE_NORTH_SOUTH = 34, /* 0x22 */
    USB_RQ_CANCEL_GUIDE_EAST_WEST = 33 /* 0x21 */
};

#define USB_TIMEOUT         5000

/* USB Bulk endpoint to grab data from */
#define BUFFER_ENDPOINT     0x82

/* Image size. Values must be even numbers. */
#define IMAGE_WIDTH         1280
#define IMAGE_HEIGHT        1024

/* Horizontal Blanking (in pixels) */
#define HORIZONTAL_BLANKING 244
/* Vertical Blanking (in rows) */
#define VERTICAL_BLANKING   25

/* Buffer size is determined by image size + horizontal/vertical blanking */
#define BUFFER_WIDTH        (IMAGE_WIDTH + HORIZONTAL_BLANKING)
#define BUFFER_HEIGHT       (IMAGE_HEIGHT + VERTICAL_BLANKING + 1)
#define BUFFER_SIZE         (BUFFER_WIDTH * BUFFER_HEIGHT)

/* Number of pixel columns/rows to skip. Values must be even numbers. */
#define ROW_START           12
#define COLUMN_START        20

/* Shutter width */
#define SHUTTER_WIDTH       (IMAGE_HEIGHT + VERTICAL_BLANKING)

/* Pixel offset appears to be calculated based on the dimensions of the chip.
 * 31 = 16 + 4 + 4 + 7 and there are 8 rows of optically black pixels. At the
 * moment, I'm not exactly sure why this would be required. It also appears to
 * change randomly at times. */
#define PIXEL_OFFSET        (8 * (BUFFER_WIDTH + 31))

/* Number of seconds to wait for camera to renumerate after loading firmware */
#define RENUMERATE_TIMEOUT  10

using namespace OpenSSAG;

SSAG::SSAG()
{
    //usb_init();
    // the context should be given, several elements have access to the libusb
}

struct device_info *SSAG::EnumerateDevices()
{

    struct device_info *head = NULL, *last = NULL, *current = NULL;

    libusb_device **list;

    ssize_t cnt = libusb_get_device_list(NULL, &list);
    if (cnt < 0)
    {
        DBG("No USB device found.");
        return head;
    }

    for (ssize_t i = 0; i < cnt; i++) 
    {
        libusb_device *device = list[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0)
        {
            DBG("Device description querying failed for device %d.", i);
            continue;
        }



        if (desc.idVendor == SSAG_VENDOR_ID && desc.idProduct == SSAG_PRODUCT_ID) 
        {
            libusb_device_handle *handle;
            r = libusb_open(device, &handle);
            if (r < 0) 
            {
                if (r == LIBUSB_ERROR_ACCESS) 
                {
                    DBG("Device open failed due to a permission denied error.");
                    DBG("libusb requires write access to USB device nodes.");
                }
                DBG("could not open device, error %d", r);
                continue;
            }    
        
            if(handle == NULL)
            {
                DBG("Device open failed: handle null while not an error in opening the device.");
                continue;
            }

            current = (struct device_info *)malloc(sizeof(struct device_info *));
            current->next = NULL;


            /* Copy serial */
            r = libusb_get_string_descriptor_ascii(
                    handle, 
                    desc.iSerialNumber & 0xff, 
                    (unsigned char*)current->serial, 
                    (int) sizeof(current->serial));
            if (r < 0)
            {
                DBG("Device open failed: cannot get the serial from the handle.");
                libusb_close(handle);
                continue;
            }
            
            
            libusb_close(handle);

            if (!head)
            {
                head = current;
            }
            if (last)
            {
                last->next = current;
            }

            last = current;
        }

    }
    
    libusb_free_device_list(list, 1);

    return head;
}

bool SSAG::Connect(bool bootload)
{
    if (!usb_open_device(&this->handle, SSAG_VENDOR_ID, SSAG_PRODUCT_ID, NULL)) {
        if (bootload) {
            Loader *loader = new Loader();
            if (loader->Connect()) {
                if (!loader->LoadFirmware()) {
                    fprintf(stderr, "ERROR:  Failed to upload firmware to the device\n");
                    return false;
                }
                loader->Disconnect();
                for (time_t t = time(NULL) + RENUMERATE_TIMEOUT; time(NULL) < t;) {
                    DBG("Checking if camera has renumerated...");
                    if (EnumerateDevices()) {
                        DBG("Yes\n");
                        return this->Connect(false);
                    }
                    DBG("No\n");
                    sleep(1);
                }
                DBG("ERROR:  Device did not renumerate. Timed out.\n");
                /* Timed out */
                return false;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    this->SetBufferMode();
    this->SetGain(1);
    this->InitSequence();

    return true;
}

bool SSAG::Connect()
{
    return this->Connect(true);
}

void SSAG::Disconnect()
{
    if (this->handle)
        libusb_close(this->handle);
    this->handle = NULL;
}

void SSAG::SetBufferMode()
{
    unsigned char data[4];
    //usb_control_msg(this->handle, 0xc0, USB_RQ_SET_BUFFER_MODE, 0x00, 0x63, data, sizeof(data), USB_TIMEOUT);

    int r = libusb_control_transfer(
        this->handle, 
        0xc0 & 0xff,
        USB_RQ_SET_BUFFER_MODE & 0xff, 
        0x00 & 0xffff, 
        0x63 & 0xffff, 
        data, 
        sizeof(data) & 0xffff,
        USB_TIMEOUT);

    if (r < 0)
    {
        DBG("SSAG::SetBufferMode: error sending command");
    }

    DBG("Buffer Mode Data: %02x%02x%02x%02x\n", data[0], data[1], data[2], data[3]);
}

bool SSAG::IsConnected()
{
    return (this->handle != NULL);
}

struct raw_image *SSAG::Expose(int duration)
{
    this->InitSequence();
    unsigned char data[16];
    //usb_control_msg(this->handle, 0xc0, USB_RQ_EXPOSE, duration, 0, data, 2, USB_TIMEOUT);
    
    int r = libusb_control_transfer(
        this->handle, 
        0xc0 & 0xff,
        USB_RQ_EXPOSE & 0xff, 
        duration & 0xffff, 
        0 & 0xffff, 
        data, 
        2,
        USB_TIMEOUT);
    if (r < 0)
    {
        DBG("SSAG::Expose: error sending command");
    }

    

    struct raw_image *image = (raw_image *)malloc(sizeof(struct raw_image));
    image->width = IMAGE_WIDTH;
    image->height = IMAGE_HEIGHT;
    image->data = this->ReadBuffer(duration + USB_TIMEOUT);

    DBG("Pixel Offset: %d\n", PIXEL_OFFSET);
    DBG("Buffer Size: %d\n", BUFFER_SIZE);
    DBG("  Buffer Width: %d\n", BUFFER_WIDTH);
    DBG("  Buffer Height: %d\n", BUFFER_HEIGHT);

    if (!image->data) {
        free(image);
        return NULL;
    }

    return image;
}

void SSAG::CancelExposure()
{
    /* Not tested */
    char data = 0;
    //usb_bulk_read(this->handle, 0, (char *)&data, 1, USB_TIMEOUT);
    int actual_length;
    int r = libusb_bulk_transfer(
      this->handle, 
      (0 | LIBUSB_ENDPOINT_IN) & 0xff, 
      (unsigned char *)&data, 
      1,
      &actual_length, 
      USB_TIMEOUT);
	
    /* if we timed out but did transfer some data, report as successful short
     * read. FIXME: is this how libusb-0.1 works? */
    if (r == 0 || (r == LIBUSB_ERROR_TIMEOUT && actual_length > 0))
    {
        DBG("SSAG::CancelExposure: read %d bytes but received a timeout", actual_length);
    }
        
}

void SSAG::Guide(int direction, int duration)
{
    this->Guide(direction, duration, duration);
}

void SSAG::Guide(int direction, int yduration, int xduration)
{
    unsigned char data[8];

    memcpy(data    , &xduration, 4);
    memcpy(data + 4, &yduration, 4);

    //usb_control_msg(this->handle, 0x40, USB_RQ_GUIDE, 0, (int)direction, data, sizeof(data), USB_TIMEOUT);
    
    int r = libusb_control_transfer(
        this->handle, 
        0x40 & 0xff,
        USB_RQ_GUIDE & 0xff, 
        0 & 0xffff, 
        (int)direction & 0xffff, 
        data, 
        sizeof(data),
        USB_TIMEOUT);
    
    if (r < 0)
    {
        DBG("SSAG::Guide: error sending command");
    }        
}

void SSAG::InitSequence()
{
    unsigned char init_packet[18] = {
        /* Gain settings */
        0x00, static_cast<unsigned char>(this->gain), /* G1 Gain */
        0x00, static_cast<unsigned char>(this->gain), /* B  Gain */
        0x00, static_cast<unsigned char>(this->gain), /* R  Gain */
        0x00, static_cast<unsigned char>(this->gain), /* G2 Gain */

        /* Vertical Offset. Reg0x01 */
        ROW_START >> 8, ROW_START & 0xff,

        /* Horizontal Offset. Reg0x02 */
        COLUMN_START >> 8, COLUMN_START & 0xff,

        /* Image height - 1. Reg0x03 */
        (IMAGE_HEIGHT - 1) >> 8, (IMAGE_HEIGHT - 1) & 0xff,

        /* Image width - 1. Reg0x04 */
        (IMAGE_WIDTH - 1) >> 8, (IMAGE_WIDTH - 1) & 0xff,

        /* Shutter Width. Reg0x09 */
        SHUTTER_WIDTH >> 8, SHUTTER_WIDTH & 0xff
    };

    int wValue = BUFFER_SIZE & 0xffff;
    int wIndex = BUFFER_SIZE  >> 16;

    //usb_control_msg(this->handle, 0x40, USB_RQ_SET_INIT_PACKET, wValue, wIndex, init_packet, sizeof(init_packet), USB_TIMEOUT);
    
    int r = libusb_control_transfer(
        this->handle, 
        0x40 & 0xff,
        USB_RQ_SET_INIT_PACKET & 0xff, 
        wValue & 0xffff, 
        wIndex & 0xffff, 
        init_packet, 
        sizeof(init_packet),
        USB_TIMEOUT);    
    if (r < 0)
    {
        DBG("SSAG::InitSequence: error sending command");
    }       
    
    
    //usb_control_msg(this->handle, 0x40, USB_RQ_PRE_EXPOSE, PIXEL_OFFSET, 0, NULL, 0, USB_TIMEOUT);
    
    r = libusb_control_transfer(
        this->handle, 
        0x40 & 0xff,
        USB_RQ_PRE_EXPOSE & 0xff, 
        PIXEL_OFFSET & 0xffff, 
        0 & 0xffff, 
        NULL, 
        0,
        USB_TIMEOUT);
    
    if (r < 0)
    {
        DBG("SSAG::InitSequence: error sending command (2)");
    }      
}

unsigned char *SSAG::ReadBuffer(int timeout)
{
    char *data = (char *)malloc(BUFFER_SIZE);
    char *dptr, *iptr;
    
    // Raffi: this function does not look like sending the actual number of bytes read from the device
    // I am wondering why we are using ret a bit below. I replaced it by actual_length.
    //int ret = usb_bulk_read(this->handle, BUFFER_ENDPOINT, data, BUFFER_SIZE, timeout);

    int actual_length = 0;
    int ret = libusb_bulk_transfer(
      this->handle, 
      (BUFFER_ENDPOINT | LIBUSB_ENDPOINT_IN) & 0xff, 
      (unsigned char *)data, 
      BUFFER_SIZE,
      &actual_length, 
      timeout);


    if (ret < 0)
    {
        DBG("SSAG::ReadBuffer: error sending command");
        free(data);
        return NULL;
    }    


    if (actual_length != BUFFER_SIZE) {
        DBG("Expected %d bytes of image data but got %d bytes\n", BUFFER_SIZE, actual_length);
        free(data);
        return NULL;
    } else {
        DBG("Received %d bytes of image data\n", actual_length);
    }

    char *image = (char *)malloc(IMAGE_WIDTH * IMAGE_HEIGHT);

    dptr = data;
    iptr = image;
    for (int i = 0; i < IMAGE_HEIGHT; i++) {
        memcpy(iptr, dptr, IMAGE_WIDTH);
        /* Horizontal Blanking can be ignored */
        dptr += BUFFER_WIDTH;
        iptr += IMAGE_WIDTH;
    }

    free(data);

    return (unsigned char *)image;
}

void SSAG::SetGain(int gain)
{
    /* See the MT9M001 datasheet for more information on the following code. */
    if (gain < 1 || gain > 15) {
        DBG("Gain was out of valid range: %d (Should be 1-15)\n", gain);
        return;
    }

    /* Default PHD Setting */
    if (gain == 7) {
        this->gain = 0x3b;
    } else if (gain <= 4) {
        this->gain = gain * 8;
    } else if (gain <= 8) {
        this->gain = (gain * 4) + 0x40;
    } else if (gain <= 15) {
        this->gain = (gain - 8) + 0x60;
    }

    DBG("Setting gain to %d (Register value 0x%02x)\n", gain, this->gain);
}

void SSAG::FreeRawImage(raw_image *image)
{
    free(image->data);
    free(image);
}
