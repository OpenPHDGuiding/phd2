/*
 * mock_usb_camera.h
 * PHD Guiding - Camera Module Tests
 *
 * Mock objects for USB camera interfaces (ZWO, QHY, etc.)
 * Provides controllable behavior for USB camera SDK operations
 */

#ifndef MOCK_USB_CAMERA_H
#define MOCK_USB_CAMERA_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class USBCameraSimulator;

// Mock USB camera interface (generic for ZWO, QHY, etc.)
class MockUSBCamera {
public:
    // Camera enumeration and connection
    MOCK_METHOD0(GetNumOfConnectedCameras, int());
    MOCK_METHOD2(GetCameraInfo, bool(int cameraId, void* info));
    MOCK_METHOD1(OpenCamera, bool(int cameraId));
    MOCK_METHOD1(CloseCamera, bool(int cameraId));
    MOCK_METHOD1(InitCamera, bool(int cameraId));
    MOCK_METHOD1(IsConnected, bool(int cameraId));
    
    // Camera properties and capabilities
    MOCK_METHOD2(GetCameraProperty, bool(int cameraId, void* property));
    MOCK_METHOD3(GetControlCaps, bool(int cameraId, int controlType, void* caps));
    MOCK_METHOD4(GetControlValue, bool(int cameraId, int controlType, long* value, bool* isAuto));
    MOCK_METHOD4(SetControlValue, bool(int cameraId, int controlType, long value, bool isAuto));
    
    // Image capture and format
    MOCK_METHOD5(SetROIFormat, bool(int cameraId, int width, int height, int binning, int imageType));
    MOCK_METHOD3(SetStartPos, bool(int cameraId, int startX, int startY));
    MOCK_METHOD3(StartExposure, bool(int cameraId, long exposureTime, bool isDark));
    MOCK_METHOD1(StopExposure, bool(int cameraId));
    MOCK_METHOD1(GetExposureStatus, int(int cameraId));
    MOCK_METHOD3(GetImageData, bool(int cameraId, unsigned char* buffer, long bufferSize));
    
    // Video mode (for some cameras)
    MOCK_METHOD2(StartVideoCapture, bool(int cameraId, int captureMode));
    MOCK_METHOD1(StopVideoCapture, bool(int cameraId));
    MOCK_METHOD3(GetVideoData, bool(int cameraId, unsigned char* buffer, long bufferSize));
    
    // Temperature control (for cooled cameras)
    MOCK_METHOD2(SetCoolerOn, bool(int cameraId, bool on));
    MOCK_METHOD2(SetTargetTemp, bool(int cameraId, long temperature));
    MOCK_METHOD2(GetCoolerStatus, bool(int cameraId, bool* isOn));
    MOCK_METHOD2(GetCoolerPower, bool(int cameraId, int* power));
    MOCK_METHOD2(GetTemperature, bool(int cameraId, long* temperature));
    
    // ST4 guiding (for cameras with ST4 port)
    MOCK_METHOD3(PulseGuideOn, bool(int cameraId, int direction, long duration));
    MOCK_METHOD2(PulseGuideOff, bool(int cameraId, int direction));
    MOCK_METHOD1(IsPulseGuiding, bool(int cameraId));
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(GetSDKVersion, wxString());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetCameraCount, void(int count));
    MOCK_METHOD3(SetCameraInfo, void(int cameraId, const wxString& name, const wxString& model));
    MOCK_METHOD3(SetImageData, void(int cameraId, const wxSize& size, const std::vector<unsigned short>& data));
    MOCK_METHOD2(SimulateExposure, void(int cameraId, bool success));
    MOCK_METHOD3(SimulatePulseGuide, void(int cameraId, int direction, bool success));
    
    static MockUSBCamera* instance;
    static MockUSBCamera* GetInstance();
    static void SetInstance(MockUSBCamera* inst);
};

