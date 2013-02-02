#ifndef _FCAPIUSER_H
#define _FCAPIUSER_H
//#include "StdAfx.h"
//#include "afx.h"

typedef ULONG   U32, *PU32;
typedef USHORT  U16, *PU16;
typedef UCHAR   U8, *PU8;


struct CapInfoStruct {
    unsigned char *Buffer;
    unsigned long Height;
    unsigned long Width;
    unsigned long OffsetX;
    unsigned long OffsetY;
    unsigned long Exposure;
    unsigned char Gain[3];
    unsigned char Control;
    unsigned char InternalUse;
    unsigned char ColorOff[3];
    unsigned char Reserved[4];
};

typedef enum tagOS
{
    UNKNOWN_OS,
    WIN98,
    WINNT,
    WIN2K
} OSTYPE;


#define ResSuccess                  0x0000
#define ResNullHandleErr            0x0001
#define ResNullPointerErr           0x0002
#define ResFileOpenErr              0x0003
#define ResNoDeviceErr              0x0004
#define ResInvalidParameterErr      0x0005
#define ResOutOfMemoryErr           0x0006
#define ResNoPreviewRunningErr      0x0007
#define ResOSVersionErr             0x0008
#define ResUsbNotAvailableErr       0x0009
#define ResNotSupportedErr          0x000a
#define ResNoSerialString           0x000b
#define ResVerificationErr          0x000c
//#define ResTimeoutErr             0x000d
#define ResScaleModeErr             0x000f
#define ResUnknownErr               0x00ff

#define WM_MOVEPOINT    WM_USER+10




typedef int FCL_RETURN_CODE;

#ifdef FCAPI_EXPORTS
#define FCL_API extern "C" __declspec(dllexport) FCL_RETURN_CODE WINAPI
#else
#define FCL_API extern "C" __declspec(dllimport) FCL_RETURN_CODE WINAPI
#endif

typedef
VOID
(WINAPI * LPFC_CALLBACK)(LPVOID lpParam);


//#include "FcTypes.h"

/******************************************************************************
 *
 * Function: FclConvertRawToRgb()
 *
 * Description: converts it from raw data  to 24 bpp color data
 *
 ******************************************************************************/
FCL_API
FclConvertRawToRgb(IN HANDLE hImager,       // Modual device handle
                        IN BYTE* pSrc,      // Ptr to the source buffer
                        IN U32 width,       //image width
                        IN U32 height,      //image height
                        OUT BYTE* pDest);   // Ptr to the destination buffer

/******************************************************************************
 *
 * Function: FclGetFrameRate()
 *
 * Description: Get Current Frame rate
 *
 ******************************************************************************/
FCL_API
FclGetFrameRate(IN HANDLE hImager,  // Modual device handle
                    OUT float* pfFrameRate);// pointer to the current frame rate


/******************************************************************************
 *
 * Function: FclGetViewWin()
 *
 * Description: Get View window information
 *
 ******************************************************************************/
FCL_API
FclGetViewWin(IN HANDLE hImager,            // Modual device handle
                OUT PRECT pSubWindowRect);  // subwindow rectangle



/******************************************************************************
 *
 * Function: FclInitialize()
 *
 * Description: Inintialize the device
 *
 ******************************************************************************/
FCL_API
FclInitialize(  IN LPCSTR pFilterName,      // device name
                OUT int& index,             // index
                IN CapInfoStruct capInfo,
                OUT HANDLE* hImager);       // Modual device handle pointer

/******************************************************************************
 *
 * Function: FclPauseView()
 *
 * Description:  Pause the View
 *
 ******************************************************************************/
FCL_API
FclPauseView(IN HANDLE hImager,         // Modual device handle
                IN U32 pause);          // 1-pause,0-replay


/******************************************************************************
 *
 * Function: FclResetViewWin()
 *
 * Description:  Reset view window to the imager sub window size
 *
 ******************************************************************************/
FCL_API
FclResetViewWin(IN HANDLE hImager); // Modual device handle


/******************************************************************************
 *
 * Function: FclSetViewWin()
 *
 * Description:  Set client rectangle of View window
 *
 ******************************************************************************/
FCL_API
FclSetViewWin(IN HANDLE hImager,        // Modual device handle
                    IN RECT* pClientRect);  // desired client rectangle


/******************************************************************************
 *
 * Function: FclStartView()
 *
 * Description:  Start the View
 *
 ******************************************************************************/
FCL_API
FclStartView(IN HANDLE hImager,         // Modual device handle
                LPSTR title="My View",  //window title
                U32 style=WS_OVERLAPPEDWINDOW|WS_VISIBLE,//style
                U32 x=0,                    //left
                U32 y=0,                    //top
                U32 width=-1,               //width
                U32 height=-1,              //height
                HWND parent=NULL,           //handle of parent window
                U32 nId=0,                  //child window id
                IN int ViewDataThreadPriority=THREAD_PRIORITY_NORMAL,//View window's data thread priority
                IN int ViewDrawThreadPriority=THREAD_PRIORITY_NORMAL);//View window's draw thread priority

/******************************************************************************
 *
 * Function: FclStopView()
 *
 * Description:  Stop the View
 *
 ******************************************************************************/
FCL_API
FclStopView(IN HANDLE hImager);         // Modual device handle


/******************************************************************************
 *
 * Function: FclUninitialize()
 *
 * Description: Unnintialize the device
 *
 ******************************************************************************/
FCL_API
FclUninitialize(IN HANDLE* hImager);        // Ptr to Modual device handle

/******************************************************************************
 *
 * Function: FclSetCapInfo()
 *
 * Description: Set all the paramters for capturing a image
 *
 ******************************************************************************/
