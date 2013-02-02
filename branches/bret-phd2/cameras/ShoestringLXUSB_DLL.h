// ShoestringLXUSB_DLL.h
// Copyright 2005 Shoestring Astronomy
// www.ShoestringAstronomy.com

// These are possible return values for the shutter status in LXUSB_Frame1Status and LXUSB_Status
// They are also the possible input values for the frame1 parameter of LXUSB_SetAll
const int LXUSB_FRAME1_ASSERTED = 2;
const int LXUSB_FRAME1_DEASSERTED = 1;

// These are possible return values for the shutter status in LXUSB_Frame2Status and LXUSB_Status
// They are also the possible input values for the frame2 parameter of LXUSB_SetAll
const int LXUSB_FRAME2_ASSERTED = 2;
const int LXUSB_FRAME2_DEASSERTED = 1;

// These are possible return values for the shutter status in LXUSB_ShutterStatus and LXUSB_Status
// They are also the possible input values for the shutter parameter of LXUSB_SetAll
const int LXUSB_SHUTTER_ASSERTED = 2;
const int LXUSB_SHUTTER_DEASSERTED = 1;

// These are possible return values for the shutter status in LXUSB_CCDAmpStatus and LXUSB_Status
// They are also the possible input values for the ccdamp parameter of LXUSB_SetAll
const int LXUSB_CCDAMP_ASSERTED = 2;
const int LXUSB_CCDAMP_DEASSERTED = 1;

// These are possible return values for the LED status in LXUSB_LEDStatus and LXUSB_Status
// They are also the possible input values for the led parameter of LXUSB_SetAll
const int LXUSB_LED_ON_RED = 4;
const int LXUSB_LED_ON_GREEN = 3;
const int LXUSB_LED_OFF_RED = 2;
const int LXUSB_LED_OFF_GREEN = 1;

__declspec(dllimport) bool __stdcall LXUSB_Open( void );            // Opens the LXUSB and prepares it for use
__declspec(dllimport) void __stdcall LXUSB_Close( void );           // Closes the LXUSB after use is complete
__declspec(dllimport) bool __stdcall LXUSB_Reset( void );           // Resets the state of the LXUSB to its power-up default
__declspec(dllimport) bool __stdcall LXUSB_Frame1Assert( void );        // Asserts the Frame 1 Transfer signal
__declspec(dllimport) bool __stdcall LXUSB_Frame1Deassert( void );  // Deasserts the Frame 1 Transfer signal
__declspec(dllimport) bool __stdcall LXUSB_Frame2Assert( void );        // Asserts the Frame 2 Transfer signal
__declspec(dllimport) bool __stdcall LXUSB_Frame2Deassert( void );  // Deasserts the Frame 2 Transfer signal
__declspec(dllimport) bool __stdcall LXUSB_ShutterAssert( void );       // Asserts the Shutter signal
__declspec(dllimport) bool __stdcall LXUSB_ShutterDeassert( void ); // Deasserts the Shutter signal
__declspec(dllimport) bool __stdcall LXUSB_CCDAmpAssert( void );        // Asserts the CCDAmp signal
__declspec(dllimport) bool __stdcall LXUSB_CCDAmpDeassert( void );  // Deasserts the CCDAmp signal
__declspec(dllimport) bool __stdcall LXUSB_AllControlDeassert( void );  // Deasserts all four control signals
__declspec(dllimport) bool __stdcall LXUSB_LEDOn( void );           // Turns the front panel LED on
__declspec(dllimport) bool __stdcall LXUSB_LEDOff( void );          // Turns the front panel LED off
__declspec(dllimport) bool __stdcall LXUSB_LEDRed( void );          // Sets the current LED color to red
__declspec(dllimport) bool __stdcall LXUSB_LEDGreen( void );        // Sets the current LED color to green
__declspec(dllimport) bool __stdcall LXUSB_SetAll(int frame1, int frame2, int shutter, int ccdamp, int led);    // Allows Frame 1 Transfer, Frame 2 Transfer, Shutter, CCDAmp, and LED states to all be set simulataneously

__declspec(dllimport) bool __stdcall LXUSB_Frame1Status(int *status);   // Checks the status of the Frame 1 Transfer signal
__declspec(dllimport) bool __stdcall LXUSB_Frame2Status(int *status);   // Checks the status of the Frame 2 Transfer signal
__declspec(dllimport) bool __stdcall LXUSB_ShutterStatus(int *status);  // Checks the status of the Shutter signal
__declspec(dllimport) bool __stdcall LXUSB_CCDAmpStatus(int *status);   // Checks the status of the CCDAmp signal
__declspec(dllimport) bool __stdcall LXUSB_LEDStatus(int *status);  // Checks the status of the front panel LED
__declspec(dllimport) bool __stdcall LXUSB_Status(int *frame1_status, int *frame2_status, int *shutter_status, int *ccdamp_status, int *led_status );   // Checks the status of Frame 1 Transfer, Frame 2 Transfer, Shutter, CCDAmp, and LED simulataneously

