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
#include "HID_Utilities_External.h"
#include <IOKit/hid/IOHIDLib.h>
IOHIDDeviceRef pGPUSB = NULL;
IOHIDDeviceInterface *pGPUSB_interface = NULL;
int GPUSB_Model = 0;



IOHIDDeviceRef FindDevice(IOHIDManagerRef manager, long vendorId, long productId)
{
    IOHIDDeviceRef theDevice = NULL;

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

    IOHIDManagerSetDeviceMatching(manager, dictionary);
    CFSetRef foundDevices = IOHIDManagerCopyDevices(manager);
    CFIndex foundCount = foundDevices ? CFSetGetCount(foundDevices) : 0; // what the API does not say is that it could be null
    if(foundCount > 0) 
    {
        CFTypeRef* array = new CFTypeRef[foundCount]; // array of IOHIDDeviceRef
        CFSetGetValues(foundDevices, array);
        // get first matching device
        theDevice = (IOHIDDeviceRef)array[0];
        CFRetain(theDevice);
        delete [] array;
    }

    if(foundDevices)
    {
        CFRelease(foundDevices);
    }
    CFRelease(dictionary);

    return theDevice;
}

/*



unsigned long HIDCreateOpenDeviceInterface (UInt32 hidDevice, IOHIDDeviceRef pDevice)
{
    IOHIDManagerRef managerRef = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    
    CFMutableDictionaryRef resultMutableDict = CFDictionaryCreateMutable(
                                                  kCFAllocatorDefault, 
                                                  0, 
                                                  &kCFTypeDictionaryKeyCallBacks,
                                                  &kCFTypeDictionaryValueCallBacks);
    
    IOReturn result = kIOReturnSuccess;
    HRESULT plugInResult = S_OK;
    SInt32 score = 0;
    IOCFPlugInInterface ** ppPlugInInterface = NULL;

    if (NULL == pGPUSB_interface)
    {
        result = IOCreatePlugInInterfaceForService (
                      hidDevice, 
                      kIOHIDDeviceUserClientTypeID,
											kIOCFPlugInInterfaceID, 
                      &ppPlugInInterface, 
                      &score);
        if (kIOReturnSuccess == result)
        {
            // Call a method of the intermediate plug-in to create the device interface
            plugInResult = (*ppPlugInInterface)->QueryInterface (
                                ppPlugInInterface,
                                CFUUIDGetUUIDBytes (kIOHIDDeviceInterfaceID), 
                                (void *) &(pGPUSB_interface));
            if (S_OK != plugInResult)
                HIDReportErrorNum ("Couldn’t query HID class device interface from plugInInterface", plugInResult);
            IODestroyPlugInInterface (ppPlugInInterface); // replace (*ppPlugInInterface)->Release (ppPlugInInterface)
		}
		else
			HIDReportErrorNum ("Failed to create **plugInInterface via IOCreatePlugInInterfaceForService.", result);
	}
	if (NULL != pGPUSB_interface)
	{
		result = pGPUSB_interface->open (pGPUSB_interface, 0);
		if (kIOReturnSuccess != result)
			HIDReportErrorNum ("Failed to open pDevice->interface via open.", result);
	}
    return result; 
}

*/

IOHIDElementRef GetFirstOutputElement(IOHIDDeviceRef device)
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
				
				switch ( type )
        {
            case kIOHIDElementTypeOutput:
            {
                output = tIOHIDElementRef;
                break;
            }          
            default:
                break;
        }
        
        if(output)
        {
            break;
        }
    }

    CFRelease(arrayElements);
    return output;
}


IOHIDElementRef GetNextOutputElement(IOHIDDeviceRef device, IOHIDElementRef previousElement)
{
    bool found = false;
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
        
        if(!found)
        {
            if(previousElement == tIOHIDElementRef)
            {
                CFRelease(previousElement); // do we need this ?
                found = true;
            }
            continue;
        }
        
        
				IOHIDElementType type = IOHIDElementGetType(tIOHIDElementRef);
				
				switch ( type )
        {
            case kIOHIDElementTypeOutput:
            {
                output = tIOHIDElementRef;
                break;
            }          
            default:
                break;
        }
        
        if(output)
        {
            break;
        }
    }

    CFRelease(arrayElements);
    return output;
}


