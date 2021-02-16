#ifndef __altaircam_h__
#define __altaircam_h__

/* Version: 48.18195.2020.1222 */
/*
   Platform & Architecture:
       (1) Win32:
              (a) x86: XP SP3 or above; CPU supports SSE2 instruction set or above
              (b) x64: Win7 or above
              (c) arm: Win10 or above
              (d) arm64: Win10 or above
       (2) WinRT: x86, x64, arm, arm64; Win10 or above
       (3) macOS: universal (x64 + x86); macOS 10.10 or above
       (4) Linux: kernel 2.6.27 or above
              (a) x86: CPU supports SSE3 instruction set or above; GLIBC 2.8 or above
              (b) x64: GLIBC 2.14 or above
              (c) armel: GLIBC 2.17 or above; built by toolchain arm-linux-gnueabi (version 4.9.2)
              (d) armhf: GLIBC 2.17 or above; built by toolchain arm-linux-gnueabihf (version 4.9.2)
              (e) arm64: GLIBC 2.17 or above; built by toolchain aarch64-linux-gnu (version 4.9.2)
       (5) Android: arm, arm64, x86, x64; built by android-ndk-r18b; __ANDROID_API__ = 23
*/
/*
    doc:
       (1) en.html, English
       (2) hans.html, Simplified Chinese
*/

#ifdef _WIN32
#ifndef _INC_WINDOWS
#include <windows.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus) && (__cplusplus >= 201402L)
#define ALTAIRCAM_DEPRECATED  [[deprecated]]
#elif defined(_MSC_VER)
#define ALTAIRCAM_DEPRECATED  __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#define ALTAIRCAM_DEPRECATED  __attribute__((deprecated))
#else
#define ALTAIRCAM_DEPRECATED
#endif

#ifdef _WIN32 /* Windows */

#pragma pack(push, 8)
#ifdef ALTAIRCAM_EXPORTS
#define ALTAIRCAM_API(x)    __declspec(dllexport)   x   __stdcall  /* in Windows, we use __stdcall calling convention, see https://docs.microsoft.com/en-us/cpp/cpp/stdcall */
#elif !defined(ALTAIRCAM_NOIMPORTS)
#define ALTAIRCAM_API(x)    __declspec(dllimport)   x   __stdcall
#else
#define ALTAIRCAM_API(x)    x   __stdcall
#endif

#else   /* Linux or macOS */

#define ALTAIRCAM_API(x)    x
#if (!defined(HRESULT)) && (!defined(__COREFOUNDATION_CFPLUGINCOM__)) /* CFPlugInCOM.h */
#define HRESULT int
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr)      (((HRESULT)(hr)) < 0)
#endif

#ifndef __stdcall
#define __stdcall
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

#ifndef TDIBWIDTHBYTES
#define TDIBWIDTHBYTES(bits)  ((unsigned)(((bits) + 31) & (~31)) / 8)
#endif

/********************************************************************************/
/* HRESULT                                                                      */
/*    |----------------|---------------------------------------|------------|   */
/*    | S_OK           |   Operation successful                | 0x00000000 |   */
/*    | S_FALSE        |   Operation successful                | 0x00000001 |   */
/*    | E_FAIL         |   Unspecified failure                 | 0x80004005 |   */
/*    | E_ACCESSDENIED |   General access denied error         | 0x80070005 |   */
/*    | E_INVALIDARG   |   One or more arguments are not valid | 0x80070057 |   */
/*    | E_NOTIMPL      |   Not supported or not implemented    | 0x80004001 |   */
/*    | E_NOINTERFACE  |   Interface not supported             | 0x80004002 |   */
/*    | E_POINTER      |   Pointer that is not valid           | 0x80004003 |   */
/*    | E_UNEXPECTED   |   Unexpected failure                  | 0x8000FFFF |   */
/*    | E_OUTOFMEMORY  |   Out of memory                       | 0x8007000E |   */
/*    | E_WRONG_THREAD |   call function in the wrong thread   | 0x8001010E |   */
/*    | E_GEN_FAILURE  |   device not functioning              | 0x8007001F |   */
/*    |----------------|---------------------------------------|------------|   */
/********************************************************************************/
/*                                                                              */
/* Please note that the return value >= 0 means success                         */
/* (especially S_FALSE is also successful, indicating that the internal value and the value set by the user is equivalent, which means "no operation"). */
/* Therefore, the SUCCEEDEDand FAILED macros should generally be used to determine whether the return value is successful or failed. */
/* (Unless there are special needs, do not use "==S_OK" or "==0" to judge the return value) */
/*                                                                              */
/* #define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)                               */
/* #define FAILED(hr)      (((HRESULT)(hr)) < 0)                                */
/*                                                                              */
/********************************************************************************/

/* handle */
typedef struct AltaircamT { int unused; } *HAltaircam, *HAltairCam;

#define ALTAIRCAM_MAX                      16
                                         
#define ALTAIRCAM_FLAG_CMOS                0x00000001  /* cmos sensor */
#define ALTAIRCAM_FLAG_CCD_PROGRESSIVE     0x00000002  /* progressive ccd sensor */
#define ALTAIRCAM_FLAG_CCD_INTERLACED      0x00000004  /* interlaced ccd sensor */
#define ALTAIRCAM_FLAG_ROI_HARDWARE        0x00000008  /* support hardware ROI */
#define ALTAIRCAM_FLAG_MONO                0x00000010  /* monochromatic */
#define ALTAIRCAM_FLAG_BINSKIP_SUPPORTED   0x00000020  /* support bin/skip mode, see Altaircam_put_Mode and Altaircam_get_Mode */
#define ALTAIRCAM_FLAG_USB30               0x00000040  /* usb3.0 */
#define ALTAIRCAM_FLAG_TEC                 0x00000080  /* Thermoelectric Cooler */
#define ALTAIRCAM_FLAG_USB30_OVER_USB20    0x00000100  /* usb3.0 camera connected to usb2.0 port */
#define ALTAIRCAM_FLAG_ST4                 0x00000200  /* ST4 port */
#define ALTAIRCAM_FLAG_GETTEMPERATURE      0x00000400  /* support to get the temperature of the sensor */
#define ALTAIRCAM_FLAG_RAW10               0x00001000  /* pixel format, RAW 10bits */
#define ALTAIRCAM_FLAG_RAW12               0x00002000  /* pixel format, RAW 12bits */
#define ALTAIRCAM_FLAG_RAW14               0x00004000  /* pixel format, RAW 14bits */
#define ALTAIRCAM_FLAG_RAW16               0x00008000  /* pixel format, RAW 16bits */
#define ALTAIRCAM_FLAG_FAN                 0x00010000  /* cooling fan */
#define ALTAIRCAM_FLAG_TEC_ONOFF           0x00020000  /* Thermoelectric Cooler can be turn on or off, support to set the target temperature of TEC */
#define ALTAIRCAM_FLAG_ISP                 0x00040000  /* ISP (Image Signal Processing) chip */
#define ALTAIRCAM_FLAG_TRIGGER_SOFTWARE    0x00080000  /* support software trigger */
#define ALTAIRCAM_FLAG_TRIGGER_EXTERNAL    0x00100000  /* support external trigger */
#define ALTAIRCAM_FLAG_TRIGGER_SINGLE      0x00200000  /* only support trigger single: one trigger, one image */
#define ALTAIRCAM_FLAG_BLACKLEVEL          0x00400000  /* support set and get the black level */
#define ALTAIRCAM_FLAG_AUTO_FOCUS          0x00800000  /* support auto focus */
#define ALTAIRCAM_FLAG_BUFFER              0x01000000  /* frame buffer */
#define ALTAIRCAM_FLAG_DDR                 0x02000000  /* use very large capacity DDR (Double Data Rate SDRAM) for frame buffer */
#define ALTAIRCAM_FLAG_CG                  0x04000000  /* Conversion Gain: HCG, LCG */
#define ALTAIRCAM_FLAG_YUV411              0x08000000  /* pixel format, yuv411 */
#define ALTAIRCAM_FLAG_VUYY                0x10000000  /* pixel format, yuv422, VUYY */
#define ALTAIRCAM_FLAG_YUV444              0x20000000  /* pixel format, yuv444 */
#define ALTAIRCAM_FLAG_RGB888              0x40000000  /* pixel format, RGB888 */
#define ALTAIRCAM_FLAG_RAW8                0x80000000  /* pixel format, RAW 8 bits */
#define ALTAIRCAM_FLAG_GMCY8               0x0000000100000000  /* pixel format, GMCY, 8bits */
#define ALTAIRCAM_FLAG_GMCY12              0x0000000200000000  /* pixel format, GMCY, 12bits */
#define ALTAIRCAM_FLAG_UYVY                0x0000000400000000  /* pixel format, yuv422, UYVY */
#define ALTAIRCAM_FLAG_CGHDR               0x0000000800000000  /* Conversion Gain: HCG, LCG, HDR */
#define ALTAIRCAM_FLAG_GLOBALSHUTTER       0x0000001000000000  /* global shutter */
#define ALTAIRCAM_FLAG_FOCUSMOTOR          0x0000002000000000  /* support focus motor */
#define ALTAIRCAM_FLAG_PRECISE_FRAMERATE   0x0000004000000000  /* support precise framerate & bandwidth, see ALTAIRCAM_OPTION_PRECISE_FRAMERATE & ALTAIRCAM_OPTION_BANDWIDTH */
#define ALTAIRCAM_FLAG_HEAT                0x0000008000000000  /* heat to prevent fogging up */
#define ALTAIRCAM_FLAG_LOW_NOISE           0x0000010000000000  /* low noise mode */
#define ALTAIRCAM_FLAG_LEVELRANGE_HARDWARE 0x0000020000000000  /* hardware level range, put(get)_LevelRangeV2 */
#define ALTAIRCAM_FLAG_EVENT_HARDWARE      0x0000040000000000  /* hardware event, such as exposure start & stop */

