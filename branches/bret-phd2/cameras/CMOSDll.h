// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CMOSDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CMOSDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CMOSDLL_EXPORTS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

EXPORT int   _stdcall checkdevice(int n);
EXPORT void  _stdcall closeUSB();
EXPORT void  _stdcall CameraReset();
EXPORT void  _stdcall CancelExposure();
EXPORT DWORD _stdcall openUSB(int i);
EXPORT DWORD _stdcall ProgramCameraRGB(DWORD x,DWORD y,DWORD w,DWORD h,DWORD gr, DWORD gg, DWORD gb  );
EXPORT DWORD _stdcall ProgramCamera(DWORD x,DWORD y,DWORD w,DWORD h,DWORD gain  );
EXPORT DWORD _stdcall GuideCommand(DWORD GuideCommand, DWORD PulseTime);
EXPORT void _stdcall ThreadedExposure( DWORD expolength, BYTE *imgdata );
EXPORT void _stdcall Exposure( DWORD expolength, BYTE *imgdata );
EXPORT DWORD _stdcall isExposing();
EXPORT DWORD _stdcall DEBUG( DWORD entry );
void _stdcall SETBUFFERMODE(DWORD mode );
EXPORT DWORD _stdcall getVIDPID();
EXPORT void  _stdcall debayerImage(BYTE *src,BYTE *tgt, DWORD w, DWORD h);
EXPORT void  _stdcall debayerImageW(WORD *src,BYTE *tgt, DWORD w, DWORD h);
EXPORT void  _stdcall debayerBuffer(BYTE *tgt, DWORD w, DWORD h);
EXPORT void *  _stdcall  getbufptr();
EXPORT void _stdcall GETBUFFER(void *x, DWORD s);
EXPORT void _stdcall SetNoiseReduction( int n );


