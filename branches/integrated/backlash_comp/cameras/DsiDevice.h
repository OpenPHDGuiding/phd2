class DsiDevice
{
public:
    DsiDevice();
    ~DsiDevice();
    bool Open(unsigned int DevNum);
    bool Initialize(unsigned int exposure = 1000, unsigned int gain = 63, unsigned int offset = 255, bool fast=true, unsigned int dualexposurethreshold = 0);
    void Close();
    static unsigned int EnumDsiDevices();

    bool GetRawImage(unsigned short **evenBuffer, unsigned short **oddBuffer);
    bool GetImage(unsigned short *ImageData, bool Async = false);
    void AbortImage();
    bool ImageReady;

    void SetExposureTime(unsigned int value); // in ms
    void SetFastReadoutSpeed(bool value);
    void SetGain(unsigned int value);  // 0 - 63
    void SetHighGain(bool value);
    void SetOffset(unsigned int value); // 0 - 510
    void SetDualExposureThreshold(unsigned int value);
    void SetBinMode(unsigned int value); // 1=1x1, 2=2x2
    double GetTemperature(); // returns -99C on DSI I models

    unsigned int GetWidth();
    unsigned int GetHeight();
    unsigned int GetRawWidth();
    unsigned int GetRawHeight();
    unsigned int GetRawHeightEven();
    unsigned int GetRawHeightOdd();
    unsigned int GetOffsetX();
    unsigned int GetOffsetY();
    unsigned int GetBinMode();

    bool IsColor;
    bool IsDsiII;
    bool IsDsiIII;
    bool IsUSB2;
    bool IsBinCapable;
    bool AmpControl;
    int ModelNumber;
    char *ErrorMessage;
    char *ModelName;
    char *CcdName;
    unsigned short *HostPointer;

protected:
    enum DsiBusSpeed
    {
        Full,
        High
    };

    enum DsiVddMode
    {
        Automatic,
        AlwaysOn,
        AlwaysOff
    };

    enum DsiReadoutSpeed
    {
        Normal,
        Fast
    };

    enum DeviceCommand
    {
        CMD_ABORT = 2,
        CMD_AD_READ = 0x68,
        CMD_AD_WRITE = 0x69,
        CMD_CCD_VDD_OFF = 0x67,
        CMD_CCD_VDD_ON = 0x66,
        CMD_CLEAR_TS = 4,
        CMD_ERASE_EEPROM = 110,
        CMD_GET_CLEAN_MODE = 0x3e,
        CMD_GET_DEBUG_VALUE = 0x6b,
        CMD_GET_EEPROM_BYTE = 0x1f,
        CMD_GET_EEPROM_LENGTH = 30,
        CMD_GET_EEPROM_VIDPID = 0x6c,
        CMD_GET_EXP_MODE = 0x38,
        CMD_GET_EXP_TIME = 0x36,
        CMD_GET_EXP_TIMER_COUNT = 0x4b,
        CMD_GET_FLUSH_MODE = 60,
        CMD_GET_GAIN = 50,
        CMD_GET_NORM_READOUT_DELAY = 0x44,
        CMD_GET_OFFSET = 0x34,
        CMD_GET_READOUT_MODE = 0x42,
        CMD_GET_READOUT_SPD = 0x40,
        CMD_GET_ROW_COUNT_EVEN = 0x48,
        CMD_GET_ROW_COUNT_ODD = 70,
        CMD_GET_STATUS = 0x15,
        CMD_GET_TEMP = 0x4a,
        CMD_GET_TIMESTAMP = 0x16,
        CMD_GET_VDD_MODE = 0x3a,
        CMD_GET_VERSION = 20,
        CMD_PING = 0,
        CMD_PS_OFF = 0x65,
        CMD_PS_ON = 100,
        CMD_RESET = 1,
        CMD_SET_CLEAN_MODE = 0x3f,
        CMD_SET_EEPROM_BYTE = 0x20,
        CMD_SET_EEPROM_VIDPID = 0x6d,
        CMD_SET_EXP_MODE = 0x39,
        CMD_SET_EXP_TIME = 0x37,
        CMD_SET_FLUSH_MODE = 0x3d,
        CMD_SET_GAIN = 0x33,
        CMD_SET_NORM_READOUT_DELAY = 0x45,
        CMD_SET_OFFSET = 0x35,
        CMD_SET_READOUT_MODE = 0x43,
        CMD_SET_READOUT_SPD = 0x41,
        CMD_SET_ROW_COUNT_EVEN = 0x49,
        CMD_SET_ROW_COUNT_ODD = 0x47,
        CMD_SET_VDD_MODE = 0x3b,
        CMD_TEST_PATTERN = 0x6a,
        CMD_TRIGGER = 3
    };

    enum DsiReadoutMode
    {
        DualExposure,
        SingleExposure,
        OddFieldsOnly,
        EvenFieldsOnly
    };

    enum DsiFlushMode
    {
        Continuous,
        BeforeExposure,
        Never
    };

    enum DsiAdRegister
    {
        Configuration,
        MuxConfig,
        RedPga,
        GreenPga,
        BluePga,
        RedOffset,
        GreenOffset,
        BlueOffset
    };

/*  enum DsiExposureMode {
        Normal = 1,
        Bin2X2
    };
*/
    unsigned int Command(DsiDevice::DeviceCommand command, unsigned int setting);
    char *GetCCDNumber();
    void SetExposureParameters();
    void ResetExposureParameters();
    static void GetImageAsync(void *ImageBuffer);
    bool OpenDsi(unsigned int Devnum);
    void CloseDsi();
    bool BulkWrite(unsigned int ep, void *bytes, unsigned int size, unsigned int timeout);
    unsigned int BulkRead(unsigned int ep, void *bytes, unsigned int size, unsigned int timeout);

private:
    bool Closing;
    bool Exposing;
    bool Error;
    bool Connected;
    bool Interlaced;
    int RawHeight;
    int RawHeightEven;
    int RawHeightOdd;
    int RawWidth;
    int ImgHeight;
    int ImgOffsetX;
    int ImgOffsetY;
    int ImgWidth;

    unsigned int Gain[2];
    unsigned int Offset[2];
    unsigned int ExposureTime[2];
    bool FastReadoutMode[2];
    bool highGain;
    unsigned int DualExposureThreshold;
    unsigned int BinMode;

    unsigned short *imageBuffer;
    unsigned int evenMemorySize;
    unsigned int oddMemorySize;
    bool abortable;

    bool AbortRequested;
    DsiBusSpeed busSpeed;
    unsigned char commandId;
    unsigned char firmwareRevision;
    unsigned char firmwareVersion;
    unsigned char imagerFamily;
    unsigned char imagerModel;
    unsigned int eepromLength;

#ifdef __MACH__
    IOUSBInterfaceInterface220     **interface;
    IOUSBDeviceInterface        **dev;
#endif
#ifdef WIN32
    HANDLE Handle;
#endif
};
