// gxeth: Gx Camera Ethernet Adapter Driver

# pragma once
# include "windows.h"

# define EXPORT_ __declspec(dllimport)

// messages notifying camera connect state change
// sent to HWND passed as RegisterNotifyHWND parameter
#define WM_CAMERA_CONNECT         1034
#define WM_CAMERA_DISCONNECT      1035

// GetBooleanParameter indexes
#define gbpConnected                 0
#define gbpSubFrame                  1
#define gbpReadModes                 2
#define gbpShutter                   3
#define gbpCooler                    4
#define gbpFan                       5
#define gbpFilters                   6
#define gbpGuide                     7
#define gbpWindowHeating             8
#define gbpPreflash                  9
#define gbpAsymmetricBinning        10
#define gbpMicrometerFilterOffsets  11
#define gbpPowerUtilization         12
#define gbpGain                     13
#define gbpElectronicShutter        14
#define gbpReadBeforeClose          15
#define gbpConfigured              127
#define gbpRGB                     128
#define gbpCMY                     129
#define gbpCMYG                    130
#define gbpDebayerXOdd             131
#define gbpDebayerYOdd             132
#define gbpInterlaced              256

// GetIntegerParameter indexes
#define gipCameraId                  0
#define gipChipW                     1
#define gipChipD                     2
#define gipPixelW                    3
#define gipPixelD                    4
#define gipMaxBinningX               5
#define gipMaxBinningY               6
#define gipReadModes                 7
#define gipFilters                   8
#define gipMinimalExposure           9
#define gipMaximalExposure          10
#define gipMaximalMoveTime          11
#define gipDefaultReadMode          12
#define gipPreviewReadMode          13
#define gipMaxWindowHeating         14
#define gipMaxFan                   15
#define gipMaxGain                  16
#define gipMaxPossiblePixelValue    17

#define gipFirmwareMajor           128
#define gipFirmwareMinor           129
#define gipFirmwareBuild           130
#define gipDriverMajor             131
#define gipDriverMinor             132
#define gipDriverBuild             133
#define gipFlashMajor              134
#define gipFlashMinor              135
#define gipFlashBuild              136

// GetStringParameter indexes
#define gspCameraDescription         0
#define gspManufacturer              1
#define gspCameraSerial              2
#define gspChipDescription           3

// GetValue indexes
#define gvChipTemperature            0
#define gvHotTemperature             1
#define gvCameraTemperature          2
#define gvEnvironmentTemperature     3
#define gvSupplyVoltage             10
#define gvPowerUtilization          11
#define gvADCGain                   20

namespace gxetha {

typedef int            INTEGER;
typedef short          INT16;
typedef unsigned       CARDINAL;
typedef unsigned char  CARD8;
typedef float          REAL;
typedef double         LONGREAL;
typedef char           CHAR;
typedef unsigned char  BOOLEAN;
typedef void *         ADDRESS;

typedef void (__cdecl *TEnumerateCallback)( CARDINAL );

struct CCamera;

extern "C" EXPORT_ void     __cdecl Enumerate( void (__cdecl *CallbackProc)(CARDINAL ));
extern "C" EXPORT_ CCamera *__cdecl Initialize( CARDINAL Id);
extern "C" EXPORT_ void     __cdecl Configure( CCamera *PCamera, HWND ParentHWND );
extern "C" EXPORT_ void     __cdecl Release( CCamera *PCamera );

extern "C" EXPORT_ void     __cdecl RegisterNotifyHWND( CCamera *PCamera, HWND NotifyHWND );

extern "C" EXPORT_ BOOLEAN  __cdecl GetBooleanParameter( CCamera *PCamera, CARDINAL Index, BOOLEAN *Boolean );
extern "C" EXPORT_ BOOLEAN  __cdecl GetIntegerParameter( CCamera *PCamera, CARDINAL Index, CARDINAL *Num );
extern "C" EXPORT_ BOOLEAN  __cdecl GetStringParameter( CCamera *PCamera, CARDINAL Index, CARDINAL String_HIGH, CHAR *String );
extern "C" EXPORT_ BOOLEAN  __cdecl GetValue( CCamera *PCamera, CARDINAL Index, REAL *Value) ;

extern "C" EXPORT_ BOOLEAN  __cdecl SetTemperature( CCamera *PCamera, REAL Temperature );
extern "C" EXPORT_ BOOLEAN  __cdecl SetTemperatureRamp( CCamera *PCamera, REAL TemperatureRamp );
extern "C" EXPORT_ BOOLEAN  __cdecl SetBinning( CCamera *PCamera, CARDINAL x, CARDINAL y );

extern "C" EXPORT_ BOOLEAN  __cdecl StartExposure( CCamera *PCamera, LONGREAL ExpTime, BOOLEAN UseShutter, INTEGER x, INTEGER y, INTEGER w, INTEGER d );
extern "C" EXPORT_ BOOLEAN  __cdecl AbortExposure( CCamera *PCamera, BOOLEAN DownloadFlag );
extern "C" EXPORT_ BOOLEAN  __cdecl ImageReady( CCamera *PCamera, BOOLEAN *Ready );
extern "C" EXPORT_ BOOLEAN  __cdecl ReadImage( CCamera *PCamera, CARDINAL BufferLen, ADDRESS BufferAdr );

extern "C" EXPORT_ BOOLEAN  __cdecl EnumerateReadModes( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description );
extern "C" EXPORT_ BOOLEAN  __cdecl SetReadMode( CCamera *PCamera, CARDINAL mode );
extern "C" EXPORT_ BOOLEAN  __cdecl SetGain( CCamera *PCamera, CARDINAL gain );
extern "C" EXPORT_ BOOLEAN  __cdecl ConvertGain( CCamera *PCamera, CARDINAL gain, LONGREAL *dB, LONGREAL *times );
extern "C" EXPORT_ BOOLEAN  __cdecl EnumerateFilters( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description, CARDINAL *Color );
extern "C" EXPORT_ BOOLEAN  __cdecl EnumerateFilters2( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description, CARDINAL *Color, INTEGER *Offset );
extern "C" EXPORT_ BOOLEAN  __cdecl SetFilter( CCamera *PCamera, CARDINAL index );
extern "C" EXPORT_ BOOLEAN  __cdecl SetFan( CCamera *PCamera, CARD8 Speed );
extern "C" EXPORT_ BOOLEAN  __cdecl SetWindowHeating( CCamera *PCamera, CARD8 Heating );
extern "C" EXPORT_ BOOLEAN  __cdecl SetPreflash( CCamera *PCamera, LONGREAL PreflashTime, CARDINAL ClearNum );

extern "C" EXPORT_ BOOLEAN  __cdecl MoveTelescope( CCamera *PCamera, INT16 RADurationMs, INT16 DecDurationMs );
extern "C" EXPORT_ BOOLEAN  __cdecl MoveInProgress( CCamera *PCamera, BOOLEAN *Moving );

extern "C" EXPORT_ void     __cdecl GetLastErrorString( CCamera *PCamera, CARDINAL ErrorString_HIGH, CHAR *ErrorString );

}; // namespace gxetha
