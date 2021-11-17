/**************************************************
this is the second version of release SVB Camera SVBs
any question feel free contact us

here is the suggested procedure to operate the camera.

--> SVBGetNumOfConnectedCameras
----> SVBGetCameraProperty for each camera

--> SVBOpenCamera
--> SVBGetNumOfControls
----> SVBGetControlCaps for each contronl and set or get value from them

--> SVBSetROIFormat

--> SVBSetCameraMode

--> SVBStartVideoCapture

//this is recommended to do in another thread
while(1)
{
	SVBGetVideoData
	...
}

--> SVBStopVideoCapture
--> SVBCloseCamera

***************************************************/
#ifndef SVBCAMERA2_H
#define SVBCAMERA2_H




#ifdef _WINDOWS
#ifdef SVBCAMERA_EXPORTS
	#define SVBCAMERA_API __declspec(dllexport)
#else
	#define SVBCAMERA_API __declspec(dllimport)
#endif
#else
	#define SVBCAMERA_API 
#endif

#define SVBCAMERA_ID_MAX 128

typedef enum SVB_BAYER_PATTERN{
	SVB_BAYER_RG=0,
	SVB_BAYER_BG,
	SVB_BAYER_GR,
	SVB_BAYER_GB
}SVB_BAYER_PATTERN;

typedef enum SVB_IMG_TYPE{ //Supported Video Format 
	SVB_IMG_RAW8 = 0,
	SVB_IMG_RAW10,
	SVB_IMG_RAW12,
	SVB_IMG_RAW14,
	SVB_IMG_RAW16,
	SVB_IMG_Y8,
	SVB_IMG_Y10,
	SVB_IMG_Y12,
	SVB_IMG_Y14,
	SVB_IMG_Y16,
	SVB_IMG_RGB24,
	SVB_IMG_RGB32,
	SVB_IMG_END = -1

}SVB_IMG_TYPE;

typedef enum SVB_GUIDE_DIRECTION{ //Guider Direction
	SVB_GUIDE_NORTH=0,
	SVB_GUIDE_SOUTH,
	SVB_GUIDE_EAST,
	SVB_GUIDE_WEST
}SVB_GUIDE_DIRECTION;



typedef enum SVB_FLIP_STATUS {
	SVB_FLIP_NONE = 0,//: original
	SVB_FLIP_HORIZ,//: horizontal flip
	SVB_FLIP_VERT,// vertical flip
	SVB_FLIP_BOTH,//:both horizontal and vertical flip

}SVB_FLIP_STATUS;

typedef enum SVB_CAMERA_MODE {
	SVB_MODE_NORMAL = 0,
	SVB_MODE_TRIG_SOFT,
	SVB_MODE_TRIG_RISE_EDGE,
	SVB_MODE_TRIG_FALL_EDGE,
	SVB_MODE_TRIG_DOUBLE_EDGE,
	SVB_MODE_TRIG_HIGH_LEVEL,
	SVB_MODE_TRIG_LOW_LEVEL,
	SVB_MODE_END = -1
}SVB_CAMERA_MODE;

typedef enum SVB_TRIG_OUTPUT {
	SVB_TRIG_OUTPUT_PINA = 0,//: Only Pin A output
	SVB_TRIG_OUTPUT_PINB,//: Only Pin B output
	SVB_TRIG_OUTPUT_NONE = -1
}SVB_TRIG_OUTPUT_PIN;