// Mock ZWO ASI camera interface
class MockZWOCamera {
public:
    // ZWO-specific methods
    MOCK_METHOD0(ASIGetNumOfConnectedCameras, int());
    MOCK_METHOD2(ASIGetCameraProperty, int(void* info, int cameraId));
    MOCK_METHOD1(ASIOpenCamera, int(int cameraId));
    MOCK_METHOD1(ASIInitCamera, int(int cameraId));
    MOCK_METHOD1(ASICloseCamera, int(int cameraId));
    MOCK_METHOD3(ASIGetNumOfControls, int(int cameraId, int* numControls));
    MOCK_METHOD3(ASIGetControlCaps, int(int cameraId, int controlIndex, void* caps));
    MOCK_METHOD4(ASIGetControlValue, int(int cameraId, int controlType, long* value, int* isAuto));
    MOCK_METHOD4(ASISetControlValue, int(int cameraId, int controlType, long value, int isAuto));
    MOCK_METHOD5(ASISetROIFormat, int(int cameraId, int width, int height, int binning, int imageType));
    MOCK_METHOD3(ASISetStartPos, int(int cameraId, int startX, int startY));
    MOCK_METHOD3(ASIStartExposure, int(int cameraId, int isDark));
    MOCK_METHOD1(ASIStopExposure, int(int cameraId));
    MOCK_METHOD2(ASIGetExpStatus, int(int cameraId, int* status));
    MOCK_METHOD3(ASIGetDataAfterExp, int(int cameraId, unsigned char* buffer, long bufferSize));
    MOCK_METHOD3(ASIPulseGuideOn, int(int cameraId, int direction));
    MOCK_METHOD2(ASIPulseGuideOff, int(int cameraId, int direction));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetCameraCount, void(int count));
    
    static MockZWOCamera* instance;
    static MockZWOCamera* GetInstance();
    static void SetInstance(MockZWOCamera* inst);
};

// Mock QHY camera interface
class MockQHYCamera {
public:
    // QHY-specific methods
    MOCK_METHOD0(InitQHYCCDResource, int());
    MOCK_METHOD0(ReleaseQHYCCDResource, int());
    MOCK_METHOD0(ScanQHYCCD, int());
    MOCK_METHOD2(GetQHYCCDId, int(int index, char* id));
    MOCK_METHOD1(OpenQHYCCD, void*(char* id));
    MOCK_METHOD1(CloseQHYCCD, int(void* handle));
    MOCK_METHOD1(InitQHYCCD, int(void* handle));
    MOCK_METHOD2(IsQHYCCDControlAvailable, int(void* handle, int controlId));
    MOCK_METHOD4(SetQHYCCDParam, int(void* handle, int controlId, double value));
    MOCK_METHOD3(GetQHYCCDParam, double(void* handle, int controlId));
    MOCK_METHOD6(SetQHYCCDResolution, int(void* handle, int startX, int startY, int sizeX, int sizeY));
    MOCK_METHOD2(SetQHYCCDBinMode, int(void* handle, int binMode));
    MOCK_METHOD2(SetQHYCCDBitsMode, int(void* handle, int bitsMode));
    MOCK_METHOD2(ExpQHYCCDSingleFrame, int(void* handle));
    MOCK_METHOD2(GetQHYCCDSingleFrame, int(void* handle, int* w, int* h, int* bpp, int* channels, unsigned char* imgdata));
    MOCK_METHOD1(CancelQHYCCDExposingAndReadout, int(void* handle));
    MOCK_METHOD3(ControlQHYCCDTemp, int(void* handle, double targetTemp));
    MOCK_METHOD2(GetQHYCCDTemp, int(void* handle, double* currentTemp));
    MOCK_METHOD4(ControlQHYCCDGuide, int(void* handle, int direction, int duration));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetCameraCount, void(int count));
    
    static MockQHYCamera* instance;
    static MockQHYCamera* GetInstance();
    static void SetInstance(MockQHYCamera* inst);
};

// USB camera simulator for comprehensive testing
class USBCameraSimulator {
public:
    enum CameraType {
        CAMERA_ZWO = 0,
        CAMERA_QHY = 1,
        CAMERA_SBIG = 2,
        CAMERA_GENERIC = 3
    };
    
