/*
 	File:		HID_CFM.c

	Contains:   Implementation of the CFM interfaces to the HID Utilities bundle
    
	DRI: George Warner

	Copyright:	Copyright © 2002 Apple Computer, Inc., All Rights Reserved

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//includes

#include "HID_Utilities_CFM.h"

// ==================================

// types

typedef unsigned char (*fpHIDBuildDeviceListtype) (unsigned long usagePage, unsigned long usage);
typedef void (*fpHIDReleaseDeviceListtype) (void);
typedef unsigned char (*fpHIDHaveDeviceListtype) (void);
typedef unsigned long (*fpHIDCountDevicestype) (void);
typedef unsigned long (*fpHIDCountDeviceElementstype) (pRecDevice pDevice, HIDElementTypeMask typeMask);
typedef pRecDevice (*fpHIDGetFirstDevicetype) (void);
typedef pRecDevice (*fpHIDGetNextDevicetype) (pRecDevice pDevice);
typedef pRecElement (*fpHIDGetFirstDeviceElementtype) (pRecDevice pDevice, HIDElementTypeMask typeMask);
typedef pRecElement (*fpHIDGetNextDeviceElementtype) (pRecElement pElement, HIDElementTypeMask typeMask);
typedef pRecElement (*fpHIDGetPreviousDeviceElementtype) (pRecElement pElement, HIDElementTypeMask typeMask);
typedef void (*fpHIDGetTypeNametype) (unsigned long type, char * cstrName);
typedef void (*fpHIDGetUsageNametype) (long valueUsagePage, long valueUsage, char * cstrName);
typedef unsigned long  (*fpHIDQueueElementtype) (pRecDevice pDevice, pRecElement pElement);
typedef unsigned long  (*fpHIDQueueDevicetype) (pRecDevice pDevice);
typedef unsigned long  (*fpHIDDequeueElementtype) (pRecDevice pDevice, pRecElement pElement);
typedef unsigned long  (*fpHIDDequeueDevicetype) (pRecDevice pDevice);
typedef unsigned char (*fpHIDGetEventtype) (pRecDevice pDevice, void * pHIDEvent);
typedef long (*fpHIDGetElementValuetype) (pRecDevice pDevice, pRecElement pElement);
typedef long (*fpHIDCalibrateValuetype) (long value, pRecElement pElement);
typedef long (*fpHIDScaleValuetype) (long value, pRecElement pElement);
typedef unsigned char (*fpHIDConfigureActiontype) (pRecDevice * ppDevice, pRecElement * ppElement, float timeout);
typedef void (*fpHIDSaveElementConfigtype) (FILE * fileRef, pRecDevice pDevice, pRecElement pElement, long actionCookie);
typedef long (*fpHIDRestoreElementConfigtype) (FILE * fileRef, pRecDevice * ppDevice, pRecElement * ppElement);

// function pointers
fpHIDBuildDeviceListtype fpHIDBuildDeviceList = NULL;
fpHIDReleaseDeviceListtype fpHIDReleaseDeviceList = NULL;
fpHIDHaveDeviceListtype fpHIDHaveDeviceList = NULL;
fpHIDCountDevicestype fpHIDCountDevices = NULL;
fpHIDCountDeviceElementstype fpHIDCountDeviceElements = NULL;
fpHIDGetFirstDevicetype fpHIDGetFirstDevice = NULL;
fpHIDGetNextDevicetype fpHIDGetNextDevice = NULL;
fpHIDGetFirstDeviceElementtype fpHIDGetFirstDeviceElement = NULL;
fpHIDGetNextDeviceElementtype fpHIDGetNextDeviceElement = NULL;
fpHIDGetPreviousDeviceElementtype fpHIDGetPreviousDeviceElement = NULL;
fpHIDGetTypeNametype fpHIDGetTypeName = NULL;
fpHIDGetUsageNametype fpHIDGetUsageName = NULL;
fpHIDQueueElementtype fpHIDQueueElement = NULL;
fpHIDQueueDevicetype fpHIDQueueDevice = NULL;
fpHIDDequeueElementtype fpHIDDequeueElement = NULL;
fpHIDDequeueDevicetype fpHIDDequeueDevice = NULL;
fpHIDGetEventtype fpHIDGetEvent = NULL;
fpHIDGetElementValuetype fpHIDGetElementValue = NULL;
fpHIDCalibrateValuetype fpHIDCalibrateValue = NULL;
fpHIDScaleValuetype fpHIDScaleValue = NULL;
fpHIDConfigureActiontype fpHIDConfigureAction = NULL;
fpHIDSaveElementConfigtype fpHIDSaveElementConfig = NULL;
fpHIDRestoreElementConfigtype fpHIDRestoreElementConfig = NULL;

//globals

CFURLRef gBundleURL = NULL;
CFBundleRef gBundle = NULL;

// ==================================

OSStatus SetupHIDCFM (void)
{
	Boolean didLoad = false; //	Flag that indicates the status returned when attempting to load a bundle's executable code.
	CFBundleRef refMainBundle = NULL;
	CFURLRef refMainBundleURL = NULL, refPathBundleURL = NULL; 

	//	See the Core Foundation URL Services chapter for details.
	// get app bundle (even for a CFM app!)
	refMainBundle = CFBundleGetMainBundle(); 
	if (!refMainBundle)
	{
        DebugStr ("\pCould open main bundle");
		return paramErr;
	}
	// create a URL to the app bundle
	refMainBundleURL = CFBundleCopyBundleURL (refMainBundle); 
	if (!refMainBundleURL)
	{
        DebugStr ("\pCould not copy main bundle URL");
		return paramErr;
	}
	// create a URL that points to the app's directory
	refPathBundleURL = CFURLCreateCopyDeletingLastPathComponent (NULL, refMainBundleURL); 
	if (!refPathBundleURL)
	{
        DebugStr ("\pCould not create new parent URL deleting last path component");
		if (refMainBundleURL != NULL) 
			CFRelease (refMainBundleURL);
		return paramErr;
	}
	// create a URL to the HID library bundle
	gBundleURL = CFURLCreateCopyAppendingPathComponent (NULL, refPathBundleURL, CFSTR("HID.bundle"), true); 
	// release created URLs
	if (refMainBundleURL != NULL) 
		CFRelease (refMainBundleURL);
	if (refPathBundleURL != NULL) 
		CFRelease (refPathBundleURL);
	// did we actaully get a bundle URL
	if (!gBundleURL)
	{
        DebugStr ("\pCould create HID bundle URL");
		return paramErr;
    }
	// get the actual bundle for the HID library
	gBundle = CFBundleCreate (NULL, gBundleURL);
	if (!gBundle)
	{
        DebugStr ("\pCould not create HID MachO library bundle");
		return paramErr;
	}
    if (!CFBundleLoadExecutable (gBundle)) // If the code was successfully loaded, look for our function.
	{
    	DebugStr ("\pCould not load MachO executable");
    	return paramErr;
	}
	// Now that the code is loaded, search for the function we want by name.
	fpHIDBuildDeviceList = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDBuildDeviceList"));
	fpHIDReleaseDeviceList = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDReleaseDeviceList"));
	fpHIDHaveDeviceList = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDHaveDeviceList"));
	fpHIDCountDevices = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDCountDevices"));
	fpHIDCountDeviceElements = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDCountDeviceElements"));
	fpHIDGetFirstDevice = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetFirstDevice"));
	fpHIDGetNextDevice = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetNextDevice"));
	fpHIDGetFirstDeviceElement = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetFirstDeviceElement"));
	fpHIDGetNextDeviceElement = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetNextDeviceElement"));
	fpHIDGetPreviousDeviceElement = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetPreviousDeviceElement"));
	fpHIDGetTypeName = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetTypeName"));
	fpHIDGetUsageName = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetUsageName"));
	fpHIDQueueElement = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDQueueElement"));
	fpHIDQueueDevice = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDQueueDevice"));
	fpHIDDequeueElement = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDDequeueElement"));
	fpHIDDequeueDevice = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDDequeueDevice"));
	fpHIDGetEvent = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetEvent"));
	fpHIDGetElementValue = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDGetElementValue"));
	fpHIDCalibrateValue = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDCalibrateValue"));
	fpHIDScaleValue = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDScaleValue"));
	fpHIDConfigureAction = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDConfigureAction"));
	fpHIDSaveElementConfig = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDSaveElementConfig"));
	fpHIDRestoreElementConfig = (void *) CFBundleGetFunctionPointerForName (gBundle, CFSTR("HIDRestoreElementConfig"));
	return noErr;
}

void TearDownHIDCFM (void)
{
	// disassociate function pointers
	fpHIDBuildDeviceList = NULL;
	fpHIDReleaseDeviceList = NULL;
	fpHIDHaveDeviceList = NULL;
	fpHIDCountDevices = NULL;
	fpHIDCountDeviceElements = NULL;
	fpHIDGetFirstDevice = NULL;
	fpHIDGetNextDevice = NULL;
	fpHIDGetFirstDeviceElement = NULL;
	fpHIDGetNextDeviceElement = NULL;
	fpHIDGetPreviousDeviceElement = NULL;
	fpHIDGetTypeName = NULL;
	fpHIDGetUsageName = NULL;
	fpHIDQueueElement = NULL;
	fpHIDQueueDevice = NULL;
	fpHIDDequeueElement = NULL;
	fpHIDDequeueDevice = NULL;
	fpHIDGetEvent = NULL;
	fpHIDGetElementValue = NULL;
	fpHIDCalibrateValue = NULL;
	fpHIDScaleValue = NULL;
	fpHIDConfigureAction = NULL;
	fpHIDSaveElementConfig = NULL;
	fpHIDRestoreElementConfig = NULL;
	if (gBundle != NULL)
	{
        CFBundleUnloadExecutable (gBundle);			//	Unload the bundle's executable code. 
        if (gBundleURL != NULL) 
            CFRelease (gBundleURL);
        CFRelease (gBundle);
	}
}

unsigned char HIDBuildDeviceList (unsigned long usagePage, unsigned long usage)
{
	return (*fpHIDBuildDeviceList) (usagePage, usage);
}

void HIDReleaseDeviceList (void)
{
	(*fpHIDReleaseDeviceList) ();
}

unsigned char HIDHaveDeviceList (void)
{
	return (*fpHIDHaveDeviceList) ();
}

unsigned long HIDCountDevices (void)
{
	return (*fpHIDCountDevices) ();
}

unsigned long HIDCountDeviceElements (pRecDevice pDevice, HIDElementTypeMask typeMask)
{
	return (*fpHIDCountDeviceElements) (pDevice, typeMask);
}

pRecDevice HIDGetFirstDevice (void)
{
	return (*fpHIDGetFirstDevice) ();
}

pRecDevice HIDGetNextDevice (pRecDevice pDevice)
{
	return (*fpHIDGetNextDevice) (pDevice);
}

pRecElement HIDGetFirstDeviceElement (pRecDevice pDevice, HIDElementTypeMask typeMask)
{
	return (*fpHIDGetFirstDeviceElement) (pDevice, typeMask);
}

pRecElement HIDGetNextDeviceElement (pRecElement pElement, HIDElementTypeMask typeMask)
{
	return (*fpHIDGetNextDeviceElement) (pElement, typeMask);
}

pRecElement HIDGetPreviousDeviceElement (pRecElement pElement, HIDElementTypeMask typeMask)
{
	return (*fpHIDGetPreviousDeviceElement) (pElement, typeMask);
}

void HIDGetTypeName (unsigned long type, char * cstrName)
{
	(*fpHIDGetTypeName) (type, cstrName);
}

void HIDGetUsageName (long valueUsagePage, long valueUsage, char * cstrName)
{
	(*fpHIDGetUsageName) (valueUsagePage, valueUsage, cstrName);
}

// ==================================

// Element Event Queue and Value Interfaces

unsigned long  HIDQueueElement (pRecDevice pDevice, pRecElement pElement)
{
	return (*fpHIDQueueElement) (pDevice, pElement);
}

unsigned long  HIDQueueDevice (pRecDevice pDevice)
{
	return (*fpHIDQueueDevice) (pDevice);
}

unsigned long  HIDDequeueElement (pRecDevice pDevice, pRecElement pElement)
{
	return (*fpHIDDequeueElement) (pDevice, pElement);
}

unsigned long  HIDDequeueDevice (pRecDevice pDevice)
{
	return (*fpHIDDequeueDevice) (pDevice);
}

unsigned char HIDGetEvent (pRecDevice pDevice, void * pHIDEvent)
{
	return (*fpHIDGetEvent) (pDevice, pHIDEvent);
}

long HIDGetElementValue (pRecDevice pDevice, pRecElement pElement)
{
	return (*fpHIDGetElementValue) (pDevice, pElement);
}

long HIDCalibrateValue (long value, pRecElement pElement)
{
	return (*fpHIDCalibrateValue) (value, pElement);
}

long HIDScaleValue (long value, pRecElement pElement)
{
	return (*fpHIDScaleValue) (value, pElement);
}

// ==================================

// Conguration and Save Interfaces

unsigned char HIDConfigureAction (pRecDevice * ppDevice, pRecElement * ppElement, float timeout)
{
	return (*fpHIDConfigureAction) (ppDevice, ppElement, timeout);
}

void HIDSaveElementConfig (FILE * fileRef, pRecDevice pDevice, pRecElement pElement, long actionCookie)
{
	(*fpHIDSaveElementConfig) (fileRef, pDevice, pElement, actionCookie);
}

long HIDRestoreElementConfig (FILE * fileRef, pRecDevice * ppDevice, pRecElement * ppElement)
{
	return (*fpHIDRestoreElementConfig) (fileRef, ppDevice, ppElement);
}
