//
//  libMallincamTest.h
//  libMallincamTest
//
//  Created by pufahl on 5/18/16.
//  Copyright (c) 2016 rD. All rights reserved.
//

#ifndef libMallincamTest_libMallincamTest_h
#define libMallincamTest_libMallincamTest_h

#ifdef _WIN32
#ifndef _INC_WINDOWS
#include <windows.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
    
#ifdef _WIN32
    
#pragma pack(push, 8)
#ifdef MALLINCAMCAM_EXPORTS
#define mallincam_ports(x)    __declspec(dllexport)   x   __stdcall
#elif !defined(MALLINCAM_NOIMPORTS)
#define mallincam_ports(x)    __declspec(dllimport)   x   __stdcall
#else
#define mallincam_ports(x)    x   __stdcall
#endif
    
#else
    
#define mallincam_ports(x)    x
    
#ifndef MRESULT
#define MRESULT unsigned long
#endif
    
#ifndef __stdcall
#define __stdcall
#endif
    
#ifndef CALLBACK
#define CALLBACK
#endif
    
#ifndef BOOL
#define BOOL int
#endif
    
#ifndef __BITMAPINFOHEADER_DEFINED__
#define __BITMAPINFOHEADER_DEFINED__
    typedef struct {
        unsigned        biSize;
        int             biWidth;
        int             biHeight;
        unsigned short  biPlanes;
        unsigned short  biBitCount;
        unsigned        biCompression;
        unsigned        biSizeImage;
        int             biXPelsPerMeter;
        int             biYPelsPerMeter;
        unsigned        biClrUsed;
        unsigned        biClrImportant;
    } BITMAPINFOHEADER;
#endif
    
#ifndef __RECT_DEFINED__
#define __RECT_DEFINED__
    typedef struct {
        int left;
        int top;
        int right;
        int bottom;
    } RECT, *PRECT;
#endif
    
#endif
    
    /* handle */
    typedef struct MallincamT { int unused; } *HMallinCam;
    
#define MALLINCAM_MAX                     16
    
#define MALLINCAM_FLAG_CMOS               0x00000001  /* cmos sensor */
#define MALLINCAM_FLAG_CCD_PROGRESSIVE    0x00000002  /* progressive ccd sensor */
#define MALLINCAM_FLAG_CCD_INTERLACED     0x00000004  /* interlaced ccd sensor */
#define MALLINCAM_FLAG_ROI_HARDWARE       0x00000008  /* support hardware ROI */
#define MALLINCAM_FLAG_MONO               0x00000010  /* monochromatic */
#define MALLINCAM_FLAG_BINSKIP_SUPPORTED  0x00000020  /* support bin/skip mode, see Mallincam_put_Mode and Mallincam_get_Mode */
#define MALLINCAM_FLAG_USB30              0x00000040  /* USB 3.0 */
#define MALLINCAM_FLAG_TEC                0x00000080  /* Thermoelectric Cooler */
#define MALLINCAM_FLAG_USB30_OVER_USB20   0x00000100  /* usb3.0 camera connected to usb2.0 port */
#define MALLINCAM_FLAG_ST4                0x00000200  /* ST4 */
#define MALLINCAM_FLAG_GETTEMPERATURE     0x00000400  /* support to get the temperature of sensor */
#define MALLINCAM_FLAG_PUTTEMPERATURE     0x00000800  /* support to put the temperature of sensor */
#define MALLINCAM_FLAG_BITDEPTH10         0x00001000  /* Maximum Bit Depth = 10 */
#define MALLINCAM_FLAG_BITDEPTH12         0x00002000  /* Maximum Bit Depth = 12 */
#define MALLINCAM_FLAG_BITDEPTH14         0x00004000  /* Maximum Bit Depth = 14 */
#define MALLINCAM_FLAG_BITDEPTH16         0x00008000  /* Maximum Bit Depth = 16 */
#define MALLINCAM_FLAG_FAN                0x00010000  /* cooling fan */
#define MALLINCAM_FLAG_TEC_ONOFF          0x00020000  /* Thermoelectric Cooler can be turn on or off */
#define MALLINCAM_FLAG_ISP                0x00040000  /* Image Signal Processing supported */
#define MALLINCAM_FLAG_TRIGGER_SOFTWARE   0x00080000  /* support software trigger */
#define MALLINCAM_FLAG_TRIGGER_EXTERNAL   0x00100000  /* support external trigger */
#define MALLINCAM_FLAG_TRIGGER_SINGLE     0x00200000  /* only support trigger single: one trigger, one image */
    
