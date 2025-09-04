/*
 * mock_guiding_hardware.h
 * PHD Guiding - Guiding Module Tests
 *
 * Mock objects for guiding hardware interfaces
 * Provides controllable behavior for guiding operations, star tracking, and mount communication
 */

#ifndef MOCK_GUIDING_HARDWARE_H
#define MOCK_GUIDING_HARDWARE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class GuidingHardwareSimulator;

// Mock guiding hardware interface
class MockGuidingHardware {
public:
    // Connection and state management
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(GetState, int()); // GUIDER_STATE
    MOCK_METHOD1(SetState, void(int state));
    
    // Star detection and tracking
    MOCK_METHOD0(IsLocked, bool());
    MOCK_METHOD1(SetLockPosition, bool(const wxPoint& position));
    MOCK_METHOD0(GetLockPosition, wxPoint());
    MOCK_METHOD0(GetCurrentPosition, wxPoint());
    MOCK_METHOD0(InvalidateCurrentPosition, void());
    MOCK_METHOD1(AutoSelect, bool(const wxRect& roi));
    
    // Image processing
    MOCK_METHOD1(UpdateCurrentPosition, bool(const void* image));
    MOCK_METHOD2(SetCurrentPosition, bool(const void* image, const wxPoint& position));
    MOCK_METHOD0(GetBoundingBox, wxRect());
    MOCK_METHOD0(GetMaxMovePixels, int());
    
    // Guiding operations
    MOCK_METHOD0(StartGuiding, bool());
    MOCK_METHOD0(StopGuiding, bool());
    MOCK_METHOD0(IsGuiding, bool());
    MOCK_METHOD0(IsPaused, bool());
    MOCK_METHOD1(SetPaused, bool(bool paused));
    
    // Calibration
    MOCK_METHOD0(IsCalibrating, bool());
    MOCK_METHOD0(BeginCalibration, bool());
    MOCK_METHOD0(CompleteCalibration, bool());
    MOCK_METHOD0(AbortCalibration, void());
    MOCK_METHOD0(ClearCalibration, void());
    
    // Multi-star support
    MOCK_METHOD0(GetMultiStarMode, bool());
    MOCK_METHOD1(SetMultiStarMode, void(bool enabled));
    MOCK_METHOD0(GetStarCount, int());
    MOCK_METHOD0(ClearSecondaryStars, void());
    
    // Configuration and settings
    MOCK_METHOD0(GetSettingsSummary, wxString());
    MOCK_METHOD0(ShowPropertyDialog, void());
    MOCK_METHOD0(LoadProfileSettings, void());
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(ClearError, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetStarPosition, void(int x, int y));
    MOCK_METHOD1(SimulateStarLoss, void(bool lost));
    MOCK_METHOD2(SimulateGuideStep, void(double raOffset, double decOffset));
    MOCK_METHOD1(SimulateCalibrationStep, void(bool success));
    
    static MockGuidingHardware* instance;
    static MockGuidingHardware* GetInstance();
    static void SetInstance(MockGuidingHardware* inst);
};

// Mock star detector interface
class MockStarDetector {
public:
    // Star detection
    MOCK_METHOD2(FindStar, bool(const void* image, wxPoint* position));
    MOCK_METHOD3(FindStars, bool(const void* image, std::vector<wxPoint>* positions, int maxStars));
    MOCK_METHOD3(GetStarQuality, bool(const void* image, const wxPoint& position, double* quality));
    MOCK_METHOD3(GetStarSNR, bool(const void* image, const wxPoint& position, double* snr));
    MOCK_METHOD3(GetStarHFD, bool(const void* image, const wxPoint& position, double* hfd));
    
    // Star tracking
    MOCK_METHOD4(TrackStar, bool(const void* image, const wxPoint& lastPosition, wxPoint* newPosition, double* quality));
    MOCK_METHOD2(IsStarLost, bool(const void* image, const wxPoint& position));
    MOCK_METHOD3(RefineStarPosition, bool(const void* image, wxPoint* position, double* quality));
    
    // Configuration
    MOCK_METHOD1(SetSearchRegion, void(int radius));
    MOCK_METHOD0(GetSearchRegion, int());
    MOCK_METHOD1(SetMinStarSNR, void(double snr));
    MOCK_METHOD0(GetMinStarSNR, double());
    MOCK_METHOD1(SetMaxStarHFD, void(double hfd));
    MOCK_METHOD0(GetMaxStarHFD, double());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetStarData, void(const wxPoint& position, double quality));
    MOCK_METHOD1(SimulateStarLoss, void(bool lost));
    
    static MockStarDetector* instance;
    static MockStarDetector* GetInstance();
    static void SetInstance(MockStarDetector* inst);
};

