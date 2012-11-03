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
pRecDevice pGPUSB=NULL;
int GPUSB_Model = 0;

bool GPUSB_Open() {
	int i, ndevices, GPUSB_DevNum;
	
	// VID = 4938  PID = 36897
	
	HIDBuildDeviceList (NULL, NULL);
	GPUSB_DevNum = -1;
	ndevices = (int) HIDCountDevices();
	
	i=0;
	// Locate the GPUSB
	while ((i<ndevices) && (GPUSB_DevNum < 0)) {		
		if (i==0) pGPUSB = HIDGetFirstDevice();
		else pGPUSB = HIDGetNextDevice (pGPUSB);
		if ((pGPUSB->vendorID == 4938) && (pGPUSB->productID == 36896)) { // got it
			GPUSB_DevNum = i;
			if (pGPUSB->inputs == 1) GPUSB_Model = 1;
		}
		else i++;
	}
	if (GPUSB_DevNum == -1) {
		pGPUSB = NULL;
		return false;
	}
	return true;
}

bool GPUSB_Close() {
	if (!pGPUSB) return false;
	HIDReleaseDeviceList();
	return true;
}

void GPUSB_SetBit(int bit, int val) {
	if (!pGPUSB) return;  // no device, bail
	pRecElement pCurrentHIDElement = NULL;
	IOHIDEventStruct HID_IOEvent;
	int i;
	static int bitarray[8]={0,0,0,0,1,1,0,0};
	static unsigned char GPUSB_reg = 0x30;

	

	if (GPUSB_Model) { // Newer models - use a single byte
		pCurrentHIDElement =  HIDGetFirstDeviceElement (pGPUSB, kHIDElementTypeOutput);
//		HIDTransactionAddElement(pGPUSB,pCurrentHIDElement);
		unsigned char bmask = 1;
		bmask = bmask << bit;
		if (val)
			GPUSB_reg = GPUSB_reg | bmask;
		else
			GPUSB_reg = GPUSB_reg & ~bmask;
		HID_IOEvent.longValueSize = 0;
		HID_IOEvent.longValue = nil;
		(*(IOHIDDeviceInterface**) pGPUSB->interface)->getElementValue (pGPUSB->interface, pCurrentHIDElement->cookie, &HID_IOEvent);
		
		HID_IOEvent.value = (SInt32) GPUSB_reg;
//		wxMessageBox(wxString::Format("%x - %x %x    %d %d",foo,GPUSB_reg,bmask,bit,val));
//		HID_IOEvent.type = (IOHIDElementType) pCurrentHIDElement->type;
		HIDSetElementValue(pGPUSB,pCurrentHIDElement,&HID_IOEvent);
//		HIDTransactionCommit(pGPUSB);
	}
	else {
		// Generic bit-set routine.  For older adapters, we can't send a whole
		// byte and things are setup as SInt32's per bit with 8 bits total...		
		IOHIDEventStruct hidstruct = {kIOHIDElementTypeOutput};
		bitarray[bit]=val;	
//		std::cout << "Setting to ";
		for (i=0; i<8; i++) {  // write
//			std::cout << " " << bitarray[i];
			if (i==0)
				pCurrentHIDElement =  HIDGetFirstDeviceElement (pGPUSB, kHIDElementTypeOutput);
			else
				pCurrentHIDElement = HIDGetNextDeviceElement(pCurrentHIDElement,kHIDElementTypeOutput);
			HIDTransactionAddElement(pGPUSB,pCurrentHIDElement);
			hidstruct.type = (IOHIDElementType) pCurrentHIDElement->type;
			hidstruct.value = (SInt32) bitarray[i];
			HIDTransactionSetElementValue(pGPUSB,pCurrentHIDElement,&hidstruct);
		}
//		std::cout << "\n";
		HIDTransactionCommit(pGPUSB);
	}

//	wxMilliSleep(30);
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

bool ScopeGpUsb::Guide(const GUIDE_DIRECTION direction, int duration) {
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
    return false;
}

#endif /* GUIDE_GPUSB */