#define MALLINCAM_TEMP_DEF                6503
#define MALLINCAM_TEMP_MIN                2000
#define MALLINCAM_TEMP_MAX                15000
#define MALLINCAM_TINT_DEF                1000
#define MALLINCAM_TINT_MIN                200
#define MALLINCAM_TINT_MAX                2500
#define MALLINCAM_HUE_DEF                 0
#define MALLINCAM_HUE_MIN                 (-180)
#define MALLINCAM_HUE_MAX                 180
#define MALLINCAM_SATURATION_DEF          128
#define MALLINCAM_SATURATION_MIN          0
#define MALLINCAM_SATURATION_MAX          255
#define MALLINCAM_BRIGHTNESS_DEF          0
#define MALLINCAM_BRIGHTNESS_MIN          (-64)
#define MALLINCAM_BRIGHTNESS_MAX          64
#define MALLINCAM_CONTRAST_DEF            0
#define MALLINCAM_CONTRAST_MIN            (-100)
#define MALLINCAM_CONTRAST_MAX            100
#define MALLINCAM_GAMMA_DEF               100
#define MALLINCAM_GAMMA_MIN               20
#define MALLINCAM_GAMMA_MAX               180
#define MALLINCAM_AETARGET_DEF            120
#define MALLINCAM_AETARGET_MIN            16
#define MALLINCAM_AETARGET_MAX            235
#define MALLINCAM_WBGAIN_DEF              0
#define MALLINCAM_WBGAIN_MIN              (-128)
#define MALLINCAM_WBGAIN_MAX              128
    
    typedef struct{
        unsigned    width;
        unsigned    height;
    }MallincamResolution;
    
    /* In Windows platform, we always use UNICODE wchar_t */
    /* In Linux or OSX, we use char */
    typedef struct{
#ifdef _WIN32
        const wchar_t*      name;       /* model name */
#else
        const char*         name;
#endif
        unsigned            flag;       /* MALLINCAM_FLAG_xxx */
        unsigned            maxspeed;   /* number of speed level, same as Mallincam_get_MaxSpeed(), the speed range = [0, maxspeed], closed interval */
        unsigned            preview;    /* number of preview resolution, same as Mallincam_get_ResolutionNumber() */
        unsigned            still;      /* number of still resolution, same as Mallincam_get_StillResolutionNumber() */
        MallincamResolution   res[MALLINCAM_MAX];
    }MallincamModel;
    
    typedef struct{
#ifdef _WIN32
        wchar_t             displayname[64];    /* display name */
        wchar_t             id[64];     /* unique and opaque id of a connected camera, for Mallincam_Open */
#else
        char                displayname[64];    /* display name */
        char                id[64];     /* unique and opaque id of a connected camera, for Mallincam_Open */
#endif
        const MallincamModel* model;
    }MallincamInst;
    
    
    typedef enum MC_ERROR_CODE{ //ASI ERROR CODE
        MC_SUCCESS=0,
        MC_ERROR_INVALID_INDEX, //no camera connected or index value out of boundary
        MC_ERROR_INVALID_ID, //invalid ID
        MC_ERROR_INVALID_CONTROL_TYPE, //invalid control type
        MC_ERROR_CAMERA_CLOSED, //camera didn't open
        MC_ERROR_CAMERA_REMOVED, //failed to find the camera, maybe the camera has been removed
        MC_ERROR_INVALID_PATH, //cannot find the path of the file
        MC_ERROR_INVALID_FILEFORMAT,
        MC_ERROR_INVALID_SIZE, //wrong video format size
        MC_ERROR_INVALID_IMGTYPE, //unsupported image formate
        MC_ERROR_OUTOF_BOUNDARY, //the image is out of boundary
        MC_ERROR_TIMEOUT, //timeout
        MC_ERROR_INVALID_SENQUENCE,//stop capture first
        MC_ERROR_BUFFER_TOO_SMALL, //buffer size is not big enough
        MC_ERROR_VIDEO_MODE_ACTIVE,
        MC_ERROR_EXPOSURE_IN_PROGRESS,
        MC_ERROR_GENERAL_ERROR,//general error, eg: value is out of valid range
        MC_ERROR_END
    }MC_ERROR_CODE;
    typedef void (__stdcall*    PMALLINCAM_EVENT_CALLBACK)(unsigned nEvent, void* pCallbackCtx);
  
    typedef void (__stdcall*    PMALLINCAM_DATA_CALLBACK)(const void* pData, const BITMAPINFOHEADER* pHeader, BOOL bSnap, void* pCallbackCtx);
    
    
    class  MallincamGuider
    {
    public:
        HMallinCam m_Hmallincam;
        MallincamInst m_ti[MALLINCAM_MAX];
        int m_nIndex;
        MallincamGuider();
        ~MallincamGuider();
        
        //bool OpenDevice(const char* id);
#ifdef _WIN32
        mallincam_ports(HMallinCam) Mallincam_Open(const wchar_t* id);
#else
        mallincam_ports(HMallinCam) Mallincam_Open(const char* id);
#endif
       
        //int EnumCameras();
        mallincam_ports(unsigned) Mallincam_Enum(MallincamInst pti[MALLINCAM_MAX]);
        
        //MC_ERROR_CODE StartPullModeWithCallback(HMallinCam h, PMALLINCAM_EVENT_CALLBACK pEventCallback, void* pCallbackContext);
        mallincam_ports(MRESULT)      Mallincam_StartPullModeWithCallback(HMallinCam h, PMALLINCAM_EVENT_CALLBACK pEventCallback, void* pCallbackContext);
        
        MallincamModel GetCameraProperty(int i);
        
        MC_ERROR_CODE GetVideoData(int iCameraID, unsigned char* pBuffer, long lBuffSize, int iWaitms);
        
        
        //MC_ERROR_CODE PullImage(HMallinCam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
        mallincam_ports(MRESULT)      Mallincam_PullImage(HMallinCam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
        
        //MC_ERROR_CODE Stop(HMallinCam h);
        mallincam_ports(MRESULT)  Mallincam_Stop(HMallinCam h);
        
        //MC_ERROR_CODE GetResolution(HMallinCam h, unsigned nResolutionIndex, int* pWidth, int* pHeight );
        mallincam_ports(MRESULT)  Mallincam_get_Resolution(HMallinCam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
        
        mallincam_ports(MRESULT)  Mallincam_ST4PulseGuide(HMallinCam h, unsigned nDirect, unsigned nDuration);

        mallincam_ports(MRESULT)  Mallincam_get_ExpoAGainRange(HMallinCam h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef);

        mallincam_ports(MRESULT)  Mallincam_get_ExpoAGain(HMallinCam h, unsigned short* AGain); /* percent, such as 300 */
        mallincam_ports(MRESULT)  Mallincam_put_ExpoAGain(HMallinCam h, unsigned short AGain); /* percent */
        mallincam_ports(MRESULT)  Mallincam_get_ExpoTime(HMallinCam h, unsigned* Time); /* in microseconds */
        mallincam_ports(MRESULT)  Mallincam_put_ExpoTime(HMallinCam h, unsigned Time); /* in microseconds */

         mallincam_ports(MRESULT)  Mallincam_StartPushMode(HMallinCam h, PMALLINCAM_DATA_CALLBACK pDataCallback, void* pCallbackCtx);

    };
    
    /*
     get the version of this dll, which is: 1.8.7291.20160427
     */
#ifdef _WIN32
    mallincam_ports(const wchar_t*)   Mallincam_Version();
#else
    mallincam_ports(const char*)      Mallincam_Version();
#endif
    /*
     enumerate the cameras connected to the computer, return the number of enumerated.
     
     MallincamInst arr[MALLINCAM_MAX];
     unsigned cnt = Mallincam_Enum(arr);
     for (unsigned i = 0; i < cnt; ++i)
     ...
     
     if pti == NULL, then, only the number is returned.
     */
    mallincam_ports(unsigned) Mallincam_Enum(MallincamInst pti[MALLINCAM_MAX]);
    
    /* use the id of MallincamInst, which is enumerated by Mallincam_Enum.
     if id is NULL, Mallincam_Open will open the first camera.
     */
#ifdef _WIN32
    mallincam_ports(HMallinCam) Mallincam_Open(const wchar_t* id);
#else
    mallincam_ports(HMallinCam) Mallincam_Open(const char* id);
#endif
    
    /*
     the same with Mallincam_Open, but use the index as the parameter. such as:
     index == 0, open the first camera,
     index == 1, open the second camera,
     etc
     */
    mallincam_ports(HMallinCam) Mallincam_OpenByIndex(unsigned index);
    
    mallincam_ports(void)     Mallincam_Close(HMallinCam h); /* close the handle */
    
#define MALLINCAM_EVENT_EXPOSURE      0x0001    /* exposure time changed */
#define MALLINCAM_EVENT_TEMPTINT      0x0002    /* white balance changed, Temp/Tint mode */
#define MALLINCAM_EVENT_CHROME        0x0003    /* reversed, do not use it */
#define MALLINCAM_EVENT_IMAGE         0x0004    /* live image arrived, use Mallincam_PullImage to get this image */
#define MALLINCAM_EVENT_STILLIMAGE    0x0005    /* snap (still) frame arrived, use Mallincam_PullStillImage to get this frame */
#define MALLINCAM_EVENT_WBGAIN        0x0006    /* white balance changed, RGB Gain mode */
#define MALLINCAM_EVENT_ERROR         0x0080    /* something error happens */
#define MALLINCAM_EVENT_DISCONNECTED  0x0081    /* camera disconnected */
    
#ifdef _WIN32
    mallincam_ports(MRESULT)      Mallincam_StartPullModeWithWndMsg(HMallinCam h, HWND hWnd, UINT nMsg);
#endif
    
    mallincam_ports(MRESULT)      Mallincam_StartPullModeWithCallback(HMallinCam h, PMALLINCAM_EVENT_CALLBACK pEventCallback, void* pCallbackContext);
    
    /*
     bits: 24 (RGB24), 32 (RGB32), or 8 (Grey). Int RAW mode, this parameter is ignored.
     pnWidth, pnHeight: OUT parameter
     */
    mallincam_ports(MRESULT)      Mallincam_PullImage(HMallinCam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
    mallincam_ports(MRESULT)      Mallincam_PullStillImage(HMallinCam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
    
    /*
     (NULL == pData) means that something is error
     pCallbackCtx is the callback context which is passed by Mallincam_Start
     bSnap: TRUE if Mallincam_Snap
     
     pDataCallback is callbacked by an internal thread of toupcam.dll, so please pay attention to multithread problem
     */
    typedef void (__stdcall*    PMALLINCAM_DATA_CALLBACK)(const void* pData, const BITMAPINFOHEADER* pHeader, BOOL bSnap, void* pCallbackCtx);
    mallincam_ports(MRESULT)  Mallincam_StartPushMode(HMallinCam h, PMALLINCAM_DATA_CALLBACK pDataCallback, void* pCallbackCtx);
    
    mallincam_ports(MRESULT)  Mallincam_Stop(HMallinCam h);
    mallincam_ports(MRESULT)  Mallincam_Pause(HMallinCam h, BOOL bPause);
    
    /*  for pull mode: MALLINCAM_EVENT_STILLIMAGE, and then Mallincam_PullStillImage
     for push mode: the snapped image will be return by PMALLINCAM_DATA_CALLBACK, with the parameter 'bSnap' set to 'TRUE' */
    mallincam_ports(MRESULT)  Mallincam_Snap(HMallinCam h, unsigned nResolutionIndex);  /* still image snap */
    
    /*
     soft trigger:
     nNumber:    0xffff:     trigger continuously
     0:          cancel trigger
     others:     number of images to be triggered
     */
    mallincam_ports(MRESULT)  Mallincam_Trigger(HMallinCam h, unsigned short nNumber);
    /*
     put_Size, put_eSize, can be used to set the video output resolution BEFORE ToupCam_Start.
     put_Size use width and height parameters, put_eSize use the index parameter.
     for example, UCMOS03100KPA support the following resolutions:
     index 0:    2048,   1536
     index 1:    1024,   768
     index 2:    680,    510
     so, we can use put_Size(h, 1024, 768) or put_eSize(h, 1). Both have the same effect.
     */
    mallincam_ports(MRESULT)  Mallincam_put_Size(HMallinCam h, int nWidth, int nHeight);
    mallincam_ports(MRESULT)  Mallincam_get_Size(HMallinCam h, int* pWidth, int* pHeight);
    mallincam_ports(MRESULT)  Mallincam_put_eSize(HMallinCam h, unsigned nResolutionIndex);
    mallincam_ports(MRESULT)  Mallincam_get_eSize(HMallinCam h, unsigned* pnResolutionIndex);
    
    mallincam_ports(MRESULT)  Mallincam_get_ResolutionNumber(HMallinCam h);
    mallincam_ports(MRESULT)  Mallincam_get_Resolution(HMallinCam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
    mallincam_ports(MRESULT)  Mallincam_get_ResolutionRatio(HMallinCam h, unsigned nResolutionIndex, int* pNumerator, int* pDenominator);
    mallincam_ports(MRESULT)  Mallincam_get_Field(HMallinCam h);
    
    /*
     FourCC:
     MAKEFOURCC('G', 'B', 'R', 'G')
     MAKEFOURCC('R', 'G', 'G', 'B')
     MAKEFOURCC('B', 'G', 'G', 'R')
     MAKEFOURCC('G', 'R', 'B', 'G')
     MAKEFOURCC('Y', 'U', 'Y', 'V')
     MAKEFOURCC('Y', 'Y', 'Y', 'Y')
     */
    mallincam_ports(MRESULT)  Mallincam_get_RawFormat(HMallinCam h, unsigned* nFourCC, unsigned* bitdepth);
    
    /*
     ------------------------------------------------------------------|
     | Parameter               |   Range       |   Default             |
     |-----------------------------------------------------------------|
     | Auto Exposure Target    |   10~230      |   120                 |
     | Temp                    |   2000~15000  |   6503                |
     | Tint                    |   200~2500    |   1000                |
     | LevelRange              |   0~255       |   Low = 0, High = 255 |
     | Contrast                |   -100~100    |   0                   |
     | Hue                     |   -180~180    |   0                   |
     | Saturation              |   0~255       |   128                 |
     | Brightness              |   -64~64      |   0                   |
     | Gamma                   |   20~180      |   100                 |
     | WBGain                  |   -128~128    |   0                   |
     ------------------------------------------------------------------|
     */
    
#ifndef __MALLINCAM_CALLBACK_DEFINED__
#define __MALLINCAM_CALLBACK_DEFINED__
    typedef void (__stdcall* PIMALLINCAM_EXPOSURE_CALLBACK)(void* pCtx);
    typedef void (__stdcall* PIMALLINCAM_WHITEBALANCE_CALLBACK)(const int aGain[3], void* pCtx);
    typedef void (__stdcall* PIMALLINCAM_TEMPTINT_CALLBACK)(const int nTemp, const int nTint, void* pCtx);
    typedef void (__stdcall* PIMALLINCAM_HISTOGRAM_CALLBACK)(const float aHistY[256], const float aHistR[256], const float aHistG[256], const float aHistB[256], void* pCtx);
    typedef void (__stdcall* PIMALLINCAM_CHROME_CALLBACK)(void* pCtx);
#endif
    
    mallincam_ports(MRESULT)  Mallincam_get_AutoExpoEnable(HMallinCam h, BOOL* bAutoExposure);
    mallincam_ports(MRESULT)  Mallincam_put_AutoExpoEnable(HMallinCam h, BOOL bAutoExposure);
    mallincam_ports(MRESULT)  Mallincam_get_AutoExpoTarget(HMallinCam h, unsigned short* Target);
    mallincam_ports(MRESULT)  Mallincam_put_AutoExpoTarget(HMallinCam h, unsigned short Target);
    
    /*set the maximum auto exposure time and analog agin. The default maximum auto exposure time is 350ms */
    mallincam_ports(MRESULT)  Mallincam_put_MaxAutoExpoTimeAGain(HMallinCam h, unsigned maxTime, unsigned short maxAGain);
    
    mallincam_ports(MRESULT)  Mallincam_get_ExpoTime(HMallinCam h, unsigned* Time); /* in microseconds */
    mallincam_ports(MRESULT)  Mallincam_put_ExpoTime(HMallinCam h, unsigned Time); /* in microseconds */
    mallincam_ports(MRESULT)  Mallincam_get_ExpTimeRange(HMallinCam h, unsigned* nMin, unsigned* nMax, unsigned* nDef);
    
    mallincam_ports(MRESULT)  Mallincam_get_ExpoAGain(HMallinCam h, unsigned short* AGain); /* percent, such as 300 */
    mallincam_ports(MRESULT)  Mallincam_put_ExpoAGain(HMallinCam h, unsigned short AGain); /* percent */
    mallincam_ports(MRESULT)  Mallincam_get_ExpoAGainRange(HMallinCam h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef);
    
    /* Auto White Balance, Temp/Tint Mode */
    mallincam_ports(MRESULT)  Mallincam_AwbOnePush(HMallinCam h, PIMALLINCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx); /* auto white balance "one push". The function must be called AFTER Mallincam_StartXXXX */
    
    /* Auto White Balance, RGB Gain Mode */
    mallincam_ports(MRESULT)  Mallincam_AwbInit(HMallinCam h, PIMALLINCAM_WHITEBALANCE_CALLBACK fnWBProc, void* pWBCtx);
    
    /* White Balance, Temp/Tint mode */
    mallincam_ports(MRESULT)  Mallincam_put_TempTint(HMallinCam h, int nTemp, int nTint);
    mallincam_ports(MRESULT)  Mallincam_get_TempTint(HMallinCam h, int* nTemp, int* nTint);
    
    /* White Balance, RGB Gain mode */
    mallincam_ports(MRESULT)  Mallincam_put_WhiteBalanceGain(HMallinCam h, int aGain[3]);
    mallincam_ports(MRESULT)  Mallincam_get_WhiteBalanceGain(HMallinCam h, int aGain[3]);
    
    mallincam_ports(MRESULT)  Mallincam_put_Hue(HMallinCam h, int Hue);
    mallincam_ports(MRESULT)  Mallincam_get_Hue(HMallinCam h, int* Hue);
    mallincam_ports(MRESULT)  Mallincam_put_Saturation(HMallinCam h, int Saturation);
    mallincam_ports(MRESULT)  Mallincam_get_Saturation(HMallinCam h, int* Saturation);
    mallincam_ports(MRESULT)  Mallincam_put_Brightness(HMallinCam h, int Brightness);
    mallincam_ports(MRESULT)  Mallincam_get_Brightness(HMallinCam h, int* Brightness);
    mallincam_ports(MRESULT)  Mallincam_get_Contrast(HMallinCam h, int* Contrast);
    mallincam_ports(MRESULT)  Mallincam_put_Contrast(HMallinCam h, int Contrast);
    mallincam_ports(MRESULT)  Mallincam_get_Gamma(HMallinCam h, int* Gamma); /* percent */
    mallincam_ports(MRESULT)  Mallincam_put_Gamma(HMallinCam h, int Gamma);  /* percent */
    
    mallincam_ports(MRESULT)  Mallincam_get_Chrome(HMallinCam h, BOOL* bChrome);  /* monochromatic mode */
    mallincam_ports(MRESULT)  Mallincam_put_Chrome(HMallinCam h, BOOL bChrome);
    
    mallincam_ports(MRESULT)  Mallincam_get_VFlip(HMallinCam h, BOOL* bVFlip);  /* vertical flip */
    mallincam_ports(MRESULT)  Mallincam_put_VFlip(HMallinCam h, BOOL bVFlip);
    mallincam_ports(MRESULT)  Mallincam_get_HFlip(HMallinCam h, BOOL* bHFlip);
    mallincam_ports(MRESULT)  Mallincam_put_HFlip(HMallinCam h, BOOL bHFlip); /* horizontal flip */
    
    mallincam_ports(MRESULT)  Mallincam_get_Negative(HMallinCam h, BOOL* bNegative);  /* negative film */
    mallincam_ports(MRESULT)  Mallincam_put_Negative(HMallinCam h, BOOL bNegative);
    
    mallincam_ports(MRESULT)  Mallincam_put_Speed(HMallinCam h, unsigned short nSpeed);
    mallincam_ports(MRESULT)  Mallincam_get_Speed(HMallinCam h, unsigned short* pSpeed);
    mallincam_ports(MRESULT)  Mallincam_get_MaxSpeed(HMallinCam h); /* get the maximum speed, see "Frame Speed Level", the speed range = [0, max], closed interval */
    
    mallincam_ports(MRESULT)  Mallincam_get_FanMaxSpeed(HMallinCam h); /* get the maximum fan speed, the fan speed range = [0, max], closed interval */
    
    mallincam_ports(MRESULT)  Mallincam_get_MaxBitDepth(HMallinCam h); /* get the max bit depth of this camera, such as 8, 10, 12, 14, 16 */
    
    /* power supply:
     0 -> 60HZ AC
     1 -> 50Hz AC
     2 -> DC
     */
    mallincam_ports(MRESULT)  Mallincam_put_HZ(HMallinCam h, int nHZ);
    mallincam_ports(MRESULT)  Mallincam_get_HZ(HMallinCam h, int* nHZ);
    
    mallincam_ports(MRESULT)  Mallincam_put_Mode(HMallinCam h, BOOL bSkip); /* skip or bin */
    mallincam_ports(MRESULT)  Mallincam_get_Mode(HMallinCam h, BOOL* bSkip); /* If the model don't support bin/skip mode, return E_NOTIMPL */
    
    mallincam_ports(MRESULT)  Mallincam_put_AWBAuxRect(HMallinCam h, const RECT* pAuxRect); /* auto white balance ROI */
    mallincam_ports(MRESULT)  Mallincam_get_AWBAuxRect(HMallinCam h, RECT* pAuxRect);
    mallincam_ports(MRESULT)  Mallincam_put_AEAuxRect(HMallinCam h, const RECT* pAuxRect);  /* auto exposure ROI */
    mallincam_ports(MRESULT)  Mallincam_get_AEAuxRect(HMallinCam h, RECT* pAuxRect);
    
    /*
     S_FALSE:    color mode
     S_OK:       mono mode, such as EXCCD00300KMA and UHCCD01400KMA
     */
    mallincam_ports(MRESULT)  Mallincam_get_MonoMode(HMallinCam h);
    
    mallincam_ports(MRESULT)  Mallincam_get_StillResolutionNumber(HMallinCam h);
    mallincam_ports(MRESULT)  Mallincam_get_StillResolution(HMallinCam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
    
    /* default: FALSE */
    mallincam_ports(MRESULT)  Mallincam_put_RealTime(HMallinCam h, BOOL bEnable);
    mallincam_ports(MRESULT)  Mallincam_get_RealTime(HMallinCam h, BOOL* bEnable);
    
    mallincam_ports(MRESULT)  Mallincam_Flush(HMallinCam h);  /* discard the current internal frame cache */
    
    /* get the temperature of sensor, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius)
     return E_NOTIMPL if not supported
     */
    mallincam_ports(MRESULT)  Mallincam_get_Temperature(HMallinCam h, short* pTemperature);
    
    /* set the temperature of sensor, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius)
     return E_NOTIMPL if not supported
     */
    mallincam_ports(MRESULT)  Mallincam_put_Temperature(HMallinCam h, short nTemperature);
    
    /*
     get the serial number which is always 32 chars which is zero-terminated such as "TP110826145730ABCD1234FEDC56787"
     */
    mallincam_ports(MRESULT)  Mallincam_get_SerialNumber(HMallinCam h, char sn[32]);
    
    /*
     get the camera firmware version, such as: 3.2.1.20140922
     */
    mallincam_ports(MRESULT)  Mallincam_get_FwVersion(HMallinCam h, char fwver[16]);
    
    /*
     get the camera hardware version, such as: 3.2.1.20140922
     */
    mallincam_ports(MRESULT)  Mallincam_get_HwVersion(HMallinCam h, char hwver[16]);
    
    /*
     get the production date, such as: 20150327
     */
    mallincam_ports(MRESULT)  Mallincam_get_ProductionDate(HMallinCam h, char pdate[10]);
    
    /*
     get the sensor pixel size, such as: 2.4um
     */
    mallincam_ports(MRESULT)  Mallincam_get_PixelSize(HMallinCam h, unsigned nResolutionIndex, float* x, float* y);
    
    mallincam_ports(MRESULT)  Mallincam_put_LevelRange(HMallinCam h, unsigned short aLow[4], unsigned short aHigh[4]);
    mallincam_ports(MRESULT)  Mallincam_get_LevelRange(HMallinCam h, unsigned short aLow[4], unsigned short aHigh[4]);
    
    mallincam_ports(MRESULT)  Mallincam_put_ExpoCallback(HMallinCam h, PIMALLINCAM_EXPOSURE_CALLBACK fnExpoProc, void* pExpoCtx);
    mallincam_ports(MRESULT)  Mallincam_put_ChromeCallback(HMallinCam h, PIMALLINCAM_CHROME_CALLBACK fnChromeProc, void* pChromeCtx);
    
    /*
     The following functions must be called AFTER Mallincam_StartPushMode or Mallincam_StartPullModeWithWndMsg or Mallincam_StartPullModeWithCallback
     */
    mallincam_ports(MRESULT)  Mallincam_LevelRangeAuto(HMallinCam h);
    mallincam_ports(MRESULT)  Mallincam_GetHistogram(HMallinCam h, PIMALLINCAM_HISTOGRAM_CALLBACK fnHistogramProc, void* pHistogramCtx);
    
    /* led state:
     iLed: Led index, (0, 1, 2, ...)
     iState: 1 -> Ever bright; 2 -> Flashing; other -> Off
     iPeriod: Flashing Period (>= 500ms)
     */
    mallincam_ports(MRESULT)  Mallincam_put_LEDState(HMallinCam h, unsigned short iLed, unsigned short iState, unsigned short iPeriod);
    
    mallincam_ports(MRESULT)  Mallincam_write_EEPROM(HMallinCam h, unsigned addr, const unsigned char* pBuffer, unsigned nBufferLen);
    mallincam_ports(MRESULT)  Mallincam_read_EEPROM(HMallinCam h, unsigned addr, unsigned char* pBuffer, unsigned nBufferLen);
    
    mallincam_ports(MRESULT)  Mallincam_write_UART(HMallinCam h, const unsigned char* pData, unsigned nDataLen);
    mallincam_ports(MRESULT)  Mallincam_read_UART(HMallinCam h, unsigned char* pBuffer, unsigned nBufferLen);
    
#define MALLINCAM_TEC_TARGET_MIN      -300 /* -30.0 degrees Celsius */
#define MALLINCAM_TEC_TARGET_DEF      -100 /* -10.0 degrees Celsius */
#define MALLINCAM_TEC_TARGET_MAX      300  /* 30.0 degrees Celsius */
    
#define MALLINCAM_OPTION_NOFRAME_TIMEOUT      0x01    /* iValue: 1 = enable; 0 = disable. default: enable */
#define MALLINCAM_OPTION_THREAD_PRIORITY      0x02    /* set the priority of the internal thread which grab data from the usb device. iValue: 0 = THREAD_PRIORITY_NORMAL; 1 = THREAD_PRIORITY_ABOVE_NORMAL; 2 = THREAD_PRIORITY_HIGHEST; default: 0; see: msdn SetThreadPriority */
#define MALLINCAM_OPTION_PROCESSMODE          0x03    /*  0 = better image quality, more cpu usage. this is the default value
1 = lower image quality, less cpu usage */
#define MALLINCAM_OPTION_RAW                  0x04    /* raw mode, read the sensor data. This can be set only BEFORE Mallincam_StartXXX() */
#define MALLINCAM_OPTION_HISTOGRAM            0x05    /* 0 = only one, 1 = continue mode */
#define MALLINCAM_OPTION_BITDEPTH             0x06    /* 0 = 8 bits mode, 1 = 16 bits mode */
#define MALLINCAM_OPTION_FAN                  0x07    /* 0 = turn off the cooling fan, [1, max] = fan speed */
#define MALLINCAM_OPTION_TEC                  0x08    /* 0 = turn off the thermoelectric cooler, 1 = turn on the thermoelectric cooler */
#define MALLINCAM_OPTION_LINEAR               0x09    /* 0 = turn off the builtin linear tone mapping, 1 = turn on the builtin linear tone mapping, default value: 1 */
#define MALLINCAM_OPTION_CURVE                0x0a    /* 0 = turn off the builtin curve tone mapping, 1 = turn on the builtin curve tone mapping, default value: 1 */
#define MALLINCAM_OPTION_TRIGGER              0x0b    /* 0 = video mode, 1 = software or simulated trigger mode, 2 = external trigger mode, default value =  0 */
#define MALLINCAM_OPTION_RGB48                0x0c    /* enable RGB48 format when bitdepth > 8 */
#define MALLINCAM_OPTION_COLORMATIX           0x0d    /* enable or disable the builtin color matrix, default value: 1 */
#define MALLINCAM_OPTION_WBGAIN               0x0e    /* enable or disable the builtin white balance gain, default value: 1 */
#define MALLINCAM_OPTION_TECTARGET            0x0f    /* get or set the target temperature of the thermoelectric cooler, in 0.1 degree Celsius. For example, 125 means 12.5 degree Celsius */
#define MALLINCAM_OPTION_AGAIN                0x10    /* enable or disable adjusting the analog gain when auto exposure is enabled. default value: enable */
#define MALLINCAM_OPTION_FRAMERATE            0x11    /* limit the frame rate, range=[0, 63], the default value 0 means no limit. frame rate control is auto disabled in trigger mode */
    
    mallincam_ports(MRESULT)  Mallincam_put_Option(HMallinCam h, unsigned iOption, int iValue);
    mallincam_ports(MRESULT)  Mallincam_get_Option(HMallinCam h, unsigned iOption, int* piValue);
    
    mallincam_ports(MRESULT)  Mallincam_put_Roi(HMallinCam h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
    mallincam_ports(MRESULT)  Mallincam_get_Roi(HMallinCam h, unsigned* pxOffset, unsigned* pyOffset, unsigned* pxWidth, unsigned* pyHeight);
    
    /*
     get the frame rate: framerate (fps) = Frame * 1000.0 / nTime
     */
    mallincam_ports(MRESULT)  Mallincam_get_FrameRate(HMallinCam h, unsigned* nFrame, unsigned* nTime, unsigned* nTotalFrame);
    
    /* astronomy: for ST4 guide, please see: ASCOM Platform Help ICameraV2.
     nDirect: 0 = North, 1 = South, 2 = East, 3 = West, 4 = Stop
     nDuration: in milliseconds
     */
    mallincam_ports(MRESULT)  Mallincam_ST4PulseGuide(HMallinCam h, unsigned nDirect, unsigned nDuration);
    
    /* S_OK: pulse guiding
     S_FALSE: not pulse guiding
     */
    mallincam_ports(MRESULT)  Mallincam_ST4PlusGuideState(HMallinCam h);
    
    /*
     calculate the clarity factor:
     pImageData: pointer to the image data
     bits: 8(Grey), 24 (RGB24), 32(RGB32)
     nImgWidth, nImgHeight: the image width and height
     */
    mallincam_ports(double)   Mallincam_calc_ClarityFactor(const void* pImageData, int bits, unsigned nImgWidth, unsigned nImgHeight);
    
    mallincam_ports(void)     Mallincam_deBayer(unsigned nBayer, int nW, int nH, const void* input, void* output, unsigned char nBitDepth);
    
#ifndef _WIN32
    
    typedef void (*PMALLINCAM_HOTPLUG)(void* pCallbackCtx);
    mallincam_ports(void)   Mallincam_HotPlug(PMALLINCAM_HOTPLUG pHotPlugCallback, void* pCallbackCtx);
    
#else
    /*
     strRegPath, such as: Software\xxxCompany\yyyApplication.
     If we call this function to enable this feature, the camera parameters will be save in the Registry at HKEY_CURRENT_USER\Software\XxxCompany\yyyApplication\{CameraModelName} when we close the handle,
     and then, the next time, we open the camera, the parameters will be loaded automatically.
     */
    mallincam_ports(void)     Mallincam_EnableReg(const wchar_t* strRegPath);
    
    /* Mallincam_Start is obsolete, it's a synonyms for Mallincam_StartPushMode. */
    mallincam_ports(MRESULT)  Mallincam_Start(HMallinCam h, PMALLINCAM_DATA_CALLBACK pDataCallback, void* pCallbackCtx);
    
    /* Mallincam_put_TempTintInit is obsolete, it's a synonyms for Mallincam_AwbOnePush. */
    mallincam_ports(MRESULT)  Mallincam_put_TempTintInit(HMallinCam h, PIMALLINCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx);
    
    /*
     obsolete, please use Mallincam_put_Option or Mallincam_get_Option to set or get the process mode: MALLINCAM_PROCESSMODE_FULL or MALLINCAM_PROCESSMODE_FAST.
     default is MALLINCAM_PROCESSMODE_FULL.
     */
#ifndef __MALLINCAM_PROCESSMODE_DEFINED__
#define __MALLINCAM_PROCESSMODE_DEFINED__
#define MALLINCAM_PROCESSMODE_FULL        0x00    /* better image quality, more cpu usage. this is the default value */
#define MALLINCAM_PROCESSMODE_FAST        0x01    /* lower image quality, less cpu usage */
#endif
    
    mallincam_ports(MRESULT)  Mallincam_put_ProcessMode(HMallinCam h, unsigned nProcessMode);
    mallincam_ports(MRESULT)  Mallincam_get_ProcessMode(HMallinCam h, unsigned* pnProcessMode);
    
#endif
    
    /* obsolete, please use Mallincam_put_Roi and Mallincam_get_Roi */
    mallincam_ports(MRESULT)  Mallincam_put_RoiMode(HMallinCam h, BOOL bRoiMode, int xOffset, int yOffset);
    mallincam_ports(MRESULT)  Mallincam_get_RoiMode(HMallinCam h, BOOL* pbRoiMode, int* pxOffset, int* pyOffset);
    
    /* obsolete:
     ------------------------------------------------------------|
     | Parameter         |   Range       |   Default             |
     |-----------------------------------------------------------|
     | VidgetAmount      |   -100~100    |   0                   |
     | VignetMidPoint    |   0~100       |   50                  |
     -------------------------------------------------------------
     */
    mallincam_ports(MRESULT)  Mallincam_put_VignetEnable(HMallinCam h, BOOL bEnable);
    mallincam_ports(MRESULT)  Mallincam_get_VignetEnable(HMallinCam h, BOOL* bEnable);
    mallincam_ports(MRESULT)  Mallincam_put_VignetAmountInt(HMallinCam h, int nAmount);
    mallincam_ports(MRESULT)  Mallincam_get_VignetAmountInt(HMallinCam h, int* nAmount);
    mallincam_ports(MRESULT)  Mallincam_put_VignetMidPointInt(HMallinCam h, int nMidPoint);
    mallincam_ports(MRESULT)  Mallincam_get_VignetMidPointInt(HMallinCam h, int* nMidPoint);
    
#ifdef _WIN32
#pragma pack(pop)
#endif
    
#ifdef __cplusplus
}
#endif

#endif
