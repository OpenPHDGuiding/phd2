#ifndef __altair_h__
#define __altair_h__

/* Version: 1.6.5744.20150602 */

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
#ifdef ALTAIR_EXPORTS
#define altair_ports(x)    __declspec(dllexport)   x   __stdcall
#elif !defined(ALTAIR_NOIMPORTS)
#define altair_ports(x)    __declspec(dllimport)   x   __stdcall
#else
#define altair_ports(x)    x   __stdcall
#endif

#else

#define altair_ports(x)    x

#ifndef HRESULT
#define HRESULT int
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
	typedef struct AltairT { int unused; } *HAltair;

#define ALTAIR_MAX                     16

#define ALTAIR_FLAG_CMOS               0x00000001  /* cmos sensor */
#define ALTAIR_FLAG_CCD_PROGRESSIVE    0x00000002  /* progressive ccd sensor */
#define ALTAIR_FLAG_CCD_INTERLACED     0x00000004  /* interlaced ccd sensor */
#define ALTAIR_FLAG_ROI_HARDWARE       0x00000008  /* support hardware ROI */
#define ALTAIR_FLAG_MONO               0x00000010  /* monochromatic */
#define ALTAIR_FLAG_BINSKIP_SUPPORTED  0x00000020  /* support bin/skip mode, see Altair_put_Mode and Altair_get_Mode */
#define ALTAIR_FLAG_USB30              0x00000040  /* USB 3.0 */
#define ALTAIR_FLAG_TEC             0x00000080  /* Cooled */
#define ALTAIR_FLAG_USB30_OVER_USB20   0x00000100  /* usb3.0 camera connected to usb2.0 port */
#define ALTAIR_FLAG_ST4                0x00000200  /* ST4 */
#define ALTAIR_FLAG_GETTEMPERATURE     0x00000400  /* support to get the temperature of sensor */
#define ALTAIR_FLAG_PUTTEMPERATURE     0x00000800  /* support to put the temperature of sensor */
#define ALTAIR_FLAG_BITDEPTH10         0x00001000  /* Maximum Bit Depth = 10 */
#define ALTAIR_FLAG_BITDEPTH12         0x00002000  /* Maximum Bit Depth = 12 */
#define ALTAIR_FLAG_BITDEPTH14         0x00004000  /* Maximum Bit Depth = 14 */
#define ALTAIR_FLAG_BITDEPTH16         0x00008000  /* Maximum Bit Depth = 16 */
#define ALTAIR_FLAG_FAN                0x00010000  /* cooling fan */
#define ALTAIR_FLAG_TECONOFF        0x00020000  /* cooler can be turn on or off */
#define ALTAIR_FLAG_ISP                0x00040000  /* image signal processing supported */
#define ALTAIR_FLAG_TRIGGER_SOFTWARE   0x00080000  /* support software trigger */
#define ALTAIR_FLAG_TRIGGER_EXTERNAL   0x00100000  /* support external trigger */
#define ALTAIR_FLAG_TRIGGER_SINGLE     0x00200000  /* only support trigger single: one trigger, one image */