    enum ExposureStatus {
        EXPOSURE_IDLE = 0,
        EXPOSURE_WORKING = 1,
        EXPOSURE_SUCCESS = 2,
        EXPOSURE_FAILED = 3
    };
    
    struct CameraInfo {
        int cameraId;
        CameraType type;
        wxString name;
        wxString model;
        int maxWidth, maxHeight;
        bool isColorCamera;
        int bayerPattern;
        double pixelSize;
        bool hasCooler;
        bool hasShutter;
        bool hasGuidePort;
        bool isUSB3;
        int maxBinning;
        wxArrayString supportedBins;
        wxArrayString supportedFormats;
        bool isConnected;
        bool shouldFail;
        
        CameraInfo() : cameraId(0), type(CAMERA_ZWO), name("USB Camera"), model("Generic"),
                      maxWidth(1280), maxHeight(1024), isColorCamera(false), bayerPattern(0),
                      pixelSize(5.2), hasCooler(false), hasShutter(false), hasGuidePort(false),
                      isUSB3(false), maxBinning(4), isConnected(false), shouldFail(false) {}
    };
    
    struct ExposureInfo {
        bool isExposing;
        bool isPulseGuiding;
        ExposureStatus status;
        long exposureDuration;
        bool isDark;
        int width, height, binning, imageType;
        int startX, startY;
        wxDateTime exposureStartTime;
        std::vector<unsigned short> imageData;
        bool shouldFail;
        
        ExposureInfo() : isExposing(false), isPulseGuiding(false), status(EXPOSURE_IDLE),
                        exposureDuration(1000), isDark(false), width(1280), height(1024),
                        binning(1), imageType(1), startX(0), startY(0), shouldFail(false) {}
    };
    
    struct ControlInfo {
        std::map<int, long> controlValues;
        std::map<int, bool> controlAuto;
        std::map<int, long> controlMin;
        std::map<int, long> controlMax;
        std::map<int, long> controlDefault;
        bool coolerOn;
        long targetTemperature;
        long currentTemperature;
        int coolerPower;
        
        ControlInfo() : coolerOn(false), targetTemperature(-10), currentTemperature(20), coolerPower(0) {
            // Initialize default control values
            controlValues[0] = 50;    // Gain
            controlValues[1] = 1000;  // Exposure
            controlValues[2] = 10;    // Offset
            controlMin[0] = 0;        // Gain min
            controlMax[0] = 100;      // Gain max
            controlDefault[0] = 50;   // Gain default
        }
    };
    
    // Component management
    void SetupCamera(int cameraId, const CameraInfo& info);
    void SetupExposure(int cameraId, const ExposureInfo& info);
    void SetupControls(int cameraId, const ControlInfo& info);
    
    // State management
    CameraInfo GetCameraInfo(int cameraId) const;
    ExposureInfo GetExposureInfo(int cameraId) const;
    ControlInfo GetControlInfo(int cameraId) const;
    
    // Camera enumeration and connection
    int GetNumOfConnectedCameras() const;
    bool GetCameraInfo(int cameraId, void* info) const;
    bool OpenCamera(int cameraId);
    bool CloseCamera(int cameraId);
    bool InitCamera(int cameraId);
    bool IsConnected(int cameraId) const;
    
    // Exposure simulation
    bool StartExposure(int cameraId, long exposureTime, bool isDark);
    bool StopExposure(int cameraId);
    ExposureStatus GetExposureStatus(int cameraId) const;
    void UpdateExposure(int cameraId, double deltaTime);
    bool GetImageData(int cameraId, unsigned char* buffer, long bufferSize);
    
    // Control simulation
    bool SetControlValue(int cameraId, int controlType, long value, bool isAuto);
    bool GetControlValue(int cameraId, int controlType, long* value, bool* isAuto) const;
    bool SetROIFormat(int cameraId, int width, int height, int binning, int imageType);
    bool SetStartPos(int cameraId, int startX, int startY);
    
