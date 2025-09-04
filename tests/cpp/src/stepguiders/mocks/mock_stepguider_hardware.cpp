/*
 * mock_stepguider_hardware.cpp
 * PHD Guiding - Stepguider Module Tests
 *
 * Implementation of mock stepguider hardware objects
 */

#include "mock_stepguider_hardware.h"
#include <cmath>

// Static instance declarations
MockStepguiderHardware* MockStepguiderHardware::instance = nullptr;
MockStepguiderPositionTracker* MockStepguiderPositionTracker::instance = nullptr;
MockStepguiderCalibration* MockStepguiderCalibration::instance = nullptr;

// MockStepguiderHardwareManager static members
MockStepguiderHardware* MockStepguiderHardwareManager::mockHardware = nullptr;
MockStepguiderPositionTracker* MockStepguiderHardwareManager::mockPositionTracker = nullptr;
MockStepguiderCalibration* MockStepguiderHardwareManager::mockCalibration = nullptr;
std::unique_ptr<StepguiderHardwareSimulator> MockStepguiderHardwareManager::simulator = nullptr;

// MockStepguiderHardware implementation
MockStepguiderHardware* MockStepguiderHardware::GetInstance() {
    return instance;
}

void MockStepguiderHardware::SetInstance(MockStepguiderHardware* inst) {
    instance = inst;
}

// MockStepguiderPositionTracker implementation
MockStepguiderPositionTracker* MockStepguiderPositionTracker::GetInstance() {
    return instance;
}

void MockStepguiderPositionTracker::SetInstance(MockStepguiderPositionTracker* inst) {
    instance = inst;
}

// MockStepguiderCalibration implementation
MockStepguiderCalibration* MockStepguiderCalibration::GetInstance() {
    return instance;
}

void MockStepguiderCalibration::SetInstance(MockStepguiderCalibration* inst) {
    instance = inst;
}

// StepguiderHardwareSimulator implementation
void StepguiderHardwareSimulator::SetupStepguider(const StepguiderInfo& info) {
    stepguiderInfo = info;
}

void StepguiderHardwareSimulator::SetupCalibration(const CalibrationInfo& info) {
    calibrationInfo = info;
}

void StepguiderHardwareSimulator::SetupBump(const BumpInfo& info) {
    bumpInfo = info;
}

StepguiderHardwareSimulator::StepguiderInfo StepguiderHardwareSimulator::GetStepguiderInfo() const {
    return stepguiderInfo;
}

StepguiderHardwareSimulator::CalibrationInfo StepguiderHardwareSimulator::GetCalibrationInfo() const {
    return calibrationInfo;
}

StepguiderHardwareSimulator::BumpInfo StepguiderHardwareSimulator::GetBumpInfo() const {
    return bumpInfo;
}

bool StepguiderHardwareSimulator::ConnectStepguider() {
    if (stepguiderInfo.shouldFail) {
        stepguiderInfo.lastError = "Connection failed";
        return false;
    }
    
    stepguiderInfo.isConnected = true;
    stepguiderInfo.lastError = "";
    
    // Initialize position to center
    stepguiderInfo.currentX = 0;
    stepguiderInfo.currentY = 0;
    
    return true;
}

bool StepguiderHardwareSimulator::DisconnectStepguider() {
    stepguiderInfo.isConnected = false;
    calibrationInfo.isCalibrating = false;
    bumpInfo.bumpInProgress = false;
    isPulseGuiding = false;
    return true;
}

bool StepguiderHardwareSimulator::IsConnected() const {
    return stepguiderInfo.isConnected;
}

StepguiderHardwareSimulator::StepResult StepguiderHardwareSimulator::ExecuteStep(int direction, int steps) {
    if (!stepguiderInfo.isConnected || stepguiderInfo.shouldFail) {
        stepguiderInfo.lastError = "Cannot execute step";
        return STEP_ERROR;
    }
    
    // Check if step would hit limit
    if (WouldHitLimit(direction, steps)) {
        stepguiderInfo.lastError = "Step would hit limit";
        return STEP_LIMIT_REACHED;
    }
    
    // Execute the step
    wxPoint newPosition = CalculateNewPosition(direction, steps);
    stepguiderInfo.currentX = newPosition.x;
    stepguiderInfo.currentY = newPosition.y;
    
    // Update limit flags
    UpdatePositionLimits();
    
    return STEP_OK;
}

bool StepguiderHardwareSimulator::IsAtLimit(int direction) const {
    switch (direction) {
        case 0: // NORTH/UP
            return stepguiderInfo.atLimitNorth;
        case 1: // SOUTH/DOWN
            return stepguiderInfo.atLimitSouth;
        case 2: // EAST/RIGHT
            return stepguiderInfo.atLimitEast;
        case 3: // WEST/LEFT
            return stepguiderInfo.atLimitWest;
        default:
            return false;
    }
}