#define ALTAIR_TEMP_DEF                6503
#define ALTAIR_TEMP_MIN                2000
#define ALTAIR_TEMP_MAX                15000
#define ALTAIR_TINT_DEF                1000
#define ALTAIR_TINT_MIN                200
#define ALTAIR_TINT_MAX                2500
#define ALTAIR_HUE_DEF                 0
#define ALTAIR_HUE_MIN                 (-180)
#define ALTAIR_HUE_MAX                 180
#define ALTAIR_SATURATION_DEF          128
#define ALTAIR_SATURATION_MIN          0
#define ALTAIR_SATURATION_MAX          255
#define ALTAIR_BRIGHTNESS_DEF          0
#define ALTAIR_BRIGHTNESS_MIN          (-64)
#define ALTAIR_BRIGHTNESS_MAX          64
#define ALTAIR_CONTRAST_DEF            0
#define ALTAIR_CONTRAST_MIN            (-100)
#define ALTAIR_CONTRAST_MAX            100
#define ALTAIR_GAMMA_DEF               100
#define ALTAIR_GAMMA_MIN               20
#define ALTAIR_GAMMA_MAX               180
#define ALTAIR_AETARGET_DEF            120
#define ALTAIR_AETARGET_MIN            16
#define ALTAIR_AETARGET_MAX            235
#define ALTAIR_WBGAIN_DEF              0
#define ALTAIR_WBGAIN_MIN              (-128)
#define ALTAIR_WBGAIN_MAX              128

	typedef struct{
		unsigned    width;
		unsigned    height;
	}AltairResolution;

	/* In Windows platform, we always use UNICODE wchar_t */
	/* In Linux or OSX, we use char */
	typedef struct{
#ifdef _WIN32
		const wchar_t*      name;       /* model name */
#else
		const char*         name;
#endif
		unsigned            flag;       /* ALTAIR_FLAG_xxx */
		unsigned            maxspeed;   /* number of speed level, same as Altair_get_MaxSpeed(), the speed range = [0, maxspeed], closed interval */
		unsigned            preview;    /* number of preview resolution, same as Altair_get_ResolutionNumber() */
		unsigned            still;      /* number of still resolution, same as Altair_get_StillResolutionNumber() */
		AltairResolution   res[ALTAIR_MAX];
	}AltairModel;

	typedef struct{
#ifdef _WIN32
		wchar_t             displayname[64];    /* display name */
		wchar_t             id[64];     /* unique and opaque id of a connected camera, for Altair_Open */
#else
		char                displayname[64];    /* display name */
		char                id[64];     /* unique and opaque id of a connected camera, for Altair_Open */
#endif
		const AltairModel* model;
	}AltairInst;

	/*
	get the version of this dll, which is: 1.6.5744.20150602
	*/
#ifdef _WIN32
	altair_ports(const wchar_t*)   Altair_Version();
#else
	altair_ports(const char*)      Altair_Version();
#endif
	/*
	enumerate the cameras connected to the computer, return the number of enumerated.

	AltairInst arr[ALTAIR_MAX];
	unsigned cnt = Altair_Enum(arr);
	for (unsigned i = 0; i < cnt; ++i)
	...

	if pti == NULL, then, only the number is returned.
	*/
	altair_ports(unsigned) Altair_Enum(AltairInst pti[ALTAIR_MAX]);

	/* use the id of AltairInst, which is enumerated by Altair_Enum.
	if id is NULL, Altair_Open will open the first camera.
	*/
#ifdef _WIN32
	altair_ports(HAltair) Altair_Open(const wchar_t* id);
#else
	altair_ports(HAltair) Altair_Open(const char* id);
#endif

	/*
	the same with Altair_Open, but use the index as the parameter. such as:
	index == 0, open the first camera,
	index == 1, open the second camera,
	etc
	*/
	altair_ports(HAltair) Altair_OpenByIndex(unsigned index);

	altair_ports(void)     Altair_Close(HAltair h); /* close the handle */

#define ALTAIR_EVENT_EXPOSURE      0x0001    /* exposure time changed */
#define ALTAIR_EVENT_TEMPTINT      0x0002    /* white balance changed, Temp/Tint mode */
#define ALTAIR_EVENT_CHROME        0x0003    /* reversed, do not use it */
#define ALTAIR_EVENT_IMAGE         0x0004    /* live image arrived, use Altair_PullImage to get this image */
#define ALTAIR_EVENT_STILLIMAGE    0x0005    /* snap (still) frame arrived, use Altair_PullStillImage to get this frame */
#define ALTAIR_EVENT_WBGAIN        0x0006    /* white balance changed, RGB Gain mode */
#define ALTAIR_EVENT_ERROR         0x0080    /* something error happens */
#define ALTAIR_EVENT_DISCONNECTED  0x0081    /* camera disconnected */