typedef enum SVB_ERROR_CODE{ //SVB ERROR CODE
	SVB_SUCCESS=0,
	SVB_ERROR_INVALID_INDEX, //no camera connected or index value out of boundary
	SVB_ERROR_INVALID_ID, //invalid ID
	SVB_ERROR_INVALID_CONTROL_TYPE, //invalid control type
	SVB_ERROR_CAMERA_CLOSED, //camera didn't open
	SVB_ERROR_CAMERA_REMOVED, //failed to find the camera, maybe the camera has been removed
	SVB_ERROR_INVALID_PATH, //cannot find the path of the file
	SVB_ERROR_INVALID_FILEFORMAT, 
	SVB_ERROR_INVALID_SIZE, //wrong video format size
	SVB_ERROR_INVALID_IMGTYPE, //unsupported image formate
	SVB_ERROR_OUTOF_BOUNDARY, //the startpos is out of boundary
	SVB_ERROR_TIMEOUT, //timeout
	SVB_ERROR_INVALID_SEQUENCE,//stop capture first
	SVB_ERROR_BUFFER_TOO_SMALL, //buffer size is not big enough
	SVB_ERROR_VIDEO_MODE_ACTIVE,
	SVB_ERROR_EXPOSURE_IN_PROGRESS,
	SVB_ERROR_GENERAL_ERROR,//general error, eg: value is out of valid range
	SVB_ERROR_INVALID_MODE,//the current mode is wrong
	SVB_ERROR_INVALID_DIRECTION,//invalid guide direction
	SVB_ERROR_UNKNOW_SENSOR_TYPE,//unknow sensor type
	SVB_ERROR_END
}SVB_ERROR_CODE;

typedef enum SVB_BOOL{
	SVB_FALSE =0,
	SVB_TRUE
}SVB_BOOL;

typedef struct SVB_CAMERA_INFO
{
	char FriendlyName[32];
	char CameraSN[32];
	char PortType[32];
	unsigned int DeviceID;
	int CameraID;
} SVB_CAMERA_INFO;

typedef struct SVB_CAMERA_PROPERTY
{
	long MaxHeight; //the max height of the camera
	long MaxWidth;	//the max width of the camera

	SVB_BOOL IsColorCam;
	SVB_BAYER_PATTERN BayerPattern; 
	int SupportedBins[16]; //1 means bin1 which is supported by every camera, 2 means bin 2 etc.. 0 is the end of supported binning method
	SVB_IMG_TYPE SupportedVideoFormat[8];

	int MaxBitDepth;
	SVB_BOOL IsTriggerCam;
}SVB_CAMERA_PROPERTY;

typedef struct SVB_CAMERA_PROPERTY_EX
{
	SVB_BOOL bSupportPulseGuide;
	SVB_BOOL bSupportControlTemp;
	int Unused[64];
}SVB_CAMERA_PROPERTY_EX;

#define SVB_BRIGHTNESS SVB_OFFSET
#define SVB_AUTO_MAX_BRIGHTNESS SVB_AUTO_TARGET_BRIGHTNESS

typedef enum SVB_CONTROL_TYPE{ //Control type//
	SVB_GAIN = 0,
	SVB_EXPOSURE,
	SVB_GAMMA,
	SVB_GAMMA_CONTRAST,
	SVB_WB_R,
	SVB_WB_G,
	SVB_WB_B,
	SVB_FLIP,//reference: enum SVB_FLIP_STATUS
	SVB_FRAME_SPEED_MODE,//0:low speed, 1:medium speed, 2:high speed
	SVB_CONTRAST,
	SVB_SHARPNESS,
	SVB_SATURATION,

	SVB_AUTO_TARGET_BRIGHTNESS,
	SVB_BLACK_LEVEL, //black level offset
	SVB_COOLER_ENABLE,  //0:disable, 1:enable
	SVB_TARGET_TEMPERATURE,  //unit is 0.1C
	SVB_CURRENT_TEMPERATURE, //unit is 0.1C
	SVB_COOLER_POWER,  //range: 0-100
}SVB_CONTROL_TYPE;

