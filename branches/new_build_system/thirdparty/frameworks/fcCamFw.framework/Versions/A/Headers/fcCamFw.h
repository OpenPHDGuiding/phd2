//
//  fcCamFw.h
//  fcCamFw
//
//  Created by Bob Piatek on 3/31/06.
//  Copyright 2006 fishcamp engineering. All rights reserved.
//

//#import <Cocoa/Cocoa.h>
//#import <Foundation/Foundation.h>



#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include <mach/mach.h>




// USB ID codes follow
#define fishcamp_USB_VendorID        0x1887

// defines for prototype starfish guide camera
#define starfish_mono_proto_raw_deviceID   0x0001
#define starfish_mono_proto_final_deviceID 0x0000

// defines for prototype starfish guide camera w/ DMA logic
#define starfish_mono_proto2_raw_deviceID   0x0004
#define starfish_mono_proto2_final_deviceID 0x0005

// defines for REV2 (production) starfish guide camera
#define starfish_mono_rev2_raw_deviceID   0x0002
#define starfish_mono_rev2_final_deviceID 0x0003




typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
//typedef unsigned long UInt32;
//typedef signed long SInt32;



// list of USB command codes
typedef enum { 

	fcNOP,
	fcRST,
	fcGETINFO,
	fcSETREG,
	fcGETREG,
	fcSETINTTIME,
	fcSTARTEXP,
	fcABORTEXP,
	fcGETSTATE,
	fcSETFGTP,
	fcRDSCANLINE,
	fcGETIMAGESTATS,
	fcSETROI,
	fcSETBIN,
	fcSETRELAY,
	fcCLRRELAY,
	fcPULSERELAY,
	fcSETLED,
	fcSETTEMP,
	fcGETTEMP,
	fcGETRAWFRAME,
	fcTURNOFFTEC,
	fcSETREADMODE
	
	} fc_cmd;






// progress codes for the fcUsb_FindCameras routine
typedef enum { 

	fcFindCam_notYetStarted,		
	fcFindCam_looking4supported,	// @"Looking for supported cameras"
	fcFindCam_initializingUSB,		// @"Initializing camera USB controller"
	fcFindCam_initializingIP,		// @"Initializing camera Image Processor"
	fcFindCam_finished				// @"Done looking for supported cameras"
		
	} fcFindCamState;







//structure used for command only messages that don't have any parameters
typedef struct {
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	cksum;
	} fc_no_param;


// structure used to hold the return information from the fcGETINFO command
typedef struct {
	UInt16	boardVersion;
	UInt16	boardRevision;
	UInt16	fpgaVersion;
	UInt16	fpgaRevision;
	UInt16	width;
	UInt16	height;
	UInt16	pixelWidth;
	UInt16	pixelHeight;
	UInt8	camSerialStr[32];	// 'C' string format
	UInt8	camNameStr[32];		// 'C' string format
	} fc_camInfo;


// structure used when calling the low level set register command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	registerAddress;
	UInt16	dataValue;
	UInt16	cksum;
	} fc_setReg_param;



// structure used when calling the low level get register command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	registerAddress;
	UInt16	cksum;
	} fc_getReg_param;


// structure used to hold the return information from the fcGETREG command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	dataValue;
	} fc_regInfo;
	

// structure used when calling the fcSETINTTIME command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	timeHi;
	UInt16	timeLo;
	UInt16	cksum;
	} fc_setIntTime_param;
	

// structure used when calling the fcSETFGTP command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	state;
	UInt16	cksum;
	} fc_setFgTp_param;



// structure used when calling the fcRDSCANLINE command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	LineNum;
	UInt16	padZero;
	UInt16	Xmin;
	UInt16	Xmax;
	UInt16	cksum;
	} fc_rdScanLine_param;



// structure used to hold the return information from the fcRDSCANLINE command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	LineNum;
	UInt16	padZero;
	UInt16	Xmin;
	UInt16	Xmax;
	UInt16	lineBuffer[2048];
	} fc_scanLineInfo;




