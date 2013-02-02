/* --------------------------------------------------------------------------
// All or portions of this software are copyrighted by D-image.
// Copyright 1999-2012  D-image Corporation.
// Company proprietary.
// --------------------------------------------------------------------------
//******************************************************************************
/**
 *  \file           DSCAMAPI.h
 *  \brief          Defines the API for the DC080 DLL application
 *  \author         Mike
 *  \version        \$ Revision: 0.1 \$
 *  \arg            first implemetation
 *  \date           2010/09/17 14:52:00
  Revision | Submission Date | Description of Change
  0x02       20120510          first release
 */
#ifndef _DSCAMAPI_H_
#define _DSCAMAPI_H_
#include "DSDefine.h"
//#include "stdafx.h"
// DECLDIR will perform an export for us

#ifdef DLL_EXPORT
    #define DT_API extern "C" __declspec(dllexport)
#else
    #define DT_API extern "C" __declspec(dllimport)
#endif
/*==============================================================
Name:   DSCameraGetSDKRevision
Desc:   .
Param: *pRevision Revision number pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetSDKRevision(IN BYTE *pRevision);

/*==============================================================
Name:   DSCameraSetMessage
Desc:   .
Param:  MsHWND  control handle which get message
        MessageID Message ID

Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetVideoMessage(HWND MsHWND, UINT MessageID);
/*==============================================================
Name:   DSCameraInitDisplay
Desc:   .
Param:  hWndDisplay  handle which use to display the video

Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note: if you not need display please use DSCameraInitDisplay(NULL);
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraInitDisplay(IN HWND hWndDisplay);
/*==============================================================
Name:   DSCameraInitCallbackFunction
Desc:   Initialize image process callback function
Param:  pCallbackFunction
        lpThreadparam Video thread create address

Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraInitCallbackFunction(DS_SNAP_PROC pCallbackFunction, IN LPVOID lpThreadparam);
/*==============================================================
Name:   DSCameraInit
Desc:  Initialize video equipment

Param:  uiResolution  Resolution index

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraInit( IN DS_RESOLUTION uiResolution);

/*==============================================================
Name:   DSCameraUnInit
Desc:   Anti-initialization equipment
Param:
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:   It must call when exit the program for releasing the memory allocation space.
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraUnInit(void);
/*==============================================================
Name:   DSCameraGrabFrame
Desc:   grab a frame to user allocate buffer

Param:  pImageBuffer buffer address
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:   this data is raw data what sensor output.
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGrabFrame(BYTE *pImageBuffer);
/*==============================================================
Name:   DSCameraImageProcess
Desc:   process raw image data to RGB24

Param:   pImageBuffer raw image data buffer
         pImageRGB24 RGB24 data buffer
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraImageProcess(BYTE *pImageBuffer, BYTE *pImageRGB24);
/*==============================================================
Name:   DSCameraDisplayRGB24
Desc:   Display RGB24 image to the handle which use to display the video

Param:  pImageRGB24 RGB24 data buffer
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:   call DSCameraInitDisplay() to init display before use
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraDisplayRGB24( BYTE *pImageRGB24 );

/*==============================================================
Name:   DSCameraPlay
Desc:   Open the video stream
Param:
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
  --------------------------------------------------------------*/

DT_API DS_CAMERA_STATUS _stdcall DSCameraPlay(DS_SNAP_MODE SnapMode );

/*==============================================================
Name:   DSCameraGetSnapMode
Desc:   Get image snap mode
Param:   *pSnapMode  image snap mode index pointer which set at DSCameraPlay();

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:   The value of *pSnapMode refer to the specific definition of DS_SNAP_MODE
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetSnapMode(IN BYTE *pSnapMode);

/*==============================================================
Name:   DSCameraPause
Desc:   Pause the video stream
Param:
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraPause(void);

/*==============================================================
Name:   DSCameraPlay
Desc:   Stop the video stream
Param:
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraStop(void);


/*==============================================================
Name:   DSCameraCaptureFile
Desc:   Capture an image to a file, the file format will change according to FileType
Param:   strFileName  file name(include directory path)
        FileType     File type,details refer to definition of FILE_TYPE
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:   The last image will be saved when the video stream stops.
  --------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraCaptureFile(IN LPCTSTR strFileName, IN BYTE FileType);

/*==============================================================
Name:   DSCameraGetImageSize
Desc:   Read current image size
Param:   *pWidth image width pointer
        *pHeight image height pointer
Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetImageSize(int *pWidth, int *pHeight);

/*==============================================================
Name:   DSCameraSetAeState
Desc:   Set AE mode
Param:   bState - TRUE  automatic exposure
               - FALSE  manual exposure

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetAeState(IN BOOL bState);

/*==============================================================
Name:   DSCameraGetAeState
Desc:   Read AE mode
Param:   *pAeState  save state pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetAeState(BOOL *pAeState);

/*==============================================================
Name:   DSCameraSetAeTarget
Desc:   Set AE target
Param:   uiAeTarget -AE target

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetAeTarget(IN BYTE uiAeTarget);

/*==============================================================
Name:   DSCameraGetAeTarget
Desc:   Read AE target
Param:   *pAeTarget -save AE target pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetAeTarget(IN OUT BYTE *pAeTarget);
/*==============================================================
Name:   DSCameraSetExposureTime
Desc:   Set exposure time
Param:   uiExposureTime -exposure time

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetExposureTime(IN DWORD uiExposureTime);
/*==============================================================
Name:   DSCameraGetExposureTime
Desc:   Read exposure time
Param:   *pExposureTime -save exposure time pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetExposureTime(IN DWORD *pExposureTime);
/*==============================================================
Name:   DSCameraSetAnalogGain
Desc:   Set gain
Param:   usAnalogGain  gain

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetAnalogGain(IN USHORT usAnalogGain);
/*==============================================================
Name:   DSCameraGetAnalogGain
Desc:   Set gain
Param:   *pAnalogGain-save the gain pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetAnalogGain(IN USHORT *pAnalogGain);

/*==============================================================
Name:   DSCameraSetAWBState
Desc:   Set white balance mode
Param:   bAWBState - TRUE automatic white balance
                  - FALSE manual white balance

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetAWBState(BOOL bAWBState);
/*==============================================================
Name:   DSCameraGetAWBState
Desc:    Read white balance mode
Param:   *pAWBState save state pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetAWBState(BOOL *pAWBState);

/*==============================================================
Name:   DSCameraSetGain
Desc:   Set each color channel gain
Param:   RGain red channel gain
        GGain green channel gain
        BGain blue channel gain

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetGain(IN USHORT RGain, USHORT GGain, USHORT BGain);
/*==============================================================
Name:   DSCameraGetGain
Desc:   Read each color channel gain
Param:   *pRGain save red channel gain pointer
        *pGGain save green channel gain pointer
        *pBGain save blue channel gain pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetGain(IN int *pRGain, int *pGGain, int *pBGain);

/*==============================================================
Name:   DSCameraSetColorEnhancement
Desc:   set color enhancement enable
Param:  bEnable - TRUE enable
                - FALSE disable

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetColorEnhancement(IN BOOL  bEnable);
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetColorEnhancement(IN BOOL *pEnable);
/*==============================================================
Name:   DSCameraSetSaturation
Desc:   Set saturation
Param:   uiSaturation

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetSaturation(IN BYTE uiSaturation);
/*==============================================================
Name:   DSCameraGetSaturation
Desc:   Read saturation
Param:   *pSaturation save the saturation pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetSaturation(IN BYTE *pSaturation);

/*==============================================================
Name:   DSCameraSetMonochrome
Desc:   Set monochrome
Param:   bEnable - TRUE monochrome
                 - FALSE cancel monochrome

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetMonochrome(IN BOOL bEnable);
/*==============================================================
Name:   DSCameraGetMonochrome
Desc:   Read color settings
Param:   *pEnable save the state pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetMonochrome(IN BOOL *pEnable);

/*==============================================================
Name:   DSCameraSetGamma
Desc:   Set GAMMA
Param:   uiGamma

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetGamma(IN BYTE uiGamma);
/*==============================================================
Name:   DSCameraGetGamma
Desc:   Read GAMMA
Param:   *pGamma  save the GAMMA pointer

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetGamma(IN BYTE *pGamma);

/*==============================================================
Name:   DSCameraSetContrast
Desc:   Set contrast
Param:   uiContrast

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetContrast(IN BYTE uiContrast);
/*==============================================================
Name:   DSCameraGetContrast
Desc:   Read contrast
Param:   *pContrast

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetContrast(IN BYTE *pContrast);
/*==============================================================
Name:   DSCameraSetBlackLevel
Desc:   Set OB clamp level
Param:

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetBlackLevel(IN BYTE uiBlackLevel);
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetBlackLevel(IN BYTE *pBlackLevel);

/*==============================================================
Name:   DSCameraSetMirror
Desc:   Set image mirror
Parem:   uiDir direction specified,refer DS_MIRROR_DIRECTION definition,
        bEnable - TRUE mirror
                - FALSE cancel mirror

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetMirror(IN DS_MIRROR_DIRECTION uiDir, IN BOOL bEnable);
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetMirror(IN DS_MIRROR_DIRECTION uiDir, IN BOOL *bEnable);

/*==============================================================
Name:   DSCameraSetFrameSpeed
Desc:   Set frame speed
Param:   FrameSpeed index,Specific definitions refer to DS_FRAME_SPEED

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetFrameSpeed(IN DS_FRAME_SPEED FrameSpeed);
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetFrameSpeed(IN BYTE *pFrameSpeed);

DT_API DS_CAMERA_STATUS _stdcall DSCameraSaveParameter(IN DS_PARAMETER_TEAM Team);
DT_API DS_CAMERA_STATUS _stdcall DSCameraReadParameter(IN DS_PARAMETER_TEAM Team);
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetCurrentParameterTeam(IN BYTE *pTeam);
/*==============================================================
Name:   DSCameraGetRowTime
Desc:   Get one row time
Param:

Return: Call returns a STATUS_OK on success,otherwise returns an error code
Note:   units is Microsecond
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetRowTime(IN UINT *pRowTime);

DT_API DS_CAMERA_STATUS _stdcall DSCameraCancelLongExposure(BOOL bPlay);

DT_API DS_CAMERA_STATUS _stdcall DSCameraEnableDeadPixelCorrection(void);

/*==============================================================
Name:   DSCameraWriteSN
Desc:   Write product number.
Param:

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note: make sure eeprom is writeable
  the SNCnt max value is 32
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraWriteSN(char *pSN, BYTE SNCnt);
/*==============================================================
Name:   DSCameraReadSN
Desc:   read product number
Param:

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:
--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraReadSN(char *pSN, BYTE SNCnt);
/*==============================================================
Name:   DSCameraSetUartBaudRate
Desc:   set uart baud rate.
Param:  Baud- please see  DS_UART_BAUD define.

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:

--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetUartBaudRate(DS_UART_BAUD Baud);
/*==============================================================
Name:   DSCameraSendUartData
Desc:   send data .
Param:

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:

--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSendUartData(BYTE *pBuffer, BYTE Length);
DT_API DS_CAMERA_STATUS _stdcall DSCameraReceiveUartData(BYTE *pBuf, BYTE *pLength);

/*==============================================================
Name:   DSCameraSetROI
Desc:  set ROI
Param:  HOff 行偏移
        VOff 偏移
        Width 区域宽度
        Height 区域高度

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:  Height min value is 230

--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetROI(USHORT HOff, USHORT VOff, USHORT Width, USHORT Height);
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetROI(USHORT *pHOff, USHORT *pVOff, USHORT *pWidth, USHORT *pHeight);

/*==============================================================
Name:   DSCameraSetGuidingPort
Desc:   set ST4 guiding port interface
Param:  Value bit0 - RA+
             bit1 - DEC+
             bit2 - DEC-
             bit3 - RA-
  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:

--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetGuidingPort(BYTE Value);

/*==============================================================
Name:   DSCameraGetFPS
Desc:   Get frame speed
Param:

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:

--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetFPS(float* fps);
/*==============================================================
Name:   DSCameraSetHB
Desc:   set
Param:

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:

--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetHB(USHORT usHB);
/*==============================================================
Name:   DSCameraSaveBmpFile
Desc:
Param:  sfilename file name ext:d.//test.bmp
        pBuffer RGB24 iamge buffer
        width image width
        height image height
  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:

--------------------------------------------------------------*/
//DT_API DS_CAMERA_STATUS _stdcall DSCameraSaveBmpFile(CString sfilename, BYTE *pBuffer, UINT width, UINT height);

/*==============================================================
Name:   DSCameraSetDataWide
Desc:    set RAW Data wide
Param: bWordWidth - 0 8bit
                  - 1 16bit only 10bit used

  Return: Call returns a STATUS_OK on success,otherwise returns an error code
  Note:

--------------------------------------------------------------*/
DT_API DS_CAMERA_STATUS _stdcall DSCameraSetDataWide(IN BOOL bWordWidth);
DT_API DS_CAMERA_STATUS _stdcall DSCameraGetDataWide(IN BOOL *pbWordWidth);


#endif