#define ALTAIRCAM_TEMP_DEF                 6503    /* temp, default */
#define ALTAIRCAM_TEMP_MIN                 2000    /* temp, minimum */
#define ALTAIRCAM_TEMP_MAX                 15000   /* temp, maximum */
#define ALTAIRCAM_TINT_DEF                 1000    /* tint */
#define ALTAIRCAM_TINT_MIN                 200     /* tint */
#define ALTAIRCAM_TINT_MAX                 2500    /* tint */
#define ALTAIRCAM_HUE_DEF                  0       /* hue */
#define ALTAIRCAM_HUE_MIN                  (-180)  /* hue */
#define ALTAIRCAM_HUE_MAX                  180     /* hue */
#define ALTAIRCAM_SATURATION_DEF           128     /* saturation */
#define ALTAIRCAM_SATURATION_MIN           0       /* saturation */
#define ALTAIRCAM_SATURATION_MAX           255     /* saturation */
#define ALTAIRCAM_BRIGHTNESS_DEF           0       /* brightness */
#define ALTAIRCAM_BRIGHTNESS_MIN           (-64)   /* brightness */
#define ALTAIRCAM_BRIGHTNESS_MAX           64      /* brightness */
#define ALTAIRCAM_CONTRAST_DEF             0       /* contrast */
#define ALTAIRCAM_CONTRAST_MIN             (-100)  /* contrast */
#define ALTAIRCAM_CONTRAST_MAX             100     /* contrast */
#define ALTAIRCAM_GAMMA_DEF                100     /* gamma */
#define ALTAIRCAM_GAMMA_MIN                20      /* gamma */
#define ALTAIRCAM_GAMMA_MAX                180     /* gamma */
#define ALTAIRCAM_AETARGET_DEF             120     /* target of auto exposure */
#define ALTAIRCAM_AETARGET_MIN             16      /* target of auto exposure */
#define ALTAIRCAM_AETARGET_MAX             220     /* target of auto exposure */
#define ALTAIRCAM_WBGAIN_DEF               0       /* white balance gain */
#define ALTAIRCAM_WBGAIN_MIN               (-127)  /* white balance gain */
#define ALTAIRCAM_WBGAIN_MAX               127     /* white balance gain */
#define ALTAIRCAM_BLACKLEVEL_MIN           0       /* minimum black level */
#define ALTAIRCAM_BLACKLEVEL8_MAX          31              /* maximum black level for bit depth = 8 */
#define ALTAIRCAM_BLACKLEVEL10_MAX         (31 * 4)        /* maximum black level for bit depth = 10 */
#define ALTAIRCAM_BLACKLEVEL12_MAX         (31 * 16)       /* maximum black level for bit depth = 12 */
#define ALTAIRCAM_BLACKLEVEL14_MAX         (31 * 64)       /* maximum black level for bit depth = 14 */
#define ALTAIRCAM_BLACKLEVEL16_MAX         (31 * 256)      /* maximum black level for bit depth = 16 */
#define ALTAIRCAM_SHARPENING_STRENGTH_DEF  0       /* sharpening strength */
#define ALTAIRCAM_SHARPENING_STRENGTH_MIN  0       /* sharpening strength */
#define ALTAIRCAM_SHARPENING_STRENGTH_MAX  500     /* sharpening strength */
#define ALTAIRCAM_SHARPENING_RADIUS_DEF    2       /* sharpening radius */
#define ALTAIRCAM_SHARPENING_RADIUS_MIN    1       /* sharpening radius */
#define ALTAIRCAM_SHARPENING_RADIUS_MAX    10      /* sharpening radius */
#define ALTAIRCAM_SHARPENING_THRESHOLD_DEF 0       /* sharpening threshold */
#define ALTAIRCAM_SHARPENING_THRESHOLD_MIN 0       /* sharpening threshold */
#define ALTAIRCAM_SHARPENING_THRESHOLD_MAX 255     /* sharpening threshold */
#define ALTAIRCAM_AUTOEXPO_THRESHOLD_DEF   5       /* auto exposure threshold */
#define ALTAIRCAM_AUTOEXPO_THRESHOLD_MIN   2       /* auto exposure threshold */
#define ALTAIRCAM_AUTOEXPO_THRESHOLD_MAX   15      /* auto exposure threshold */
#define ALTAIRCAM_BANDWIDTH_DEF            90      /* bandwidth */
#define ALTAIRCAM_BANDWIDTH_MIN            1       /* bandwidth */
#define ALTAIRCAM_BANDWIDTH_MAX            100     /* bandwidth */
#define ALTAIRCAM_DENOISE_DEF              0       /* denoise */
#define ALTAIRCAM_DENOISE_MIN              0       /* denoise */
#define ALTAIRCAM_DENOISE_MAX              100     /* denoise */

typedef struct{
    unsigned    width;
    unsigned    height;
}AltaircamResolution;

/* In Windows platform, we always use UNICODE wchar_t */
/* In Linux or macOS, we use char */

typedef struct {
#ifdef _WIN32
    const wchar_t*      name;        /* model name, in Windows, we use unicode */
#else
    const char*         name;        /* model name */
#endif
    unsigned long long  flag;        /* ALTAIRCAM_FLAG_xxx, 64 bits */
    unsigned            maxspeed;    /* number of speed level, same as Altaircam_get_MaxSpeed(), the speed range = [0, maxspeed], closed interval */
    unsigned            preview;     /* number of preview resolution, same as Altaircam_get_ResolutionNumber() */
    unsigned            still;       /* number of still resolution, same as Altaircam_get_StillResolutionNumber() */
    unsigned            maxfanspeed; /* maximum fan speed */
    unsigned            ioctrol;     /* number of input/output control */
    float               xpixsz;      /* physical pixel size */
    float               ypixsz;      /* physical pixel size */
    AltaircamResolution   res[ALTAIRCAM_MAX];
}AltaircamModelV2; /* camera model v2 */

typedef struct {
#ifdef _WIN32
    wchar_t               displayname[64];    /* display name */
    wchar_t               id[64];             /* unique and opaque id of a connected camera, for Altaircam_Open */
#else
    char                  displayname[64];    /* display name */
    char                  id[64];             /* unique and opaque id of a connected camera, for Altaircam_Open */
#endif
    const AltaircamModelV2* model;
}AltaircamDeviceV2; /* camera instance for enumerating */

/*
    get the version of this dll/so/dylib, which is: 48.18195.2020.1222
*/
#ifdef _WIN32
ALTAIRCAM_API(const wchar_t*)   Altaircam_Version();
#else
ALTAIRCAM_API(const char*)      Altaircam_Version();
#endif

/*
    enumerate the cameras connected to the computer, return the number of enumerated.

    AltaircamDeviceV2 arr[ALTAIRCAM_MAX];
    unsigned cnt = Altaircam_EnumV2(arr);
    for (unsigned i = 0; i < cnt; ++i)
        ...

    if pti == NULL, then, only the number is returned.
    Altaircam_Enum is obsolete.
*/
ALTAIRCAM_API(unsigned) Altaircam_EnumV2(AltaircamDeviceV2 pti[ALTAIRCAM_MAX]);

/* use the id of AltaircamDeviceV2, which is enumerated by Altaircam_EnumV2.
    if id is NULL, Altaircam_Open will open the first camera.
*/
#ifdef _WIN32
ALTAIRCAM_API(HAltaircam) Altaircam_Open(const wchar_t* id);
#else
ALTAIRCAM_API(HAltaircam) Altaircam_Open(const char* id);
#endif

/*
    the same with Altaircam_Open, but use the index as the parameter. such as:
    index == 0, open the first camera,
    index == 1, open the second camera,
    etc
*/
ALTAIRCAM_API(HAltaircam) Altaircam_OpenByIndex(unsigned index);

ALTAIRCAM_API(void)     Altaircam_Close(HAltaircam h);  /* close the handle */

#define ALTAIRCAM_EVENT_EXPOSURE          0x0001    /* exposure time or gain changed */
#define ALTAIRCAM_EVENT_TEMPTINT          0x0002    /* white balance changed, Temp/Tint mode */
#define ALTAIRCAM_EVENT_IMAGE             0x0004    /* live image arrived, use Altaircam_PullImage to get this image */
#define ALTAIRCAM_EVENT_STILLIMAGE        0x0005    /* snap (still) frame arrived, use Altaircam_PullStillImage to get this frame */
#define ALTAIRCAM_EVENT_WBGAIN            0x0006    /* white balance changed, RGB Gain mode */
#define ALTAIRCAM_EVENT_TRIGGERFAIL       0x0007    /* trigger failed */
#define ALTAIRCAM_EVENT_BLACK             0x0008    /* black balance changed */
#define ALTAIRCAM_EVENT_FFC               0x0009    /* flat field correction status changed */
#define ALTAIRCAM_EVENT_DFC               0x000a    /* dark field correction status changed */
#define ALTAIRCAM_EVENT_ROI               0x000b    /* roi changed */
#define ALTAIRCAM_EVENT_LEVELRANGE        0x000c    /* level range changed */
#define ALTAIRCAM_EVENT_ERROR             0x0080    /* generic error */
#define ALTAIRCAM_EVENT_DISCONNECTED      0x0081    /* camera disconnected */
#define ALTAIRCAM_EVENT_NOFRAMETIMEOUT    0x0082    /* no frame timeout error */
#define ALTAIRCAM_EVENT_AFFEEDBACK        0x0083    /* auto focus feedback information */
#define ALTAIRCAM_EVENT_AFPOSITION        0x0084    /* auto focus sensor board positon */
#define ALTAIRCAM_EVENT_NOPACKETTIMEOUT   0x0085    /* no packet timeout */
#define ALTAIRCAM_EVENT_EXPO_START        0x4000    /* exposure start */
#define ALTAIRCAM_EVENT_EXPO_STOP         0x4001    /* exposure stop */
#define ALTAIRCAM_EVENT_TRIGGER_ALLOW     0x4002    /* next trigger allow */
#define ALTAIRCAM_EVENT_FACTORY           0x8001    /* restore factory settings */