typedef struct _SVB_CONTROL_CAPS
{
	char Name[64]; //the name of the Control like Exposure, Gain etc..
	char Description[128]; //description of this control
	long MaxValue;
	long MinValue;
	long DefaultValue;
	SVB_BOOL IsAutoSupported; //support auto set 1, don't support 0
	SVB_BOOL IsWritable; //some control like temperature can only be read by some cameras 
	SVB_CONTROL_TYPE ControlType;//this is used to get value and set value of the control
	char Unused[32];
} SVB_CONTROL_CAPS;

typedef enum SVB_EXPOSURE_STATUS {
	SVB_EXP_IDLE = 0,//: idle states, you can start exposure now
	SVB_EXP_WORKING,//: exposing
	SVB_EXP_SUCCESS,//: exposure finished and waiting for download
	SVB_EXP_FAILED,//:exposure failed, you need to start exposure again

}SVB_EXPOSURE_STATUS;

typedef struct _SVB_ID{
	unsigned char id[64];
}SVB_ID;

typedef SVB_ID SVB_SN;

typedef struct _SVB_SUPPORTED_MODE{
	SVB_CAMERA_MODE SupportedCameraMode[16];// this array will content with the support camera mode type.SVB_MODE_END is the end of supported camera mode
}SVB_SUPPORTED_MODE;



#ifndef __cplusplus
#define SVB_CONTROL_TYPE int
#define SVB_BOOL int
#define SVB_ERROR_CODE int
#define SVB_FLIP_STATUS int
#define SVB_IMG_TYPE int
#define SVB_GUIDE_DIRECTION int
#define SVB_BAYER_PATTERN int
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
Descriptions: 
this should be the first API to be called
get number of connected SVB cameras,

Paras: 

return:number of connected SVB cameras. 1 means 1 camera connected.
***************************************************************************/
SVBCAMERA_API  int SVBGetNumOfConnectedCameras(); 

