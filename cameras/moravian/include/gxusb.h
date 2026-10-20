// gxusb: Cx/Gx Camera USB Driver

# pragma once
# include "windows.h"

# define EXPORT_ __declspec(dllimport)

// messages notifying camera connect state change
// sent to HWND passed as RegisterNotifyHWND parameter
#define WM_CAMERA_CONNECT         1034
#define WM_CAMERA_DISCONNECT      1035

// NOTE legacy index prefixes used 3 letters (gbpXXXX, gipXXXX, gspXXXX), following the original function name GetBooleanParameneter, ...
// cameraGetBoolean() indexes
#define gbConnected                 0
#define gbSubFrame                  1
#define gbReadModes                 2
#define gbShutter                   3
#define gbCooler                    4
#define gbFan                       5
#define gbFilters                   6
#define gbGuide                     7
#define gbWindowHeating             8
#define gbPreflash                  9
#define gbAsymmetricBinning        10
#define gbMicrometerFilterOffsets  11
#define gbPowerUtilization         12
#define gbGain                     13
#define gbElectronicShutter        14
#define gbGPS                      16
#define gbContinuousExposures      17
#define gbTrigger                  18

#define gbConfigured              127
#define gbRGB                     128
#define gbCMY                     129
#define gbCMYG                    130
#define gbDebayerXOdd             131
#define gbDebayerYOdd             132
#define gbInterlaced              256

// cameraGetInteger() indexes
#define giCameraId                  0
#define giChipW                     1
#define giChipD                     2
#define giPixelW                    3
#define giPixelD                    4
#define giMaxBinningX               5
#define giMaxBinningY               6
#define giReadModes                 7
#define giFilters                   8
#define giMinimalExposure           9
#define giMaximalExposure          10
#define giMaximalMoveTime          11
#define giDefaultReadMode          12
#define giPreviewReadMode          13
#define giMaxWindowHeating         14
#define giMaxFan                   15
#define giMaxGain                  16
#define giMaxPossiblePixelValue    17
#define giLineTime                 18
#define giBiasPixelValue           19

#define giFirmwareMajor           128
#define giFirmwareMinor           129
#define giFirmwareBuild           130
#define giDriverMajor             131
#define giDriverMinor             132
#define giDriverBuild             133
#define giFlashMajor              134
#define giFlashMinor              135
#define giFlashBuild              136

// GetString() indexes
#define gsCameraDescription         0
#define gsManufacturer              1
#define gsCameraSerial              2
#define gsChipDescription           3

// GetValue() indexes
#define gvChipTemperature           0
#define gvHotTemperature            1
#define gvCameraTemperature         2
#define gvEnvironmentTemperature    3
#define gvSupplyVoltage            10
#define gvPowerUtilization         11
#define gvADCGain                  20