#ifdef _WIN32
ALTAIRCAM_API(HRESULT)  Altaircam_StartPullModeWithWndMsg(HAltaircam h, HWND hWnd, UINT nMsg);
#endif

/* Do NOT call Altaircam_Close, Altaircam_Stop in this callback context, it deadlocks. */
typedef void (__stdcall* PALTAIRCAM_EVENT_CALLBACK)(unsigned nEvent, void* pCallbackCtx);
ALTAIRCAM_API(HRESULT)  Altaircam_StartPullModeWithCallback(HAltaircam h, PALTAIRCAM_EVENT_CALLBACK pEventCallback, void* pCallbackContext);

#define ALTAIRCAM_FRAMEINFO_FLAG_SEQ          0x01 /* sequence number */
#define ALTAIRCAM_FRAMEINFO_FLAG_TIMESTAMP    0x02 /* timestamp */

typedef struct {
    unsigned            width;
    unsigned            height;
    unsigned            flag;       /* ALTAIRCAM_FRAMEINFO_FLAG_xxxx */
    unsigned            seq;        /* sequence number */
    unsigned long long  timestamp;  /* microsecond */
}AltaircamFrameInfoV2;

/*
    bits: 24 (RGB24), 32 (RGB32), 8 (Gray) or 16 (Gray). In RAW mode, this parameter is ignored.
    pnWidth, pnHeight: OUT parameter
    rowPitch: The distance from one row to the next row. rowPitch = 0 means using the default row pitch.
    
    -------------------------------------------------------------------------------------
    | format                                            | default row pitch             |
    |---------------------------------------------------|-------------------------------|
    | RGB       | RGB24                                 | TDIBWIDTHBYTES(24 * Width)    |
    |           | RGB32                                 | Width * 4                     |
    |           | RGB48                                 | TDIBWIDTHBYTES(48 * Width)    |
    |           | RGB8 grey image                       | TDIBWIDTHBYTES(8 * Width)     |
    |-----------|---------------------------------------|-------------------------------|
    | RAW       | 8bits Mode                            | Width                         |
    |           | 10bits, 12bits, 14bits, 16bits Mode   | Width * 2                     |
    |-----------|---------------------------------------|-------------------------------|
*/
ALTAIRCAM_API(HRESULT)  Altaircam_PullImageV2(HAltaircam h, void* pImageData, int bits, AltaircamFrameInfoV2* pInfo);
ALTAIRCAM_API(HRESULT)  Altaircam_PullStillImageV2(HAltaircam h, void* pImageData, int bits, AltaircamFrameInfoV2* pInfo);
ALTAIRCAM_API(HRESULT)  Altaircam_PullImageWithRowPitchV2(HAltaircam h, void* pImageData, int bits, int rowPitch, AltaircamFrameInfoV2* pInfo);
ALTAIRCAM_API(HRESULT)  Altaircam_PullStillImageWithRowPitchV2(HAltaircam h, void* pImageData, int bits, int rowPitch, AltaircamFrameInfoV2* pInfo);