// Mock mount interface for guiding
class MockMountInterface {
public:
    // Mount connection and status
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsCalibrated, bool());
    
    // Guide pulse operations
    MOCK_METHOD2(Guide, bool(int direction, int duration));
    MOCK_METHOD4(GuideComplete, bool(int direction, int duration, bool* success, wxString* error));
    MOCK_METHOD0(IsGuiding, bool());
    MOCK_METHOD0(StopGuiding, bool());
    
    // Mount properties
    MOCK_METHOD0(GetGuideRateRA, double());
    MOCK_METHOD0(GetGuideRateDec, double());
    MOCK_METHOD1(SetGuideRateRA, bool(double rate));
    MOCK_METHOD1(SetGuideRateDec, bool(double rate));
    MOCK_METHOD0(GetSiderealRate, double());
    
    // Calibration support
    MOCK_METHOD0(GetCalibrationData, wxString());
    MOCK_METHOD1(SetCalibrationData, void(const wxString& data));
    MOCK_METHOD0(ClearCalibrationData, void());
    
    // ST4 interface
    MOCK_METHOD0(HasST4Interface, bool());
    MOCK_METHOD2(ST4PulseGuide, bool(int direction, int duration));
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(ClearError, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SimulateGuideResponse, void(int direction, bool success));
    MOCK_METHOD1(SimulateCalibrationData, void(const wxString& data));
    
    static MockMountInterface* instance;
    static MockMountInterface* GetInstance();
    static void SetInstance(MockMountInterface* inst);
};

// Guiding hardware simulator for comprehensive testing
class GuidingHardwareSimulator {
public:
    enum GuiderState {
        STATE_UNINITIALIZED = 0,
        STATE_SELECTING = 1,
        STATE_SELECTED = 2,
        STATE_CALIBRATING_RA = 3,
        STATE_CALIBRATING_DEC = 4,
        STATE_CALIBRATED = 5,
        STATE_GUIDING = 6,
        STATE_STOP = 7
    };
    
    enum CalibrationState {
        CALIBRATION_STATE_CLEARED = 0,
        CALIBRATION_STATE_GOTO_PLUS_RA = 1,
        CALIBRATION_STATE_GOTO_MINUS_RA = 2,
        CALIBRATION_STATE_GOTO_PLUS_DEC = 3,
        CALIBRATION_STATE_GOTO_MINUS_DEC = 4,
        CALIBRATION_STATE_COMPLETE = 5
    };
    
    struct StarInfo {
        wxPoint position;
        double quality;
        double snr;
        double hfd;
        bool isValid;
        bool isLost;
        
        StarInfo() : position(0, 0), quality(0.0), snr(0.0), hfd(0.0), isValid(false), isLost(false) {}
        StarInfo(const wxPoint& pos, double q, double s, double h) 
            : position(pos), quality(q), snr(s), hfd(h), isValid(true), isLost(false) {}
    };
    
    struct GuiderInfo {
        GuiderState state;
        bool isConnected;
        bool isLocked;
        bool isGuiding;
        bool isPaused;
        bool isCalibrating;
        bool multiStarMode;
        wxPoint lockPosition;
        wxPoint currentPosition;
        wxRect boundingBox;
        int maxMovePixels;
        int starCount;
        bool shouldFail;
        wxString lastError;
        
        GuiderInfo() : state(STATE_UNINITIALIZED), isConnected(false), isLocked(false),
                      isGuiding(false), isPaused(false), isCalibrating(false),
                      multiStarMode(false), lockPosition(0, 0), currentPosition(0, 0),
                      boundingBox(0, 0, 100, 100), maxMovePixels(50), starCount(0),
                      shouldFail(false), lastError("") {}
    };
    
    struct CalibrationInfo {
        CalibrationState state;
        bool isActive;
        wxPoint startPosition;
        wxPoint currentPosition;
        double raAngle;
        double decAngle;
        double raRate;
        double decRate;
        int stepsCompleted;
        bool shouldFail;
        
        CalibrationInfo() : state(CALIBRATION_STATE_CLEARED), isActive(false),
                           startPosition(0, 0), currentPosition(0, 0), raAngle(0.0),
                           decAngle(90.0), raRate(1.0), decRate(1.0), stepsCompleted(0),
                           shouldFail(false) {}
    };
    
    struct MountInfo {
        bool isConnected;
        bool isCalibrated;
        bool isGuiding;
        double guideRateRA;
        double guideRateDec;
        double siderealRate;
        bool hasST4Interface;
        wxString calibrationData;
        bool shouldFail;
        wxString lastError;
        
        MountInfo() : isConnected(false), isCalibrated(false), isGuiding(false),
                     guideRateRA(0.5), guideRateDec(0.5), siderealRate(15.041),
                     hasST4Interface(true), calibrationData(""), shouldFail(false),
                     lastError("") {}
    };
    
    // Component management
    void SetupGuider(const GuiderInfo& info);
    void SetupStar(const StarInfo& info);
    void SetupCalibration(const CalibrationInfo& info);
    void SetupMount(const MountInfo& info);
    
