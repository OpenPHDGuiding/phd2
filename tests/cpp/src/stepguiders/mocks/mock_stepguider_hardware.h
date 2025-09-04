/*
 * mock_stepguider_hardware.h
 * PHD Guiding - Stepguider Module Tests
 *
 * Mock objects for stepguider hardware interfaces
 * Provides controllable behavior for stepguider operations and calibration
 */

#ifndef MOCK_STEPGUIDER_HARDWARE_H
#define MOCK_STEPGUIDER_HARDWARE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class StepguiderHardwareSimulator;

// Mock stepguider hardware interface
class MockStepguiderHardware {
public:
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(GetConnectionStatus, int());
    
    // Stepguider capabilities
    MOCK_METHOD0(HasNonGuiMove, bool());
    MOCK_METHOD0(HasSetupDialog, bool());
    MOCK_METHOD0(CanSelectStepguider, bool());
    MOCK_METHOD1(MaxPosition, int(int direction));
    MOCK_METHOD1(SetMaxPosition, bool(int steps));
    
    // Step operations
    MOCK_METHOD2(Step, int(int direction, int steps)); // Returns STEP_RESULT
    MOCK_METHOD0(Center, bool());
    MOCK_METHOD0(MoveToCenter, bool());
    MOCK_METHOD1(CurrentPosition, int(int direction));
    MOCK_METHOD2(IsAtLimit, bool(int direction, bool* atLimit));
    MOCK_METHOD2(WouldHitLimit, bool(int direction, int steps));
    
    // Calibration operations
    MOCK_METHOD0(BeginCalibration, bool());
    MOCK_METHOD1(UpdateCalibrationState, bool(const wxPoint& currentLocation));
    MOCK_METHOD0(ClearCalibration, void());
    MOCK_METHOD0(IsCalibrated, bool());
    MOCK_METHOD0(GetCalibrationData, wxString());
    
    // ST4 guiding interface
    MOCK_METHOD0(ST4HasGuideOutput, bool());
    MOCK_METHOD0(ST4HostConnected, bool());
    MOCK_METHOD0(ST4HasNonGuiMove, bool());
    MOCK_METHOD2(ST4PulseGuideScope, bool(int direction, int duration));
    
    // Configuration and dialogs
    MOCK_METHOD0(ShowPropertyDialog, void());
    MOCK_METHOD0(GetSettingsSummary, wxString());
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(ClearError, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetPosition, void(int direction, int position));
    MOCK_METHOD1(SimulateStep, void(bool success));
    MOCK_METHOD2(SimulateLimit, void(int direction, bool atLimit));
    MOCK_METHOD1(SimulateCalibration, void(bool success));
    
    static MockStepguiderHardware* instance;
    static MockStepguiderHardware* GetInstance();
    static void SetInstance(MockStepguiderHardware* inst);
};

// Mock stepguider position tracking interface
class MockStepguiderPositionTracker {
public:
    // Position tracking
    MOCK_METHOD2(UpdatePosition, void(int direction, int steps));
    MOCK_METHOD1(GetPosition, int(int direction));
    MOCK_METHOD0(GetCurrentPosition, wxPoint());
    MOCK_METHOD0(ZeroPosition, void());
    MOCK_METHOD1(SetPosition, void(const wxPoint& position));
    
    // Limit checking
    MOCK_METHOD2(CheckLimits, bool(int direction, int steps));
    MOCK_METHOD1(IsAtLimit, bool(int direction));
    MOCK_METHOD0(GetLimitStatus, int()); // Bitmask of limit states
    
    // Bump operations
    MOCK_METHOD0(InitBumpPositions, void());
    MOCK_METHOD0(IsBumpRequired, bool());
    MOCK_METHOD0(CalculateBumpDirection, int());
    MOCK_METHOD1(ExecuteBump, bool(int direction));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetLimits, void(int maxX, int maxY));
    MOCK_METHOD2(SimulatePosition, void(int x, int y));
    
    static MockStepguiderPositionTracker* instance;
    static MockStepguiderPositionTracker* GetInstance();
    static void SetInstance(MockStepguiderPositionTracker* inst);
};

// Mock stepguider calibration interface
class MockStepguiderCalibration {
public:
    // Calibration management
    MOCK_METHOD1(BeginCalibration, bool(const wxPoint& startLocation));
    MOCK_METHOD1(UpdateCalibration, bool(const wxPoint& currentLocation));
    MOCK_METHOD0(CompleteCalibration, bool());
    MOCK_METHOD0(AbortCalibration, void());
    MOCK_METHOD0(IsCalibrating, bool());
    MOCK_METHOD0(GetCalibrationState, int());
    