// structure used when calling the fcSETROI command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	left;
	UInt16	top;
	UInt16	right;
	UInt16	bottom;
	UInt16	cksum;
	} fc_setRoi_param;


// structure used when calling the fcSETBIN command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	binMode;
	UInt16	cksum;
	} fc_setBin_param;




// list of relays
typedef enum { 

	fcRELAYNORTH,
	fcRELAYSOUTH,
	fcRELAYWEST,
	fcRELAYEAST
	
	} fc_relay;




// structure used when calling the fcSETRELAY command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	relayNum;
	UInt16	cksum;
	} fc_setClrRelay_param;



// structure used when calling the fcPULSERELAY command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	UInt16	relayNum;
	UInt16	highPulseWidth;
	UInt16	lowPulseWidth;
	UInt16	repeats;
	UInt16	cksum;
	} fc_pulseRelay_param;




// structure used to hold the return information from the fcGETTEMP command
typedef struct{
	UInt16	header;
	UInt16	command;
	SInt16	tempValue;
	UInt16	TECPwrValue;
	UInt16	TECInPwrOK;
	} fc_tempInfo;





// structure used when calling the fcSETTEMP command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	SInt16	theTemp;
	UInt16	cksum;
	} fc_setTemp_param;





// list of data formats 
typedef enum { 

	fc_8b_data,
	fc_10b_data,
	fc_12b_data,
	fc_14b_data,
	fc_16b_data
		
	} fc_dataFormat;



// list of data trnasfer modes 
typedef enum { 

	fc_classicDataXfr,
	fc_DMAwFBDataXfr,
	fc_DMASensor2USBDataXfr
		
	} fc_dataXfrModes;





// structure used when calling the fcSETREADMODE command
typedef struct{
	UInt16	header;
	UInt16	command;
	UInt16	length;
	SInt16	ReadBlack;
	UInt16	DataXfrReadMode;
	UInt16	DataFormat;
	SInt16	AutoOffsetCorrection;
	UInt16	cksum;
	} fc_setReadMode_param;





// structure used to store information about any cameras that were detected by the application
typedef struct {
    UInt16						camVendor;			// FC vendor ID
    UInt16						camRawProduct;		// RAW, uninitialized camera ID
    UInt16						camFinalProduct;	// initialized camera ID
    UInt16						camRelease;			// camera serial number
	IOUSBInterfaceInterface		**camUsbIntfc;		// handle to this camera
 	} fc_Camera_Information;