bool StepguiderHardwareSimulator::WouldHitLimit(int direction, int steps) const {
    wxPoint newPosition = CalculateNewPosition(direction, steps);
    return !IsValidPosition(newPosition);
}

int StepguiderHardwareSimulator::GetCurrentPosition(int direction) const {
    switch (direction) {
        case 0: // NORTH/UP
        case 1: // SOUTH/DOWN
            return stepguiderInfo.currentY;
        case 2: // EAST/RIGHT
        case 3: // WEST/LEFT
            return stepguiderInfo.currentX;
        default:
            return 0;
    }
}

void StepguiderHardwareSimulator::SetCurrentPosition(int direction, int position) {
    switch (direction) {
        case 0: // NORTH/UP
        case 1: // SOUTH/DOWN
            stepguiderInfo.currentY = position;
            break;
        case 2: // EAST/RIGHT
        case 3: // WEST/LEFT
            stepguiderInfo.currentX = position;
            break;
    }
    
    UpdatePositionLimits();
}

bool StepguiderHardwareSimulator::MoveToCenter() {
    if (!stepguiderInfo.isConnected || stepguiderInfo.shouldFail) {
        return false;
    }
    
    stepguiderInfo.currentX = 0;
    stepguiderInfo.currentY = 0;
    UpdatePositionLimits();
    
    return true;
}

bool StepguiderHardwareSimulator::BeginCalibration(const wxPoint& startLocation) {
    if (!stepguiderInfo.isConnected || calibrationInfo.shouldFail) {
        stepguiderInfo.lastError = "Cannot begin calibration";
        return false;
    }
    
    calibrationInfo.isCalibrating = true;
    calibrationInfo.state = CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER;
    calibrationInfo.startLocation = startLocation;
    calibrationInfo.currentLocation = startLocation;
    calibrationInfo.iterationsCompleted = 0;
    
    return true;
}

bool StepguiderHardwareSimulator::UpdateCalibration(const wxPoint& currentLocation) {
    if (!calibrationInfo.isCalibrating) {
        return false;
    }
    
    calibrationInfo.currentLocation = currentLocation;
    calibrationInfo.iterationsCompleted++;
    
    // Simulate calibration state progression
    switch (calibrationInfo.state) {
        case CALIBRATION_STATE_GOTO_LOWER_RIGHT_CORNER:
            if (calibrationInfo.iterationsCompleted >= 10) {
                calibrationInfo.state = CALIBRATION_STATE_GO_LEFT;
            }
            break;
        case CALIBRATION_STATE_GO_LEFT:
            if (calibrationInfo.iterationsCompleted >= 20) {
                calibrationInfo.state = CALIBRATION_STATE_GO_UP;
            }
            break;
        case CALIBRATION_STATE_GO_UP:
            if (calibrationInfo.iterationsCompleted >= 30) {
                calibrationInfo.state = CALIBRATION_STATE_COMPLETE;
                return CompleteCalibration();
            }
            break;
        default:
            break;
    }
    
    return true;
}

bool StepguiderHardwareSimulator::CompleteCalibration() {
    if (!calibrationInfo.isCalibrating) {
        return false;
    }
    
    // Calculate calibration results
    calibrationInfo.xAngle = 0.0; // Horizontal
    calibrationInfo.yAngle = M_PI / 2.0; // Vertical
    calibrationInfo.xRate = 1.0; // pixels per step
    calibrationInfo.yRate = 1.0; // pixels per step
    calibrationInfo.quality = 0.95; // Good quality
    
    calibrationInfo.isCalibrating = false;
    calibrationInfo.state = CALIBRATION_STATE_COMPLETE;
    
    return true;
}

void StepguiderHardwareSimulator::AbortCalibration() {
    calibrationInfo.isCalibrating = false;
    calibrationInfo.state = CALIBRATION_STATE_CLEARED;
}

bool StepguiderHardwareSimulator::IsCalibrating() const {
    return calibrationInfo.isCalibrating;
}

StepguiderHardwareSimulator::CalibrationState StepguiderHardwareSimulator::GetCalibrationState() const {
    return calibrationInfo.state;
}

bool StepguiderHardwareSimulator::InitBumpPositions() {
    // Initialize bump positions based on max travel
    bumpInfo.bumpCenterTolerance = std::min(stepguiderInfo.maxStepsX, stepguiderInfo.maxStepsY) / 10;
    return true;
}

bool StepguiderHardwareSimulator::IsBumpRequired() const {
    int distanceFromCenter = DistanceFromCenter();
    return distanceFromCenter > (stepguiderInfo.maxStepsX * 0.8); // 80% of max travel
}