    // Calibration data
    MOCK_METHOD1(SetCalibrationData, void(const wxString& data));
    MOCK_METHOD0(GetCalibrationData, wxString());
    MOCK_METHOD0(ClearCalibrationData, void());
    MOCK_METHOD0(IsCalibrationValid, bool());
    
    // Calibration parameters
    MOCK_METHOD1(SetCalibrationStepsPerIteration, void(int steps));
    MOCK_METHOD0(GetCalibrationStepsPerIteration, int());
    MOCK_METHOD1(SetCalibrationSamplesToAverage, void(int samples));
    MOCK_METHOD0(GetCalibrationSamplesToAverage, int());
    
    // Calibration results
    MOCK_METHOD0(GetXAngle, double());
    MOCK_METHOD0(GetYAngle, double());
    MOCK_METHOD0(GetXRate, double());
    MOCK_METHOD0(GetYRate, double());
    MOCK_METHOD0(GetCalibrationQuality, double());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD4(SetCalibrationResults, void(double xAngle, double yAngle, double xRate, double yRate));
    MOCK_METHOD1(SimulateCalibrationStep, void(bool success));
    
    static MockStepguiderCalibration* instance;
    static MockStepguiderCalibration* GetInstance();
    static void SetInstance(MockStepguiderCalibration* inst);
};

// Stepguider hardware simulator for comprehensive testing
class StepguiderHardwareSimulator {
public:
    enum StepguiderType {
        STEPGUIDER_SIMULATOR = 0,
        STEPGUIDER_SXAO = 1,
        STEPGUIDER_SXAO_INDI = 2,
        STEPGUIDER_SBIGAO_INDI = 3
    };
    
    enum StepResult {
        STEP_OK = 0,
        STEP_LIMIT_REACHED = 1,
        STEP_ERROR = 2
    };
    
    enum CalibrationState {
        CALIBRATION_STATE_CLEARED = 0,
        CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER = 1,
        CALIBRATION_STATE_GO_LEFT = 2,
        CALIBRATION_STATE_GO_UP = 3,
        CALIBRATION_STATE_COMPLETE = 4
    };
    
    struct StepguiderInfo {
        StepguiderType type;
        wxString name;
        wxString id;
        bool isConnected;
        bool hasNonGuiMove;
        bool hasSetupDialog;
        bool canSelectStepguider;
        int maxStepsX;
        int maxStepsY;
        int currentX;
        int currentY;
        bool atLimitNorth;
        bool atLimitSouth;
        bool atLimitEast;
        bool atLimitWest;
        bool shouldFail;
        wxString lastError;
        
        StepguiderInfo() : type(STEPGUIDER_SIMULATOR), name("Simulator"), id("SIM001"),
                          isConnected(false), hasNonGuiMove(true), hasSetupDialog(false),
                          canSelectStepguider(false), maxStepsX(45), maxStepsY(45),
                          currentX(0), currentY(0), atLimitNorth(false), atLimitSouth(false),
                          atLimitEast(false), atLimitWest(false), shouldFail(false), lastError("") {}
    };
    
    struct CalibrationInfo {
        bool isCalibrating;
        CalibrationState state;
        wxPoint startLocation;
        wxPoint currentLocation;
        int stepsPerIteration;
        int samplesToAverage;
        int iterationsCompleted;
        double xAngle;
        double yAngle;
        double xRate;
        double yRate;
        double quality;
        bool shouldFail;
        
        CalibrationInfo() : isCalibrating(false), state(CALIBRATION_STATE_CLEARED),
                           startLocation(0, 0), currentLocation(0, 0), stepsPerIteration(3),
                           samplesToAverage(5), iterationsCompleted(0), xAngle(0.0), yAngle(0.0),
                           xRate(1.0), yRate(1.0), quality(1.0), shouldFail(false) {}
    };
    
    struct BumpInfo {
        bool bumpRequired;
        bool bumpInProgress;
        int bumpDirection;
        int bumpSteps;
        wxDateTime bumpStartTime;
        int bumpCenterTolerance;
        bool shouldFail;
        
        BumpInfo() : bumpRequired(false), bumpInProgress(false), bumpDirection(-1),
                    bumpSteps(0), bumpCenterTolerance(5), shouldFail(false) {}
    };
    
    // Component management
    void SetupStepguider(const StepguiderInfo& info);
    void SetupCalibration(const CalibrationInfo& info);
    void SetupBump(const BumpInfo& info);
    
    // State management
    StepguiderInfo GetStepguiderInfo() const;
    CalibrationInfo GetCalibrationInfo() const;
    BumpInfo GetBumpInfo() const;
    
    // Connection simulation
    bool ConnectStepguider();
    bool DisconnectStepguider();
    bool IsConnected() const;
    
