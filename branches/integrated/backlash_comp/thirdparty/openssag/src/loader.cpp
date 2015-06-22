/*
 * File: loader.cpp
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
#ifdef __APPLE__
  #include <unistd.h>
#endif

#include "openssag.h"
#include "openssag_priv.h"
#include "firmware.h"

#define CPUCS_ADDRESS 0xe600

/* USB commands to control device */
enum USB_REQUEST {
    USB_RQ_LOAD_FIRMWARE = 0xa0,
    USB_RQ_WRITE_SMALL_EEPROM = 0xa2
};

using namespace OpenSSAG;

/* Bootloader data */
static unsigned char bootloader[] = { SSAG_BOOTLOADER };
/* Firmware data */
static unsigned char firmware[] = { SSAG_FIRMWARE };
/* EEPROM data (shouldn't be needed) */
static unsigned char eeprom[] = { SSAG_EEPROM };

bool Loader::Connect()
{
    if (!usb_open_device(&this->handle, SSAG_LOADER_VENDOR_ID, SSAG_LOADER_PRODUCT_ID, NULL)) {
        return false;
    }
    return true;
}

void Loader::Disconnect()
{
    if (this->handle)
        libusb_close(this->handle);
    this->handle = NULL;
}

void Loader::EnterResetMode()
{
    char data = 0x01;
    //usb_control_msg(this->handle, 0x40, USB_RQ_LOAD_FIRMWARE, 0x7f92, 0, &data, 1, 5000);
    int r = libusb_control_transfer(
        this->handle, 
        0x40 & 0xff,
        USB_RQ_LOAD_FIRMWARE & 0xff, 
        0x7f92 & 0xffff, 
        0 & 0xffff, 
        (unsigned char*)&data, 
        1,
        5000);
    
    if (r < 0)
    {
        DBG("Loader::EnterResetMode: error sending command (1)");
    }           
    
    
    //usb_control_msg(this->handle, 0x40, USB_RQ_LOAD_FIRMWARE, CPUCS_ADDRESS, 0, &data, 1, 5000);
    r = libusb_control_transfer(
        this->handle, 
        0x40 & 0xff,
        USB_RQ_LOAD_FIRMWARE & 0xff, 
        CPUCS_ADDRESS & 0xffff, 
        0 & 0xffff, 
        (unsigned char*)&data, 
        1,
        5000);
    
    if (r < 0)
    {
        DBG("Loader::EnterResetMode: error sending command (2)");
    }        
}

void Loader::ExitResetMode()
{
    char data = 0x00;
    //usb_control_msg(this->handle, 0x40, USB_RQ_LOAD_FIRMWARE, 0x7f92, 0, &data, 1, 5000);
    int r = libusb_control_transfer(
        this->handle, 
        0x40 & 0xff,
        USB_RQ_LOAD_FIRMWARE & 0xff, 
        0x7f92 & 0xffff, 
        0 & 0xffff, 
        (unsigned char*)&data, 
        1,
        5000);
    
    if (r < 0)
    {
        DBG("Loader::ExitResetMode: error sending command (1)");
    }             
    
    
    //usb_control_msg(this->handle, 0x40, USB_RQ_LOAD_FIRMWARE, CPUCS_ADDRESS, 0, &data, 1, 5000);
    r = libusb_control_transfer(
        this->handle, 
        0x40 & 0xff,
        USB_RQ_LOAD_FIRMWARE & 0xff, 
        CPUCS_ADDRESS & 0xffff, 
        0 & 0xffff, 
        (unsigned char*)&data, 
        1,
        5000);
    
    if (r < 0)
    {
        DBG("Loader::ExitResetMode: error sending command (2)");
    }        
    
}

bool Loader::Upload(unsigned char *data)
{
    for (;;) {
    
        /*
        unsigned char byte_count = *data;
        if (byte_count == 0)
            break;
        unsigned short address = *(unsigned int *)(data+1);
        int received = 0;
        if ((received = usb_control_msg(this->handle, 0x40, USB_RQ_LOAD_FIRMWARE, address, 0, (char *)(data+3), byte_count, 5000)) != byte_count) {
            DBG("ERROR:  Tried to send %d bytes of data but device reported back with %d\n", byte_count, received);
            return false;
        }
        data += byte_count + 3;
        */
        
        
        unsigned char byte_count = *data;
        if (byte_count == 0)
            break;
        unsigned short address = *(unsigned int *)(data+1);
        int received = 0;
        
        //usb_control_msg(this->handle, 0x40, USB_RQ_LOAD_FIRMWARE, address, 0, (char *)(data+3), byte_count, 5000))
        received = libusb_control_transfer(
            this->handle, 
            0x40 & 0xff,
            USB_RQ_LOAD_FIRMWARE & 0xff, 
            address & 0xffff, 
            0 & 0xffff, 
            (unsigned char*)(data+3), 
            byte_count,
            5000);
        
        
        //if ((received = usb_control_msg(this->handle, 0x40, USB_RQ_LOAD_FIRMWARE, address, 0, (char *)(data+3), byte_count, 5000)) != byte_count) {

        if (received != byte_count) {

            DBG("ERROR:  Tried to send %d bytes of data but device reported back with %d\n", byte_count, received);
            return false;
        }
        data += byte_count + 3;
        
        
        
    }
    return true;
}

bool Loader::LoadFirmware()
{
    unsigned char *data = NULL;

    /* Load bootloader */
    this->EnterResetMode();
    this->EnterResetMode();
    DBG("Loading bootloader...");
    if (!this->Upload(bootloader))
        return false;
    DBG("done\n");
    this->ExitResetMode(); /* Transfer execution to the reset vector */

    sleep(1); /* Wait for renumeration */

    /* Load firmware */
    this->EnterResetMode();
    DBG("Loading firmware...");
    if (!this->Upload(firmware))
        return false;
    DBG("done\n");
    this->EnterResetMode(); /* Make sure the CPU is in reset */
    this->ExitResetMode(); /* Transfer execution to the reset vector */

    return true;
}

bool Loader::LoadEEPROM()
{
    size_t length = *eeprom;
    char *data = (char *)(eeprom+3);
    //usb_control_msg(this->handle, 0x40, USB_RQ_WRITE_SMALL_EEPROM, 0x00, 0xBEEF, data, length, 5000);
    int received = libusb_control_transfer(
            this->handle, 
            0x40 & 0xff,
            USB_RQ_WRITE_SMALL_EEPROM & 0xff, 
            0x00 & 0xffff, 
            0xBEEF & 0xffff, 
            (unsigned char*)(data), 
            length,
            5000);
    if(received != length)
    {
        DBG("ERROR: during the loading of the EEPROM");
    }
    return true;
}
