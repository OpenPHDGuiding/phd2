/*
 * mock_phd_core.h
 * PHD Guiding - Core Module Tests
 *
 * Mock objects for PHD2 core components
 * Provides controllable behavior for cameras, mounts, and guiding algorithms
 */

#ifndef MOCK_PHD_CORE_H
#define MOCK_PHD_CORE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/event.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class PHDCoreSimulator;

// Mock PHD Application
class MockPHDApp {
public:
    // Application lifecycle
    MOCK_METHOD0(OnInit, bool());
    MOCK_METHOD0(OnExit, int());
    MOCK_METHOD0(GetMainFrame, wxFrame*());
    
    // Equipment management
    MOCK_METHOD0(GetCamera, void*());
    MOCK_METHOD0(GetMount, void*());
    MOCK_METHOD0(GetStepGuider, void*());
    MOCK_METHOD0(GetRotator, void*());
    
    // Guiding state
    MOCK_METHOD0(GetState, int());
    MOCK_METHOD1(SetState, void(int state));
    MOCK_METHOD0(IsGuiding, bool());
    MOCK_METHOD0(IsCalibrating, bool());
    MOCK_METHOD0(IsLooping, bool());
    
    // Configuration
    MOCK_METHOD0(GetConfig, void*());
    MOCK_METHOD0(GetProfile, wxString());
    MOCK_METHOD1(SetProfile, bool(const wxString& profile));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateStateChange, void(int newState));
    
    static MockPHDApp* instance;
    static MockPHDApp* GetInstance();
    static void SetInstance(MockPHDApp* inst);
};

// Mock Camera interface
class MockCamera {
public:
    // Camera identification
    MOCK_METHOD0(Name, wxString());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(HasNonGuiCapture, bool());
    MOCK_METHOD0(HasShutter, bool());
    MOCK_METHOD0(HasGainControl, bool());
    MOCK_METHOD0(HasSubframes, bool());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    
    // Camera properties
    MOCK_METHOD0(FullSize, wxSize());
    MOCK_METHOD0(GetBinning, int());
    MOCK_METHOD1(SetBinning, bool(int binning));
    MOCK_METHOD0(GetGain, int());
    MOCK_METHOD1(SetGain, bool(int gain));
    
    // Image capture
    MOCK_METHOD2(Capture, bool(int duration, bool subframe));
    MOCK_METHOD0(GetImage, void*());
    MOCK_METHOD0(AbortCapture, void());
    
    // ST4 interface
    MOCK_METHOD0(ST4HasGuideOutput, bool());
    MOCK_METHOD0(ST4HostConnected, bool());
    MOCK_METHOD4(ST4PulseGuideScope, bool(int direction, int duration, bool async, bool* pulsePending));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateImage, void(void* image));
    
    static MockCamera* instance;
    static MockCamera* GetInstance();
    static void SetInstance(MockCamera* inst);
};

// Mock Mount interface
class MockMount {
public:
    // Mount identification
    MOCK_METHOD0(Name, wxString());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(IsCalibrated, bool());
    MOCK_METHOD0(IsStepGuider, bool());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    
    // Calibration
    MOCK_METHOD0(ClearCalibration, void());
    MOCK_METHOD0(GetCalibrationAngle, double());
    MOCK_METHOD1(SetCalibrationAngle, void(double angle));
    MOCK_METHOD0(GetCalibrationRate, double());
    MOCK_METHOD1(SetCalibrationRate, void(double rate));
    
    // Guiding operations
    MOCK_METHOD2(Guide, int(int direction, int duration));
    MOCK_METHOD1(CalibrationMoveSize, int(int direction));
    MOCK_METHOD1(MaxMoveSize, int(direction));
    
    // Mount state
    MOCK_METHOD0(GetGuidingEnabled, bool());
    MOCK_METHOD1(SetGuidingEnabled, void(bool enabled));
    MOCK_METHOD0(IsBusy, bool());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetCalibrated, void(bool calibrated));
    MOCK_METHOD2(SetMoveResult, void(int direction, int result));
    
    static MockMount* instance;
    static MockMount* GetInstance();
    static void SetInstance(MockMount* inst);
};

// Mock Guiding Algorithm
class MockGuidingAlgorithm {
public:
    // Algorithm identification
    MOCK_METHOD0(GetName, wxString());
    MOCK_METHOD0(GetConfigPath, wxString());
    
    // Algorithm parameters
    MOCK_METHOD0(GetMinMove, double());
    MOCK_METHOD1(SetMinMove, void(double minMove));
    MOCK_METHOD0(GetMaxMove, double());
    MOCK_METHOD1(SetMaxMove, void(double maxMove));
    MOCK_METHOD0(GetAggressiveness, double());
    MOCK_METHOD1(SetAggressiveness, void(double aggressiveness));
    
    // Guiding calculations
    MOCK_METHOD3(Result, double(double input, double guideDistance, double dt));
    MOCK_METHOD0(Reset, void());
    MOCK_METHOD0(GetHistory, std::vector<double>());
    
    // Configuration
    MOCK_METHOD0(GetConfigDialog, wxDialog*());
    MOCK_METHOD0(LoadSettings, void());
    MOCK_METHOD0(SaveSettings, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateResult, void(double result));
    
    static MockGuidingAlgorithm* instance;
    static MockGuidingAlgorithm* GetInstance();
    static void SetInstance(MockGuidingAlgorithm* inst);
};

// PHD core simulator for comprehensive testing
class PHDCoreSimulator {
public:
    enum AppState {
        STATE_STOPPED = 0,
        STATE_SELECTED = 1,
        STATE_CALIBRATING = 2,
        STATE_GUIDING = 3,
        STATE_LOOPING = 4,
        STATE_PAUSED = 5
    };
    
