/*
 *  ShoeString.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"

#ifdef GUIDE_GPUSB

#if defined (__WINDOWS__)
#include "ShoestringGPUSB_DLL.h"
#elif defined (__APPLE__) // ------------------------------  Apple routines ----------------------------

#include <IOKit/hid/IOHIDLib.h>

static IOHIDDeviceRef pGPUSB;
static int GPUSB_Model;
static IOHIDManagerRef s_managerRef;
static CFMutableArrayRef s_deviceArrayRef;

static void CFSetApplierFunctionCopyToCFArray(const void *value, void *context)
{
    CFArrayAppendValue((CFMutableArrayRef) context, value);
}

static bool FindDevice(long vendorId, long productId)
{
    pGPUSB = nullptr;

    bool first = false;
    if (!s_managerRef)
    {
        first = true;
        s_managerRef = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        if (!s_managerRef)
        {
            Debug.Write("Could not create HIDManager!\n");
            return false;
        }
    }

#if 1 // workaround problem with crash on re-connect
    if (!first)
    {
        pFrame->Alert(_("Please restart PHD2 to re-connect to the GPUSB"));
        return false;
    }
#endif

    // setup dictionary

    CFMutableDictionaryRef dictionary = CFDictionaryCreateMutable(
                                              kCFAllocatorDefault,
                                              2,
                                              &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);

    CFNumberRef cfVendorId = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &vendorId);
    CFStringRef cfVendorSt = CFStringCreateWithCString(kCFAllocatorDefault, kIOHIDVendorIDKey, kCFStringEncodingUTF8);
    CFDictionaryAddValue(dictionary, cfVendorSt, cfVendorId);
    CFRelease(cfVendorId);
    CFRelease(cfVendorSt);

    CFNumberRef cfProductId = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &productId);
    CFStringRef cfProductSt = CFStringCreateWithCString(kCFAllocatorDefault, kIOHIDProductIDKey, kCFStringEncodingUTF8);
    CFDictionaryAddValue(dictionary, cfProductSt, cfProductId);
    CFRelease(cfProductId);
    CFRelease(cfProductSt);

    // look for devices matching criteria

    IOHIDManagerSetDeviceMatching(s_managerRef, dictionary);
    CFRelease(dictionary);

    if (first)
    {
        IOReturn tIOReturn = IOHIDManagerOpen(s_managerRef, kIOHIDOptionsTypeNone);
        if (tIOReturn != kIOReturnSuccess)
        {
            Debug.Write(wxString::Format("%s: Couldn’t open IOHIDManager.\n", __PRETTY_FUNCTION__));
            return false;
        }
    }

    CFSetRef foundDevices = IOHIDManagerCopyDevices(s_managerRef);
    if (foundDevices)
    {
        if (s_deviceArrayRef)
        {
            CFRelease(s_deviceArrayRef);
        }

        // create an empty array
        s_deviceArrayRef = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

        // copy the set to the array
        CFSetApplyFunction(foundDevices, CFSetApplierFunctionCopyToCFArray, s_deviceArrayRef);

        // release the set we copied from the IOHID manager
        CFRelease(foundDevices);

        CFIndex foundCount = CFArrayGetCount(s_deviceArrayRef);

        if (foundCount == 0)
        {
            CFRelease(s_deviceArrayRef);
            s_deviceArrayRef = nullptr;
        }
        else
        {
            pGPUSB = (IOHIDDeviceRef) CFArrayGetValueAtIndex(s_deviceArrayRef, 0);
        }
    }

    return pGPUSB != nullptr;
}

static IOHIDElementRef GetFirstOutputElement(IOHIDDeviceRef device)
{
    IOHIDElementRef output = NULL;
    CFArrayRef arrayElements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
    CFIndex elementCount = CFArrayGetCount(arrayElements);
    for(CFIndex i = 0; i < elementCount; i++)
    {
        IOHIDElementRef tIOHIDElementRef = (IOHIDElementRef) CFArrayGetValueAtIndex(arrayElements, i);
        if(!tIOHIDElementRef)
        {
            continue;
        }
        IOHIDElementType type = IOHIDElementGetType(tIOHIDElementRef);

        switch (type)
        {
            case kIOHIDElementTypeOutput:
            {
                output = tIOHIDElementRef;
                break;
            }
            default:
                break;
        }

        if (output)
        {
            break;
        }
    }

    CFRelease(arrayElements);
    return output;
}

static IOHIDElementRef GetNextOutputElement(IOHIDDeviceRef device, IOHIDElementRef previousElement)
{
    bool found = false;
    IOHIDElementRef output = NULL;
    CFArrayRef arrayElements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
    CFIndex elementCount = CFArrayGetCount(arrayElements);
    for (CFIndex i = 0; i < elementCount; i++)
    {
        IOHIDElementRef tIOHIDElementRef = (IOHIDElementRef) CFArrayGetValueAtIndex(arrayElements, i);
        if (!tIOHIDElementRef)
        {
            continue;
        }

        if (!found)
        {
            if(previousElement == tIOHIDElementRef)
            {
                CFRelease(previousElement); // do we need this ?
                found = true;
            }
            continue;
        }

        IOHIDElementType type = IOHIDElementGetType(tIOHIDElementRef);

        switch (type)
        {
            case kIOHIDElementTypeOutput:
            {
                output = tIOHIDElementRef;
                break;
            }
            default:
                break;
        }

        if (output)
        {
            break;
        }
    }

    CFRelease(arrayElements);
    return output;
}

static bool GPUSB_Open()
{
    enum {
        VID = 4938,
        PID = 36896
    };

    if (!FindDevice(VID, PID))
    {
        return false;
    }

    GPUSB_Model = 0;

    CFArrayRef arrayElements = IOHIDDeviceCopyMatchingElements(pGPUSB, NULL, kIOHIDOptionsTypeNone);
    CFIndex elementCount = CFArrayGetCount(arrayElements);
    int countInputElements = 0;
    for (int i = 0; i < elementCount; i++)
    {
        IOHIDElementRef tIOHIDElementRef = (IOHIDElementRef) CFArrayGetValueAtIndex(arrayElements, i);
        if (!tIOHIDElementRef)
        {
            continue;
        }
        IOHIDElementType type = IOHIDElementGetType(tIOHIDElementRef);

        switch ( type )
        {
        case kIOHIDElementTypeInput_Misc:
        case kIOHIDElementTypeInput_Button:
        case kIOHIDElementTypeInput_Axis:
        case kIOHIDElementTypeInput_ScanCodes:
        {
            countInputElements++;
            break;
        }
        default:
            break;
        }

        CFRelease(tIOHIDElementRef);
    }

    CFRelease(arrayElements);

    if (countInputElements == 1)
    {
        GPUSB_Model = 1;
    }

    return true;
}

static void GPUSB_Close()
{
    if (!pGPUSB)
        return;

    if (s_deviceArrayRef)
    {
        CFRelease(s_deviceArrayRef);
        s_deviceArrayRef = nullptr;
    }

    pGPUSB = nullptr;
}

static void GPUSB_SetBit(int bit, int val)
{
    IOHIDElementRef pCurrentHIDElement = NULL;

    int i;
    static int bitarray[8]={0,0,0,0,1,1,0,0};
    static unsigned char GPUSB_reg = 0x30;

    if (GPUSB_Model)
    {
        // Newer models - use a single byte
        pCurrentHIDElement = GetFirstOutputElement(pGPUSB);
        if (pCurrentHIDElement == NULL)
        {
            Debug.Write(__PRETTY_FUNCTION__ + wxString(" Null element!\n"));
            return;
        }

        unsigned char bmask = 1;
        bmask = bmask << bit;
        if (val)
        {
            GPUSB_reg = GPUSB_reg | bmask;
        }
        else
        {
            GPUSB_reg = GPUSB_reg & ~bmask;
        }

        IOHIDValueRef valueToSend;
        IOReturn tIOReturn = IOHIDDeviceGetValue(pGPUSB, pCurrentHIDElement, &valueToSend);
        if (tIOReturn != kIOReturnSuccess)
        {
            CFRelease(pCurrentHIDElement);
            Debug.Write(__PRETTY_FUNCTION__ + wxString(" Cannot retrieve value (1)\n"));
            return;
        }

        assert(IOHIDValueGetLength(valueToSend) == 1);

        IOHIDValueRef valueToSendCopied = IOHIDValueCreateWithBytes(
                                        kCFAllocatorDefault,
                                        pCurrentHIDElement,
                                        IOHIDValueGetTimeStamp(valueToSend),
                                        &GPUSB_reg,
                                        1);

        tIOReturn = IOHIDDeviceSetValue(pGPUSB, pCurrentHIDElement, valueToSendCopied);
        if (tIOReturn != kIOReturnSuccess)
        {
            CFRelease(pCurrentHIDElement);
            Debug.Write(__PRETTY_FUNCTION__ + wxString(" Cannot send value (1)\n"));
            return;
        }

        CFRelease(pCurrentHIDElement);
    }
    else {
        // Generic bit-set routine.  For older adapters, we can't send a whole
        // byte and things are setup as SInt32's per bit with 8 bits total...

        IOHIDTransactionRef transaction = IOHIDTransactionCreate(
                          kCFAllocatorDefault,
                          pGPUSB,
                          kIOHIDTransactionDirectionTypeOutput,
                          kIOHIDOptionsTypeNone);

        if (transaction == NULL)
        {
            Debug.Write(__PRETTY_FUNCTION__ + wxString(" Cannot create a transaction\n"));
        }

        bitarray[bit] = val;

        for (i = 0; i < 8; i++) // write
        {
            if (i == 0)
            {
                pCurrentHIDElement = GetFirstOutputElement(pGPUSB);
            }
            else
            {
                pCurrentHIDElement = GetNextOutputElement(pGPUSB, pCurrentHIDElement);
            }

            IOHIDValueRef valueToSend;
            IOReturn tIOReturn = IOHIDDeviceGetValue(pGPUSB, pCurrentHIDElement, &valueToSend);
            if (tIOReturn != kIOReturnSuccess)
            {
                CFRelease(pCurrentHIDElement);
                Debug.Write(__PRETTY_FUNCTION__ + wxString(" Cannot retrieve value (1)\n"));
                return;
            }

            IOHIDValueRef valueToSendCopied = IOHIDValueCreateWithIntegerValue(
                                                  kCFAllocatorDefault,
                                                  pCurrentHIDElement,
                                                  IOHIDValueGetTimeStamp(valueToSend),
                                                  bitarray[i]);

            IOHIDTransactionAddElement(transaction, pCurrentHIDElement);
            IOHIDTransactionSetValue(transaction, pCurrentHIDElement, valueToSendCopied, 0);
        }
        IOHIDTransactionCommit(transaction);
    }
}

static bool GPUSB_LEDOn()
{
    // LED On/Off is bit 5

    if (!pGPUSB)
        return false;

    GPUSB_SetBit(5,1);
    return true;
}

static bool GPUSB_LEDOff()
{
    // LED On/Off is bit 5

    if (!pGPUSB)
        return false;

    GPUSB_SetBit(5,0);
    return true;
}

static bool GPUSB_LEDRed()
{
    // LED Red/Green is bit 4

    if (!pGPUSB)
        return false;

    GPUSB_SetBit(4,1);
    return true;
}

static bool GPUSB_LEDGreen()
{
    // LED Red/Green is bit 4

    if (!pGPUSB)
        return false;

    GPUSB_SetBit(4,0);
    return true;
}

static bool GPUSB_DecPAssert()
{
    if (!pGPUSB)
        return false;

    GPUSB_SetBit(3,1);
    return true;
}

static bool GPUSB_DecMAssert()
{
    if (!pGPUSB)
        return false;

    GPUSB_SetBit(2,1);
    return true;
}

static bool GPUSB_RAPAssert()
{
    if (!pGPUSB)
        return false;

    GPUSB_SetBit(1,1);
    return true;
}

static bool GPUSB_RAMAssert()
{
    if (!pGPUSB)
        return false;

    GPUSB_SetBit(0,1);
    return true;
}

static bool GPUSB_AllDirDeassert()
{
    if (!pGPUSB)
        return false;

    GPUSB_SetBit(0,0);
    GPUSB_SetBit(1,0);
    GPUSB_SetBit(2,0);
    GPUSB_SetBit(3,0);

    return true;
}

#endif // ------------------------------  Apple routines ----------------------------

bool ScopeGpUsb::Connect()
{
    if (GPUSB_Open())
    {
        GPUSB_AllDirDeassert();
        GPUSB_LEDOn();
        GPUSB_LEDRed();
        Scope::Connect();
        return false;
    }
    else
        return true;
}

bool ScopeGpUsb::Disconnect()
{
    GPUSB_LEDOff();
    GPUSB_Close();
    Scope::Disconnect();
    return false;
}

Mount::MOVE_RESULT ScopeGpUsb::Guide(GUIDE_DIRECTION direction, int duration)
{
    GPUSB_AllDirDeassert();
    GPUSB_LEDGreen();
    switch (direction) {
        case NORTH: GPUSB_DecPAssert(); break;
        case SOUTH: GPUSB_DecMAssert(); break;
        case EAST: GPUSB_RAMAssert(); break;
        case WEST: GPUSB_RAPAssert(); break;
        case NONE: break;
    }
    WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY);
    GPUSB_AllDirDeassert();
    GPUSB_LEDRed();
    return MOVE_OK;
}

bool ScopeGpUsb::HasNonGuiMove(void)
{
    return true;
}

#endif /* GUIDE_GPUSB */