#ifdef __cplusplus
extern "C" {
#endif





	
// This is the framework initialization routine and needs to be called once upon application startup
void		fcUsb_init(void);




// the prefered way of finding and opening a communications link to any of our supported cameras
// This routine will call fcUsb_OpenCameraDriver and fcUsb_CloseCameraDriver routines to do its job
//
// will return the actual number of cameras found. 
//
// be carefull, this routine can take a long time (> 5 secs) to execute
//
// routine will return the number of supported cameras discovered.
//
int			fcUsb_FindCameras(void);



// call this routine to know how what state the fcUsb_FindCameras routine is currently executing
// will return a result of fcFindCamState type
//
int fcUsb_GetFindCamsState(void);



// call this routine to know how long it will take for the fcUsb_FindCameras routine to complete.
//
float fcUsb_GetFindCamsPercentComplete(void);




// call this routine to know how many of our supported cameras are available for use.
//
int			fcUsb_GetNumCameras(void);





// close the connection to a starfish camera/
// Users need to call this just before your application quits
// This routine is also used internally by the framework.  
//
void		fcUsb_CloseCameraDriver(void);



// call to find out if we have detected any of our cameras.  Will return TRUE if at
// least one of the supported cameras are currently connected to the computer
//
Boolean		fcUsb_haveCamera(void);




// return the numeric serial number of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int			fcUsb_GetCameraSerialNum(int camNum);




// return the numeric vendorID of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int			fcUsb_GetCameraVendorID(int camNum);







// return the numeric productID of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int			fcUsb_GetCameraProductID(int camNum);







// send the nop command to the starfish camera
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
IOReturn	fcUsb_cmd_nop(int camNum);




// send the rst command to the starfish camera
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
IOReturn	fcUsb_cmd_rst(int camNum);




// send the fcGETINFO command to the starfish camera
// read the return information
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
IOReturn	fcUsb_cmd_getinfo(int camNum, fc_camInfo *camInfo);




// call to set a low level register in the micron image sensor
// valid camNum is 1 -> fcUsb_GetNumCameras()
// refer to the micron image sensor documentation for details 
// on available registers and bit definitions
//
IOReturn	fcUsb_cmd_setRegister(int camNum, UInt16 regAddress, UInt16 dataValue);




// call to get a low level register from the micron image sensor
// valid camNum is 1 -> fcUsb_GetNumCameras()
// refer to the micron image sensor documentation for details 
// on available registers and bit definitions
//
UInt16		fcUsb_cmd_getRegister(int camNum, UInt16 regAddress);




// call to set the integration time register.  
// valid camNum is 1 -> fcUsb_GetNumCameras()
// 'theTime' specifies the number of milliseconds desired for the integration.
//  only 22 LSB significant giving a range of 0.001 -> 4194 seconds
// the starfish only has a range of 0.001 -> 300 seconds
//
IOReturn	fcUsb_cmd_setIntegrationTime(int camNum, UInt32 theTime);




// send the 'start exposure' command to the camera
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
IOReturn	fcUsb_cmd_startExposure(int camNum);




// send the 'abort exposure' command to the camera
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
IOReturn	fcUsb_cmd_abortExposure(int camNum);




// send a command to get the current camera state
// valid camNum is 1 -> fcUsb_GetNumCameras()
// return values are:
//	0 - idle
//	1 - integrating
//	2 - processing image
//
UInt16		fcUsb_cmd_getState(int camNum);




// turn on/off the frame grabber's test pattern generator
// valid camNum is 1 -> fcUsb_GetNumCameras()
// 0 = off, 1 = on
//
IOReturn	fcUsb_cmd_setFrameGrabberTestPattern(int camNum, UInt16 state);




// here to read a specific scan line from the camera's frame grabber buffer
// valid camNum is 1 -> fcUsb_GetNumCameras()
// this routine can be used to get a small number of pixels from the camera's
// frame buffer.  It is a VERY inneficient way of getting images from the camera
// use fcUsb_cmd_getRawFrame() instead
//
IOReturn	fcUsb_cmd_rdScanLine(int camNum, UInt16 lineNum, UInt16 Xmin, UInt16 Xmax, UInt16 *lineBuffer);




// here to specify a new sub-frame or 'Region-of-interest' (ROI) to the sensor
// valid camNum is 1 -> fcUsb_GetNumCameras()
// X = 0 -> numPixelsWide - 1
// Y = 0 -> numPixelshigh - 1
// The 'left' and 'top' parameters should be even numbers
// The 'right' and 'bottom' parameters should be odd numbers
//
IOReturn	fcUsb_cmd_setRoi(int camNum, UInt16 left, UInt16 top, UInt16 right, UInt16 bottom);




// set the binning mode of the camera.  Valid binModes are 1, 2, 3
// valid camNum is 1 -> fcUsb_GetNumCameras()
// This command is not supported by the current starfish camera
//
IOReturn	fcUsb_cmd_setBin(int camNum, UInt16 binMode);




// turn ON one of the relays on the camera.  whichRelay is one of enum fc_relay
// valid camNum is 1 -> fcUsb_GetNumCameras()
// whichRelayis one of:
//	0 - North
//	1 - South
//	2 - West
//	3 - East
//
IOReturn	fcUsb_cmd_setRelay(int camNum, int whichRelay);




// turn OFF one of the relays on the camera.  whichRelay is one of enum fc_relay
// valid camNum is 1 -> fcUsb_GetNumCameras()
// whichRelayis one of:
//	0 - North
//	1 - South
//	2 - West
//	3 - East
//
IOReturn	fcUsb_cmd_clearRelay(int camNum, int whichRelay);




// send a short duration pulse to one of the relays on the camera.  whichRelay is one of enum fc_relay
// valid camNum is 1 -> fcUsb_GetNumCameras()
// whichRelayis one of:
//	0 - North
//	1 - South
//	2 - West
//	3 - East
//
// pulse width parameters are in mS.  you can specify the hi and lo period of the pulse.
// if 'repeats' is true then the pulse will loop.
//
// call this routine with 'onMs' = 0 to abort any pulse operation in progress
//
IOReturn	fcUsb_cmd_pulseRelay(int camNum, int whichRelay, int onMs, int offMs, Boolean repeats);




// tell the camera what temperature setpoint to use for the TEC controller
// this will also turn on cooling to the camera
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
void		fcUsb_cmd_setTemperature(int camNum, SInt16 theTemp);




// get the temperature of the image sensor
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
SInt16		fcUsb_cmd_getTemperature(int camNum);




// get the current power setting of the TEC supply
// valid camNum is 1 -> fcUsb_GetNumCameras()
// return valuse are 0 -> 100 and are interpreted as a percent
//
UInt16		fcUsb_cmd_getTECPowerLevel(int camNum);




// call to find out if the camera has detected that the TEC power cable is plugged in
// return TRUE if so.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
Boolean		fcUsb_cmd_getTECInPowerOK(int camNum);




// command camera to turn off the TEC cooler
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
void		fcUsb_cmd_turnOffCooler(int camNum);








// here to read an entire frame in RAW format.  The frameBuffer will contain the RAW pixel values
// read from the camera's image sensor after this call.
//
// This is the prefered way of getting images from the camera
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
// X = 1 -> numPixelsWide
// Y = 1 -> numPixelshigh
//
IOReturn	fcUsb_cmd_getRawFrame(int camNum, UInt16 numRows, UInt16 numCols, UInt16 *frameBuffer);




// here to read an entire frame in RAW format.  The frame buffer will contain the RAW pixel values
// read from the camera's image sensor after this call.
//
// This is the prefered way of getting images from the camera.  This routine is very similar to the 
// fcUsb_cmd_getRawFrame except it is used when doing overlapped image sensor frame reads and USB
// uploads.  In this case no command needs to be sent to the camera since the READ of image data
// is implied.
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
// X = 1 -> numPixelsWide
// Y = 1 -> numPixelshigh
//
IOReturn	fcUsb_cmd_readRawFrame(int camNum, UInt16 numRows, UInt16 numCols, UInt16 *frameBuffer);




// here to define some image readout modes of the camera.  The state of these bits will be 
// used during the call to fcUsb_cmd_startExposure.  When an exposure is started and subsequent
// image readout is begun, the camera will assume an implied fcUsb_cmd_getRawFrame command
// when the 'DataXfrReadMode' is a '1' or '2' and begin uploading pixel data to the host as the image 
// is being read from the sensor.  
//
// When the 'ReadBlack' bit is set, the first black cols of pixels will also be read from the sensor.
// camera image processingwill also try to normalize the image rows based upon the levels of the black
// cols read out of the sensor.  This can be used to reduce readout noise associated with the rows 
// of the image when using large gain settings and short integration times.  It should not be used
// when integration times are longer than about 1sec since hot pixels can interfere with this operation
// causing image degradation.
//
// DataFormat can be one of 8, 10, 12, 14, 16
//     8 - packed into a single byte
//     others - packed into a 16 bit word
//
// DoOffsetCorrection will command the camera to correct for any offset variations associated with the
// four individual bayer filter matrix analog amplifiers in the image sensor.  It is recommended that this
// flag always be turned on for best image quality.
//
IOReturn	fcUsb_cmd_setReadMode(int camNum, int DataXfrReadMode, int DataFormat);






#ifdef __cplusplus
}
#endif