    // Temperature control simulation
    bool SetCoolerOn(int cameraId, bool on);
    bool SetTargetTemp(int cameraId, long temperature);
    void UpdateTemperature(int cameraId, double deltaTime);
    
    // Pulse guiding simulation
    bool StartPulseGuide(int cameraId, int direction, long duration);
    bool StopPulseGuide(int cameraId, int direction);
    bool IsPulseGuiding(int cameraId) const;
    void UpdatePulseGuide(int cameraId, double deltaTime);
    
    // Error simulation
    void SetCameraError(int cameraId, bool error);
    void SetExposureError(int cameraId, bool error);
    void SetConnectionError(int cameraId, bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultCameras();
    void AddCamera(const CameraInfo& info);
    void RemoveCamera(int cameraId);
    
    // Image generation
    void GenerateTestImage(int cameraId);
    void GenerateNoiseImage(int cameraId, double mean, double stddev);
    void GenerateStarField(int cameraId, int numStars);
    
private:
    std::map<int, CameraInfo> cameras;
    std::map<int, ExposureInfo> exposures;
    std::map<int, ControlInfo> controls;
    
    // Pulse guiding state
    std::map<int, bool> pulseGuidingState;
    std::map<int, int> pulseDirection;
    std::map<int, long> pulseDuration;
    std::map<int, wxDateTime> pulseStartTime;
    
    void InitializeDefaults();
    void GenerateImageData(int cameraId, const ExposureInfo& exposure);
};

// Helper class to manage all USB camera mocks
class MockUSBCameraManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockUSBCamera* GetMockUSBCamera();
    static MockZWOCamera* GetMockZWOCamera();
    static MockQHYCamera* GetMockQHYCamera();
    static USBCameraSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedCamera();
    static void SetupMultipleCameras();
    static void SetupCooledCamera();
    static void SimulateUSBFailure();
    static void SimulateExposureFailure();
    
private:
    static MockUSBCamera* mockUSBCamera;
    static MockZWOCamera* mockZWOCamera;
    static MockQHYCamera* mockQHYCamera;
    static std::unique_ptr<USBCameraSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_USB_CAMERA_MOCKS() MockUSBCameraManager::SetupMocks()
#define TEARDOWN_USB_CAMERA_MOCKS() MockUSBCameraManager::TeardownMocks()
#define RESET_USB_CAMERA_MOCKS() MockUSBCameraManager::ResetMocks()

#define GET_MOCK_USB_CAMERA() MockUSBCameraManager::GetMockUSBCamera()
#define GET_MOCK_ZWO_CAMERA() MockUSBCameraManager::GetMockZWOCamera()
#define GET_MOCK_QHY_CAMERA() MockUSBCameraManager::GetMockQHYCamera()
#define GET_USB_CAMERA_SIMULATOR() MockUSBCameraManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_USB_CAMERA_ENUM_SUCCESS(count) \
    EXPECT_CALL(*GET_MOCK_USB_CAMERA(), GetNumOfConnectedCameras()) \
        .WillOnce(::testing::Return(count))

#define EXPECT_USB_CAMERA_CONNECT_SUCCESS(id) \
    EXPECT_CALL(*GET_MOCK_USB_CAMERA(), OpenCamera(id)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_USB_CAMERA_START_EXPOSURE(id, duration, isDark) \
    EXPECT_CALL(*GET_MOCK_USB_CAMERA(), StartExposure(id, duration, isDark)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_USB_CAMERA_PULSE_GUIDE(id, direction, duration) \
    EXPECT_CALL(*GET_MOCK_USB_CAMERA(), PulseGuideOn(id, direction, duration)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_USB_CAMERA_SET_CONTROL(id, controlType, value) \
    EXPECT_CALL(*GET_MOCK_USB_CAMERA(), SetControlValue(id, controlType, value, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_USB_CAMERA_SET_ROI(id, width, height, binning, imageType) \
    EXPECT_CALL(*GET_MOCK_USB_CAMERA(), SetROIFormat(id, width, height, binning, imageType)) \
        .WillOnce(::testing::Return(true))

#endif // MOCK_USB_CAMERA_H