FCL_API
FclSetCapInfo(IN HANDLE hImager,struct CapInfoStruct capInfo);

/******************************************************************************
 *
 * Function: FclSavePausedFrameAsBmp()
 *
 * Description: Save a paused frame as bmp file
 *
 ******************************************************************************/
FCL_API
FclSavePausedFrameAsBmp(IN HANDLE hImager,  // Modual device handle
                        IN LPSTR FileName);

/******************************************************************************
 *
 * Function: FclSetScanInfo2()
 *
 * Description: Change scaninfo except size
 *
 ******************************************************************************/
FCL_API
FclSetPartOfCapInfo(IN HANDLE hImager,
                IN struct CapInfoStruct scanInfo);
/******************************************************************************
 *
 * Function: FclSetDoAGC()
 *
 * Description: Set the flag to let AGC run
 *
 ******************************************************************************/
//FCL_API
//FclSetDoAGC(IN HANDLE hImager,IN BOOL bDoAgc);


//Set AGC, when AGC is done, the callback function will be called
FCL_API
FclSetDoAWB(IN HANDLE hImager,
            IN BOOL bDoAgc,     //flag to do AGC
            IN BYTE btTarget,   //The target of AWB
            IN LPVOID lpFunc,   //pointer to callback function
            IN LONG * pParam);  //An optional parameter which could send to
                                //callback function
/******************************************************************************
 *
 * Function: FclSetLUT()
 *
 * Description: Set output curve with LUT methods
 *
 * Parameters: pLut, a table of 256 values for correction of brightness of
 *              image
 *
 ******************************************************************************/
FCL_API
FclSetLUT(IN HANDLE hImager,
            IN BYTE *pLut,
            IN BOOL bLut);

/******************************************************************************
 *
 * Function: FclSetGammaValue()
 *
 * Description: Set gamma value
 *
 * Parameters: GammaValue, the standard gamma value
 *
 ******************************************************************************/
FCL_API
FclSetGammaValue(HANDLE hImager,
                 float GammaValue,
                 BOOL bGammaOn);

/******************************************************************************
 *
 * Function: FclSetBw()
 *
 * Description: Set the flag of black and white
 *
 ******************************************************************************/
FCL_API
FclSetBw(IN HANDLE hImager,
            IN BOOL bBw);


/******************************************************************************
 *
 * Function: FclGetOneFrame()
 *
 * Description: Get a frame with the scaninfo parameter
 *
 ******************************************************************************/
FCL_API
FclGetOneFrame(IN HANDLE hImager,
               IN struct CapInfoStruct capInfo);

/******************************************************************************
 *
 * Function: FclGetRgbFrame()
 *
 * Description: Get a frame of image in RGB format
 *
 ******************************************************************************/
FCL_API
FclGetRgbFrame(IN HANDLE hImager,
               IN struct CapInfoStruct capInfo,
               OUT BYTE* pDest);

/******************************************************************************
 *
 * Function: FclSetExposureAdjust()
 *
 * Description: Set the flag to adjust exposure automatically
 *
 ******************************************************************************/
FCL_API
FclSetExposureAdjust(IN HANDLE hImager,
                     IN BOOL bAdjustExp,
                     IN BYTE btTarget,
                     IN LPVOID lpFunc,
                     IN LONG * pParam);
/******************************************************************************
 *
 * Function: FclGetSerial()
 *
 * Description: Get serail number in firmware
 *
 ******************************************************************************/
//FCL_API
//FclGetSerial(IN HANDLE hImager,
//          OUT CString& strSerial);



/******************************************************************************
 *
 * Function: FclSendCommand()
 *
 * Description: send a command byte to camera
 *
 ******************************************************************************/
FCL_API
FclSendCommand(
            IN HANDLE hImager,
            IN UCHAR  uCommand);

/******************************************************************************
 *
 * Function: FclBitOperation()
 *
 * Description: Bit operation
 *
 * Parameters: bOut ,bit 0,1,2 are data bits, bit 3,4,5 are direction bits
 *             for bit 3,4,5, value 1 means write out the value of 1,2,3 , value
 *             value 0 means read in the bit 1,2 ,3
 ******************************************************************************/

FCL_API
FclBitOperation(
            IN HANDLE hImager,
            IN BYTE  bOut,
            OUT BYTE& bIn);

/******************************************************************************
 *
 * Function: FclGetNumberDevices()
 *
 * Description: Find out how many devices
 *
 * Parameters: pNumberDevices, pointer to the value of the device number
 *
 * Notes: should be called when a device is initialized but not running
 *        the preveiw.
 *
 ******************************************************************************/
FCL_API
FclGetNumberDevices(IN HANDLE hImager,
                    OUT PU32 pNumberDevices);   // Ptr to number

/******************************************************************************
 *
 * Function: FclSetCaching()
 *
 * Description: Set caching ON or OFF
 *
 * Parameters: bCaching, TRUE meaning cache the image in memory, FALSE not
 *
 * Notes: The cached image will be saved to disk when the preview window closed
 *
 ******************************************************************************/
FCL_API
FclSetCaching(IN HANDLE hImager,
            IN BOOL  bCaching);

/******************************************************************************
 *
 * Function: FclScaleView()
 *
 * Description: Set the image in an scale mode
 *
 * Parameters: nMode, 0 is normal, 1:1, 1 is mediem, 1:4, 2 is small
 *
 * Notes: In mode 1
 *
 ******************************************************************************/
FCL_API
FclScaleView(
            IN HANDLE hImager,
            IN int  nMode);



#endif //_FCAPIUSER_H