    // Step simulation
    StepResult ExecuteStep(int direction, int steps);
    bool IsAtLimit(int direction) const;
    bool WouldHitLimit(int direction, int steps) const;
    int GetCurrentPosition(int direction) const;
    void SetCurrentPosition(int direction, int position);
    bool MoveToCenter();
    
    // Calibration simulation
    bool BeginCalibration(const wxPoint& startLocation);
    bool UpdateCalibration(const wxPoint& currentLocation);
    bool CompleteCalibration();
    void AbortCalibration();
    bool IsCalibrating() const;
    CalibrationState GetCalibrationState() const;
    
    // Bump simulation
    bool InitBumpPositions();
    bool IsBumpRequired() const;
    int CalculateBumpDirection() const;
    bool ExecuteBump(int direction);
    void UpdateBump(double deltaTime);
    
    // ST4 guiding simulation
    bool StartPulseGuide(int direction, int duration);
    bool IsPulseGuiding() const;
    void UpdatePulseGuide(double deltaTime);
    
    // Error simulation
    void SetStepguiderError(bool error);
    void SetCalibrationError(bool error);
    void SetConnectionError(bool error);
    void SetBumpError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultStepguider();
    
    // Position calculations
    wxPoint CalculateNewPosition(int direction, int steps) const;
    bool IsValidPosition(const wxPoint& position) const;
    int DistanceFromCenter() const;
    
private:
    StepguiderInfo stepguiderInfo;
    CalibrationInfo calibrationInfo;
    BumpInfo bumpInfo;
    
    // ST4 guiding state
    bool isPulseGuiding;
    int pulseDirection;
    int pulseDuration;
    wxDateTime pulseStartTime;
    
    void InitializeDefaults();
    void UpdatePositionLimits();
    bool CheckPositionLimits(int direction, int steps) const;
};

// Helper class to manage all stepguider hardware mocks
class MockStepguiderHardwareManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockStepguiderHardware* GetMockHardware();
    static MockStepguiderPositionTracker* GetMockPositionTracker();
    static MockStepguiderCalibration* GetMockCalibration();
    static StepguiderHardwareSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedStepguider();
    static void SetupStepguiderWithCapabilities();
    static void SetupCalibrationScenario();
    static void SimulateStepguiderFailure();
    static void SimulateCalibrationFailure();
    
private:
    static MockStepguiderHardware* mockHardware;
    static MockStepguiderPositionTracker* mockPositionTracker;
    static MockStepguiderCalibration* mockCalibration;
    static std::unique_ptr<StepguiderHardwareSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_STEPGUIDER_HARDWARE_MOCKS() MockStepguiderHardwareManager::SetupMocks()
#define TEARDOWN_STEPGUIDER_HARDWARE_MOCKS() MockStepguiderHardwareManager::TeardownMocks()
#define RESET_STEPGUIDER_HARDWARE_MOCKS() MockStepguiderHardwareManager::ResetMocks()

#define GET_MOCK_STEPGUIDER_HARDWARE() MockStepguiderHardwareManager::GetMockHardware()
#define GET_MOCK_STEPGUIDER_POSITION_TRACKER() MockStepguiderHardwareManager::GetMockPositionTracker()
#define GET_MOCK_STEPGUIDER_CALIBRATION() MockStepguiderHardwareManager::GetMockCalibration()
#define GET_STEPGUIDER_SIMULATOR() MockStepguiderHardwareManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_STEPGUIDER_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_STEPGUIDER_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), Disconnect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_STEPGUIDER_STEP_SUCCESS(direction, steps) \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), Step(direction, steps)) \
        .WillOnce(::testing::Return(0)) // STEP_OK

#define EXPECT_STEPGUIDER_CENTER_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), Center()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_STEPGUIDER_CALIBRATION_BEGIN(startLocation) \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_CALIBRATION(), BeginCalibration(startLocation)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_STEPGUIDER_CALIBRATION_UPDATE(currentLocation) \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_CALIBRATION(), UpdateCalibration(currentLocation)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_ST4_PULSE_GUIDE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), ST4PulseGuideScope(direction, duration)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_STEPGUIDER_MAX_POSITION(direction, maxPos) \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), MaxPosition(direction)) \
        .WillOnce(::testing::Return(maxPos))

#define EXPECT_STEPGUIDER_CURRENT_POSITION(direction, currentPos) \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), CurrentPosition(direction)) \
        .WillOnce(::testing::Return(currentPos))

#define EXPECT_STEPGUIDER_AT_LIMIT(direction, atLimit) \
    EXPECT_CALL(*GET_MOCK_STEPGUIDER_HARDWARE(), IsAtLimit(direction, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(atLimit), \
                                   ::testing::Return(true)))

#endif // MOCK_STEPGUIDER_HARDWARE_H
