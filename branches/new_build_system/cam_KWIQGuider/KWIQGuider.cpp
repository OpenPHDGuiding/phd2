/*********************************************************************************************

    Copyright header?

*********************************************************************************************/


#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <time.h>

#include "KWIQGuider.h"
#include "KWIQGuider_priv.h"

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

#define USB_TIMEOUT 5000

/* USB Bulk endpoint to grab data from */
#define BUFFER_ENDPOINT 0x82

/* Image size */
#define IMAGE_WIDTH 1280
#define IMAGE_HEIGHT 1024

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
#define RENUMERATE_TIMEOUT 15


using namespace KWIQ;

KWIQGuider::KWIQGuider()
{
    usb_init();
}

struct device_info *KWIQGuider::EnumerateDevices()
{
    struct usb_bus *bus;
    struct usb_device *dev;
    struct usb_dev_handle *handle = NULL;
    struct device_info *head = NULL, *last = NULL, *current = NULL;

    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == SSAG_VENDOR_ID &&
                    dev->descriptor.idProduct == SSAG_PRODUCT_ID) {
                handle = usb_open(dev);
                if (handle) {
                    current = (struct device_info *)malloc(sizeof(struct device_info *));
                    current->next = NULL;
                    /* Copy serial */
                    usb_get_string_simple(handle, dev->descriptor.iSerialNumber, current->serial, sizeof(current->serial));
                    if (!head)
                        head = current;
                    if (last)
                        last->next = current;

                    last = current;
                }
            }
        }
    }

    return head;
}

bool KWIQGuider::Connect(bool bootload)
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

bool KWIQGuider::Connect()
{
    this->Connect(true);
}

void KWIQGuider::Disconnect()
{
    if (this->handle)
        usb_close(this->handle);
    this->handle = NULL;
}

void KWIQGuider::SetBufferMode()
{
    char data[4];
    usb_control_msg(this->handle, 0xc0, USB_RQ_SET_BUFFER_MODE, 0x00, 0x63, data, sizeof(data), USB_TIMEOUT);

    DBG("Buffer Mode Data: %02x%02x%02x%02x\n", data[0], data[1], data[2], data[3]);
}

bool KWIQGuider::IsConnected()
{
    return (this->handle != NULL);
}

struct raw_image *KWIQGuider::Expose(int duration)
{
    this->InitSequence();
    char data[16];
    usb_control_msg(this->handle, 0xc0, USB_RQ_EXPOSE, duration, 0, data, 2, USB_TIMEOUT);

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

void KWIQGuider::CancelExposure()
{
    /* Not tested */
    char data = 0;
    usb_bulk_read(this->handle, 0, (char *)&data, 1, USB_TIMEOUT);
}

void KWIQGuider::Guide(int direction, int duration)
{
    this->Guide(direction, duration, duration);
}

void KWIQGuider::Guide(int direction, int yduration, int xduration)
{
    char data[8];

    memcpy(data    , &xduration, 4);
    memcpy(data + 4, &yduration, 4);

    usb_control_msg(this->handle, 0x40, USB_RQ_GUIDE, 0, (int)direction, data, sizeof(data), USB_TIMEOUT);
}

void KWIQGuider::InitSequence()
{
    char init_packet[18] = {
        /* Gain settings */
        0x00, static_cast<char>(this->gain), /* G1 Gain */
        0x00, static_cast<char>(this->gain), /* B  Gain */
        0x00, static_cast<char>(this->gain), /* R  Gain */
        0x00, static_cast<char>(this->gain), /* G2 Gain */

        /* Vertical Offset. Reg0x01 */
        ROW_START >> 8, ROW_START & 0xff,

        /* Horizontal Offset. Reg0x02 */
        COLUMN_START >> 8, COLUMN_START & 0xff,

        /* Image height - 1. Reg0x03 */
        (IMAGE_HEIGHT - 1) >> 8, static_cast<char>((IMAGE_HEIGHT - 1) & 0xff),

        /* Image width - 1. Reg0x04 */
        (IMAGE_WIDTH - 1) >> 8, static_cast<char>((IMAGE_WIDTH - 1) & 0xff),

        /* Shutter Width. Reg0x09 */
        SHUTTER_WIDTH >> 8, SHUTTER_WIDTH & 0xff
    };

    int wValue = BUFFER_SIZE & 0xffff;
    int wIndex = BUFFER_SIZE  >> 16;

    usb_control_msg(this->handle, 0x40, USB_RQ_SET_INIT_PACKET, wValue, wIndex, init_packet, sizeof(init_packet), USB_TIMEOUT);
    usb_control_msg(this->handle, 0x40, USB_RQ_PRE_EXPOSE, PIXEL_OFFSET, 0, NULL, 0, USB_TIMEOUT);
}

unsigned char *KWIQGuider::ReadBuffer(int timeout)
{
    char *data = (char *)malloc(BUFFER_SIZE);
    char *dptr, *iptr;

    int ret = usb_bulk_read(this->handle, BUFFER_ENDPOINT, data, BUFFER_SIZE, timeout);

    if (ret != BUFFER_SIZE) {
        DBG("Expected %d bytes of image data but got %d bytes\n", BUFFER_SIZE, ret);
        free(data);
        return NULL;
    } else {
        DBG("Received %d bytes of image data\n", ret);
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

void KWIQGuider::SetGain(int gain)
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

void KWIQGuider::FreeRawImage(raw_image *image)
{
    free(image->data);
    free(image);
}
