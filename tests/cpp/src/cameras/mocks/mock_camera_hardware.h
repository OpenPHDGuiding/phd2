/*
 * mock_camera_hardware.h
 * PHD Guiding - Camera Module Tests
 *
 * Mock objects for camera hardware interfaces
 * Provides controllable behavior for camera operations and image capture
 */

#ifndef MOCK_CAMERA_HARDWARE_H
#define MOCK_CAMERA_HARDWARE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class CameraHardwareSimulator;
class usImage;

// Mock camera hardware interface
class MockCameraHardware {
public:
    // Connection management
    MOCK_METHOD1(Connect, bool(const wxString& cameraId));
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(GetConnectionStatus, int());
    
    // Camera capabilities
    MOCK_METHOD0(HasNonGuiCapture, bool());
    MOCK_METHOD0(BitsPerPixel, wxByte());
    MOCK_METHOD0(HasSubframes, bool());
    MOCK_METHOD0(HasGainControl, bool());
    MOCK_METHOD0(HasShutter, bool());
    MOCK_METHOD0(HasCooler, bool());
    MOCK_METHOD0(CanSelectCamera, bool());
    
    // Image capture
    MOCK_METHOD4(Capture, bool(int duration, void* img, int options, const wxRect& subframe));
    MOCK_METHOD0(AbortExposure, bool());
    MOCK_METHOD0(IsCapturing, bool());
    
    // Camera properties
    MOCK_METHOD0(GetFrameSize, wxSize());
    MOCK_METHOD1(SetFrameSize, bool(const wxSize& size));
    MOCK_METHOD0(GetMaxFrameSize, wxSize());
    MOCK_METHOD0(GetBinning, int());
    MOCK_METHOD1(SetBinning, bool(int binning));
    MOCK_METHOD0(GetMaxBinning, int());
    
    // Gain and exposure control
    MOCK_METHOD0(GetGain, int());
    MOCK_METHOD1(SetGain, bool(int gain));
    MOCK_METHOD0(GetMinGain, int());
    MOCK_METHOD0(GetMaxGain, int());
    MOCK_METHOD0(GetDefaultGain, int());
    
    // Pixel size and calibration
    MOCK_METHOD0(GetPixelSize, double());
    MOCK_METHOD1(SetPixelSize, bool(double pixelSize));
    MOCK_METHOD1(GetDevicePixelSize, bool(double* pixelSize));
    
    // Cooler control
    MOCK_METHOD1(SetCoolerOn, bool(bool on));
    MOCK_METHOD1(SetCoolerSetpoint, bool(double temperature));
    MOCK_METHOD4(GetCoolerStatus, bool(bool* on, double* setpoint, double* power, double* temperature));
    MOCK_METHOD1(GetSensorTemperature, bool(double* temperature));
    
    // ST4 guiding interface
    MOCK_METHOD0(ST4HasGuideOutput, bool());
    MOCK_METHOD0(ST4HostConnected, bool());
    MOCK_METHOD0(ST4HasNonGuiMove, bool());
    MOCK_METHOD2(ST4PulseGuideScope, bool(int direction, int duration));
    
    // Camera enumeration
    MOCK_METHOD2(EnumCameras, bool(wxArrayString& names, wxArrayString& ids));
    MOCK_METHOD1(HandleSelectCameraButtonClick, bool(wxCommandEvent& evt));
    
    // Configuration and dialogs
    MOCK_METHOD0(ShowPropertyDialog, void());
    MOCK_METHOD0(GetSettingsSummary, wxString());
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(ClearError, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetFrameData, void(const wxSize& size, unsigned short* data));
    MOCK_METHOD1(SimulateCapture, void(bool success));
    MOCK_METHOD2(SimulateExposure, void(int duration, bool success));
    MOCK_METHOD1(SimulateTemperature, void(double temperature));
    
    static MockCameraHardware* instance;
    static MockCameraHardware* GetInstance();
    static void SetInstance(MockCameraHardware* inst);
};

// Mock image processing interface
class MockImageProcessor {
public:
    // Image operations
    MOCK_METHOD1(SubtractDark, void(usImage& img));
    MOCK_METHOD1(ApplyDefectMap, void(usImage& img));
    MOCK_METHOD1(ApplyFlat, void(usImage& img));
    MOCK_METHOD1(Debayer, void(usImage& img));
    
    // Dark frame management
    MOCK_METHOD1(AddDark, void(usImage* dark));
    MOCK_METHOD1(SelectDark, void(int exposureDuration));
    MOCK_METHOD0(ClearDarks, void());
    MOCK_METHOD3(GetDarkLibraryProperties, void(int* numDarks, double* minExp, double* maxExp));
    
    // Defect map management
    MOCK_METHOD1(SetDefectMap, void(void* defectMap));
    MOCK_METHOD0(ClearDefectMap, void());
    
    // Image statistics
    MOCK_METHOD1(CalculateStats, void(const usImage& img));
    MOCK_METHOD0(GetMean, double());
    MOCK_METHOD0(GetStdDev, double());
    MOCK_METHOD0(GetMin, unsigned short());
    MOCK_METHOD0(GetMax, unsigned short());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetImageStats, void(double mean, double stddev));
    
    static MockImageProcessor* instance;
    static MockImageProcessor* GetInstance();
    static void SetInstance(MockImageProcessor* inst);
};

// Mock camera configuration interface
class MockCameraConfig {
public:
    // Configuration management
    MOCK_METHOD1(LoadSettings, bool(const wxString& profile));
    MOCK_METHOD1(SaveSettings, bool(const wxString& profile));
    MOCK_METHOD0(GetCurrentProfile, wxString());
    MOCK_METHOD1(SetCurrentProfile, bool(const wxString& profile));
    
    // Parameter access
    MOCK_METHOD2(GetInt, int(const wxString& key, int defaultValue));
    MOCK_METHOD2(SetInt, void(const wxString& key, int value));
    MOCK_METHOD2(GetDouble, double(const wxString& key, double defaultValue));
    MOCK_METHOD2(SetDouble, void(const wxString& key, double value));
    MOCK_METHOD2(GetString, wxString(const wxString& key, const wxString& defaultValue));
    MOCK_METHOD2(SetString, void(const wxString& key, const wxString& value));
    MOCK_METHOD2(GetBool, bool(const wxString& key, bool defaultValue));
    MOCK_METHOD2(SetBool, void(const wxString& key, bool value));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD0(ResetToDefaults, void());
    
    static MockCameraConfig* instance;
    static MockCameraConfig* GetInstance();
    static void SetInstance(MockCameraConfig* inst);
};

// Camera hardware simulator for comprehensive testing
class CameraHardwareSimulator {
public:
    enum CameraType {
        CAMERA_SIMULATOR = 0,
        CAMERA_ASCOM = 1,
        CAMERA_INDI = 2,
        CAMERA_ZWO = 3,
        CAMERA_QHY = 4,
        CAMERA_SBIG = 5,
        CAMERA_OPENCV = 6,
        CAMERA_WEBCAM = 7
    };
    
    enum CaptureMode {
        MODE_NORMAL = 0,
        MODE_DARK = 1,
        MODE_BIAS = 2,
        MODE_FLAT = 3
    };
    
    struct CameraInfo {
        CameraType type;
        wxString name;
        wxString id;
        bool isConnected;
        bool hasNonGuiCapture;
        wxByte bitsPerPixel;
        bool hasSubframes;
        bool hasGainControl;
        bool hasShutter;
        bool hasCooler;
        bool canSelectCamera;
        wxSize frameSize;
        wxSize maxFrameSize;
        int binning;
        int maxBinning;
        int gain;
        int minGain;
        int maxGain;
        int defaultGain;
        double pixelSize;
        bool shutterClosed;
        bool coolerOn;
        double coolerSetpoint;
        double sensorTemperature;
        bool shouldFail;
        wxString lastError;
        
        CameraInfo() : type(CAMERA_SIMULATOR), name("Simulator"), id("SIM001"),
                      isConnected(false), hasNonGuiCapture(true), bitsPerPixel(16),
                      hasSubframes(true), hasGainControl(true), hasShutter(false),
                      hasCooler(false), canSelectCamera(false), frameSize(1280, 1024),
                      maxFrameSize(1280, 1024), binning(1), maxBinning(4),
                      gain(50), minGain(0), maxGain(100), defaultGain(50),
                      pixelSize(5.2), shutterClosed(false), coolerOn(false),
                      coolerSetpoint(-10.0), sensorTemperature(20.0), shouldFail(false),
                      lastError("") {}
    };
    
    struct CaptureInfo {
        bool isCapturing;
        int exposureDuration;
        CaptureMode mode;
        wxRect subframe;
        int captureOptions;
        wxDateTime captureStartTime;
        bool shouldFail;
        
        CaptureInfo() : isCapturing(false), exposureDuration(0), mode(MODE_NORMAL),
                       subframe(0, 0, 0, 0), captureOptions(0), shouldFail(false) {}
    };
    
    struct ImageInfo {
        wxSize size;
        std::vector<unsigned short> data;
        double mean;
        double stddev;
        unsigned short minValue;
        unsigned short maxValue;
        bool hasValidData;
        
        ImageInfo() : size(0, 0), mean(0.0), stddev(0.0), minValue(0), maxValue(0), hasValidData(false) {}
    };
    
    // Component management
    void SetupCamera(const CameraInfo& info);
    void SetupCapture(const CaptureInfo& info);
    void SetupImage(const ImageInfo& info);
    
    // State management
    CameraInfo GetCameraInfo() const;
    CaptureInfo GetCaptureInfo() const;
    ImageInfo GetImageInfo() const;
    
    // Connection simulation
    bool ConnectCamera(const wxString& cameraId);
    bool DisconnectCamera();
    bool IsConnected() const;
    
    // Capture simulation
    bool StartCapture(int duration, int options, const wxRect& subframe);
    bool IsCapturing() const;
    void UpdateCapture(double deltaTime);
    bool CompleteCapture(usImage& img);
    bool AbortCapture();
    
    // Property simulation
    bool SetGain(int gain);
    int GetGain() const;
    bool SetBinning(int binning);
    int GetBinning() const;
    bool SetCoolerOn(bool on);
    bool SetCoolerSetpoint(double temperature);
    void UpdateTemperature(double deltaTime);
    
    // ST4 guiding simulation
    bool StartPulseGuide(int direction, int duration);
    bool IsPulseGuiding() const;
    void UpdatePulseGuide(double deltaTime);
    
    // Image generation
    void GenerateTestImage(usImage& img, const wxRect& subframe);
    void GenerateNoiseImage(usImage& img, double mean, double stddev);
    void GenerateStarField(usImage& img, int numStars);
    void AddNoise(usImage& img, double noiseLevel);
    
    // Error simulation
    void SetCameraError(bool error);
    void SetCaptureError(bool error);
    void SetConnectionError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultCamera();
    
    // Camera enumeration simulation
    void AddAvailableCamera(const wxString& name, const wxString& id);
    wxArrayString GetAvailableCameraNames() const;
    wxArrayString GetAvailableCameraIds() const;
    void ClearAvailableCameras();
    
private:
    CameraInfo cameraInfo;
    CaptureInfo captureInfo;
    ImageInfo imageInfo;
    
    // Available cameras for enumeration
    wxArrayString availableCameraNames;
    wxArrayString availableCameraIds;
    
    // ST4 guiding state
    bool isPulseGuiding;
    int pulseDirection;
    int pulseDuration;
    wxDateTime pulseStartTime;
    
    void InitializeDefaults();
    void CalculateImageStats(const std::vector<unsigned short>& data, const wxSize& size);
    unsigned short GeneratePixelValue(int x, int y, const wxSize& size, double mean, double stddev);
};

// Helper class to manage all camera hardware mocks
class MockCameraHardwareManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockCameraHardware* GetMockHardware();
    static MockImageProcessor* GetMockProcessor();
    static MockCameraConfig* GetMockConfig();
    static CameraHardwareSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedCamera();
    static void SetupCameraWithCapabilities();
    static void SetupImageCapture();
    static void SimulateCameraFailure();
    static void SimulateCaptureFailure();
    
private:
    static MockCameraHardware* mockHardware;
    static MockImageProcessor* mockProcessor;
    static MockCameraConfig* mockConfig;
    static std::unique_ptr<CameraHardwareSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_CAMERA_HARDWARE_MOCKS() MockCameraHardwareManager::SetupMocks()
#define TEARDOWN_CAMERA_HARDWARE_MOCKS() MockCameraHardwareManager::TeardownMocks()
#define RESET_CAMERA_HARDWARE_MOCKS() MockCameraHardwareManager::ResetMocks()

#define GET_MOCK_CAMERA_HARDWARE() MockCameraHardwareManager::GetMockHardware()
#define GET_MOCK_IMAGE_PROCESSOR() MockCameraHardwareManager::GetMockProcessor()
#define GET_MOCK_CAMERA_CONFIG() MockCameraHardwareManager::GetMockConfig()
#define GET_CAMERA_SIMULATOR() MockCameraHardwareManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_CAMERA_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CAMERA_HARDWARE(), Connect(::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CAMERA_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CAMERA_HARDWARE(), Disconnect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CAMERA_CAPTURE_SUCCESS(duration, options) \
    EXPECT_CALL(*GET_MOCK_CAMERA_HARDWARE(), Capture(duration, ::testing::_, options, ::testing::_)) \
        .WillOnce(::testing::Return(false))

#define EXPECT_CAMERA_SET_GAIN_SUCCESS(gain) \
    EXPECT_CALL(*GET_MOCK_CAMERA_HARDWARE(), SetGain(gain)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CAMERA_GET_FRAME_SIZE(size) \
    EXPECT_CALL(*GET_MOCK_CAMERA_HARDWARE(), GetFrameSize()) \
        .WillOnce(::testing::Return(size))

#define EXPECT_ST4_PULSE_GUIDE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_CAMERA_HARDWARE(), ST4PulseGuideScope(direction, duration)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CAMERA_ENUM_SUCCESS(names, ids) \
    EXPECT_CALL(*GET_MOCK_CAMERA_HARDWARE(), EnumCameras(::testing::_, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(names), \
                                   ::testing::SetArgReferee<1>(ids), \
                                   ::testing::Return(false)))

#endif // MOCK_CAMERA_HARDWARE_H
