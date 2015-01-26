#ifdef CMOSDLL_EXPORTS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

EXPORT int   _stdcall _SSAG_checkdevice();
EXPORT void  _stdcall _SSAG_closeUSB();
EXPORT void  _stdcall _SSAG_CameraReset();
EXPORT void  _stdcall _SSAG_CancelExposure();
EXPORT DWORD _stdcall _SSAG_openUSB();
EXPORT DWORD _stdcall _SSAG_ProgramCameraMONO(DWORD x,DWORD y,DWORD w,DWORD h,DWORD gr, DWORD gg, DWORD gb , DWORD gg2 );
EXPORT DWORD _stdcall _SSAG_ProgramCameraRGB(DWORD x,DWORD y,DWORD w,DWORD h,DWORD gr, DWORD gg, DWORD gb  );
EXPORT DWORD _stdcall _SSAG_ProgramCamera(DWORD x,DWORD y,DWORD w,DWORD h,DWORD gain  );
EXPORT DWORD _stdcall _SSAG_GuideCommand(DWORD GuideCommand, DWORD PulseTime);
EXPORT DWORD _stdcall _SSAG_GuideCommandXY(DWORD GC, DWORD PulseTimeX, DWORD PulseTimeY);
EXPORT DWORD _stdcall _SSAG_CancelGuide( DWORD x, DWORD y );
EXPORT void  _stdcall _SSAG_ThreadedExposure( DWORD expolength, BYTE *imgdata );
EXPORT void  _stdcall _SSAG_Exposure( DWORD expolength, BYTE *imgdata );
EXPORT DWORD _stdcall _SSAG_isExposing();
EXPORT void  _stdcall _SSAG_SETBUFFERMODE(DWORD mode );
EXPORT DWORD _stdcall _SSAG_getVIDPID();
EXPORT void  _stdcall _SSAG_debayerImage(BYTE *src,BYTE *tgt, DWORD w, DWORD h);
EXPORT void  _stdcall _SSAG_debayerImageW(WORD *src,BYTE *tgt, DWORD w, DWORD h);
EXPORT void  _stdcall _SSAG_debayerBuffer(BYTE *tgt, DWORD w, DWORD h);
EXPORT void  _stdcall _SSAG_bufferCopy(BYTE *src, BYTE *tgt, int size );
EXPORT void *_stdcall _SSAG_getbufptr();
EXPORT void *_stdcall _SSAG_getbufptr16();
EXPORT void  _stdcall _SSAG_GETBUFFER(void *x, DWORD s);
EXPORT void  _stdcall _SSAG_SetNoiseReduction( int n );
EXPORT void  _stdcall _SSAG_SetExposureTime( int E);