/***************************************************************************
Descriptions:
get the information of the connected cameras, you can do this without open the camera.
here is the sample code:

int iNumofConnectCameras = SVBGetNumOfConnectedCameras();
SVB_CAMERA_INFO **ppSVBCameraInfo = (SVB_CAMERA_INFO **)malloc(sizeof(SVB_CAMERA_INFO *)*iNumofConnectCameras);
for(int i = 0; i < iNumofConnectCameras; i++)
{
ppSVBCameraInfo[i] = (SVB_CAMERA_INFO *)malloc(sizeof(SVB_CAMERA_INFO ));
SVBGetCameraInfo(ppSVBCameraInfo[i], i);
}
				
Paras:		
	SVB_CAMERA_INFO *pSVBCameraInfo: Pointer to structure containing the information of camera
									user need to malloc the buffer
	int iCameraIndex: 0 means the first connect camera, 1 means the second connect camera

return:
	SVB_SUCCESS: Operation is successful
	SVB_ERROR_INVALID_INDEX  :no camera connected or index value out of boundary

***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBGetCameraInfo(SVB_CAMERA_INFO *pSVBCameraInfo, int iCameraIndex);

/***************************************************************************
Descriptions:
get the property of the connected cameras
here is the sample code:

Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraProperty
SVB_CAMERA_PROPERTY *pCameraProperty: Pointer to structure containing the property of camera
								user need to malloc the buffer

return:
SVB_SUCCESS: Operation is successful
SVB_ERROR_INVALID_INDEX  :no camera connected or index value out of boundary

***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBGetCameraProperty(int iCameraID, SVB_CAMERA_PROPERTY *pCameraProperty);

/***************************************************************************
Descriptions:
get the property of the connected cameras
here is the sample code:

Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraProperty
SVB_CAMERA_PROPERTY_EX *pCameraPorpertyEx: Pointer to structure containing the property of camera
user need to malloc the buffer

return:
SVB_SUCCESS: Operation is successful
SVB_ERROR_INVALID_INDEX  :no camera connected or index value out of boundary

***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBGetCameraPropertyEx(int iCameraID, SVB_CAMERA_PROPERTY_EX *pCameraPorpertyEx);

/***************************************************************************
Descriptions:
	open the camera before any operation to the camera, this will not affect the camera which is capturing
	All APIs below need to open the camera at first.

Paras:		
	int CameraID: this is get from the camera property use the API SVBGetCameraInfo

return:
SVB_SUCCESS: Operation is successful
SVB_ERROR_INVALID_ID  : no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_CAMERA_REMOVED: failed to find the camera, maybe camera has been removed

***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBOpenCamera(int iCameraID);



/***************************************************************************
Descriptions:
you need to close the camera to free all the resource


Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo

return:
SVB_SUCCESS :it will return success even the camera already closed
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBCloseCamera(int iCameraID);




/***************************************************************************
Descriptions:
Get number of controls available for this camera. the camera need be opened at first.



Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
int * piNumberOfControls: pointer to an int to save the number of controls

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBGetNumOfControls(int iCameraID, int * piNumberOfControls);


/***************************************************************************
Descriptions:
Get controls property available for this camera. the camera need be opened at first.
user need to malloc and maintain the buffer.



Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
int iControlIndex: index of control, NOT control type
SVB_CONTROL_CAPS * pControlCaps: Pointer to structure containing the property of the control
user need to malloc the buffer

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBGetControlCaps(int iCameraID, int iControlIndex, SVB_CONTROL_CAPS * pControlCaps);

/***************************************************************************
Descriptions:
Get controls property value and auto value
note:the value of the temperature is the float value * 10 to convert it to long type, control name is "Temperature"
because long is the only type for control(except cooler's target temperature, because it is an integer)

Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
int ControlType: this is get from control property use the API SVBGetControlCaps
long *plValue: pointer to the value you want to save the value get from control
SVB_BOOL *pbAuto: pointer to the SVB_BOOL type

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_INVALID_CONTROL_TYPE, //invalid Control type
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBGetControlValue(int  iCameraID, SVB_CONTROL_TYPE  ControlType, long *plValue, SVB_BOOL *pbAuto);

/***************************************************************************
Descriptions:
Set controls property value and auto value
it will return success and set the max value or min value if the value is beyond the boundary


Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
int ControlType: this is get from control property use the API SVBGetControlCaps
long lValue: the value set to the control
SVB_BOOL bAuto: set the control auto

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_INVALID_CONTROL_TYPE, //invalid Control type
SVB_ERROR_GENERAL_ERROR,//general error, eg: value is out of valid range; operate to camera hareware failed
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBSetControlValue(int  iCameraID, SVB_CONTROL_TYPE  ControlType, long lValue, SVB_BOOL bAuto);

/***************************************************************************
Descriptions:
Get the output image type.

Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
SVB_IMG_TYPE *pImageType: pointer to current image type.

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_GENERAL_ERROR,//general error, eg: value is out of valid range; operate to camera hareware failed
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBGetOutputImageType(int iCameraID, SVB_IMG_TYPE *pImageType);

/***************************************************************************
Descriptions:
Set the output image type, The value set must be the type supported by the SVBGetCameraProperty function.

Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
SVB_IMG_TYPE *pImageType: pointer to current image type.

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_INVALID_IMGTYPE, //invalid image type
SVB_ERROR_GENERAL_ERROR,//general error, eg: value is out of valid range; operate to camera hareware failed
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBSetOutputImageType(int iCameraID, SVB_IMG_TYPE ImageType);

/***************************************************************************
Descriptions:
set the ROI area before capture.
you must stop capture before call it.
the width and height is the value after binning.
ie. you need to set width to 640 and height to 480 if you want to run at 640X480@BIN2
SVB120's data size must be times of 1024 which means width*height%1024=0SVBSetStartPos
Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
int iWidth,  the width of the ROI area. Make sure iWidth%8 = 0. 
int iHeight,  the height of the ROI area. Make sure iHeight%2 = 0, 
further, for USB2.0 camera SVB120, please make sure that iWidth*iHeight%1024=0. 
int iBin,   binning method. bin1=1, bin2=2

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_INVALID_SIZE, //wrong video format size
SVB_ERROR_INVALID_IMGTYPE, //unsupported image format, make sure iWidth and iHeight and binning is set correct
***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBSetROIFormat(int iCameraID, int iStartX, int iStartY, int iWidth, int iHeight, int iBin);


/***************************************************************************
Descriptions:
Get the current ROI area setting .

Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
int *piWidth,  pointer to the width of the ROI area   
int *piHeight, pointer to the height of the ROI area.
int *piBin,   pointer to binning method. bin1=1, bin2=2

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBGetROIFormat(int iCameraID, int *piStartX, int *piStartY, int *piWidth, int *piHeight,  int *piBin);

/***************************************************************************
Descriptions:
Get the droped frames .
drop frames happen when USB is traffic or harddisk write speed is slow
it will reset to 0 after stop capture

Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
int *piDropFrames pointer to drop frames

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBGetDroppedFrames(int iCameraID,int *piDropFrames); 


/***************************************************************************
Descriptions:
Start video capture
then you can get the data from the API SVBGetVideoData


Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo

return:
SVB_SUCCESS : Operation is successful, it will return success if already started
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_EXPOSURE_IN_PROGRESS: snap mode is working, you need to stop snap first
***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBStartVideoCapture(int iCameraID);

/***************************************************************************
Descriptions:
Stop video capture


Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo

return:
SVB_SUCCESS : Operation is successful, it will return success if already stopped
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBStopVideoCapture(int iCameraID);

/***************************************************************************
Descriptions:
get data from the video buffer.the buffer is very small 
you need to call this API as fast as possible, otherwise frame will be discarded
so the best way is maintain one buffer loop and call this API in a loop
please make sure the buffer size is biger enough to hold one image
otherwise the this API will crash


Paras:		
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
unsigned char* pBuffer, caller need to malloc the buffer, make sure the size is big enough
		the size in byte:
		8bit mono:width*height
		16bit mono:width*height*2
		RGB24:width*height*3

int iWaitms, this API will block and wait iWaitms to get one image. the unit is ms
		-1 means wait forever. this value is recommend set to exposure*2+500ms

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_TIMEOUT: no image get and timeout
***************************************************************************/
SVBCAMERA_API  SVB_ERROR_CODE SVBGetVideoData(int iCameraID, unsigned char* pBuffer, long lBuffSize, int iWaitms);

