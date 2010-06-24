/*
 *  GC_USBST4.cpp
 *  PHD
 *
 *  Created by Craig Stark on 6/5/09.
 *  Copyright 2009 Craig Stark, Stark Labs. All rights reserved.
 *
 */
#include "phd.h"
#include "scope.h"
#include "GC_USBST4.h"
#include	<sys/ioctl.h>
#include <termios.h>

#ifdef __LINUX__
#include <errno.h>
#endif


#define _U(String)  wxString(String, wxConvUTF8).c_str()


int portFID;

#ifdef __APPLE__
#include	<IOKit/serial/IOSerialKeys.h>

#define IOSSDATALAT    _IOW('T', 0, unsigned long)


kern_return_t createSerialIterator(io_iterator_t *serialIterator)
{
    kern_return_t	kernResult;
    mach_port_t		masterPort;
    CFMutableDictionaryRef	classesToMatch;
    if ((kernResult=IOMasterPort(NULL, &masterPort)) != KERN_SUCCESS)
    {
        printf("IOMasterPort returned %d\n", kernResult);
        return kernResult;
    }
    if ((classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
    {
        printf("IOServiceMatching returned NULL\n");
        return kernResult;
    }
	// GC device is a "modem"
    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
//                         CFSTR(kIOSerialBSDRS232Type));
							CFSTR(kIOSerialBSDModemType));
	kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, serialIterator);
    if (kernResult != KERN_SUCCESS)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }
    return kernResult;
}

char * getRegistryString(io_object_t sObj, char *propName)
{
    static char resultStr[256];
	//   CFTypeRef	nameCFstring;
	CFStringRef nameCFstring;
    resultStr[0] = 0;
    nameCFstring = (CFStringRef) IORegistryEntryCreateCFProperty(sObj,
																 CFStringCreateWithCString(kCFAllocatorDefault, propName, kCFStringEncodingASCII),
																 kCFAllocatorDefault, 0);
    if (nameCFstring) {
        CFStringGetCString(nameCFstring, resultStr, sizeof(resultStr),
						   kCFStringEncodingASCII);
        CFRelease(nameCFstring);
    }
    return resultStr;
}

#endif



void GCUSBST4_PulseGuideScope(int direction, int duration) {
	char buf[16];
	switch (direction) {
		case NORTH:
			sprintf(buf,":Mg0%4d#",duration);
			break;
		case SOUTH:
			sprintf(buf,":Mg1%4d#",duration);
			break;
		case EAST:
			sprintf(buf,":Mg2%4d#",duration);
			break;
		case WEST:
			sprintf(buf,":Mg3%4d#",duration);
			break;
	}
//	wxMessageBox(wxString::Format("Sending -%s-",buf));
	int num_bytes = write(portFID,buf,strlen(buf));
	if (num_bytes == -1) {
		wxMessageBox(wxString::Format(_T("Error writing to GC USB ST4: %s(%d)"),_U(strerror(errno)),errno));
//		close(portFID);
//		return false;
	}
//	wxMessageBox(wxString::Format("send %d vs %d",strlen(buf),num_bytes));
	wxMilliSleep(duration + 50);
}

bool GCUSBST4_Connect() {

#ifdef __APPLE__
	wxArrayString DeviceNames;
	wxArrayString PortNames;
	char tempstr[256];
	io_iterator_t	theSerialIterator;
	io_object_t		theObject;
	
	if (createSerialIterator(&theSerialIterator) != KERN_SUCCESS) {
		wxMessageBox(_T("Error in finding serial ports"),_T("Error"));
		return false;
	}
	bool found_device = false;
	while (theObject = IOIteratorNext(theSerialIterator)) {  // Find the device   should be usbmodem1*

		strcpy(tempstr, getRegistryString(theObject, kIOTTYDeviceKey));
		if (strstr(tempstr,"usbmodem")) {
			strcpy(tempstr, getRegistryString(theObject, kIODialinDeviceKey));
			found_device = true;
			break;
		}
	}
	IOObjectRelease(theSerialIterator);	// Release the iterator.
	
	if (!found_device) {
		wxMessageBox("Could not find device - searched for usbmodem* to no avail...");
		return false;
	}

#endif   //__APPLE__


#ifdef  __LINUX__
       char tempstr[256] = "/dev/ttyACM0";
#endif

	portFID = open(tempstr, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (portFID == -1) { // error on opening
		wxMessageBox(wxString::Format(_T("Error opening serial port %s: %s(%d)"),
										_U(tempstr), _U(strerror(errno)), errno));
		return false;
	}
	ioctl(portFID, TIOCEXCL);
	fcntl(portFID, F_SETFL, 0);
	
	// Setup port
	struct termios	options;
	//options = gOriginalTTYAttrs;
	if (tcgetattr(portFID, &options) == -1) {
		wxMessageBox(_T("Error getting port options"));
		close(portFID);
		return false;
	}
	cfmakeraw(&options);
	options.c_cflag = CREAD | CLOCAL;
	options.c_cflag |= CS8;
	options.c_iflag |= IXON | IXOFF;
//	options.c_cc[VMIN] = 1;
 //   options.c_cc[VTIME] = 10;
	options.c_cc[VSTART]=0x11;
	options.c_cc[VSTOP]=0x13;
	cfsetspeed(&options, B9600);
	//options.c_cflag = 0x8b00;
	/*wxMessageBox(wxString::Format("SET termios: iFlag %x  oFlag %x  cFlag %x  lFlag %x  speed %d\n",
		   options.c_iflag,
		   options.c_oflag,
		   options.c_cflag,
		   options.c_lflag,
		   options.c_ispeed));*/
	if (tcsetattr(portFID, TCSANOW, &options) == -1) {
		wxMessageBox(_T("Error setting port options"));
		close(portFID);
		return false;
	}
/*	int handshake;
	if (ioctl(portFID, TIOCMGET, &handshake) == -1) {
		wxMessageBox("Error getting port handshake");
		close(portFID);
		return false;		
	}
	unsigned long mics = 1UL;
	if (ioctl(portFID, IOSSDATALAT, &mics) == -1) {
		wxMessageBox("Error setting port latency");
		close(portFID);
		return false;
		
	}*/
//	wxMessageBox(wxString::Format("%d",(int) cfgetispeed(&options)));
	
	// Init / check the device
	char buf[2];
	int num_bytes;

	// Send the '#' needed to kickstart things
	buf[0]='#';
	buf[1]=0;
	num_bytes = write(portFID,buf,1);
	if (num_bytes == -1) {
		wxMessageBox(wxString::Format(_T("Error during initial kickstart: %s(%d)"),_U(strerror(errno)),errno));
		close(portFID);
		return false;
	}
	
	// Do a quick check
	buf[0]=0x6;
	buf[1]=0;
	num_bytes = write(portFID,buf,1);
	if (num_bytes == -1) {
		wxMessageBox(wxString::Format(_T("Error during test polling of device: %s(%d)"),_U(strerror(errno)),errno));
		close(portFID);
		return false;
	}
	num_bytes = read(portFID,buf,1);
	if (num_bytes == -1) {
		wxMessageBox(_T("Error during test read of device"));
		close(portFID);
		return false;
	}
	if (buf[0] != 'A') {
		wxMessageBox(wxString::Format(_T("Device returned %x instead of %x on test poll"),buf[0],'A'));
		close(portFID);
		return false;
	}
	


	return true;
}

void GCUSBST4_Disconnect() {
	if (portFID > 0) {
		close(portFID);
		portFID = 0;
	}		
}