    struct CameraInfo {
        wxString name;
        bool isConnected;
        wxSize fullSize;
        int binning;
        int gain;
        bool hasShutter;
        bool hasGainControl;
        bool hasST4;
        bool shouldFail;
        
        CameraInfo() : name("Simulator"), isConnected(false), fullSize(1024, 768),
                      binning(1), gain(50), hasShutter(true), hasGainControl(true),
                      hasST4(true), shouldFail(false) {}
    };
    
    struct MountInfo {
        wxString name;
        bool isConnected;
        bool isCalibrated;
        bool isStepGuider;
        double calibrationAngle;
        double calibrationRate;
        bool guidingEnabled;
        bool isBusy;
        bool shouldFail;
        std::map<int, int> moveResults; // direction -> result
        
        MountInfo() : name("On-camera"), isConnected(false), isCalibrated(false),
                     isStepGuider(false), calibrationAngle(0.0), calibrationRate(1.0),
                     guidingEnabled(true), isBusy(false), shouldFail(false) {}
    };
    
    struct AppInfo {
        AppState state;
        wxString currentProfile;
        bool shouldFail;
        
        AppInfo() : state(STATE_STOPPED), currentProfile("Default"), shouldFail(false) {}
    };
    
    struct AlgorithmInfo {
        wxString name;
        double minMove;
        double maxMove;
        double aggressiveness;
        std::vector<double> history;
        bool shouldFail;
        
        AlgorithmInfo() : name("Hysteresis"), minMove(0.15), maxMove(5.0),
                         aggressiveness(100.0), shouldFail(false) {}
    };
    
    // Component management
    void SetupCamera(const CameraInfo& info);
    void SetupMount(const MountInfo& info);
    void SetupApp(const AppInfo& info);
    void SetupAlgorithm(const AlgorithmInfo& info);
    
    // State management
    CameraInfo GetCameraInfo() const;
    MountInfo GetMountInfo() const;
    AppInfo GetAppInfo() const;
    AlgorithmInfo GetAlgorithmInfo() const;
    
    // Equipment simulation
    bool ConnectCamera();
    bool DisconnectCamera();
    bool CaptureImage(int duration, bool subframe = false);
    bool ConnectMount();
    bool DisconnectMount();
    int GuideMount(int direction, int duration);
    
    // Guiding simulation
    bool StartCalibration();
    bool CompleteCalibration();
    bool StartGuiding();
    bool StopGuiding();
    bool PauseGuiding();
    bool ResumeGuiding();
    
    // Algorithm simulation
    double CalculateGuideCorrection(double error, double dt);
    void ResetAlgorithm();
    
    // State changes
    void SetAppState(AppState newState);
    void SetCameraConnected(bool connected);
    void SetMountConnected(bool connected);
    void SetMountCalibrated(bool calibrated);
    
    // Error simulation
    void SetCameraError(bool error);
    void SetMountError(bool error);
    void SetAppError(bool error);
    void SetAlgorithmError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultComponents();
    
private:
    CameraInfo cameraInfo;
    MountInfo mountInfo;
    AppInfo appInfo;
    AlgorithmInfo algorithmInfo;
    
    void InitializeDefaults();
    bool ValidateStateTransition(AppState fromState, AppState toState);
};

// Helper class to manage all PHD core mocks
class MockPHDCoreManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockPHDApp* GetMockApp();
    static MockCamera* GetMockCamera();
    static MockMount* GetMockMount();
    static MockGuidingAlgorithm* GetMockAlgorithm();
    static PHDCoreSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedEquipment();
    static void SetupCalibratedMount();
    static void SetupGuidingSession();
    static void SimulateEquipmentFailure();
    static void SimulateCalibrationFailure();
    
private:
    static MockPHDApp* mockApp;
    static MockCamera* mockCamera;
    static MockMount* mockMount;
    static MockGuidingAlgorithm* mockAlgorithm;
    static std::unique_ptr<PHDCoreSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_PHD_CORE_MOCKS() MockPHDCoreManager::SetupMocks()
#define TEARDOWN_PHD_CORE_MOCKS() MockPHDCoreManager::TeardownMocks()
#define RESET_PHD_CORE_MOCKS() MockPHDCoreManager::ResetMocks()

#define GET_MOCK_PHD_APP() MockPHDCoreManager::GetMockApp()
#define GET_MOCK_CAMERA() MockPHDCoreManager::GetMockCamera()
#define GET_MOCK_MOUNT() MockPHDCoreManager::GetMockMount()
#define GET_MOCK_ALGORITHM() MockPHDCoreManager::GetMockAlgorithm()
#define GET_PHD_CORE_SIMULATOR() MockPHDCoreManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_CAMERA_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CAMERA(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_MOUNT_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_MOUNT(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CAPTURE_SUCCESS(duration) \
    EXPECT_CALL(*GET_MOCK_CAMERA(), Capture(duration, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_GUIDE_SUCCESS(direction, duration) \
    EXPECT_CALL(*GET_MOCK_MOUNT(), Guide(direction, duration)) \
        .WillOnce(::testing::Return(0)) // 0 = success

#define EXPECT_STATE_CHANGE(newState) \
    EXPECT_CALL(*GET_MOCK_PHD_APP(), SetState(newState)) \
        .WillOnce(::testing::Return())

#define EXPECT_ALGORITHM_RESULT(input, result) \
    EXPECT_CALL(*GET_MOCK_ALGORITHM(), Result(input, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(result))

#endif // MOCK_PHD_CORE_H