bool GPUSB_Open() {

    // VID = 4938  PID = 36897
    IOHIDManagerRef managerRef = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    pGPUSB = FindDevice(managerRef, 4938, 36896);
    if(!pGPUSB)
    {
      CFRelease(managerRef);
      return false;
    }
    

    GPUSB_Model = 0;
    CFArrayRef arrayElements = IOHIDDeviceCopyMatchingElements(pGPUSB, NULL, kIOHIDOptionsTypeNone);
    CFIndex elementCount = CFArrayGetCount(arrayElements);
    int countInputElements = 0;
    for(int i = 0; i < elementCount; i++)
    {
        IOHIDElementRef tIOHIDElementRef = (IOHIDElementRef) CFArrayGetValueAtIndex(arrayElements, i);
				if(!tIOHIDElementRef)
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
    if(countInputElements == 1)
         GPUSB_Model = 1;


    // now opening the device for communication
    IOReturn ioReturnValue = IOHIDDeviceOpen(pGPUSB, kIOHIDOptionsTypeSeizeDevice);
    if(ioReturnValue != kIOReturnSuccess)
    {
      CFRelease(pGPUSB);
      pGPUSB = NULL;
      CFRelease(managerRef);
      return false;
      
    }
    

    
    CFRelease(managerRef); // check if this is ok
    return true;
}

bool GPUSB_Close() {
    if (!pGPUSB) return false;
    IOReturn ioReturnValue = IOHIDDeviceClose(pGPUSB, kIOHIDOptionsTypeSeizeDevice);
    if(ioReturnValue != kIOReturnSuccess)
    {
        wxMessageBox ("GPUSB_Close: error while closing the device",_("Error"));
    }
    
    CFRelease(pGPUSB_interface);
    CFRelease(pGPUSB);
    return ioReturnValue == kIOReturnSuccess;
}





void GPUSB_SetBit(int bit, int val)
{
    if (!pGPUSB) return;  // no device, bail
    IOHIDElementRef pCurrentHIDElement = NULL;
    //IOHIDEventStruct HID_IOEvent;
    
    int i;
    static int bitarray[8]={0,0,0,0,1,1,0,0};
    static unsigned char GPUSB_reg = 0x30;

    if (GPUSB_Model)
    { 
        // Newer models - use a single byte
        pCurrentHIDElement = GetFirstOutputElement(pGPUSB);
        if(pCurrentHIDElement == NULL)
        {
            wxMessageBox(std::string(__PRETTY_FUNCTION__) + " Null element");
            return;
        }
//      HIDTransactionAddElement(pGPUSB,pCurrentHIDElement);
        
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
        
        //HID_IOEvent.longValueSize = 0;
        //HID_IOEvent.longValue = nil;

        /*IOHIDValueRef valueToSend = IOHIDValueCreateWithBytes(
                                        kCFAllocatorDefault, 
                                        pCurrentHIDElement, 
                                        uint64_t        inTimeStamp,
                                        &GPUSB_reg,
                                        1);
        */

        //IOHIDElementCookie cookie = IOHIDElementGetCookie(pCurrentHIDElement);
        IOHIDValueRef valueToSend;
        IOReturn tIOReturn = IOHIDDeviceGetValue(pGPUSB, pCurrentHIDElement, &valueToSend);
        if(tIOReturn != kIOReturnSuccess)
        {
            CFRelease(pCurrentHIDElement);
            wxMessageBox(std::string(__PRETTY_FUNCTION__) + " Cannot retrieve value (1)");
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
        if(tIOReturn != kIOReturnSuccess)
        {
            CFRelease(pCurrentHIDElement);
            wxMessageBox(std::string(__PRETTY_FUNCTION__) + " Cannot send value (1)");
            return;
        }

        
        
        //pGPUSB_interface->getElementValue (pGPUSB_interface, cookie, &HID_IOEvent);

        //HID_IOEvent.value = (SInt32) GPUSB_reg;
//      wxMessageBox(wxString::Format("%x - %x %x    %d %d",foo,GPUSB_reg,bmask,bit,val));
//      HID_IOEvent.type = (IOHIDElementType) pCurrentHIDElement->type;
        //HIDSetElementValue(pGPUSB,pCurrentHIDElement,&HID_IOEvent);
//      HIDTransactionCommit(pGPUSB);
        CFRelease(pCurrentHIDElement);
    }
    else {
        // Generic bit-set routine.  For older adapters, we can't send a whole
        // byte and things are setup as SInt32's per bit with 8 bits total...
        //IOHIDEventStruct hidstruct = {kIOHIDElementTypeOutput};
        
        
        IOHIDTransactionRef transaction = IOHIDTransactionCreate(
                          kCFAllocatorDefault,
                          pGPUSB,
                          kIOHIDTransactionDirectionTypeOutput, 
                          kIOHIDOptionsTypeNone);    
                          
        if(transaction == NULL)
        {
            wxMessageBox(std::string(__PRETTY_FUNCTION__) + " Cannot create a transaction");
        }
        
        
        bitarray[bit]=val;
//      std::cout << "Setting to ";
        for (i=0; i<8; i++) {  // write
//          std::cout << " " << bitarray[i];
            if (i==0)
            {
                pCurrentHIDElement = GetFirstOutputElement(pGPUSB);
            }
            else
            {
                pCurrentHIDElement = GetNextOutputElement(pGPUSB, pCurrentHIDElement);
            }
            
            IOHIDValueRef valueToSend;
            IOReturn tIOReturn = IOHIDDeviceGetValue(pGPUSB, pCurrentHIDElement, &valueToSend);
            if(tIOReturn != kIOReturnSuccess)
            {
                CFRelease(pCurrentHIDElement);
                wxMessageBox(std::string(__PRETTY_FUNCTION__) + " Cannot retrieve value (1)");
                return;
            }
            
            //CFTypeID tID = IOHIDValueGetTypeID(valueToSend);
            
            /*IOHIDValueRef valueToSendCopied = IOHIDValueCreateWithBytes(
                                                  kCFAllocatorDefault, 
                                                  pCurrentHIDElement, 
                                                  IOHIDValueGetTimeStamp(valueToSend),
                                                  &bitarray[i],
                                                  sizeof(bitarray[i]));   */
                                                  
            IOHIDValueRef valueToSendCopied = IOHIDValueCreateWithIntegerValue(
                                                  kCFAllocatorDefault,
                                                  pCurrentHIDElement,
                                                  IOHIDValueGetTimeStamp(valueToSend),
                                                  bitarray[i]);
            
            IOHIDTransactionAddElement(transaction, pCurrentHIDElement);
            IOHIDTransactionSetValue(transaction, pCurrentHIDElement, valueToSendCopied, 0);
            
            
            //HIDTransactionAddElement(pGPUSB,pCurrentHIDElement);
            //hidstruct.type = (IOHIDElementType) pCurrentHIDElement->type;
            //hidstruct.value = (SInt32) bitarray[i];
            //HIDTransactionSetElementValue(pGPUSB,pCurrentHIDElement,&hidstruct);
        }
//      std::cout << "\n";
        IOHIDTransactionCommit(transaction);
    }

//  wxMilliSleep(30);
}

bool GPUSB_LEDOn() {
    // LED On/Off is bit 5
    if (!pGPUSB) return false;
    GPUSB_SetBit(5,1);
    return true;
}
bool GPUSB_LEDOff() {
    // LED On/Off is bit 5
    if (!pGPUSB) return false;
    GPUSB_SetBit(5,0);
    return true;
}
bool GPUSB_LEDRed() {
    // LED Red/Green is bit 4
    if (!pGPUSB) return false;
    GPUSB_SetBit(4,1);
    return true;
}
bool GPUSB_LEDGreen() {
    // LED Red/Green is bit 4
    if (!pGPUSB) return false;
    GPUSB_SetBit(4,0);
    return true;
}

bool GPUSB_DecPAssert() {
    if (!pGPUSB) return false;
    GPUSB_SetBit(3,1);

    return true;
}
bool GPUSB_DecMAssert() {
    if (!pGPUSB) return false;
    GPUSB_SetBit(2,1);

    return true;
}
bool GPUSB_RAPAssert() {
    if (!pGPUSB) return false;
    GPUSB_SetBit(1,1);

    return true;
}
bool GPUSB_RAMAssert() {
    if (!pGPUSB) return false;
    GPUSB_SetBit(0,1);

    return true;
}
bool GPUSB_AllDirDeassert() {
    if (!pGPUSB) return false;
    GPUSB_SetBit(0,0);
    GPUSB_SetBit(1,0);
    GPUSB_SetBit(2,0);
    GPUSB_SetBit(3,0);

    return true;
}
#endif // ------------------------------  Apple routines ----------------------------

bool ScopeGpUsb::Connect() {
    if (GPUSB_Open()) {
        GPUSB_AllDirDeassert();
        GPUSB_LEDOn();
        GPUSB_LEDRed();
        Scope::Connect();
        return false;
    }
    else
        return true;
}

bool ScopeGpUsb::Disconnect() {
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
    }
    wxMilliSleep(duration);
    GPUSB_AllDirDeassert();
    GPUSB_LEDRed();
    return MOVE_OK;
}

#endif /* GUIDE_GPUSB */