ALTAIRCAM_API(HRESULT)  Altaircam_PullImage(HAltaircam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
ALTAIRCAM_API(HRESULT)  Altaircam_PullStillImage(HAltaircam h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
ALTAIRCAM_API(HRESULT)  Altaircam_PullImageWithRowPitch(HAltaircam h, void* pImageData, int bits, int rowPitch, unsigned* pnWidth, unsigned* pnHeight);
ALTAIRCAM_API(HRESULT)  Altaircam_PullStillImageWithRowPitch(HAltaircam h, void* pImageData, int bits, int rowPitch, unsigned* pnWidth, unsigned* pnHeight);

/*
    (NULL == pData) means that something is error
    pCallbackCtx is the callback context which is passed by Altaircam_StartPushModeV3
    bSnap: TRUE if Altaircam_Snap

    pDataCallback is callbacked by an internal thread of altaircam.dll, so please pay attention to multithread problem.
    Do NOT call Altaircam_Close, Altaircam_Stop in this callback context, it deadlocks.
*/
typedef void (__stdcall* PALTAIRCAM_DATA_CALLBACK_V3)(const void* pData, const AltaircamFrameInfoV2* pInfo, int bSnap, void* pCallbackCtx);
ALTAIRCAM_API(HRESULT)  Altaircam_StartPushModeV3(HAltaircam h, PALTAIRCAM_DATA_CALLBACK_V3 pDataCallback, void* pDataCallbackCtx, PALTAIRCAM_EVENT_CALLBACK pEventCallback, void* pEventCallbackContext);

ALTAIRCAM_API(HRESULT)  Altaircam_Stop(HAltaircam h);
ALTAIRCAM_API(HRESULT)  Altaircam_Pause(HAltaircam h, int bPause);

/*  for pull mode: ALTAIRCAM_EVENT_STILLIMAGE, and then Altaircam_PullStillImage
    for push mode: the snapped image will be return by PALTAIRCAM_DATA_CALLBACK(V2), with the parameter 'bSnap' set to 'TRUE'
*/
ALTAIRCAM_API(HRESULT)  Altaircam_Snap(HAltaircam h, unsigned nResolutionIndex);  /* still image snap */
ALTAIRCAM_API(HRESULT)  Altaircam_SnapN(HAltaircam h, unsigned nResolutionIndex, unsigned nNumber);  /* multiple still image snap */
/*
    soft trigger:
    nNumber:    0xffff:     trigger continuously
                0:          cancel trigger
                others:     number of images to be triggered
*/
ALTAIRCAM_API(HRESULT)  Altaircam_Trigger(HAltaircam h, unsigned short nNumber);

/*
    put_Size, put_eSize, can be used to set the video output resolution BEFORE Altaircam_StartXXXX.
    put_Size use width and height parameters, put_eSize use the index parameter.
    for example, UCMOS03100KPA support the following resolutions:
            index 0:    2048,   1536
            index 1:    1024,   768
            index 2:    680,    510
    so, we can use put_Size(h, 1024, 768) or put_eSize(h, 1). Both have the same effect.
*/
ALTAIRCAM_API(HRESULT)  Altaircam_put_Size(HAltaircam h, int nWidth, int nHeight);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Size(HAltaircam h, int* pWidth, int* pHeight);
ALTAIRCAM_API(HRESULT)  Altaircam_put_eSize(HAltaircam h, unsigned nResolutionIndex);
ALTAIRCAM_API(HRESULT)  Altaircam_get_eSize(HAltaircam h, unsigned* pnResolutionIndex);

/*
    final size after ROI, rotate, binning
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_FinalSize(HAltaircam h, int* pWidth, int* pHeight);

ALTAIRCAM_API(HRESULT)  Altaircam_get_ResolutionNumber(HAltaircam h);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Resolution(HAltaircam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
/*
    numerator/denominator, such as: 1/1, 1/2, 1/3
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_ResolutionRatio(HAltaircam h, unsigned nResolutionIndex, int* pNumerator, int* pDenominator);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Field(HAltaircam h);

/*
see: http://www.fourcc.org
FourCC:
    MAKEFOURCC('G', 'B', 'R', 'G'), see http://www.siliconimaging.com/RGB%20Bayer.htm
    MAKEFOURCC('R', 'G', 'G', 'B')
    MAKEFOURCC('B', 'G', 'G', 'R')
    MAKEFOURCC('G', 'R', 'B', 'G')
    MAKEFOURCC('Y', 'Y', 'Y', 'Y'), monochromatic sensor
    MAKEFOURCC('Y', '4', '1', '1'), yuv411
    MAKEFOURCC('V', 'U', 'Y', 'Y'), yuv422
    MAKEFOURCC('U', 'Y', 'V', 'Y'), yuv422
    MAKEFOURCC('Y', '4', '4', '4'), yuv444
    MAKEFOURCC('R', 'G', 'B', '8'), RGB888

#ifndef MAKEFOURCC
#define MAKEFOURCC(a, b, c, d) ((unsigned)(unsigned char)(a) | ((unsigned)(unsigned char)(b) << 8) | ((unsigned)(unsigned char)(c) << 16) | ((unsigned)(unsigned char)(d) << 24))
#endif
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_RawFormat(HAltaircam h, unsigned* nFourCC, unsigned* bitsperpixel);

/*
    ------------------------------------------------------------------|
    | Parameter               |   Range       |   Default             |
    |-----------------------------------------------------------------|
    | Auto Exposure Target    |   10~220      |   120                 |
    | Temp                    |   2000~15000  |   6503                |
    | Tint                    |   200~2500    |   1000                |
    | LevelRange              |   0~255       |   Low = 0, High = 255 |
    | Contrast                |   -100~100    |   0                   |
    | Hue                     |   -180~180    |   0                   |
    | Saturation              |   0~255       |   128                 |
    | Brightness              |   -64~64      |   0                   |
    | Gamma                   |   20~180      |   100                 |
    | WBGain                  |   -127~127    |   0                   |
    ------------------------------------------------------------------|
*/

#ifndef __ALTAIRCAM_CALLBACK_DEFINED__
#define __ALTAIRCAM_CALLBACK_DEFINED__
typedef void (__stdcall* PIALTAIRCAM_EXPOSURE_CALLBACK)(void* pCtx);                                     /* auto exposure */
typedef void (__stdcall* PIALTAIRCAM_WHITEBALANCE_CALLBACK)(const int aGain[3], void* pCtx);             /* once white balance, RGB Gain mode */
typedef void (__stdcall* PIALTAIRCAM_BLACKBALANCE_CALLBACK)(const unsigned short aSub[3], void* pCtx);   /* once black balance */
typedef void (__stdcall* PIALTAIRCAM_TEMPTINT_CALLBACK)(const int nTemp, const int nTint, void* pCtx);   /* once white balance, Temp/Tint Mode */
typedef void (__stdcall* PIALTAIRCAM_HISTOGRAM_CALLBACK)(const float aHistY[256], const float aHistR[256], const float aHistG[256], const float aHistB[256], void* pCtx);
typedef void (__stdcall* PIALTAIRCAM_CHROME_CALLBACK)(void* pCtx);
#endif

ALTAIRCAM_API(HRESULT)  Altaircam_get_AutoExpoEnable(HAltaircam h, int* bAutoExposure);
ALTAIRCAM_API(HRESULT)  Altaircam_put_AutoExpoEnable(HAltaircam h, int bAutoExposure);
ALTAIRCAM_API(HRESULT)  Altaircam_get_AutoExpoTarget(HAltaircam h, unsigned short* Target);
ALTAIRCAM_API(HRESULT)  Altaircam_put_AutoExpoTarget(HAltaircam h, unsigned short Target);

/*set the maximum/minimal auto exposure time and agin. The default maximum auto exposure time is 350ms */
ALTAIRCAM_API(HRESULT)  Altaircam_put_MaxAutoExpoTimeAGain(HAltaircam h, unsigned maxTime, unsigned short maxAGain);
ALTAIRCAM_API(HRESULT)  Altaircam_get_MaxAutoExpoTimeAGain(HAltaircam h, unsigned* maxTime, unsigned short* maxAGain);
ALTAIRCAM_API(HRESULT)  Altaircam_put_MinAutoExpoTimeAGain(HAltaircam h, unsigned minTime, unsigned short minAGain);
ALTAIRCAM_API(HRESULT)  Altaircam_get_MinAutoExpoTimeAGain(HAltaircam h, unsigned* minTime, unsigned short* minAGain);

ALTAIRCAM_API(HRESULT)  Altaircam_get_ExpoTime(HAltaircam h, unsigned* Time); /* in microseconds */
ALTAIRCAM_API(HRESULT)  Altaircam_put_ExpoTime(HAltaircam h, unsigned Time); /* in microseconds */
ALTAIRCAM_API(HRESULT)  Altaircam_get_RealExpoTime(HAltaircam h, unsigned* Time); /* in microseconds, based on 50HZ/60HZ/DC */
ALTAIRCAM_API(HRESULT)  Altaircam_get_ExpTimeRange(HAltaircam h, unsigned* nMin, unsigned* nMax, unsigned* nDef);

ALTAIRCAM_API(HRESULT)  Altaircam_get_ExpoAGain(HAltaircam h, unsigned short* AGain); /* percent, such as 300 */
ALTAIRCAM_API(HRESULT)  Altaircam_put_ExpoAGain(HAltaircam h, unsigned short AGain); /* percent */
ALTAIRCAM_API(HRESULT)  Altaircam_get_ExpoAGainRange(HAltaircam h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef);

/* Auto White Balance "Once", Temp/Tint Mode */
ALTAIRCAM_API(HRESULT)  Altaircam_AwbOnce(HAltaircam h, PIALTAIRCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx); /* auto white balance "once". This function must be called AFTER Altaircam_StartXXXX */

/* Auto White Balance, RGB Gain Mode */
ALTAIRCAM_API(HRESULT)  Altaircam_AwbInit(HAltaircam h, PIALTAIRCAM_WHITEBALANCE_CALLBACK fnWBProc, void* pWBCtx);

/* White Balance, Temp/Tint mode */
ALTAIRCAM_API(HRESULT)  Altaircam_put_TempTint(HAltaircam h, int nTemp, int nTint);
ALTAIRCAM_API(HRESULT)  Altaircam_get_TempTint(HAltaircam h, int* nTemp, int* nTint);

/* White Balance, RGB Gain mode */
ALTAIRCAM_API(HRESULT)  Altaircam_put_WhiteBalanceGain(HAltaircam h, int aGain[3]);
ALTAIRCAM_API(HRESULT)  Altaircam_get_WhiteBalanceGain(HAltaircam h, int aGain[3]);

/* Black Balance */
ALTAIRCAM_API(HRESULT)  Altaircam_AbbOnce(HAltaircam h, PIALTAIRCAM_BLACKBALANCE_CALLBACK fnBBProc, void* pBBCtx); /* auto black balance "once". This function must be called AFTER Altaircam_StartXXXX */
ALTAIRCAM_API(HRESULT)  Altaircam_put_BlackBalance(HAltaircam h, unsigned short aSub[3]);
ALTAIRCAM_API(HRESULT)  Altaircam_get_BlackBalance(HAltaircam h, unsigned short aSub[3]);

/* Flat Field Correction */
ALTAIRCAM_API(HRESULT)  Altaircam_FfcOnce(HAltaircam h);
#ifdef _WIN32
ALTAIRCAM_API(HRESULT)  Altaircam_FfcExport(HAltaircam h, const wchar_t* filepath);
ALTAIRCAM_API(HRESULT)  Altaircam_FfcImport(HAltaircam h, const wchar_t* filepath);
#else
ALTAIRCAM_API(HRESULT)  Altaircam_FfcExport(HAltaircam h, const char* filepath);
ALTAIRCAM_API(HRESULT)  Altaircam_FfcImport(HAltaircam h, const char* filepath);
#endif

/* Dark Field Correction */
ALTAIRCAM_API(HRESULT)  Altaircam_DfcOnce(HAltaircam h);

#ifdef _WIN32
ALTAIRCAM_API(HRESULT)  Altaircam_DfcExport(HAltaircam h, const wchar_t* filepath);
ALTAIRCAM_API(HRESULT)  Altaircam_DfcImport(HAltaircam h, const wchar_t* filepath);
#else
ALTAIRCAM_API(HRESULT)  Altaircam_DfcExport(HAltaircam h, const char* filepath);
ALTAIRCAM_API(HRESULT)  Altaircam_DfcImport(HAltaircam h, const char* filepath);
#endif

ALTAIRCAM_API(HRESULT)  Altaircam_put_Hue(HAltaircam h, int Hue);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Hue(HAltaircam h, int* Hue);
ALTAIRCAM_API(HRESULT)  Altaircam_put_Saturation(HAltaircam h, int Saturation);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Saturation(HAltaircam h, int* Saturation);
ALTAIRCAM_API(HRESULT)  Altaircam_put_Brightness(HAltaircam h, int Brightness);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Brightness(HAltaircam h, int* Brightness);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Contrast(HAltaircam h, int* Contrast);
ALTAIRCAM_API(HRESULT)  Altaircam_put_Contrast(HAltaircam h, int Contrast);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Gamma(HAltaircam h, int* Gamma); /* percent */
ALTAIRCAM_API(HRESULT)  Altaircam_put_Gamma(HAltaircam h, int Gamma);  /* percent */

ALTAIRCAM_API(HRESULT)  Altaircam_get_Chrome(HAltaircam h, int* bChrome);  /* monochromatic mode */
ALTAIRCAM_API(HRESULT)  Altaircam_put_Chrome(HAltaircam h, int bChrome);

ALTAIRCAM_API(HRESULT)  Altaircam_get_VFlip(HAltaircam h, int* bVFlip);  /* vertical flip */
ALTAIRCAM_API(HRESULT)  Altaircam_put_VFlip(HAltaircam h, int bVFlip);
ALTAIRCAM_API(HRESULT)  Altaircam_get_HFlip(HAltaircam h, int* bHFlip);
ALTAIRCAM_API(HRESULT)  Altaircam_put_HFlip(HAltaircam h, int bHFlip); /* horizontal flip */

ALTAIRCAM_API(HRESULT)  Altaircam_get_Negative(HAltaircam h, int* bNegative);  /* negative film */
ALTAIRCAM_API(HRESULT)  Altaircam_put_Negative(HAltaircam h, int bNegative);

ALTAIRCAM_API(HRESULT)  Altaircam_put_Speed(HAltaircam h, unsigned short nSpeed);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Speed(HAltaircam h, unsigned short* pSpeed);
ALTAIRCAM_API(HRESULT)  Altaircam_get_MaxSpeed(HAltaircam h); /* get the maximum speed, see "Frame Speed Level", the speed range = [0, max], closed interval */

ALTAIRCAM_API(HRESULT)  Altaircam_get_FanMaxSpeed(HAltaircam h); /* get the maximum fan speed, the fan speed range = [0, max], closed interval */

ALTAIRCAM_API(HRESULT)  Altaircam_get_MaxBitDepth(HAltaircam h); /* get the max bit depth of this camera, such as 8, 10, 12, 14, 16 */

/* power supply of lighting:
        0 -> 60HZ AC
        1 -> 50Hz AC
        2 -> DC
*/
ALTAIRCAM_API(HRESULT)  Altaircam_put_HZ(HAltaircam h, int nHZ);
ALTAIRCAM_API(HRESULT)  Altaircam_get_HZ(HAltaircam h, int* nHZ);

ALTAIRCAM_API(HRESULT)  Altaircam_put_Mode(HAltaircam h, int bSkip); /* skip or bin */
ALTAIRCAM_API(HRESULT)  Altaircam_get_Mode(HAltaircam h, int* bSkip); /* If the model don't support bin/skip mode, return E_NOTIMPL */

ALTAIRCAM_API(HRESULT)  Altaircam_put_AWBAuxRect(HAltaircam h, const RECT* pAuxRect); /* auto white balance ROI */
ALTAIRCAM_API(HRESULT)  Altaircam_get_AWBAuxRect(HAltaircam h, RECT* pAuxRect);
ALTAIRCAM_API(HRESULT)  Altaircam_put_AEAuxRect(HAltaircam h, const RECT* pAuxRect);  /* auto exposure ROI */
ALTAIRCAM_API(HRESULT)  Altaircam_get_AEAuxRect(HAltaircam h, RECT* pAuxRect);

ALTAIRCAM_API(HRESULT)  Altaircam_put_ABBAuxRect(HAltaircam h, const RECT* pAuxRect); /* auto black balance ROI */
ALTAIRCAM_API(HRESULT)  Altaircam_get_ABBAuxRect(HAltaircam h, RECT* pAuxRect);

/*
    S_FALSE:    color mode
    S_OK:       mono mode, such as EXCCD00300KMA and UHCCD01400KMA
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_MonoMode(HAltaircam h);

ALTAIRCAM_API(HRESULT)  Altaircam_get_StillResolutionNumber(HAltaircam h);
ALTAIRCAM_API(HRESULT)  Altaircam_get_StillResolution(HAltaircam h, unsigned nResolutionIndex, int* pWidth, int* pHeight);

/*  0: stop grab frame when frame buffer deque is full, until the frames in the queue are pulled away and the queue is not full
    1: realtime
          use minimum frame buffer. When new frame arrive, drop all the pending frame regardless of whether the frame buffer is full.
          If DDR present, also limit the DDR frame buffer to only one frame.
    2: soft realtime
          Drop the oldest frame when the queue is full and then enqueue the new frame
    default: 0
*/
ALTAIRCAM_API(HRESULT)  Altaircam_put_RealTime(HAltaircam h, int val);
ALTAIRCAM_API(HRESULT)  Altaircam_get_RealTime(HAltaircam h, int* val);

/* discard the current internal frame cache.
    If DDR present, also discard the frames in the DDR.
*/
ALTAIRCAM_API(HRESULT)  Altaircam_Flush(HAltaircam h);

/* get the temperature of the sensor, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius, -35 means -3.5 degree Celsius)
    return E_NOTIMPL if not supported
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_Temperature(HAltaircam h, short* pTemperature);

/* set the target temperature of the sensor or TEC, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius, -35 means -3.5 degree Celsius)
    return E_NOTIMPL if not supported
*/
ALTAIRCAM_API(HRESULT)  Altaircam_put_Temperature(HAltaircam h, short nTemperature);

/*
    get the revision
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_Revision(HAltaircam h, unsigned short* pRevision);

/*
    get the serial number which is always 32 chars which is zero-terminated such as "TP110826145730ABCD1234FEDC56787"
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_SerialNumber(HAltaircam h, char sn[32]);

/*
    get the camera firmware version, such as: 3.2.1.20140922
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_FwVersion(HAltaircam h, char fwver[16]);

/*
    get the camera hardware version, such as: 3.12
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_HwVersion(HAltaircam h, char hwver[16]);

/*
    get the production date, such as: 20150327, YYYYMMDD, (YYYY: year, MM: month, DD: day)
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_ProductionDate(HAltaircam h, char pdate[10]);

/*
    get the FPGA version, such as: 1.13
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_FpgaVersion(HAltaircam h, char fpgaver[16]);

/*
    get the sensor pixel size, such as: 2.4um
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_PixelSize(HAltaircam h, unsigned nResolutionIndex, float* x, float* y);

/* software level range */
ALTAIRCAM_API(HRESULT)  Altaircam_put_LevelRange(HAltaircam h, unsigned short aLow[4], unsigned short aHigh[4]);
ALTAIRCAM_API(HRESULT)  Altaircam_get_LevelRange(HAltaircam h, unsigned short aLow[4], unsigned short aHigh[4]);

/* hardware level range mode */
#define ALTAIRCAM_LEVELRANGE_MANUAL       0x0000  /* manual */
#define ALTAIRCAM_LEVELRANGE_ONCE         0x0001  /* once */
#define ALTAIRCAM_LEVELRANGE_CONTINUE     0x0002  /* continue */
#define ALTAIRCAM_LEVELRANGE_ROI          0xffff  /* update roi rect only */
ALTAIRCAM_API(HRESULT)  Altaircam_put_LevelRangeV2(HAltaircam h, unsigned short mode, const RECT* pRoiRect, unsigned short aLow[4], unsigned short aHigh[4]);
ALTAIRCAM_API(HRESULT)  Altaircam_get_LevelRangeV2(HAltaircam h, unsigned short* pMode, RECT* pRoiRect, unsigned short aLow[4], unsigned short aHigh[4]);

/*
    The following functions must be called AFTER Altaircam_StartPushMode or Altaircam_StartPullModeWithWndMsg or Altaircam_StartPullModeWithCallback
*/
ALTAIRCAM_API(HRESULT)  Altaircam_LevelRangeAuto(HAltaircam h);  /* software level range */
ALTAIRCAM_API(HRESULT)  Altaircam_GetHistogram(HAltaircam h, PIALTAIRCAM_HISTOGRAM_CALLBACK fnHistogramProc, void* pHistogramCtx);

/* led state:
    iLed: Led index, (0, 1, 2, ...)
    iState: 1 -> Ever bright; 2 -> Flashing; other -> Off
    iPeriod: Flashing Period (>= 500ms)
*/
ALTAIRCAM_API(HRESULT)  Altaircam_put_LEDState(HAltaircam h, unsigned short iLed, unsigned short iState, unsigned short iPeriod);

ALTAIRCAM_API(HRESULT)  Altaircam_write_EEPROM(HAltaircam h, unsigned addr, const unsigned char* pBuffer, unsigned nBufferLen);
ALTAIRCAM_API(HRESULT)  Altaircam_read_EEPROM(HAltaircam h, unsigned addr, unsigned char* pBuffer, unsigned nBufferLen);

ALTAIRCAM_API(HRESULT)  Altaircam_read_Pipe(HAltaircam h, unsigned pipeNum, void* pBuffer, unsigned nBufferLen);
ALTAIRCAM_API(HRESULT)  Altaircam_write_Pipe(HAltaircam h, unsigned pipeNum, const void* pBuffer, unsigned nBufferLen);
ALTAIRCAM_API(HRESULT)  Altaircam_feed_Pipe(HAltaircam h, unsigned pipeNum);

#define ALTAIRCAM_TEC_TARGET_MIN               (-300)     /* -30.0 degrees Celsius */
#define ALTAIRCAM_TEC_TARGET_DEF               0          /* 0.0 degrees Celsius */
#define ALTAIRCAM_TEC_TARGET_MAX               300        /* 30.0 degrees Celsius */
                                             
#define ALTAIRCAM_OPTION_NOFRAME_TIMEOUT       0x01       /* no frame timeout: 1 = enable; 0 = disable. default: disable */
#define ALTAIRCAM_OPTION_THREAD_PRIORITY       0x02       /* set the priority of the internal thread which grab data from the usb device. iValue: 0 = THREAD_PRIORITY_NORMAL; 1 = THREAD_PRIORITY_ABOVE_NORMAL; 2 = THREAD_PRIORITY_HIGHEST; default: 0; see: msdn SetThreadPriority */
#define ALTAIRCAM_OPTION_PROCESSMODE           0x03       /* 0 = better image quality, more cpu usage. this is the default value
                                                           1 = lower image quality, less cpu usage
                                                        */
#define ALTAIRCAM_OPTION_RAW                   0x04       /* raw data mode, read the sensor "raw" data. This can be set only BEFORE Altaircam_StartXXX(). 0 = rgb, 1 = raw, default value: 0 */
#define ALTAIRCAM_OPTION_HISTOGRAM             0x05       /* 0 = only one, 1 = continue mode */
#define ALTAIRCAM_OPTION_BITDEPTH              0x06       /* 0 = 8 bits mode, 1 = 16 bits mode, subset of ALTAIRCAM_OPTION_PIXEL_FORMAT */
#define ALTAIRCAM_OPTION_FAN                   0x07       /* 0 = turn off the cooling fan, [1, max] = fan speed */
#define ALTAIRCAM_OPTION_TEC                   0x08       /* 0 = turn off the thermoelectric cooler, 1 = turn on the thermoelectric cooler */
#define ALTAIRCAM_OPTION_LINEAR                0x09       /* 0 = turn off the builtin linear tone mapping, 1 = turn on the builtin linear tone mapping, default value: 1 */
#define ALTAIRCAM_OPTION_CURVE                 0x0a       /* 0 = turn off the builtin curve tone mapping, 1 = turn on the builtin polynomial curve tone mapping, 2 = logarithmic curve tone mapping, default value: 2 */
#define ALTAIRCAM_OPTION_TRIGGER               0x0b       /* 0 = video mode, 1 = software or simulated trigger mode, 2 = external trigger mode, 3 = external + software trigger, default value = 0 */
#define ALTAIRCAM_OPTION_RGB                   0x0c       /* 0 => RGB24; 1 => enable RGB48 format when bitdepth > 8; 2 => RGB32; 3 => 8 Bits Gray (only for mono camera); 4 => 16 Bits Gray (only for mono camera when bitdepth > 8) */
#define ALTAIRCAM_OPTION_COLORMATIX            0x0d       /* enable or disable the builtin color matrix, default value: 1 */
#define ALTAIRCAM_OPTION_WBGAIN                0x0e       /* enable or disable the builtin white balance gain, default value: 1 */
#define ALTAIRCAM_OPTION_TECTARGET             0x0f       /* get or set the target temperature of the thermoelectric cooler, in 0.1 degree Celsius. For example, 125 means 12.5 degree Celsius, -35 means -3.5 degree Celsius */
#define ALTAIRCAM_OPTION_AUTOEXP_POLICY        0x10       /* auto exposure policy:
                                                            0: Exposure Only
                                                            1: Exposure Preferred
                                                            2: Gain Only
                                                            3: Gain Preferred
                                                            default value: 1
                                                        */
#define ALTAIRCAM_OPTION_FRAMERATE             0x11       /* limit the frame rate, range=[0, 63], the default value 0 means no limit */
#define ALTAIRCAM_OPTION_DEMOSAIC              0x12       /* demosaic method for both video and still image: BILINEAR = 0, VNG(Variable Number of Gradients interpolation) = 1, PPG(Patterned Pixel Grouping interpolation) = 2, AHD(Adaptive Homogeneity-Directed interpolation) = 3, see https://en.wikipedia.org/wiki/Demosaicing, default value: 0 */
#define ALTAIRCAM_OPTION_DEMOSAIC_VIDEO        0x13       /* demosaic method for video */
#define ALTAIRCAM_OPTION_DEMOSAIC_STILL        0x14       /* demosaic method for still image */
#define ALTAIRCAM_OPTION_BLACKLEVEL            0x15       /* black level */
#define ALTAIRCAM_OPTION_MULTITHREAD           0x16       /* multithread image processing */
#define ALTAIRCAM_OPTION_BINNING               0x17       /* binning, 0x01 (no binning), 0x02 (add, 2*2), 0x03 (add, 3*3), 0x04 (add, 4*4), 0x05 (add, 5*5), 0x06 (add, 6*6), 0x07 (add, 7*7), 0x08 (add, 8*8), 0x82 (average, 2*2), 0x83 (average, 3*3), 0x84 (average, 4*4), 0x85 (average, 5*5), 0x86 (average, 6*6), 0x87 (average, 7*7), 0x88 (average, 8*8). The final image size is rounded down to an even number, such as 640/3 to get 212 */
#define ALTAIRCAM_OPTION_ROTATE                0x18       /* rotate clockwise: 0, 90, 180, 270 */
#define ALTAIRCAM_OPTION_CG                    0x19       /* Conversion Gain: 0 = LCG, 1 = HCG, 2 = HDR */
#define ALTAIRCAM_OPTION_PIXEL_FORMAT          0x1a       /* pixel format, ALTAIRCAM_PIXELFORMAT_xxxx */
#define ALTAIRCAM_OPTION_FFC                   0x1b       /* flat field correction
                                                            set:
                                                                 0: disable
                                                                 1: enable
                                                                -1: reset
                                                                (0xff000000 | n): set the average number to n, [1~255]
                                                            get:
                                                                 (val & 0xff): 0 -> disable, 1 -> enable, 2 -> inited
                                                                 ((val & 0xff00) >> 8): sequence
                                                                 ((val & 0xff0000) >> 8): average number
                                                        */
#define ALTAIRCAM_OPTION_DDR_DEPTH             0x1c       /* the number of the frames that DDR can cache
                                                                1: DDR cache only one frame
                                                                0: Auto:
                                                                        ->one for video mode when auto exposure is enabled
                                                                        ->full capacity for others
                                                               -1: DDR can cache frames to full capacity
                                                        */
#define ALTAIRCAM_OPTION_DFC                   0x1d       /* dark field correction
                                                            set:
                                                                0: disable
                                                                1: enable
                                                               -1: reset
                                                                (0xff000000 | n): set the average number to n, [1~255]
                                                            get:
                                                                (val & 0xff): 0 -> disable, 1 -> enable, 2 -> inited
                                                                ((val & 0xff00) >> 8): sequence
                                                                ((val & 0xff0000) >> 8): average number
                                                        */
#define ALTAIRCAM_OPTION_SHARPENING            0x1e       /* Sharpening: (threshold << 24) | (radius << 16) | strength)
                                                            strength: [0, 500], default: 0 (disable)
                                                            radius: [1, 10]
                                                            threshold: [0, 255]
                                                        */
#define ALTAIRCAM_OPTION_FACTORY               0x1f       /* restore the factory settings */
#define ALTAIRCAM_OPTION_TEC_VOLTAGE           0x20       /* get the current TEC voltage in 0.1V, 59 mean 5.9V; readonly */
#define ALTAIRCAM_OPTION_TEC_VOLTAGE_MAX       0x21       /* get the TEC maximum voltage in 0.1V; readonly */
#define ALTAIRCAM_OPTION_DEVICE_RESET          0x22       /* reset usb device, simulate a replug */
#define ALTAIRCAM_OPTION_UPSIDE_DOWN           0x23       /* upsize down:
                                                            1: yes
                                                            0: no
                                                            default: 1 (win), 0 (linux/macos)
                                                        */
#define ALTAIRCAM_OPTION_AFPOSITION            0x24       /* auto focus sensor board positon */
#define ALTAIRCAM_OPTION_AFMODE                0x25       /* auto focus mode (0:manul focus; 1:auto focus; 2:once focus; 3:conjugate calibration) */
#define ALTAIRCAM_OPTION_AFZONE                0x26       /* auto focus zone */
#define ALTAIRCAM_OPTION_AFFEEDBACK            0x27       /* auto focus information feedback; 0:unknown; 1:focused; 2:focusing; 3:defocus; 4:up; 5:down */
#define ALTAIRCAM_OPTION_TESTPATTERN           0x28       /* test pattern:
                                                           0: TestPattern Off
                                                           3: monochrome diagonal stripes
                                                           5: monochrome vertical stripes
                                                           7: monochrome horizontal stripes
                                                           9: chromatic diagonal stripes
                                                        */
#define ALTAIRCAM_OPTION_AUTOEXP_THRESHOLD     0x29       /* threshold of auto exposure, default value: 5, range = [2, 15] */
#define ALTAIRCAM_OPTION_BYTEORDER             0x2a       /* Byte order, BGR or RGB: 0->RGB, 1->BGR, default value: 1(Win), 0(macOS, Linux, Android) */
#define ALTAIRCAM_OPTION_NOPACKET_TIMEOUT      0x2b       /* no packet timeout: 0 = disable, positive value = timeout milliseconds. default: disable */
#define ALTAIRCAM_OPTION_MAX_PRECISE_FRAMERATE 0x2c       /* precise frame rate maximum value in 0.1 fps, such as 115 means 11.5 fps */
#define ALTAIRCAM_OPTION_PRECISE_FRAMERATE     0x2d       /* precise frame rate current value in 0.1 fps */
#define ALTAIRCAM_OPTION_BANDWIDTH             0x2e       /* bandwidth, [1-100]% */
#define ALTAIRCAM_OPTION_RELOAD                0x2f       /* reload the last frame in trigger mode */
#define ALTAIRCAM_OPTION_CALLBACK_THREAD       0x30       /* dedicated thread for callback */
#define ALTAIRCAM_OPTION_FRAME_DEQUE_LENGTH    0x31       /* frame buffer deque length, range: [2, 1024], default: 3 */
#define ALTAIRCAM_OPTION_MIN_PRECISE_FRAMERATE 0x32       /* precise frame rate minimum value in 0.1 fps, such as 15 means 1.5 fps */
#define ALTAIRCAM_OPTION_SEQUENCER_ONOFF       0x33       /* sequencer trigger: on/off */
#define ALTAIRCAM_OPTION_SEQUENCER_NUMBER      0x34       /* sequencer trigger: number, range = [1, 255] */
#define ALTAIRCAM_OPTION_SEQUENCER_EXPOTIME    0x01000000 /* sequencer trigger: exposure time, iOption = ALTAIRCAM_OPTION_SEQUENCER_EXPOTIME | index, iValue = exposure time
                                                             For example, to set the exposure time of the third group to 50ms, call:
                                                                Altaircam_put_Option(ALTAIRCAM_OPTION_SEQUENCER_EXPOTIME | 3, 50000)
                                                        */
#define ALTAIRCAM_OPTION_SEQUENCER_EXPOGAIN    0x02000000 /* sequencer trigger: exposure gain, iOption = ALTAIRCAM_OPTION_SEQUENCER_EXPOGAIN | index, iValue = gain */
#define ALTAIRCAM_OPTION_DENOISE               0x35       /* denoise, strength range: [0, 100], 0 means disable */
#define ALTAIRCAM_OPTION_HEAT_MAX              0x36       /* maximum level: heat to prevent fogging up */
#define ALTAIRCAM_OPTION_HEAT                  0x37       /* heat to prevent fogging up */
#define ALTAIRCAM_OPTION_LOW_NOISE             0x38       /* low noise mode: 1 => enable */
#define ALTAIRCAM_OPTION_POWER                 0x39       /* get power consumption, unit: milliwatt */
#define ALTAIRCAM_OPTION_GLOBAL_RESET_MODE     0x3a       /* global reset mode */
#define ALTAIRCAM_OPTION_OPEN_USB_ERRORCODE    0x3b       /* open usb error code */

/* pixel format */
#define ALTAIRCAM_PIXELFORMAT_RAW8             0x00
#define ALTAIRCAM_PIXELFORMAT_RAW10            0x01
#define ALTAIRCAM_PIXELFORMAT_RAW12            0x02
#define ALTAIRCAM_PIXELFORMAT_RAW14            0x03
#define ALTAIRCAM_PIXELFORMAT_RAW16            0x04
#define ALTAIRCAM_PIXELFORMAT_YUV411           0x05
#define ALTAIRCAM_PIXELFORMAT_VUYY             0x06
#define ALTAIRCAM_PIXELFORMAT_YUV444           0x07
#define ALTAIRCAM_PIXELFORMAT_RGB888           0x08
#define ALTAIRCAM_PIXELFORMAT_GMCY8            0x09   /* map to RGGB 8 bits */
#define ALTAIRCAM_PIXELFORMAT_GMCY12           0x0a   /* map to RGGB 12 bits */
#define ALTAIRCAM_PIXELFORMAT_UYVY             0x0b

ALTAIRCAM_API(HRESULT)  Altaircam_put_Option(HAltaircam h, unsigned iOption, int iValue);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Option(HAltaircam h, unsigned iOption, int* piValue);

/*
    xOffset, yOffset, xWidth, yHeight: must be even numbers
*/
ALTAIRCAM_API(HRESULT)  Altaircam_put_Roi(HAltaircam h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Roi(HAltaircam h, unsigned* pxOffset, unsigned* pyOffset, unsigned* pxWidth, unsigned* pyHeight);

/*  simulate replug:
    return > 0, the number of device has been replug
    return = 0, no device found
    return E_ACCESSDENIED if without UAC Administrator privileges
    for each device found, it will take about 3 seconds
*/
#ifdef _WIN32
ALTAIRCAM_API(HRESULT) Altaircam_Replug(const wchar_t* id);
#else
ALTAIRCAM_API(HRESULT) Altaircam_Replug(const char* id);
#endif

#ifndef __ALTAIRCAMAFPARAM_DEFINED__
#define __ALTAIRCAMAFPARAM_DEFINED__
typedef struct {
    int imax;    /* maximum auto focus sensor board positon */
    int imin;    /* minimum auto focus sensor board positon */
    int idef;    /* conjugate calibration positon */
    int imaxabs; /* maximum absolute auto focus sensor board positon, micrometer */
    int iminabs; /* maximum absolute auto focus sensor board positon, micrometer */
    int zoneh;   /* zone horizontal */
    int zonev;   /* zone vertical */
}AltaircamAfParam;
#endif

ALTAIRCAM_API(HRESULT)  Altaircam_get_AfParam(HAltaircam h, AltaircamAfParam* pAfParam);

#define ALTAIRCAM_IOCONTROLTYPE_GET_SUPPORTEDMODE           0x01 /* 0x01->Input, 0x02->Output, (0x01 | 0x02)->support both Input and Output */
#define ALTAIRCAM_IOCONTROLTYPE_GET_GPIODIR                 0x03 /* 0x00->Input, 0x01->Output */
#define ALTAIRCAM_IOCONTROLTYPE_SET_GPIODIR                 0x04
#define ALTAIRCAM_IOCONTROLTYPE_GET_FORMAT                  0x05 /*
                                                                   0x00-> not connected
                                                                   0x01-> Tri-state: Tri-state mode (Not driven)
                                                                   0x02-> TTL: TTL level signals
                                                                   0x03-> LVDS: LVDS level signals
                                                                   0x04-> RS422: RS422 level signals
                                                                   0x05-> Opto-coupled
                                                               */
#define ALTAIRCAM_IOCONTROLTYPE_SET_FORMAT                  0x06
#define ALTAIRCAM_IOCONTROLTYPE_GET_OUTPUTINVERTER          0x07 /* boolean, only support output signal */
#define ALTAIRCAM_IOCONTROLTYPE_SET_OUTPUTINVERTER          0x08
#define ALTAIRCAM_IOCONTROLTYPE_GET_INPUTACTIVATION         0x09 /* 0x00->Positive, 0x01->Negative */
#define ALTAIRCAM_IOCONTROLTYPE_SET_INPUTACTIVATION         0x0a
#define ALTAIRCAM_IOCONTROLTYPE_GET_DEBOUNCERTIME           0x0b /* debouncer time in microseconds, [0, 20000] */
#define ALTAIRCAM_IOCONTROLTYPE_SET_DEBOUNCERTIME           0x0c
#define ALTAIRCAM_IOCONTROLTYPE_GET_TRIGGERSOURCE           0x0d /*
                                                                  0x00-> Opto-isolated input
                                                                  0x01-> GPIO0
                                                                  0x02-> GPIO1
                                                                  0x03-> Counter
                                                                  0x04-> PWM
                                                                  0x05-> Software
                                                               */
#define ALTAIRCAM_IOCONTROLTYPE_SET_TRIGGERSOURCE           0x0e
#define ALTAIRCAM_IOCONTROLTYPE_GET_TRIGGERDELAY            0x0f /* Trigger delay time in microseconds, [0, 5000000] */
#define ALTAIRCAM_IOCONTROLTYPE_SET_TRIGGERDELAY            0x10
#define ALTAIRCAM_IOCONTROLTYPE_GET_BURSTCOUNTER            0x11 /* Burst Counter: 1, 2, 3 ... 1023 */
#define ALTAIRCAM_IOCONTROLTYPE_SET_BURSTCOUNTER            0x12
#define ALTAIRCAM_IOCONTROLTYPE_GET_COUNTERSOURCE           0x13 /* 0x00-> Opto-isolated input, 0x01-> GPIO0, 0x02-> GPIO1 */
#define ALTAIRCAM_IOCONTROLTYPE_SET_COUNTERSOURCE           0x14
#define ALTAIRCAM_IOCONTROLTYPE_GET_COUNTERVALUE            0x15 /* Counter Value: 1, 2, 3 ... 1023 */
#define ALTAIRCAM_IOCONTROLTYPE_SET_COUNTERVALUE            0x16
#define ALTAIRCAM_IOCONTROLTYPE_SET_RESETCOUNTER            0x18
#define ALTAIRCAM_IOCONTROLTYPE_GET_PWM_FREQ                0x19
#define ALTAIRCAM_IOCONTROLTYPE_SET_PWM_FREQ                0x1a
#define ALTAIRCAM_IOCONTROLTYPE_GET_PWM_DUTYRATIO           0x1b
#define ALTAIRCAM_IOCONTROLTYPE_SET_PWM_DUTYRATIO           0x1c
#define ALTAIRCAM_IOCONTROLTYPE_GET_PWMSOURCE               0x1d /* 0x00-> Opto-isolated input, 0x01-> GPIO0, 0x02-> GPIO1 */
#define ALTAIRCAM_IOCONTROLTYPE_SET_PWMSOURCE               0x1e
#define ALTAIRCAM_IOCONTROLTYPE_GET_OUTPUTMODE              0x1f /*
                                                                  0x00-> Frame Trigger Wait
                                                                  0x01-> Exposure Active
                                                                  0x02-> Strobe
                                                                  0x03-> User output
                                                               */
#define ALTAIRCAM_IOCONTROLTYPE_SET_OUTPUTMODE              0x20
#define ALTAIRCAM_IOCONTROLTYPE_GET_STROBEDELAYMODE         0x21 /* boolean, 0-> pre-delay, 1-> delay; compared to exposure active signal */
#define ALTAIRCAM_IOCONTROLTYPE_SET_STROBEDELAYMODE         0x22
#define ALTAIRCAM_IOCONTROLTYPE_GET_STROBEDELAYTIME         0x23 /* Strobe delay or pre-delay time in microseconds, [0, 5000000] */
#define ALTAIRCAM_IOCONTROLTYPE_SET_STROBEDELAYTIME         0x24
#define ALTAIRCAM_IOCONTROLTYPE_GET_STROBEDURATION          0x25 /* Strobe duration time in microseconds, [0, 5000000] */
#define ALTAIRCAM_IOCONTROLTYPE_SET_STROBEDURATION          0x26
#define ALTAIRCAM_IOCONTROLTYPE_GET_USERVALUE               0x27 /* 
                                                                  bit0-> Opto-isolated output
                                                                  bit1-> GPIO0 output
                                                                  bit2-> GPIO1 output
                                                               */
#define ALTAIRCAM_IOCONTROLTYPE_SET_USERVALUE               0x28
#define ALTAIRCAM_IOCONTROLTYPE_GET_UART_ENABLE             0x29 /* enable: 1-> on; 0-> off */
#define ALTAIRCAM_IOCONTROLTYPE_SET_UART_ENABLE             0x2a
#define ALTAIRCAM_IOCONTROLTYPE_GET_UART_BAUDRATE           0x2b /* baud rate: 0-> 9600; 1-> 19200; 2-> 38400; 3-> 57600; 4-> 115200 */
#define ALTAIRCAM_IOCONTROLTYPE_SET_UART_BAUDRATE           0x2c
#define ALTAIRCAM_IOCONTROLTYPE_GET_UART_LINEMODE           0x2d /* line mode: 0-> TX(GPIO_0)/RX(GPIO_1); 1-> TX(GPIO_1)/RX(GPIO_0) */
#define ALTAIRCAM_IOCONTROLTYPE_SET_UART_LINEMODE           0x2e

ALTAIRCAM_API(HRESULT)  Altaircam_IoControl(HAltaircam h, unsigned index, unsigned nType, int outVal, int* inVal);

ALTAIRCAM_API(HRESULT)  Altaircam_write_UART(HAltaircam h, const unsigned char* pData, unsigned nDataLen);
ALTAIRCAM_API(HRESULT)  Altaircam_read_UART(HAltaircam h, unsigned char* pBuffer, unsigned nBufferLen);

ALTAIRCAM_API(HRESULT)  Altaircam_put_Linear(HAltaircam h, const unsigned char* v8, const unsigned short* v16);
ALTAIRCAM_API(HRESULT)  Altaircam_put_Curve(HAltaircam h, const unsigned char* v8, const unsigned short* v16);
ALTAIRCAM_API(HRESULT)  Altaircam_put_ColorMatrix(HAltaircam h, const double v[9]);
ALTAIRCAM_API(HRESULT)  Altaircam_put_InitWBGain(HAltaircam h, const unsigned short v[3]);

/*
    get the frame rate: framerate (fps) = Frame * 1000.0 / nTime
*/
ALTAIRCAM_API(HRESULT)  Altaircam_get_FrameRate(HAltaircam h, unsigned* nFrame, unsigned* nTime, unsigned* nTotalFrame);

/* astronomy: for ST4 guide, please see: ASCOM Platform Help ICameraV2.
    nDirect: 0 = North, 1 = South, 2 = East, 3 = West, 4 = Stop
    nDuration: in milliseconds
*/
ALTAIRCAM_API(HRESULT)  Altaircam_ST4PlusGuide(HAltaircam h, unsigned nDirect, unsigned nDuration);

/* S_OK: ST4 pulse guiding
   S_FALSE: ST4 not pulse guiding
*/
ALTAIRCAM_API(HRESULT)  Altaircam_ST4PlusGuideState(HAltaircam h);

/*
    calculate the clarity factor:
    pImageData: pointer to the image data
    bits: 8(Grey), 24 (RGB24), 32(RGB32)
    nImgWidth, nImgHeight: the image width and height
*/
ALTAIRCAM_API(double)   Altaircam_calc_ClarityFactor(const void* pImageData, int bits, unsigned nImgWidth, unsigned nImgHeight);

/*
    nBitCount: output bitmap bit count
    when nBitDepth == 8:
        nBitCount must be 24 or 32
    when nBitDepth > 8
        nBitCount:  24 -> RGB24
                    32 -> RGB32
                    48 -> RGB48
                    64 -> RGB64
*/
ALTAIRCAM_API(void)     Altaircam_deBayerV2(unsigned nBayer, int nW, int nH, const void* input, void* output, unsigned char nBitDepth, unsigned char nBitCount);

/*
    obsolete, please use Altaircam_deBayerV2
*/
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(void)     Altaircam_deBayer(unsigned nBayer, int nW, int nH, const void* input, void* output, unsigned char nBitDepth);

typedef void (__stdcall* PALTAIRCAM_DEMOSAIC_CALLBACK)(unsigned nBayer, int nW, int nH, const void* input, void* output, unsigned char nBitDepth, void* pCallbackCtx);
ALTAIRCAM_API(HRESULT)  Altaircam_put_Demosaic(HAltaircam h, PALTAIRCAM_DEMOSAIC_CALLBACK pCallback, void* pCallbackCtx);

/*
    obsolete, please use AltaircamModelV2
*/
typedef struct {
#ifdef _WIN32
    const wchar_t*      name;       /* model name, in Windows, we use unicode */
#else
    const char*         name;       /* model name */
#endif
    unsigned            flag;       /* ALTAIRCAM_FLAG_xxx */
    unsigned            maxspeed;   /* number of speed level, same as Altaircam_get_MaxSpeed(), the speed range = [0, maxspeed], closed interval */
    unsigned            preview;    /* number of preview resolution, same as Altaircam_get_ResolutionNumber() */
    unsigned            still;      /* number of still resolution, same as Altaircam_get_StillResolutionNumber() */
    AltaircamResolution   res[ALTAIRCAM_MAX];
}AltaircamModel; /* camera model */

/*
    obsolete, please use AltaircamDeviceV2
*/
typedef struct {
#ifdef _WIN32
    wchar_t             displayname[64];    /* display name */
    wchar_t             id[64];             /* unique and opaque id of a connected camera, for Altaircam_Open */
#else
    char                displayname[64];    /* display name */
    char                id[64];             /* unique and opaque id of a connected camera, for Altaircam_Open */
#endif
    const AltaircamModel* model;
}AltaircamDevice; /* camera instance for enumerating */

/*
    obsolete, please use Altaircam_EnumV2
*/
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(unsigned) Altaircam_Enum(AltaircamDevice pti[ALTAIRCAM_MAX]);

typedef PALTAIRCAM_DATA_CALLBACK_V3 PALTAIRCAM_DATA_CALLBACK_V2;
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_StartPushModeV2(HAltaircam h, PALTAIRCAM_DATA_CALLBACK_V2 pDataCallback, void* pCallbackCtx);

typedef void (__stdcall* PALTAIRCAM_DATA_CALLBACK)(const void* pData, const BITMAPINFOHEADER* pHeader, int bSnap, void* pCallbackCtx);
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_StartPushMode(HAltaircam h, PALTAIRCAM_DATA_CALLBACK pDataCallback, void* pCallbackCtx);

ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_put_ExpoCallback(HAltaircam h, PIALTAIRCAM_EXPOSURE_CALLBACK fnExpoProc, void* pExpoCtx);
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_put_ChromeCallback(HAltaircam h, PIALTAIRCAM_CHROME_CALLBACK fnChromeProc, void* pChromeCtx);

/* Altaircam_FfcOnePush is obsolete, it's a synonyms for Altaircam_FfcOnce. */
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_FfcOnePush(HAltaircam h);

/* Altaircam_DfcOnePush is obsolete, it's a synonyms for Altaircam_DfcOnce. */
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_DfcOnePush(HAltaircam h);

/* Altaircam_AwbOnePush is obsolete, it's a synonyms for Altaircam_AwbOnce. */
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_AwbOnePush(HAltaircam h, PIALTAIRCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx);

/* Altaircam_AbbOnePush is obsolete, it's a synonyms for Altaircam_AbbOnce. */
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_AbbOnePush(HAltaircam h, PIALTAIRCAM_BLACKBALANCE_CALLBACK fnBBProc, void* pBBCtx);

#ifndef _WIN32

/*
This function is only available on macOS and Linux, it's unnecessary on Windows.
  (1) To process the device plug in / pull out in Windows, please refer to the MSDN
       (a) Device Management, http://msdn.microsoft.com/en-us/library/windows/desktop/aa363224(v=vs.85).aspx
       (b) Detecting Media Insertion or Removal, http://msdn.microsoft.com/en-us/library/windows/desktop/aa363215(v=vs.85).aspx
  (2) To process the device plug in / pull out in Linux / macOS, please call this function to register the callback function.
      When the device is inserted or pulled out, you will be notified by the callback funcion, and then call Altaircam_EnumV2(...) again to enum the cameras.
Recommendation: for better rubustness, when notify of device insertion arrives, don't open handle of this device immediately, but open it after delaying a short time (e.g., 200 milliseconds).
*/
typedef void (*PALTAIRCAM_HOTPLUG)(void* pCallbackCtx);
ALTAIRCAM_API(void)   Altaircam_HotPlug(PALTAIRCAM_HOTPLUG pHotPlugCallback, void* pCallbackCtx);

#else

/* Altaircam_Start is obsolete, it's a synonyms for Altaircam_StartPushMode. */
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_Start(HAltaircam h, PALTAIRCAM_DATA_CALLBACK pDataCallback, void* pCallbackCtx);

/* Altaircam_put_TempTintInit is obsolete, it's a synonyms for Altaircam_AwbOnce. */
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_put_TempTintInit(HAltaircam h, PIALTAIRCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx);

/*
    obsolete, please use Altaircam_put_Option or Altaircam_get_Option to set or get the process mode: ALTAIRCAM_PROCESSMODE_FULL or ALTAIRCAM_PROCESSMODE_FAST.
    default is ALTAIRCAM_PROCESSMODE_FULL.
*/
#ifndef __ALTAIRCAM_PROCESSMODE_DEFINED__
#define __ALTAIRCAM_PROCESSMODE_DEFINED__
#define ALTAIRCAM_PROCESSMODE_FULL        0x00    /* better image quality, more cpu usage. this is the default value */
#define ALTAIRCAM_PROCESSMODE_FAST        0x01    /* lower image quality, less cpu usage */
#endif

ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_put_ProcessMode(HAltaircam h, unsigned nProcessMode);
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_get_ProcessMode(HAltaircam h, unsigned* pnProcessMode);

#endif

/* obsolete, please use Altaircam_put_Roi and Altaircam_get_Roi */
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_put_RoiMode(HAltaircam h, int bRoiMode, int xOffset, int yOffset);
ALTAIRCAM_DEPRECATED
ALTAIRCAM_API(HRESULT)  Altaircam_get_RoiMode(HAltaircam h, int* pbRoiMode, int* pxOffset, int* pyOffset);

/* obsolete:
     ------------------------------------------------------------|
     | Parameter         |   Range       |   Default             |
     |-----------------------------------------------------------|
     | VidgetAmount      |   -100~100    |   0                   |
     | VignetMidPoint    |   0~100       |   50                  |
     -------------------------------------------------------------
*/
ALTAIRCAM_API(HRESULT)  Altaircam_put_VignetEnable(HAltaircam h, int bEnable);
ALTAIRCAM_API(HRESULT)  Altaircam_get_VignetEnable(HAltaircam h, int* bEnable);
ALTAIRCAM_API(HRESULT)  Altaircam_put_VignetAmountInt(HAltaircam h, int nAmount);
ALTAIRCAM_API(HRESULT)  Altaircam_get_VignetAmountInt(HAltaircam h, int* nAmount);
ALTAIRCAM_API(HRESULT)  Altaircam_put_VignetMidPointInt(HAltaircam h, int nMidPoint);
ALTAIRCAM_API(HRESULT)  Altaircam_get_VignetMidPointInt(HAltaircam h, int* nMidPoint);

/* obsolete flags */
#define ALTAIRCAM_FLAG_BITDEPTH10    ALTAIRCAM_FLAG_RAW10  /* pixel format, RAW 10bits */
#define ALTAIRCAM_FLAG_BITDEPTH12    ALTAIRCAM_FLAG_RAW12  /* pixel format, RAW 12bits */
#define ALTAIRCAM_FLAG_BITDEPTH14    ALTAIRCAM_FLAG_RAW14  /* pixel format, RAW 14bits */
#define ALTAIRCAM_FLAG_BITDEPTH16    ALTAIRCAM_FLAG_RAW16  /* pixel format, RAW 16bits */

#ifdef _WIN32
ALTAIRCAM_API(HRESULT)  Altaircam_put_Name(const wchar_t* id, const char* name);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Name(const wchar_t* id, char name[64]);
#else
ALTAIRCAM_API(HRESULT)  Altaircam_put_Name(const char* id, const char* name);
ALTAIRCAM_API(HRESULT)  Altaircam_get_Name(const char* id, char name[64]);
#endif
ALTAIRCAM_API(unsigned) Altaircam_EnumWithName(AltaircamDeviceV2 pti[ALTAIRCAM_MAX]);

#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif
