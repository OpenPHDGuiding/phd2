/*
 * mock_ascom_camera.h
 * PHD Guiding - Camera Module Tests
 *
 * Mock objects for ASCOM camera interfaces
 * Provides controllable behavior for COM automation and ASCOM camera drivers
 */

#ifndef MOCK_ASCOM_CAMERA_H
#define MOCK_ASCOM_CAMERA_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>

#ifdef _WIN32
#include <windows.h>
#include <oleauto.h>
#include <comdef.h>
#endif

#include <memory>
#include <vector>
#include <map>

// Forward declarations
class ASCOMCameraSimulator;

#ifdef _WIN32

// Mock IDispatch interface for ASCOM camera objects
class MockASCOMCameraDispatch {
public:
    // IUnknown methods
    MOCK_METHOD2(QueryInterface, HRESULT(REFIID riid, void** ppvObject));
    MOCK_METHOD0(AddRef, ULONG());
    MOCK_METHOD0(Release, ULONG());
    
    // IDispatch methods
    MOCK_METHOD1(GetTypeInfoCount, HRESULT(UINT* pctinfo));
    MOCK_METHOD3(GetTypeInfo, HRESULT(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo));
    MOCK_METHOD5(GetIDsOfNames, HRESULT(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId));
    MOCK_METHOD8(Invoke, HRESULT(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, 
                                DISPPARAMS* pDispParams, VARIANT* pVarResult, 
                                EXCEPINFO* pExcepInfo, UINT* puArgErr));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetProperty, void(const wxString& name, const VARIANT& value));
    MOCK_METHOD1(GetProperty, VARIANT(const wxString& name));
    
    static MockASCOMCameraDispatch* instance;
    static MockASCOMCameraDispatch* GetInstance();
    static void SetInstance(MockASCOMCameraDispatch* inst);
};

// Mock ASCOM camera interface
class MockASCOMCamera {
public:
    // Connection management
    MOCK_METHOD0(get_Connected, bool());
    MOCK_METHOD1(put_Connected, void(bool connected));
    MOCK_METHOD0(get_Name, wxString());
    MOCK_METHOD0(get_Description, wxString());
    MOCK_METHOD0(get_DriverInfo, wxString());
    MOCK_METHOD0(get_DriverVersion, wxString());
    
    // Camera capabilities
    MOCK_METHOD0(get_CanAbortExposure, bool());
    MOCK_METHOD0(get_CanAsymmetricBin, bool());
    MOCK_METHOD0(get_CanGetCoolerPower, bool());
    MOCK_METHOD0(get_CanPulseGuide, bool());
    MOCK_METHOD0(get_CanSetCCDTemperature, bool());
    MOCK_METHOD0(get_CanStopExposure, bool());
    MOCK_METHOD0(get_HasShutter, bool());
    
    // Camera properties
    MOCK_METHOD0(get_CameraXSize, int());
    MOCK_METHOD0(get_CameraYSize, int());
    MOCK_METHOD0(get_MaxBinX, int());
    MOCK_METHOD0(get_MaxBinY, int());
    MOCK_METHOD0(get_PixelSizeX, double());
    MOCK_METHOD0(get_PixelSizeY, double());
    MOCK_METHOD0(get_BinX, int());
    MOCK_METHOD1(put_BinX, void(int binX));
    MOCK_METHOD0(get_BinY, int());
    MOCK_METHOD1(put_BinY, void(int binY));
    
    // Subframe properties
    MOCK_METHOD0(get_StartX, int());
    MOCK_METHOD1(put_StartX, void(int startX));
    MOCK_METHOD0(get_StartY, int());
    MOCK_METHOD1(put_StartY, void(int startY));
    MOCK_METHOD0(get_NumX, int());
    MOCK_METHOD1(put_NumX, void(int numX));
    MOCK_METHOD0(get_NumY, int());
    MOCK_METHOD1(put_NumY, void(int numY));
    
    // Exposure control
    MOCK_METHOD2(StartExposure, void(double duration, bool light));
    MOCK_METHOD0(AbortExposure, void());
    MOCK_METHOD0(StopExposure, void());
    MOCK_METHOD0(get_CameraState, int());
    MOCK_METHOD0(get_ImageReady, bool());
    MOCK_METHOD0(get_ImageArray, VARIANT());
    MOCK_METHOD0(get_ImageArrayVariant, VARIANT());
    
    // Gain control
    MOCK_METHOD0(get_Gain, int());
    MOCK_METHOD1(put_Gain, void(int gain));
    MOCK_METHOD0(get_GainMin, int());
    MOCK_METHOD0(get_GainMax, int());
    MOCK_METHOD0(get_Gains, VARIANT());
    
    // Cooler control
    MOCK_METHOD0(get_CoolerOn, bool());
    MOCK_METHOD1(put_CoolerOn, void(bool coolerOn));
    MOCK_METHOD0(get_CoolerPower, double());
    MOCK_METHOD0(get_CCDTemperature, double());
    MOCK_METHOD0(get_SetCCDTemperature, double());
    MOCK_METHOD1(put_SetCCDTemperature, void(double temperature));
    
    // Pulse guiding
    MOCK_METHOD2(PulseGuide, void(int direction, int duration));
    MOCK_METHOD0(get_IsPulseGuiding, bool());
    
    // Configuration
    MOCK_METHOD0(SetupDialog, void());
    MOCK_METHOD0(get_SupportedActions, VARIANT());
    MOCK_METHOD2(Action, wxString(const wxString& actionName, const wxString& actionParameters));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetImageData, void(const wxSize& size, const std::vector<unsigned short>& data));
    MOCK_METHOD1(SimulateExposure, void(bool success));
    MOCK_METHOD2(SimulatePulseGuide, void(int direction, bool success));
    
    static MockASCOMCamera* instance;
    static MockASCOMCamera* GetInstance();
    static void SetInstance(MockASCOMCamera* inst);
};

// Mock ASCOM camera chooser for device selection
class MockASCOMCameraChooser {
public:
    // Device selection
    MOCK_METHOD1(Choose, wxString(const wxString& progID));
    MOCK_METHOD0(GetProfiles, wxArrayString());
    MOCK_METHOD1(GetProfile, wxString(const wxString& progID));
    MOCK_METHOD2(SetProfile, void(const wxString& progID, const wxString& profile));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetAvailableDevices, void(const wxArrayString& devices));
    MOCK_METHOD1(SetSelectedDevice, void(const wxString& device));
    
    static MockASCOMCameraChooser* instance;
    static MockASCOMCameraChooser* GetInstance();
    static void SetInstance(MockASCOMCameraChooser* inst);
};

#endif // _WIN32

// ASCOM camera simulator for comprehensive testing
class ASCOMCameraSimulator {
public:
    struct CameraInfo {
        wxString progID;
        wxString name;
        wxString description;
        wxString driverVersion;
        bool isConnected;
        bool canAbortExposure;
        bool canAsymmetricBin;
        bool canGetCoolerPower;
        bool canPulseGuide;
        bool canSetCCDTemperature;
        bool canStopExposure;
        bool hasShutter;
        int cameraXSize, cameraYSize;
        int maxBinX, maxBinY;
        double pixelSizeX, pixelSizeY;
        int binX, binY;
        int startX, startY, numX, numY;
        int gain, gainMin, gainMax;
        bool coolerOn;
        double coolerPower;
        double ccdTemperature;
        double setCCDTemperature;
        bool shouldFail;
        wxString lastError;
        
        CameraInfo() : progID("Simulator.Camera"), name("ASCOM Camera Simulator"),
                      description("Simulated ASCOM Camera"), driverVersion("1.0"),
                      isConnected(false), canAbortExposure(true), canAsymmetricBin(false),
                      canGetCoolerPower(true), canPulseGuide(true), canSetCCDTemperature(true),
                      canStopExposure(true), hasShutter(false), cameraXSize(1280), cameraYSize(1024),
                      maxBinX(4), maxBinY(4), pixelSizeX(5.2), pixelSizeY(5.2),
                      binX(1), binY(1), startX(0), startY(0), numX(1280), numY(1024),
                      gain(50), gainMin(0), gainMax(100), coolerOn(false), coolerPower(0.0),
                      ccdTemperature(20.0), setCCDTemperature(-10.0), shouldFail(false), lastError("") {}
    };
    
    struct ExposureInfo {
        bool isExposing;
        bool isPulseGuiding;
        double exposureDuration;
        bool lightFrame;
        int cameraState; // 0=Idle, 1=Waiting, 2=Exposing, 3=Reading, 4=Download, 5=Error
        bool imageReady;
        wxDateTime exposureStartTime;
        std::vector<unsigned short> imageData;
        bool shouldFail;
        
        ExposureInfo() : isExposing(false), isPulseGuiding(false), exposureDuration(0.0),
                        lightFrame(true), cameraState(0), imageReady(false), shouldFail(false) {}
    };
    
    struct ChooserInfo {
        wxArrayString availableDevices;
        wxString selectedDevice;
        bool shouldFail;
        
        ChooserInfo() : shouldFail(false) {
            availableDevices.Add("Simulator.Camera");
            availableDevices.Add("ASCOM.Simulator.Camera");
            selectedDevice = "Simulator.Camera";
        }
    };
    
    // Component management
    void SetupCamera(const CameraInfo& info);
    void SetupExposure(const ExposureInfo& info);
    void SetupChooser(const ChooserInfo& info);
    
    // State management
    CameraInfo GetCameraInfo() const;
    ExposureInfo GetExposureInfo() const;
    ChooserInfo GetChooserInfo() const;
    
    // Connection simulation
    bool ConnectCamera();
    bool DisconnectCamera();
    bool IsConnected() const;
    
    // Exposure simulation
    bool StartExposure(double duration, bool light);
    bool IsExposing() const;
    void UpdateExposure(double deltaTime);
    bool CompleteExposure();
    bool AbortExposure();
    
    // Pulse guiding simulation
    bool StartPulseGuide(int direction, int duration);
    bool IsPulseGuiding() const;
    void CompletePulseGuide();
    
    // Device selection simulation
    wxString ChooseDevice(const wxString& deviceType);
    wxArrayString GetAvailableDevices() const;
    
    // Error simulation
    void SetCameraError(bool error);
    void SetExposureError(bool error);
    void SetChooserError(bool error);
    void SetConnectionError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultCamera();
    
    // COM simulation helpers
#ifdef _WIN32
    HRESULT SimulateGetProperty(const wxString& propertyName, VARIANT* result);
    HRESULT SimulateSetProperty(const wxString& propertyName, const VARIANT& value);
    HRESULT SimulateMethodCall(const wxString& methodName, const VARIANT* params, int paramCount, VARIANT* result);
#endif
    
private:
    CameraInfo cameraInfo;
    ExposureInfo exposureInfo;
    ChooserInfo chooserInfo;
    
    void InitializeDefaults();
    void GenerateImageData();
};

// Helper class to manage all ASCOM camera mocks
class MockASCOMCameraManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
#ifdef _WIN32
    static MockASCOMCameraDispatch* GetMockDispatch();
    static MockASCOMCamera* GetMockCamera();
    static MockASCOMCameraChooser* GetMockChooser();
#endif
    static ASCOMCameraSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedCamera();
    static void SetupCameraCapabilities();
    static void SetupDeviceChooser();
    static void SimulateASCOMFailure();
    static void SimulateConnectionFailure();
    
private:
#ifdef _WIN32
    static MockASCOMCameraDispatch* mockDispatch;
    static MockASCOMCamera* mockCamera;
    static MockASCOMCameraChooser* mockChooser;
#endif
    static std::unique_ptr<ASCOMCameraSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_ASCOM_CAMERA_MOCKS() MockASCOMCameraManager::SetupMocks()
#define TEARDOWN_ASCOM_CAMERA_MOCKS() MockASCOMCameraManager::TeardownMocks()
#define RESET_ASCOM_CAMERA_MOCKS() MockASCOMCameraManager::ResetMocks()

#ifdef _WIN32
#define GET_MOCK_ASCOM_CAMERA_DISPATCH() MockASCOMCameraManager::GetMockDispatch()
#define GET_MOCK_ASCOM_CAMERA() MockASCOMCameraManager::GetMockCamera()
#define GET_MOCK_ASCOM_CAMERA_CHOOSER() MockASCOMCameraManager::GetMockChooser()
#endif
#define GET_ASCOM_CAMERA_SIMULATOR() MockASCOMCameraManager::GetSimulator()

// Helper macros for common expectations
#ifdef _WIN32
#define EXPECT_ASCOM_CAMERA_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_ASCOM_CAMERA(), put_Connected(true)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_CAMERA_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_ASCOM_CAMERA(), put_Connected(false)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_CAMERA_START_EXPOSURE(duration, light) \
    EXPECT_CALL(*GET_MOCK_ASCOM_CAMERA(), StartExposure(duration, light)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_CAMERA_PULSE_GUIDE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_ASCOM_CAMERA(), PulseGuide(direction, duration)) \
        .WillOnce(::testing::Return())

#define EXPECT_ASCOM_CAMERA_GET_SIZE(width, height) \
    EXPECT_CALL(*GET_MOCK_ASCOM_CAMERA(), get_CameraXSize()) \
        .WillOnce(::testing::Return(width)); \
    EXPECT_CALL(*GET_MOCK_ASCOM_CAMERA(), get_CameraYSize()) \
        .WillOnce(::testing::Return(height))

#define EXPECT_ASCOM_CAMERA_CHOOSER_SUCCESS(device) \
    EXPECT_CALL(*GET_MOCK_ASCOM_CAMERA_CHOOSER(), Choose(::testing::_)) \
        .WillOnce(::testing::Return(device))
#endif

#endif // MOCK_ASCOM_CAMERA_H
