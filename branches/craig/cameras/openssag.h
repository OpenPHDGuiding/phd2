#ifndef __OPEN_SSAG_H__ 
#define __OPEN_SSAG_H__ 

#define VENDOR_ID 0x1856 /* Orion Telescopes VID */
#define PRODUCT_ID 0x0012 /* SSAG IO PID */

// #define LOADER_VENDOR_ID 0x04b4 /* Default Cypress VID */
// #define LOADER_PRODUCT_ID 0x8613 /* Default Cypress PID */
#define LOADER_VENDOR_ID 0x1856 /* Orion Telescopes VID */
#define LOADER_PRODUCT_ID 0x0011 /* Loader PID for loading firmware */

typedef struct usb_dev_handle usb_dev_handle;

#ifdef __cplusplus
namespace OpenSSAG
{
#endif
    /* Struct used to return image data */
    struct raw_image {
        unsigned int width;
        unsigned int height;
        unsigned char *data;
    };

    /* Guide Directions (cardinal directions) */
    enum guide_direction {
        guide_east  = 0x10,
        guide_south = 0x20,
        guide_north = 0x40,
        guide_west  = 0x80,
    };
#ifdef __cplusplus
    class SSAG
    {
    private:
        /* Sends init packet and pre expose request */
        void InitSequence();

        /* Gets the data from the autoguider's internal buffer */
        unsigned char *ReadBuffer(int timeout);

        /* Holds the converted gain */
        unsigned int gain;

        /* Handle to the device */
        usb_dev_handle *handle;
    public:
        /* Constructor */
        SSAG();

        /* Connect to the autoguider. If bootload is set to true and the camera
         * cannot be found, it will attempt to connect to the base device and
         * load the firmware. Defaults to true. */
        bool Connect(bool bootload);
        bool Connect();

        /* Disconnect from the autoguider */
        void Disconnect();

        /* Returns true if the device is currently connected. */
        bool IsConnected();

        /* Gain should be a value between 1 and 15 */
        void SetGain(int gain);

        /* Expose and return the image in raw gray format. Function is blocking. */
        struct raw_image *Expose(int duration);

        /* Cancels an exposure */
        void CancelExposure();

        /* Issue a guide command through the guider relays. Guide directions
         * can be OR'd together to move in X and Y at the same time.
         *
         * EX. Guide(guide_north | guide_west, 100, 200); */
        void Guide(int direction, int yduration, int xduration);
        void Guide(int direction, int duration);

        /* Frees a raw_image struct */
        void FreeRawImage(raw_image *image);

    };

    /* This class is used for loading the firmware onto the device after it is
     * plugged in.
     *
     * See Cypress EZUSB fx2 datasheet for more information
     * http://www.keil.com/dd/docs/datashts/cypress/fx2_trm.pdf */
    class Loader
    {
    private:
        /* Puts the device into reset mode by writing 0x01 to CPUCS */
        void EnterResetMode();

        /* Makes the device exit reset mode by writing 0x00 to CPUCS */
        void ExitResetMode();

        /* Sends firmware to the device */
        void Upload(unsigned char *data);

        /* Handle to the cypress device */
        usb_dev_handle *handle;
    public:
        /* Connects to SSAG Base */
        bool Connect();

        /* Disconnects from SSAG Base */
        void Disconnect();

        /* Loads the firmware into SSAG's RAM */
        void LoadFirmware();

        /* Loads the SSAG eeprom onto the camera. You shouldn't use this if you
         * don't know what you're doing.
         * See http://www.cypress.com/?id=4&rID=34127 for more information. */
        void LoadEEPROM();
    };
}
#endif // __cplusplus
 
#endif /* __OPEN_SSAG_H__ */ 