int StepguiderHardwareSimulator::CalculateBumpDirection() const {
    // Calculate direction to move back toward center
    if (abs(stepguiderInfo.currentX) > abs(stepguiderInfo.currentY)) {
        return (stepguiderInfo.currentX > 0) ? 3 : 2; // WEST or EAST
    } else {
        return (stepguiderInfo.currentY > 0) ? 1 : 0; // SOUTH or NORTH
    }
}

bool StepguiderHardwareSimulator::ExecuteBump(int direction) {
    if (bumpInfo.shouldFail) {
        return false;
    }
    
    bumpInfo.bumpInProgress = true;
    bumpInfo.bumpDirection = direction;
    bumpInfo.bumpSteps = stepguiderInfo.maxStepsX / 4; // 25% of max travel
    bumpInfo.bumpStartTime = wxDateTime::Now();
    
    // Simulate bump movement
    ExecuteStep(direction, bumpInfo.bumpSteps);
    
    return true;
}

void StepguiderHardwareSimulator::UpdateBump(double deltaTime) {
    if (!bumpInfo.bumpInProgress) return;
    
    // Simulate bump completion after some time
    wxTimeSpan elapsed = wxDateTime::Now() - bumpInfo.bumpStartTime;
    if (elapsed.GetMilliseconds() >= 1000) { // 1 second bump duration
        bumpInfo.bumpInProgress = false;
    }
}

bool StepguiderHardwareSimulator::StartPulseGuide(int direction, int duration) {
    if (!stepguiderInfo.isConnected || stepguiderInfo.shouldFail) {
        stepguiderInfo.lastError = "Cannot pulse guide";
        return false;
    }
    
    isPulseGuiding = true;
    pulseDirection = direction;
    pulseDuration = duration;
    pulseStartTime = wxDateTime::Now();
    
    return true;
}

bool StepguiderHardwareSimulator::IsPulseGuiding() const {
    return isPulseGuiding;
}

void StepguiderHardwareSimulator::UpdatePulseGuide(double deltaTime) {
    if (!isPulseGuiding) return;
    
    wxTimeSpan elapsed = wxDateTime::Now() - pulseStartTime;
    if (elapsed.GetMilliseconds() >= pulseDuration) {
        isPulseGuiding = false;
    }
}

void StepguiderHardwareSimulator::SetStepguiderError(bool error) {
    stepguiderInfo.shouldFail = error;
    if (error) {
        stepguiderInfo.lastError = "Stepguider error simulated";
    } else {
        stepguiderInfo.lastError = "";
    }
}

void StepguiderHardwareSimulator::SetCalibrationError(bool error) {
    calibrationInfo.shouldFail = error;
}

void StepguiderHardwareSimulator::SetConnectionError(bool error) {
    if (error) {
        stepguiderInfo.isConnected = false;
        stepguiderInfo.lastError = "Connection error";
    }
}

void StepguiderHardwareSimulator::SetBumpError(bool error) {
    bumpInfo.shouldFail = error;
}

void StepguiderHardwareSimulator::Reset() {
    stepguiderInfo = StepguiderInfo();
    calibrationInfo = CalibrationInfo();
    bumpInfo = BumpInfo();
    
    isPulseGuiding = false;
    pulseDirection = -1;
    pulseDuration = 0;
    
    SetupDefaultStepguider();
}

void StepguiderHardwareSimulator::SetupDefaultStepguider() {
    // Set up default stepguider
    stepguiderInfo.type = STEPGUIDER_SIMULATOR;
    stepguiderInfo.name = "Stepguider Simulator";
    stepguiderInfo.id = "SIM001";
    stepguiderInfo.hasNonGuiMove = true;
    stepguiderInfo.hasSetupDialog = false;
    stepguiderInfo.canSelectStepguider = false;
    stepguiderInfo.maxStepsX = 45;
    stepguiderInfo.maxStepsY = 45;
    stepguiderInfo.currentX = 0;
    stepguiderInfo.currentY = 0;
    
    // Set up default calibration
    calibrationInfo.stepsPerIteration = 3;
    calibrationInfo.samplesToAverage = 5;
    
    // Set up default bump
    InitBumpPositions();
}

wxPoint StepguiderHardwareSimulator::CalculateNewPosition(int direction, int steps) const {
    wxPoint newPosition(stepguiderInfo.currentX, stepguiderInfo.currentY);
    
    switch (direction) {
        case 0: // NORTH/UP
            newPosition.y += steps;
            break;
        case 1: // SOUTH/DOWN
            newPosition.y -= steps;
            break;
        case 2: // EAST/RIGHT
            newPosition.x += steps;
            break;
        case 3: // WEST/LEFT
            newPosition.x -= steps;
            break;
    }
    
    return newPosition;
}