    // State management
    GuiderInfo GetGuiderInfo() const;
    StarInfo GetStarInfo() const;
    CalibrationInfo GetCalibrationInfo() const;
    MountInfo GetMountInfo() const;
    
    // Guiding simulation
    bool ConnectGuider();
    bool DisconnectGuider();
    bool SetLockPosition(const wxPoint& position);
    bool StartGuiding();
    bool StopGuiding();
    bool UpdateGuideStep(double raOffset, double decOffset);
    
    // Star simulation
    bool FindStar(const wxPoint& searchCenter, wxPoint* position);
    bool TrackStar(const wxPoint& lastPosition, wxPoint* newPosition);
    bool IsStarLost() const;
    void SimulateStarMovement(double raOffset, double decOffset);
    
    // Calibration simulation
    bool BeginCalibration();
    bool UpdateCalibration();
    bool CompleteCalibration();
    void AbortCalibration();
    
    // Mount simulation
    bool ConnectMount();
    bool DisconnectMount();
    bool SendGuidePulse(int direction, int duration);
    bool IsGuidePulseActive() const;
    void UpdateGuidePulse(double deltaTime);
    
    // Error simulation
    void SetGuiderError(bool error);
    void SetStarError(bool error);
    void SetCalibrationError(bool error);
    void SetMountError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultGuider();
    void SetupDefaultStar();
    void SetupDefaultMount();
    
    // Image simulation
    void GenerateTestImage(int width, int height);
    void AddStarToImage(const wxPoint& position, double brightness);
    void AddNoiseToImage(double level);
    
private:
    GuiderInfo guiderInfo;
    StarInfo starInfo;
    CalibrationInfo calibrationInfo;
    MountInfo mountInfo;
    
    // Guide pulse state
    bool isGuidePulseActive;
    int guidePulseDirection;
    int guidePulseDuration;
    wxDateTime guidePulseStartTime;
    
    // Multi-star state
    std::vector<StarInfo> secondaryStars;
    
    void InitializeDefaults();
    void UpdateGuiderState();
    void UpdateCalibrationState();
    wxPoint CalculateStarMovement(double raOffset, double decOffset) const;
};

// Helper class to manage all guiding hardware mocks
class MockGuidingHardwareManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockGuidingHardware* GetMockHardware();
    static MockStarDetector* GetMockStarDetector();
    static MockMountInterface* GetMockMount();
    static GuidingHardwareSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedGuider();
    static void SetupLockedGuider();
    static void SetupGuidingScenario();
    static void SetupCalibrationScenario();
    static void SetupMultiStarScenario();
    static void SimulateGuidingFailure();
    static void SimulateStarLoss();
    static void SimulateCalibrationFailure();
    
private:
    static MockGuidingHardware* mockHardware;
    static MockStarDetector* mockStarDetector;
    static MockMountInterface* mockMount;
    static std::unique_ptr<GuidingHardwareSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_GUIDING_HARDWARE_MOCKS() MockGuidingHardwareManager::SetupMocks()
#define TEARDOWN_GUIDING_HARDWARE_MOCKS() MockGuidingHardwareManager::TeardownMocks()
#define RESET_GUIDING_HARDWARE_MOCKS() MockGuidingHardwareManager::ResetMocks()

#define GET_MOCK_GUIDING_HARDWARE() MockGuidingHardwareManager::GetMockHardware()
#define GET_MOCK_STAR_DETECTOR() MockGuidingHardwareManager::GetMockStarDetector()
#define GET_MOCK_MOUNT_INTERFACE() MockGuidingHardwareManager::GetMockMount()
#define GET_GUIDING_SIMULATOR() MockGuidingHardwareManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_GUIDER_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_GUIDING_HARDWARE(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_GUIDER_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_GUIDING_HARDWARE(), Disconnect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_STAR_FOUND(position) \
    EXPECT_CALL(*GET_MOCK_STAR_DETECTOR(), FindStar(::testing::_, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(position), \
                                   ::testing::Return(true)))

#define EXPECT_STAR_TRACKED(oldPos, newPos) \
    EXPECT_CALL(*GET_MOCK_STAR_DETECTOR(), TrackStar(::testing::_, oldPos, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<2>(newPos), \
                                   ::testing::Return(true)))

#define EXPECT_GUIDE_PULSE_SUCCESS(direction, duration) \
    EXPECT_CALL(*GET_MOCK_MOUNT_INTERFACE(), Guide(direction, duration)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CALIBRATION_BEGIN_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_GUIDING_HARDWARE(), BeginCalibration()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_GUIDING_START_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_GUIDING_HARDWARE(), StartGuiding()) \
        .WillOnce(::testing::Return(true))

#endif // MOCK_GUIDING_HARDWARE_H