/***************************************************************************
Descriptions:
White balance once time. If success(return SVB_SUCCESS), please get SVB_WB_R, SVB_WB_G and SVB_WB_B values to update UI display.
Paras:

int CameraID: this is get from the camera property use the API SVBGetCameraInfo

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_GENERAL_ERROR : white balance failed
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBWhiteBalanceOnce(int iCameraID);

/***************************************************************************
Descriptions:
get version string, like "1, 13, 0503"
***************************************************************************/
SVBCAMERA_API const char* SVBGetSDKVersion();

/***************************************************************************
Description:
Get the camera supported mode, only need to call when the IsTriggerCam in the CameraInfo is true.
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
SVB_SUPPORTED_MODE: the camera supported mode

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBGetCameraSupportMode(int iCameraID, SVB_SUPPORTED_MODE* pSupportedMode);

/***************************************************************************
Description:
Get the camera current mode, only need to call when the IsTriggerCam in the CameraInfo is true 
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
SVB_CAMERA_MODE *mode: the current camera mode

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBGetCameraMode(int iCameraID, SVB_CAMERA_MODE* mode);

/***************************************************************************
Description:
Set the camera mode, only need to call when the IsTriggerCam in the CameraInfo is true 
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
SVB_CAMERA_MODE: this is get from the camera property use the API SVBGetCameraProperty

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_SEQUENCE : camera is in capture now, need to stop capture first.
SVB_ERROR_INVALID_MODE  : mode is out of boundary or this camera do not support this mode
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBSetCameraMode(int iCameraID, SVB_CAMERA_MODE mode);

/***************************************************************************
Description:
Send out a softTrigger. For edge trigger, it only need to set true which means send a
rising trigger to start exposure. For level trigger, it need to set true first means 
start exposure, and set false means stop exposure.it only need to call when the 
IsTriggerCam in the CameraInfo is true
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBSendSoftTrigger(int iCameraID);

/***************************************************************************
Description:
Get a serial number from a camera.
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo
SVB_SN* pSN: pointer to SN

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_GENERAL_ERROR : camera does not have Serial Number
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBGetSerialNumber(int iCameraID, SVB_SN* pSN);

/***************************************************************************
Description:
Config the output pin (A or B) of Trigger port. If lDuration <= 0, this output pin will be closed. 
Only need to call when the IsTriggerCam in the CameraInfo is true 

Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo.
SVB_TRIG_OUTPUT_STATUS pin: Select the pin for output
SVB_BOOL bPinAHigh: If true, the selected pin will output a high level as a signal
					when it is effective. Or it will output a low level as a signal.
long lDelay: the time between the camera receive a trigger signal and the output 
			of the valid level.From 0 microsecond to 2000*1000*1000 microsecond.
long lDuration: the duration time of the valid level output.From 0 microsecond to 
			2000*1000*1000 microsecond.

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_GENERAL_ERROR : the parameter is not right
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBSetTriggerOutputIOConf(int iCameraID, SVB_TRIG_OUTPUT_PIN pin, SVB_BOOL bPinHigh, long lDelay, long lDuration);


/***************************************************************************
Description:
Get the output pin configuration, only need to call when the IsTriggerCam in the CameraInfo is true 
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo.
SVB_TRIG_OUTPUT_STATUS pin: Select the pin for getting the configuration
SVB_BOOL *bPinAHigh: Get the current status of valid level.
long *lDelay: get the time between the camera receive a trigger signal and the output of the valid level.
long *lDuration: get the duration time of the valid level output.

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_CAMERA_CLOSED : camera didn't open
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_GENERAL_ERROR : the parameter is not right
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBGetTriggerOutputIOConf(int iCameraID, SVB_TRIG_OUTPUT_PIN pin, SVB_BOOL *bPinHigh, long *lDelay, long *lDuration);

/***************************************************************************
Description:
Send a PulseGuide command to camera to control the telescope
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo.
SVB_GUIDE_DIRECTION direction: the direction
int duration: the duration of pulse, unit is milliseconds

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_GENERAL_ERROR : the parameter is not right
SVB_ERROR_INVALID_DIRECTION : invalid guide direction
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBPulseGuide(int iCameraID, SVB_GUIDE_DIRECTION direction, int duration);

/***************************************************************************
Description:
Get sensor pixel size in microns
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo.
float *fPixelSize: sensor pixel size in microns

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
SVB_ERROR_UNKNOW_SENSOR_TYPE : unknow sensor type
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE  SVBGetSensorPixelSize(int iCameraID, float *fPixelSize);

/***************************************************************************
Description:
Get whether to support pulse guide
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo.
SVB_BOOL *pIsSupportPulseGuide: if SVB_TRUE then support pulse guide

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBCanPulseGuide(int iCameraID, SVB_BOOL *pCanPulseGuide);

/***************************************************************************
Description:
Whether to save the parameter file automatically
Paras:
int CameraID: this is get from the camera property use the API SVBGetCameraInfo.
SVB_BOOL enable: if SVB_TRUE then save the parameter file automatically.

return:
SVB_SUCCESS : Operation is successful
SVB_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
SVBCAMERA_API SVB_ERROR_CODE SVBSetAutoSaveParam(int iCameraID, SVB_BOOL enable);

#ifdef __cplusplus
}
#endif

#endif