bool StepguiderHardwareSimulator::IsValidPosition(const wxPoint& position) const {
    return (abs(position.x) <= stepguiderInfo.maxStepsX && 
            abs(position.y) <= stepguiderInfo.maxStepsY);
}

int StepguiderHardwareSimulator::DistanceFromCenter() const {
    return static_cast<int>(sqrt(stepguiderInfo.currentX * stepguiderInfo.currentX + 
                                stepguiderInfo.currentY * stepguiderInfo.currentY));
}

void StepguiderHardwareSimulator::InitializeDefaults() {
    Reset();
}

void StepguiderHardwareSimulator::UpdatePositionLimits() {
    stepguiderInfo.atLimitNorth = (stepguiderInfo.currentY >= stepguiderInfo.maxStepsY);
    stepguiderInfo.atLimitSouth = (stepguiderInfo.currentY <= -stepguiderInfo.maxStepsY);
    stepguiderInfo.atLimitEast = (stepguiderInfo.currentX >= stepguiderInfo.maxStepsX);
    stepguiderInfo.atLimitWest = (stepguiderInfo.currentX <= -stepguiderInfo.maxStepsX);
}

bool StepguiderHardwareSimulator::CheckPositionLimits(int direction, int steps) const {
    wxPoint newPosition = CalculateNewPosition(direction, steps);
    return IsValidPosition(newPosition);
}

// MockStepguiderHardwareManager implementation
void MockStepguiderHardwareManager::SetupMocks() {
    // Create all mock instances
    mockHardware = new MockStepguiderHardware();
    mockPositionTracker = new MockStepguiderPositionTracker();
    mockCalibration = new MockStepguiderCalibration();
    
    // Set static instances
    MockStepguiderHardware::SetInstance(mockHardware);
    MockStepguiderPositionTracker::SetInstance(mockPositionTracker);
    MockStepguiderCalibration::SetInstance(mockCalibration);
    
    // Create simulator
    simulator = std::make_unique<StepguiderHardwareSimulator>();
    simulator->SetupDefaultStepguider();
}

void MockStepguiderHardwareManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockHardware;
    delete mockPositionTracker;
    delete mockCalibration;
    
    // Reset pointers
    mockHardware = nullptr;
    mockPositionTracker = nullptr;
    mockCalibration = nullptr;
    
    // Reset static instances
    MockStepguiderHardware::SetInstance(nullptr);
    MockStepguiderPositionTracker::SetInstance(nullptr);
    MockStepguiderCalibration::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockStepguiderHardwareManager::ResetMocks() {
    if (mockHardware) {
        testing::Mock::VerifyAndClearExpectations(mockHardware);
    }
    if (mockPositionTracker) {
        testing::Mock::VerifyAndClearExpectations(mockPositionTracker);
    }
    if (mockCalibration) {
        testing::Mock::VerifyAndClearExpectations(mockCalibration);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockStepguiderHardware* MockStepguiderHardwareManager::GetMockHardware() { return mockHardware; }
MockStepguiderPositionTracker* MockStepguiderHardwareManager::GetMockPositionTracker() { return mockPositionTracker; }
MockStepguiderCalibration* MockStepguiderHardwareManager::GetMockCalibration() { return mockCalibration; }
StepguiderHardwareSimulator* MockStepguiderHardwareManager::GetSimulator() { return simulator.get(); }

void MockStepguiderHardwareManager::SetupConnectedStepguider() {
    if (simulator) {
        simulator->ConnectStepguider();
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, Connect())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockStepguiderHardwareManager::SetupStepguiderWithCapabilities() {
    SetupConnectedStepguider();
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, HasNonGuiMove())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, HasSetupDialog())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, MaxPosition(::testing::_))
            .WillRepeatedly(::testing::Return(45));
    }
}

void MockStepguiderHardwareManager::SetupCalibrationScenario() {
    SetupStepguiderWithCapabilities();
    
    if (mockCalibration) {
        EXPECT_CALL(*mockCalibration, BeginCalibration(::testing::_))
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockCalibration, UpdateCalibration(::testing::_))
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockCalibration, IsCalibrating())
            .WillRepeatedly(::testing::Return(false));
    }
}

void MockStepguiderHardwareManager::SimulateStepguiderFailure() {
    if (simulator) {
        simulator->SetStepguiderError(true);
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, Connect())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, IsConnected())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("Stepguider error")));
    }
}

void MockStepguiderHardwareManager::SimulateCalibrationFailure() {
    SetupConnectedStepguider();
    
    if (simulator) {
        simulator->SetCalibrationError(true);
    }
    
    if (mockCalibration) {
        EXPECT_CALL(*mockCalibration, BeginCalibration(::testing::_))
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockCalibration, UpdateCalibration(::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
}
