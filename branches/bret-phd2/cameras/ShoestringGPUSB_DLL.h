// ShoestringGPUSB_DLL.h
// Copyright 2005 Shoestring Astronomy
// www.ShoestringAstronomy.com

// These are possible return values for the shutter status in GPUSB_RAMStatus and GPUSB_Status
// They are also the possible input values for the ram parameter of GPUSB_SetAll
const int GPUSB_RAM_ASSERTED = 2;
const int GPUSB_RAM_DEASSERTED = 1;

// These are possible return values for the shutter status in GPUSB_RAPStatus and GPUSB_Status
// They are also the possible input values for the rap parameter of GPUSB_SetAll
const int GPUSB_RAP_ASSERTED = 2;
const int GPUSB_RAP_DEASSERTED = 1;

// These are possible return values for the shutter status in GPUSB_DecMStatus and GPUSB_Status
// They are also the possible input values for the decm parameter of GPUSB_SetAll
const int GPUSB_DECM_ASSERTED = 2;
const int GPUSB_DECM_DEASSERTED = 1;

// These are possible return values for the shutter status in GPUSB_DecPStatus and GPUSB_Status
// They are also the possible input values for the decp parameter of GPUSB_SetAll
const int GPUSB_DECP_ASSERTED = 2;
const int GPUSB_DECP_DEASSERTED = 1;

// These are possible return values for the LED status in GPUSB_LEDStatus and GPUSB_Status
// They are also the possible input values for the led parameter of GPUSB_SetAll
const int GPUSB_LED_ON_RED = 4;
const int GPUSB_LED_ON_GREEN = 3;
const int GPUSB_LED_OFF_RED = 2;
const int GPUSB_LED_OFF_GREEN = 1;

__declspec(dllimport) bool __stdcall GPUSB_Open( void );            // Opens the GPUSB and prepares it for use
__declspec(dllimport) void __stdcall GPUSB_Close( void );           // Closes the GPUSB after use is complete
__declspec(dllimport) bool __stdcall GPUSB_Reset( void );           // Resets the state of the GPUSB to its power-up default
__declspec(dllimport) bool __stdcall GPUSB_RAMAssert( void );       // Asserts the RA- signal
__declspec(dllimport) bool __stdcall GPUSB_RAMDeassert( void ); // Deasserts the RA- signal
__declspec(dllimport) bool __stdcall GPUSB_RAPAssert( void );       // Asserts the RA+ signal
__declspec(dllimport) bool __stdcall GPUSB_RAPDeassert( void ); // Deasserts the RA+ signal
__declspec(dllimport) bool __stdcall GPUSB_DecMAssert( void );      // Asserts the Dec- signal
__declspec(dllimport) bool __stdcall GPUSB_DecMDeassert( void );    // Deasserts the Dec- signal
__declspec(dllimport) bool __stdcall GPUSB_DecPAssert( void );      // Asserts the Dec+ signal
__declspec(dllimport) bool __stdcall GPUSB_DecPDeassert( void );    // Deasserts the Dec+ signal
__declspec(dllimport) bool __stdcall GPUSB_AllDirDeassert( void );  // Deasserts all direction control signals
__declspec(dllimport) bool __stdcall GPUSB_LEDOn( void );           // Turns the front panel LED on
__declspec(dllimport) bool __stdcall GPUSB_LEDOff( void );          // Turns the front panel LED off
__declspec(dllimport) bool __stdcall GPUSB_LEDRed( void );          // Sets the current LED color to red
__declspec(dllimport) bool __stdcall GPUSB_LEDGreen( void );        // Sets the current LED color to green
__declspec(dllimport) bool __stdcall GPUSB_SetAll(int ram, int rap, int decm, int decp, int led);   // Allows RA-, RA+, Dec-, Dec+, and LED states to all be set simulataneously

__declspec(dllimport) bool __stdcall GPUSB_RAMStatus(int *status);  // Checks the status of the RA- signal
__declspec(dllimport) bool __stdcall GPUSB_RAPStatus(int *status);  // Checks the status of the RA+ signal
__declspec(dllimport) bool __stdcall GPUSB_DecMStatus(int *status); // Checks the status of the Dec- signal
__declspec(dllimport) bool __stdcall GPUSB_DecPStatus(int *status); // Checks the status of the Dec+ signal
__declspec(dllimport) bool __stdcall GPUSB_LEDStatus(int *status);  // Checks the status of the front panel LED
__declspec(dllimport) bool __stdcall GPUSB_Status(int *ram_status, int *rap_status, int *decm_status, int *decp_status, int *led_status );  // Checks the status of RA-, RA+, Dec-, Dec+, and LED simulataneously