#ifdef _WIN32
	altair_ports(HRESULT)      Altair_StartPullModeWithWndMsg(HAltair h, HWND hWnd, UINT nMsg);
#endif

	typedef void(__stdcall*    PALTAIR_EVENT_CALLBACK)(unsigned nEvent, void* pCallbackCtx);
	altair_ports(HRESULT)      Altair_StartPullModeWithCallback(HAltair h, PALTAIR_EVENT_CALLBACK pEventCallback, void* pCallbackContext);

	/*
	bits: 24 (RGB24), 32 (RGB32), or 8 (Grey). Int RAW mode, this parameter is ignored.
	pnWidth, pnHeight: OUT parameter
	*/
	altair_ports(HRESULT)      Altair_PullImage(HAltair h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);
	altair_ports(HRESULT)      Altair_PullStillImage(HAltair h, void* pImageData, int bits, unsigned* pnWidth, unsigned* pnHeight);

	/*
	(NULL == pData) means that something is error
	pCallbackCtx is the callback context which is passed by Altair_Start
	bSnap: TRUE if Altair_Snap

	pDataCallback is callbacked by an internal thread of altair.dll, so please pay attention to multithread problem
	*/
	typedef void(__stdcall*    PALTAIR_DATA_CALLBACK)(const void* pData, const BITMAPINFOHEADER* pHeader, BOOL bSnap, void* pCallbackCtx);
	altair_ports(HRESULT)  Altair_StartPushMode(HAltair h, PALTAIR_DATA_CALLBACK pDataCallback, void* pCallbackCtx);

	altair_ports(HRESULT)  Altair_Stop(HAltair h);
	altair_ports(HRESULT)  Altair_Pause(HAltair h, BOOL bPause);

	/*  for pull mode: ALTAIR_EVENT_STILLIMAGE, and then Altair_PullStillImage
	for push mode: the snapped image will be return by PALTAIR_DATA_CALLBACK, with the parameter 'bSnap' set to 'TRUE' */
	altair_ports(HRESULT)  Altair_Snap(HAltair h, unsigned nResolutionIndex);  /* still image snap */

	/*
	soft trigger:
	nNumber:    0xffff:     trigger continuously
	0:          cancel trigger
	others:     number of images to be triggered
	*/
	altair_ports(HRESULT)  Altair_Trigger(HAltair h, unsigned nNumber);	
	
	/* put_Size, put_eSize, can be used to set the video output resolution BEFORE Altair_Start.
	put_Size use width and height parameters, put_eSize use the index parameter.
	for example, UCMOS03100KPA support the following resolutions:
	index 0:    2048,   1536
	index 1:    1024,   768
	index 2:    680,    510
	so, we can use put_Size(h, 1024, 768) or put_eSize(h, 1). Both have the same effect.
	*/
	altair_ports(HRESULT)  Altair_put_Size(HAltair h, int nWidth, int nHeight);
	altair_ports(HRESULT)  Altair_get_Size(HAltair h, int* pWidth, int* pHeight);
	altair_ports(HRESULT)  Altair_put_eSize(HAltair h, unsigned nResolutionIndex);
	altair_ports(HRESULT)  Altair_get_eSize(HAltair h, unsigned* pnResolutionIndex);

	altair_ports(HRESULT)  Altair_get_ResolutionNumber(HAltair h);
	altair_ports(HRESULT)  Altair_get_Resolution(HAltair h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
	altair_ports(HRESULT)  Altair_get_ResolutionRatio(HAltair h, unsigned nResolutionIndex, int* pNumerator, int* pDenominator);
	altair_ports(HRESULT)  Altair_get_Field(HAltair h);

	/*
	FourCC:
	MAKEFOURCC('G', 'B', 'R', 'G')
	MAKEFOURCC('R', 'G', 'G', 'B')
	MAKEFOURCC('B', 'G', 'G', 'R')
	MAKEFOURCC('G', 'R', 'B', 'G')
	MAKEFOURCC('Y', 'U', 'Y', 'V')
	MAKEFOURCC('Y', 'Y', 'Y', 'Y')
	*/
	altair_ports(HRESULT)  Altair_get_RawFormat(HAltair h, unsigned* nFourCC, unsigned* bitdepth);

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

#ifndef __ALTAIR_CALLBACK_DEFINED__
#define __ALTAIR_CALLBACK_DEFINED__
	typedef void(__stdcall*     PIALTAIR_EXPOSURE_CALLBACK)(void* pCtx);
	typedef void(__stdcall*     PIALTAIR_WHITEBALANCE_CALLBACK)(const int aGain[3], void* pCtx);
	typedef void(__stdcall*     PIALTAIR_TEMPTINT_CALLBACK)(const int nTemp, const int nTint, void* pCtx);
	typedef void(__stdcall*     PIALTAIR_HISTOGRAM_CALLBACK)(const float aHistY[256], const float aHistR[256], const float aHistG[256], const float aHistB[256], void* pCtx);
	typedef void(__stdcall*     PIALTAIR_CHROME_CALLBACK)(void* pCtx);
#endif

	altair_ports(HRESULT)  Altair_get_AutoExpoEnable(HAltair h, BOOL* bAutoExposure);
	altair_ports(HRESULT)  Altair_put_AutoExpoEnable(HAltair h, BOOL bAutoExposure);
	altair_ports(HRESULT)  Altair_get_AutoExpoTarget(HAltair h, unsigned short* Target);
	altair_ports(HRESULT)  Altair_put_AutoExpoTarget(HAltair h, unsigned short Target);

	/*set the maximum auto exposure time and analog agin. The default maximum auto exposure time is 350ms */
	altair_ports(HRESULT)  Altair_put_MaxAutoExpoTimeAGain(HAltair h, unsigned maxTime, unsigned short maxAGain);

	altair_ports(HRESULT)  Altair_get_ExpoTime(HAltair h, unsigned* Time); /* in microseconds */
	altair_ports(HRESULT)  Altair_put_ExpoTime(HAltair h, unsigned Time); /* in microseconds */
	altair_ports(HRESULT)  Altair_get_ExpTimeRange(HAltair h, unsigned* nMin, unsigned* nMax, unsigned* nDef);

	altair_ports(HRESULT)  Altair_get_ExpoAGain(HAltair h, unsigned short* AGain); /* percent, such as 300 */
	altair_ports(HRESULT)  Altair_put_ExpoAGain(HAltair h, unsigned short AGain); /* percent */
	altair_ports(HRESULT)  Altair_get_ExpoAGainRange(HAltair h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef);

	/* Auto White Balance, Temp/Tint Mode */
	altair_ports(HRESULT)  Altair_AwbOnePush(HAltair h, PIALTAIR_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx); /* auto white balance "one push". The function must be called AFTER Altair_StartXXXX */

	/* Auto White Balance, RGB Gain Mode */
	altair_ports(HRESULT)  Altair_AwbInit(HAltair h, PIALTAIR_WHITEBALANCE_CALLBACK fnWBProc, void* pWBCtx);

	/* White Balance, Temp/Tint mode */
	altair_ports(HRESULT)  Altair_put_TempTint(HAltair h, int nTemp, int nTint);
	altair_ports(HRESULT)  Altair_get_TempTint(HAltair h, int* nTemp, int* nTint);

	/* White Balance, RGB Gain mode */
	altair_ports(HRESULT)  Altair_put_WhiteBalanceGain(HAltair h, int aGain[3]);
	altair_ports(HRESULT)  Altair_get_WhiteBalanceGain(HAltair h, int aGain[3]);

	altair_ports(HRESULT)  Altair_put_Hue(HAltair h, int Hue);
	altair_ports(HRESULT)  Altair_get_Hue(HAltair h, int* Hue);
	altair_ports(HRESULT)  Altair_put_Saturation(HAltair h, int Saturation);
	altair_ports(HRESULT)  Altair_get_Saturation(HAltair h, int* Saturation);
	altair_ports(HRESULT)  Altair_put_Brightness(HAltair h, int Brightness);
	altair_ports(HRESULT)  Altair_get_Brightness(HAltair h, int* Brightness);
	altair_ports(HRESULT)  Altair_get_Contrast(HAltair h, int* Contrast);
	altair_ports(HRESULT)  Altair_put_Contrast(HAltair h, int Contrast);
	altair_ports(HRESULT)  Altair_get_Gamma(HAltair h, int* Gamma); /* percent */
	altair_ports(HRESULT)  Altair_put_Gamma(HAltair h, int Gamma);  /* percent */

	altair_ports(HRESULT)  Altair_get_Chrome(HAltair h, BOOL* bChrome);  /* monochromatic mode */
	altair_ports(HRESULT)  Altair_put_Chrome(HAltair h, BOOL bChrome);

	altair_ports(HRESULT)  Altair_get_VFlip(HAltair h, BOOL* bVFlip);  /* vertical flip */
	altair_ports(HRESULT)  Altair_put_VFlip(HAltair h, BOOL bVFlip);
	altair_ports(HRESULT)  Altair_get_HFlip(HAltair h, BOOL* bHFlip);
	altair_ports(HRESULT)  Altair_put_HFlip(HAltair h, BOOL bHFlip); /* horizontal flip */

	altair_ports(HRESULT)  Altair_get_Negative(HAltair h, BOOL* bNegative);  /* negative film */
	altair_ports(HRESULT)  Altair_put_Negative(HAltair h, BOOL bNegative);

	altair_ports(HRESULT)  Altair_put_Speed(HAltair h, unsigned short nSpeed);
	altair_ports(HRESULT)  Altair_get_Speed(HAltair h, unsigned short* pSpeed);
	altair_ports(HRESULT)  Altair_get_MaxSpeed(HAltair h); /* get the maximum speed, see "Frame Speed Level", the speed range = [0, max], closed interval */

	altair_ports(HRESULT)  Altair_get_FanMaxSpeed(HAltair h); /* get the maximum fan speed, the fan speed range = [0, max], closed interval */


	altair_ports(HRESULT)  Altair_get_MaxBitDepth(HAltair h); /* get the max bit depth of this camera, such as 8, 10, 12, 14, 16 */
	/* power supply:
	0 -> 60HZ AC
	1 -> 50Hz AC
	2 -> DC
	*/
	altair_ports(HRESULT)  Altair_put_HZ(HAltair h, int nHZ);
	altair_ports(HRESULT)  Altair_get_HZ(HAltair h, int* nHZ);

	altair_ports(HRESULT)  Altair_put_Mode(HAltair h, BOOL bSkip); /* skip or bin */
	altair_ports(HRESULT)  Altair_get_Mode(HAltair h, BOOL* bSkip); /* If the model don't support bin/skip mode, return E_NOTIMPL */

	altair_ports(HRESULT)  Altair_put_AWBAuxRect(HAltair h, const RECT* pAuxRect); /* auto white balance ROI */
	altair_ports(HRESULT)  Altair_get_AWBAuxRect(HAltair h, RECT* pAuxRect);
	altair_ports(HRESULT)  Altair_put_AEAuxRect(HAltair h, const RECT* pAuxRect);  /* auto exposure ROI */
	altair_ports(HRESULT)  Altair_get_AEAuxRect(HAltair h, RECT* pAuxRect);

	/*
	S_FALSE:    color mode
	S_OK:       mono mode, such as EXCCD00300KMA and UHCCD01400KMA
	*/
	altair_ports(HRESULT)  Altair_get_MonoMode(HAltair h);

	altair_ports(HRESULT)  Altair_get_StillResolutionNumber(HAltair h);
	altair_ports(HRESULT)  Altair_get_StillResolution(HAltair h, unsigned nIndex, int* pWidth, int* pHeight);

	/* default: FALSE */
	altair_ports(HRESULT)  Altair_put_RealTime(HAltair h, BOOL bEnable);
	altair_ports(HRESULT)  Altair_get_RealTime(HAltair h, BOOL* bEnable);

	altair_ports(HRESULT)  Altair_Flush(HAltair h);  /* discard the current internal frame cache */

	/* get the temperature of sensor, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius)
	return E_NOTIMPL if not supported
	*/
	altair_ports(HRESULT)  Altair_get_Temperature(HAltair h, short* pTemperature);

	/* set the temperature of sensor, in 0.1 degrees Celsius (32 means 3.2 degrees Celsius)
	return E_NOTIMPL if not supported
	*/
	altair_ports(HRESULT)  Altair_put_Temperature(HAltair h, short nTemperature);

	/*
	get the serial number which is always 32 chars which is zero-terminated such as "TP110826145730ABCD1234FEDC56787"
	*/
	altair_ports(HRESULT)  Altair_get_SerialNumber(HAltair h, char sn[32]);

	/*
	get the camera firmware version, such as: 3.2.1.20140922
	*/
	altair_ports(HRESULT)  Altair_get_FwVersion(HAltair h, char fwver[16]);

	/*
	get the camera hardware version, such as: 3.2.1.20140922
	*/
	altair_ports(HRESULT)  Altair_get_HwVersion(HAltair h, char hwver[16]);

	/*
	get the production date, such as: 20150327
	*/
	altair_ports(HRESULT)  Altair_get_ProductionDate(HAltair h, char pdate[10]);

	/*
	get the sensor pixel size, such as: 2.4um
	*/
	altair_ports(HRESULT)  Altair_get_PixelSize(HAltair h, unsigned nResolutionIndex, float* x, float* y);


	altair_ports(HRESULT)  Altair_put_LevelRange(HAltair h, unsigned short aLow[4], unsigned short aHigh[4]);
	altair_ports(HRESULT)  Altair_get_LevelRange(HAltair h, unsigned short aLow[4], unsigned short aHigh[4]);

	altair_ports(HRESULT)  Altair_put_ExpoCallback(HAltair h, PIALTAIR_EXPOSURE_CALLBACK fnExpoProc, void* pExpoCtx);
	altair_ports(HRESULT)  Altair_put_ChromeCallback(HAltair h, PIALTAIR_CHROME_CALLBACK fnChromeProc, void* pChromeCtx);

	/*
	The following functions must be called AFTER Altair_StartPushMode or Altair_StartPullModeWithWndMsg or Altair_StartPullModeWithCallback
	*/
	altair_ports(HRESULT)  Altair_LevelRangeAuto(HAltair h);
	altair_ports(HRESULT)  Altair_GetHistogram(HAltair h, PIALTAIR_HISTOGRAM_CALLBACK fnHistogramProc, void* pHistogramCtx);

	/* led state:
	iLed: Led index, (0, 1, 2, ...)
	iState: 1 -> Ever bright; 2 -> Flashing; other -> Off
	iPeriod: Flashing Period (>= 500ms)
	*/
	altair_ports(HRESULT)  Altair_put_LEDState(HAltair h, unsigned short iLed, unsigned short iState, unsigned short iPeriod);

	altair_ports(HRESULT)  Altair_write_EEPROM(HAltair h, unsigned addr, const unsigned char* pData, unsigned nDataLen);
	altair_ports(HRESULT)  Altair_read_EEPROM(HAltair h, unsigned addr, unsigned char* pBuffer, unsigned nBufferLen);

	altair_ports(HRESULT)  Altair_write_UART(HAltair h, const unsigned char* pData, unsigned nDataLen);
	altair_ports(HRESULT)  Altair_read_UART(HAltair h, unsigned char* pBuffer, unsigned nBufferLen);


#define ALTAIR_TEC_TARGET_MIN      -300/* -30.0 degrees Celsius */
#define ALTAIR_TEC_TARGET_DEF      -100/* -10.0 degrees Celsius */
#define ALTAIR_TEC_TARGET_MAX      300 /* 30.0 degrees Celsius */

#define ALTAIR_OPTION_NOFRAME_TIMEOUT      0x01    /* iValue: 1 = enable; 0 = disable. default: enable */
#define ALTAIR_OPTION_THREAD_PRIORITY      0x02    /* set the priority of the internal thread which grab data from the usb device. iValue: 0 = THREAD_PRIORITY_NORMAL; 1 = THREAD_PRIORITY_ABOVE_NORMAL; 2 = THREAD_PRIORITY_HIGHEST; default: 0; see: msdn SetThreadPriority */
#define ALTAIR_OPTION_PROCESSMODE          0x03    /*  0 = better image quality, more cpu usage. this is the default value
	1 = lower image quality, less cpu usage */
#define ALTAIR_OPTION_RAW                  0x04    /* raw mode, read the sensor data. This can be set only BEFORE Altair_StartXXX() */
#define ALTAIR_OPTION_HISTOGRAM            0x05    /* 0 = only one, 1 = continue mode */
#define ALTAIR_OPTION_BITDEPTH             0x06    /* 0 = 8bits mode, 1 = 16bits mode */
#define ALTAIR_OPTION_FAN                  0x07    /* 0 = turn off the cooling fan, 1 = turn on the cooling fan */
#define ALTAIR_OPTION_TEC				   0x08    /* 0 = turn off cooler, 1 = turn on cooler */
#define ALTAIR_OPTION_LINEAR               0x09    /* 0 = turn off tone linear, 1 = turn on tone linear */
#define ALTAIR_OPTION_CURVE                0x0a    /* 0 = turn off tone curve, 1 = turn on tone curve */
#define ALTAIR_OPTION_TRIGGER              0x0b    /* 0 = continuous mode, 1 = trigger mode, default value =  0 */
#define ALTAIR_OPTION_RGB48                0x0c    /* enable RGB48 format when bitdepth > 8 */
#define ALTAIR_OPTION_TECTARGET            0x0f    /* get or set the target temperature of the thermoelectric cooler, in degree Celsius */
#define ALTAIR_OPTION_AGAIN                0x10    /* enable or disable adjusting the analog gain when auto exposure is enabled. default value: enable */
#define ALTAIR_OPTION_FRAMERATE            0x11    /* limit the frame rate, range=[0, 63], the default value 0 means no limit. frame rate control is auto disabled in trigger mode */


	/*
	get the frame rate: framerate (fps) = Frame * 1000.0 / nTime
	*/
	altair_ports(HRESULT)  Altair_get_FrameRate(HAltair h, unsigned* nFrame, unsigned* nTime, unsigned* nTotalFrame);


	altair_ports(HRESULT)  Altair_put_Option(HAltair h, unsigned iOption, int iValue);
	altair_ports(HRESULT)  Altair_get_Option(HAltair h, unsigned iOption, int* piValue);

	altair_ports(HRESULT)  Altair_put_Roi(HAltair h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
	altair_ports(HRESULT)  Altair_get_Roi(HAltair h, unsigned* pxOffset, unsigned* pyOffset, unsigned* pxWidth, unsigned* pyHeight);

	/* astronomy: only for ST4 guide, please see: ASCOM Platform Help ITelescopeV3 */
	altair_ports(HRESULT)  Altair_ST4PlusGuide(HAltair h, unsigned nDirect, unsigned nDuration);
	altair_ports(HRESULT)  Altair_ST4PlusGuideState(HAltair h);

	/*
	calculate the clarity factor:
	pImageData: pointer to the image data
	bits: 8(Grey), 24 (RGB24), 32(RGB32)
	nImgWidth, nImgHeight: the image width and height
	*/
	altair_ports(double)   Altair_calc_ClarityFactor(const void* pImageData, int bits, unsigned nImgWidth, unsigned nImgHeight);

	altair_ports(void)     Altair_deBayer(unsigned nBayer, int nW, int nH, const void* input, void* output, unsigned char nBitDepth);

#ifndef _WIN32

	typedef void(*PALTAIR_HOTPLUG)(void* pCallbackCtx);
	altair_ports(void)   Altair_HotPlug(PALTAIR_HOTPLUG pHotPlugCallback, void* pCallbackCtx);

#else
	/*
	strRegPath, such as: Software\xxxCompany\yyyApplication.
	If we call this function to enable this feature, the camera parameters will be save in the Registry at HKEY_CURRENT_USER\Software\XxxCompany\yyyApplication\{CameraModelName} when we close the handle,
	and then, the next time, we open the camera, the parameters will be loaded automatically.
	*/
	altair_ports(void)     Altair_EnableReg(const wchar_t* strRegPath);

	/* Altair_Start is obsolete, it's a synonyms for Altair_StartPushMode. They are exactly the same. */
	altair_ports(HRESULT)  Altair_Start(HAltair h, PALTAIR_DATA_CALLBACK pDataCallback, void* pCallbackCtx);

	/* Altair_put_TempTintInit is obsolete, it's a synonyms for Altair_AwbOnePush. They are exactly the same. */
	altair_ports(HRESULT)  Altair_put_TempTintInit(HAltair h, PIALTAIR_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx);

	/*
	This is obsolete, please use:
	set or get the process mode: ALTAIR_PROCESSMODE_FULL or ALTAIR_PROCESSMODE_FAST. default is ALTAIR_PROCESSMODE_FULL.
	*/
#ifndef __ALTAIR_PROCESSMODE_DEFINED__
#define __ALTAIR_PROCESSMODE_DEFINED__
#define ALTAIR_PROCESSMODE_FULL        0x00    /* better image quality, more cpu usage. this is the default value */
#define ALTAIR_PROCESSMODE_FAST        0x01    /* lower image quality, less cpu usage */
#endif

	altair_ports(HRESULT)  Altair_put_ProcessMode(HAltair h, unsigned nProcessMode);
	altair_ports(HRESULT)  Altair_get_ProcessMode(HAltair h, unsigned* pnProcessMode);

#endif

	/* This is obsolete */
	altair_ports(HRESULT)  Altair_put_RoiMode(HAltair h, BOOL bRoiMode, int xOffset, int yOffset);
	altair_ports(HRESULT)  Altair_get_RoiMode(HAltair h, BOOL* pbRoiMode, int* pxOffset, int* pyOffset);

	/* This is obsolete
	------------------------------------------------------------|
	| Parameter         |   Range       |   Default             |
	|-----------------------------------------------------------|
	| VidgetAmount      |   -100~100    |   0                   |
	| VignetMidPoint    |   0~100       |   50                  |
	-------------------------------------------------------------
	*/
	altair_ports(HRESULT)  Altair_put_VignetEnable(HAltair h, BOOL bEnable);
	altair_ports(HRESULT)  Altair_get_VignetEnable(HAltair h, BOOL* bEnable);
	altair_ports(HRESULT)  Altair_put_VignetAmountInt(HAltair h, int nAmount);
	altair_ports(HRESULT)  Altair_get_VignetAmountInt(HAltair h, int* nAmount);
	altair_ports(HRESULT)  Altair_put_VignetMidPointInt(HAltair h, int nMidPoint);
	altair_ports(HRESULT)  Altair_get_VignetMidPointInt(HAltair h, int* nMidPoint);

#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif


#endif