namespace gXusb {

typedef int            INTEGER;
typedef short          INT16;
typedef unsigned       CARDINAL;
typedef unsigned char  CARD8;
typedef float          REAL;
typedef double         LONGREAL;
typedef char           CHAR;
typedef unsigned char  BOOLEAN;
typedef void *         ADDRESS;

struct CCamera;

// current "camera" prefixed API functions

extern "C" EXPORT_ void     __cdecl cameraEnumerate( void (__cdecl *CallbackProc)( CARDINAL));
extern "C" EXPORT_ CCamera *__cdecl cameraInitialize( CARDINAL Id );
extern "C" EXPORT_ void     __cdecl cameraRelease( CCamera *PCamera );

extern "C" EXPORT_ void     __cdecl cameraRegisterNotifyHWND( CCamera *PCamera, HWND NotifyHWND );

extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetBoolean( CCamera *PCamera, CARDINAL Index, BOOLEAN *Boolean );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetInteger( CCamera *PCamera, CARDINAL Index, CARDINAL *Num );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetString( CCamera *PCamera, CARDINAL Index, CARDINAL String_HIGH, CHAR *String );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetValue( CCamera *PCamera, CARDINAL Index, REAL *Value) ;

extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetTemperature( CCamera *PCamera, REAL Temperature );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetBinning( CCamera *PCamera, CARDINAL x, CARDINAL y );

extern "C" EXPORT_ BOOLEAN  __cdecl cameraClearSensor( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraOpen( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraClose( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraBeginExposure( CCamera *PCamera, BOOLEAN UseShutter );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraEndExposure( CCamera *PCamera, BOOLEAN UseShutter, BOOLEAN AbortData );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetImage( CCamera *PCamera, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetImage8b( CCamera *PCamera, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetImage16b( CCamera *PCamera, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );

extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetImageExposure( CCamera *PCamera, LONGREAL ExpTime, BOOLEAN UseShutter, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetImageExposure8b( CCamera *PCamera, LONGREAL ExpTime, BOOLEAN UseShutter, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetImageExposure16b( CCamera *PCamera, LONGREAL ExpTime, BOOLEAN UseShutter, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );

extern "C" EXPORT_ BOOLEAN  __cdecl cameraAdjustSubFrame( CCamera *PCamera, INTEGER *x, INTEGER *y, INTEGER *w, INTEGER *d );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraEnumerateReadModes( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetReadMode( CCamera *PCamera, CARDINAL mode );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetGain( CCamera *PCamera, CARDINAL gain );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraConvertGain( CCamera *PCamera, CARDINAL gain, LONGREAL *dB, LONGREAL *times );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraEnumerateFilters( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description, CARDINAL *Color );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraEnumerateFilters2( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description, CARDINAL *Color, INTEGER *Offset );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetFilter( CCamera *PCamera, CARDINAL index );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraReinitFilterWheel( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetFan( CCamera *PCamera, CARD8 Speed );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetWindowHeating( CCamera *PCamera, CARD8 Heating );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraSetPreflash( CCamera *PCamera, LONGREAL PreflashTime, CARDINAL ClearNum );

extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetImageTimeStamp( CCamera PCamera, INTEGER *Year, INTEGER *Month, INTEGER *Day, INTEGER *Hour, INTEGER *Minute, LONGREAL *Second );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetGPSData( CCamera PCamera, LONGREAL *Lat, LONGREAL *Lon, LONGREAL *MSL, INTEGER *Year, INTEGER *Month, INTEGER *Day, INTEGER *Hour, INTEGER *Minute, LONGREAL *Second, CARDINAL *Satellites, BOOLEAN *Fix );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraGetGPSData2( CCamera PCamera, LONGREAL *Lat, LONGREAL *Lon, LONGREAL *MSL, LONGREAL *GeoidSep, INTEGER *Year, INTEGER *Month, INTEGER *Day, INTEGER *Hour, INTEGER *Minute, LONGREAL *Second, CARDINAL *Satellites, BOOLEAN *Fix );

extern "C" EXPORT_ BOOLEAN  __cdecl cameraMoveTelescope( CCamera *PCamera, INT16 RADurationMs, INT16 DecDurationMs );
extern "C" EXPORT_ BOOLEAN  __cdecl cameraMoveInProgress( CCamera *PCamera, BOOLEAN *Moving );

extern "C" EXPORT_ void     __cdecl cameraGetLastErrorString( CCamera *PCamera, CARDINAL ErrorString_HIGH, CHAR *ErrorString );

// backward-compatible deprecated API functions

extern "C" EXPORT_ void     __cdecl Enumerate( void (__cdecl *CallbackProc)( CARDINAL));
extern "C" EXPORT_ CCamera *__cdecl Initialize( CARDINAL Id );
extern "C" EXPORT_ void     __cdecl Release( CCamera *PCamera );

extern "C" EXPORT_ void     __cdecl RegisterNotifyHWND( CCamera *PCamera, HWND NotifyHWND );

extern "C" EXPORT_ BOOLEAN  __cdecl GetBooleanParameter( CCamera *PCamera, CARDINAL Index, BOOLEAN *Boolean );
extern "C" EXPORT_ BOOLEAN  __cdecl GetIntegerParameter( CCamera *PCamera, CARDINAL Index, CARDINAL *Num );
extern "C" EXPORT_ BOOLEAN  __cdecl GetStringParameter( CCamera *PCamera, CARDINAL Index, CARDINAL String_HIGH, CHAR *String );
extern "C" EXPORT_ BOOLEAN  __cdecl GetValue( CCamera *PCamera, CARDINAL Index, REAL *Value) ;

extern "C" EXPORT_ BOOLEAN  __cdecl SetTemperature( CCamera *PCamera, REAL Temperature );
extern "C" EXPORT_ BOOLEAN  __cdecl SetBinning( CCamera *PCamera, CARDINAL x, CARDINAL y );

extern "C" EXPORT_ BOOLEAN  __cdecl ClearSensor( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl Open( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl Close( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl BeginExposure( CCamera *PCamera, BOOLEAN UseShutter );
extern "C" EXPORT_ BOOLEAN  __cdecl EndExposure( CCamera *PCamera, BOOLEAN UseShutter, BOOLEAN AbortData );
extern "C" EXPORT_ BOOLEAN  __cdecl GetImage( CCamera *PCamera, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl GetImage8b( CCamera *PCamera, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl GetImage16b( CCamera *PCamera, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );

extern "C" EXPORT_ BOOLEAN  __cdecl GetImageExposure( CCamera *PCamera, LONGREAL ExpTime, BOOLEAN UseShutter, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl GetImageExposure8b( CCamera *PCamera, LONGREAL ExpTime, BOOLEAN UseShutter, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );
extern "C" EXPORT_ BOOLEAN  __cdecl GetImageExposure16b( CCamera *PCamera, LONGREAL ExpTime, BOOLEAN UseShutter, INTEGER x, INTEGER y, INTEGER w, INTEGER d, CARDINAL BufferLen, ADDRESS BufferAdr );

extern "C" EXPORT_ BOOLEAN  __cdecl AdjustSubFrame( CCamera *PCamera, INTEGER *x, INTEGER *y, INTEGER *w, INTEGER *d );
extern "C" EXPORT_ BOOLEAN  __cdecl EnumerateReadModes( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description );
extern "C" EXPORT_ BOOLEAN  __cdecl SetReadMode( CCamera *PCamera, CARDINAL mode );
extern "C" EXPORT_ BOOLEAN  __cdecl SetGain( CCamera *PCamera, CARDINAL gain );
extern "C" EXPORT_ BOOLEAN  __cdecl ConvertGain( CCamera *PCamera, CARDINAL gain, LONGREAL *dB, LONGREAL *times );
extern "C" EXPORT_ BOOLEAN  __cdecl EnumerateFilters( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description, CARDINAL *Color );
extern "C" EXPORT_ BOOLEAN  __cdecl EnumerateFilters2( CCamera *PCamera, CARDINAL Index, CARDINAL Description_HIGH, CHAR *Description, CARDINAL *Color, INTEGER *Offset );
extern "C" EXPORT_ BOOLEAN  __cdecl SetFilter( CCamera *PCamera, CARDINAL index );
extern "C" EXPORT_ BOOLEAN  __cdecl ReinitFilterWheel( CCamera *PCamera );
extern "C" EXPORT_ BOOLEAN  __cdecl SetFan( CCamera *PCamera, CARD8 Speed );
extern "C" EXPORT_ BOOLEAN  __cdecl SetWindowHeating( CCamera *PCamera, CARD8 Heating );
extern "C" EXPORT_ BOOLEAN  __cdecl SetPreflash( CCamera *PCamera, LONGREAL PreflashTime, CARDINAL ClearNum );

extern "C" EXPORT_ BOOLEAN  __cdecl GetImageTimeStamp( CCamera PCamera, INTEGER *Year, INTEGER *Month, INTEGER *Day, INTEGER *Hour, INTEGER *Minute, LONGREAL *Second );
extern "C" EXPORT_ BOOLEAN  __cdecl GetGPSData( CCamera PCamera, LONGREAL *Lat, LONGREAL *Lon, LONGREAL *MSL, INTEGER *Year, INTEGER *Month, INTEGER *Day, INTEGER *Hour, INTEGER *Minute, LONGREAL *Second, CARDINAL *Satellites, BOOLEAN *Fix );
extern "C" EXPORT_ BOOLEAN  __cdecl GetGPSData2( CCamera PCamera, LONGREAL *Lat, LONGREAL *Lon, LONGREAL *MSL, LONGREAL *GeoidSep, INTEGER *Year, INTEGER *Month, INTEGER *Day, INTEGER *Hour, INTEGER *Minute, LONGREAL *Second, CARDINAL *Satellites, BOOLEAN *Fix );

extern "C" EXPORT_ BOOLEAN  __cdecl MoveTelescope( CCamera *PCamera, INT16 RADurationMs, INT16 DecDurationMs );
extern "C" EXPORT_ BOOLEAN  __cdecl MoveInProgress( CCamera *PCamera, BOOLEAN *Moving );

extern "C" EXPORT_ void     __cdecl GetLastErrorString( CCamera *PCamera, CARDINAL ErrorString_HIGH, CHAR *ErrorString );

}; // namespace gXusb
