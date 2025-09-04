/*
 * mock_guiding_hardware.cpp
 * PHD Guiding - Guiding Module Tests
 *
 * Implementation of mock guiding hardware objects
 */

#include "mock_guiding_hardware.h"
#include <cmath>

// Static instance declarations
MockGuidingHardware* MockGuidingHardware::instance = nullptr;
MockStarDetector* MockStarDetector::instance = nullptr;
MockMountInterface* MockMountInterface::instance = nullptr;

// MockGuidingHardwareManager static members
MockGuidingHardware* MockGuidingHardwareManager::mockHardware = nullptr;
MockStarDetector* MockGuidingHardwareManager::mockStarDetector = nullptr;
MockMountInterface* MockGuidingHardwareManager::mockMount = nullptr;
std::unique_ptr<GuidingHardwareSimulator> MockGuidingHardwareManager::simulator = nullptr;

// MockGuidingHardware implementation
MockGuidingHardware* MockGuidingHardware::GetInstance() {
    return instance;
}

void MockGuidingHardware::SetInstance(MockGuidingHardware* inst) {
    instance = inst;
}

// MockStarDetector implementation
MockStarDetector* MockStarDetector::GetInstance() {
    return instance;
}

void MockStarDetector::SetInstance(MockStarDetector* inst) {
    instance = inst;
}

// MockMountInterface implementation
MockMountInterface* MockMountInterface::GetInstance() {
    return instance;
}

void MockMountInterface::SetInstance(MockMountInterface* inst) {
    instance = inst;
}

// GuidingHardwareSimulator implementation
void GuidingHardwareSimulator::SetupGuider(const GuiderInfo& info) {
    guiderInfo = info;
}

void GuidingHardwareSimulator::SetupStar(const StarInfo& info) {
    starInfo = info;
}

void GuidingHardwareSimulator::SetupCalibration(const CalibrationInfo& info) {
    calibrationInfo = info;
}

void GuidingHardwareSimulator::SetupMount(const MountInfo& info) {
    mountInfo = info;
}

GuidingHardwareSimulator::GuiderInfo GuidingHardwareSimulator::GetGuiderInfo() const {
    return guiderInfo;
}

GuidingHardwareSimulator::StarInfo GuidingHardwareSimulator::GetStarInfo() const {
    return starInfo;
}

GuidingHardwareSimulator::CalibrationInfo GuidingHardwareSimulator::GetCalibrationInfo() const {
    return calibrationInfo;
}

GuidingHardwareSimulator::MountInfo GuidingHardwareSimulator::GetMountInfo() const {
    return mountInfo;
}

bool GuidingHardwareSimulator::ConnectGuider() {
    if (guiderInfo.shouldFail) {
        guiderInfo.lastError = "Guider connection failed";
        return false;
    }
    
    guiderInfo.isConnected = true;
    guiderInfo.state = STATE_SELECTING;
    guiderInfo.lastError = "";
    
    return true;
}

bool GuidingHardwareSimulator::DisconnectGuider() {
    guiderInfo.isConnected = false;
    guiderInfo.isLocked = false;
    guiderInfo.isGuiding = false;
    guiderInfo.isCalibrating = false;
    guiderInfo.state = STATE_UNINITIALIZED;
    
    return true;
}

bool GuidingHardwareSimulator::SetLockPosition(const wxPoint& position) {
    if (!guiderInfo.isConnected || guiderInfo.shouldFail) {
        guiderInfo.lastError = "Cannot set lock position";
        return false;
    }
    
    guiderInfo.lockPosition = position;
    guiderInfo.currentPosition = position;
    guiderInfo.isLocked = true;
    guiderInfo.state = STATE_SELECTED;
    
    // Set up star at lock position
    starInfo.position = position;
    starInfo.quality = 0.8;
    starInfo.snr = 10.0;
    starInfo.hfd = 2.5;
    starInfo.isValid = true;
    starInfo.isLost = false;
    
    return true;
}

bool GuidingHardwareSimulator::StartGuiding() {
    if (!guiderInfo.isConnected || !guiderInfo.isLocked || guiderInfo.shouldFail) {
        guiderInfo.lastError = "Cannot start guiding";
        return false;
    }
    
    guiderInfo.isGuiding = true;
    guiderInfo.state = STATE_GUIDING;
    guiderInfo.lastError = "";
    
    return true;
}

bool GuidingHardwareSimulator::StopGuiding() {
    guiderInfo.isGuiding = false;
    guiderInfo.isPaused = false;
    guiderInfo.state = STATE_SELECTED;
    
    return true;
}

bool GuidingHardwareSimulator::UpdateGuideStep(double raOffset, double decOffset) {
    if (!guiderInfo.isGuiding) {
        return false;
    }
    
    // Simulate star movement due to guide correction
    SimulateStarMovement(raOffset, decOffset);
    
    return true;
}

bool GuidingHardwareSimulator::FindStar(const wxPoint& searchCenter, wxPoint* position) {
    if (starInfo.shouldFail || starInfo.isLost) {
        return false;
    }
    
    // Simulate finding star near search center
    int searchRadius = 20; // pixels
    int dx = starInfo.position.x - searchCenter.x;
    int dy = starInfo.position.y - searchCenter.y;
    double distance = sqrt(dx * dx + dy * dy);
    
    if (distance <= searchRadius && starInfo.isValid) {
        *position = starInfo.position;
        return true;
    }
    
    return false;
}

bool GuidingHardwareSimulator::TrackStar(const wxPoint& lastPosition, wxPoint* newPosition) {
    if (starInfo.shouldFail || starInfo.isLost) {
        return false;
    }
    
    // Simulate star tracking with some noise
    wxPoint trackedPosition = starInfo.position;
    
    // Add small random movement to simulate tracking noise
    trackedPosition.x += (rand() % 3) - 1; // ±1 pixel noise
    trackedPosition.y += (rand() % 3) - 1;
    
    *newPosition = trackedPosition;
    guiderInfo.currentPosition = trackedPosition;
    
    return true;
}

bool GuidingHardwareSimulator::IsStarLost() const {
    return starInfo.isLost || !starInfo.isValid;
}

void GuidingHardwareSimulator::SimulateStarMovement(double raOffset, double decOffset) {
    // Convert RA/Dec offsets to pixel movement
    // Assume 1 arcsec = 1 pixel for simplicity
    int deltaX = static_cast<int>(raOffset);
    int deltaY = static_cast<int>(decOffset);
    
    starInfo.position.x += deltaX;
    starInfo.position.y += deltaY;
    guiderInfo.currentPosition = starInfo.position;
    
    // Check if star moved out of bounds
    if (starInfo.position.x < 0 || starInfo.position.x > 1000 ||
        starInfo.position.y < 0 || starInfo.position.y > 1000) {
        starInfo.isLost = true;
    }
}

bool GuidingHardwareSimulator::BeginCalibration() {
    if (!guiderInfo.isConnected || !guiderInfo.isLocked || calibrationInfo.shouldFail) {
        guiderInfo.lastError = "Cannot begin calibration";
        return false;
    }
    
    guiderInfo.isCalibrating = true;
    guiderInfo.state = STATE_CALIBRATING_RA;
    calibrationInfo.isActive = true;
    calibrationInfo.state = CALIBRATION_STATE_GOTO_PLUS_RA;
    calibrationInfo.startPosition = guiderInfo.currentPosition;
    calibrationInfo.stepsCompleted = 0;
    
    return true;
}

bool GuidingHardwareSimulator::UpdateCalibration() {
    if (!calibrationInfo.isActive) {
        return false;
    }
    
    calibrationInfo.stepsCompleted++;
    
    // Simulate calibration progression
    switch (calibrationInfo.state) {
        case CALIBRATION_STATE_GOTO_PLUS_RA:
            if (calibrationInfo.stepsCompleted >= 10) {
                calibrationInfo.state = CALIBRATION_STATE_GOTO_MINUS_RA;
                guiderInfo.state = STATE_CALIBRATING_RA;
            }
            break;
        case CALIBRATION_STATE_GOTO_MINUS_RA:
            if (calibrationInfo.stepsCompleted >= 20) {
                calibrationInfo.state = CALIBRATION_STATE_GOTO_PLUS_DEC;
                guiderInfo.state = STATE_CALIBRATING_DEC;
            }
            break;
        case CALIBRATION_STATE_GOTO_PLUS_DEC:
            if (calibrationInfo.stepsCompleted >= 30) {
                calibrationInfo.state = CALIBRATION_STATE_GOTO_MINUS_DEC;
                guiderInfo.state = STATE_CALIBRATING_DEC;
            }
            break;
        case CALIBRATION_STATE_GOTO_MINUS_DEC:
            if (calibrationInfo.stepsCompleted >= 40) {
                return CompleteCalibration();
            }
            break;
        default:
            break;
    }
    
    return true;
}

bool GuidingHardwareSimulator::CompleteCalibration() {
    if (!calibrationInfo.isActive) {
        return false;
    }
    
    // Calculate calibration results
    calibrationInfo.raAngle = 0.0; // Horizontal
    calibrationInfo.decAngle = M_PI / 2.0; // Vertical
    calibrationInfo.raRate = 1.0; // pixels per second
    calibrationInfo.decRate = 1.0; // pixels per second
    
    calibrationInfo.isActive = false;
    calibrationInfo.state = CALIBRATION_STATE_COMPLETE;
    guiderInfo.isCalibrating = false;
    guiderInfo.state = STATE_CALIBRATED;
    
    return true;
}

void GuidingHardwareSimulator::AbortCalibration() {
    calibrationInfo.isActive = false;
    calibrationInfo.state = CALIBRATION_STATE_CLEARED;
    guiderInfo.isCalibrating = false;
    guiderInfo.state = STATE_SELECTED;
}

bool GuidingHardwareSimulator::ConnectMount() {
    if (mountInfo.shouldFail) {
        mountInfo.lastError = "Mount connection failed";
        return false;
    }
    
    mountInfo.isConnected = true;
    mountInfo.lastError = "";
    
    return true;
}

bool GuidingHardwareSimulator::DisconnectMount() {
    mountInfo.isConnected = false;
    mountInfo.isGuiding = false;
    
    return true;
}

bool GuidingHardwareSimulator::SendGuidePulse(int direction, int duration) {
    if (!mountInfo.isConnected || mountInfo.shouldFail) {
        mountInfo.lastError = "Cannot send guide pulse";
        return false;
    }
    
    isGuidePulseActive = true;
    guidePulseDirection = direction;
    guidePulseDuration = duration;
    guidePulseStartTime = wxDateTime::Now();
    mountInfo.isGuiding = true;
    
    return true;
}

bool GuidingHardwareSimulator::IsGuidePulseActive() const {
    return isGuidePulseActive;
}

void GuidingHardwareSimulator::UpdateGuidePulse(double deltaTime) {
    if (!isGuidePulseActive) return;
    
    wxTimeSpan elapsed = wxDateTime::Now() - guidePulseStartTime;
    if (elapsed.GetMilliseconds() >= guidePulseDuration) {
        isGuidePulseActive = false;
        mountInfo.isGuiding = false;
    }
}

void GuidingHardwareSimulator::SetGuiderError(bool error) {
    guiderInfo.shouldFail = error;
    if (error) {
        guiderInfo.lastError = "Guider error simulated";
    } else {
        guiderInfo.lastError = "";
    }
}

void GuidingHardwareSimulator::SetStarError(bool error) {
    starInfo.shouldFail = error;
    if (error) {
        starInfo.isLost = true;
    }
}

void GuidingHardwareSimulator::SetCalibrationError(bool error) {
    calibrationInfo.shouldFail = error;
}

void GuidingHardwareSimulator::SetMountError(bool error) {
    mountInfo.shouldFail = error;
    if (error) {
        mountInfo.lastError = "Mount error simulated";
    } else {
        mountInfo.lastError = "";
    }
}

void GuidingHardwareSimulator::Reset() {
    guiderInfo = GuiderInfo();
    starInfo = StarInfo();
    calibrationInfo = CalibrationInfo();
    mountInfo = MountInfo();
    
    isGuidePulseActive = false;
    guidePulseDirection = -1;
    guidePulseDuration = 0;
    
    secondaryStars.clear();
    
    SetupDefaultGuider();
    SetupDefaultStar();
    SetupDefaultMount();
}

void GuidingHardwareSimulator::SetupDefaultGuider() {
    guiderInfo.maxMovePixels = 50;
    guiderInfo.boundingBox = wxRect(0, 0, 100, 100);
}

void GuidingHardwareSimulator::SetupDefaultStar() {
    starInfo.position = wxPoint(500, 500);
    starInfo.quality = 0.8;
    starInfo.snr = 10.0;
    starInfo.hfd = 2.5;
    starInfo.isValid = true;
    starInfo.isLost = false;
}

void GuidingHardwareSimulator::SetupDefaultMount() {
    mountInfo.guideRateRA = 0.5;
    mountInfo.guideRateDec = 0.5;
    mountInfo.siderealRate = 15.041;
    mountInfo.hasST4Interface = true;
}

void GuidingHardwareSimulator::GenerateTestImage(int width, int height) {
    // Placeholder for test image generation
    // In a real implementation, this would create a synthetic image
}

void GuidingHardwareSimulator::AddStarToImage(const wxPoint& position, double brightness) {
    // Placeholder for adding synthetic stars to test images
}

void GuidingHardwareSimulator::AddNoiseToImage(double level) {
    // Placeholder for adding noise to test images
}

void GuidingHardwareSimulator::InitializeDefaults() {
    Reset();
}

void GuidingHardwareSimulator::UpdateGuiderState() {
    // Update guider state based on current conditions
    if (!guiderInfo.isConnected) {
        guiderInfo.state = STATE_UNINITIALIZED;
    } else if (guiderInfo.isCalibrating) {
        // State is managed by calibration process
    } else if (guiderInfo.isGuiding) {
        guiderInfo.state = STATE_GUIDING;
    } else if (guiderInfo.isLocked) {
        guiderInfo.state = STATE_SELECTED;
    } else {
        guiderInfo.state = STATE_SELECTING;
    }
}

void GuidingHardwareSimulator::UpdateCalibrationState() {
    // Update calibration state based on progress
    if (calibrationInfo.isActive) {
        UpdateCalibration();
    }
}

wxPoint GuidingHardwareSimulator::CalculateStarMovement(double raOffset, double decOffset) const {
    // Convert RA/Dec offsets to pixel coordinates
    // This is a simplified calculation for testing
    int deltaX = static_cast<int>(raOffset * cos(calibrationInfo.raAngle));
    int deltaY = static_cast<int>(decOffset * sin(calibrationInfo.decAngle));
    
    return wxPoint(deltaX, deltaY);
}

// MockGuidingHardwareManager implementation
void MockGuidingHardwareManager::SetupMocks() {
    // Create all mock instances
    mockHardware = new MockGuidingHardware();
    mockStarDetector = new MockStarDetector();
    mockMount = new MockMountInterface();
    
    // Set static instances
    MockGuidingHardware::SetInstance(mockHardware);
    MockStarDetector::SetInstance(mockStarDetector);
    MockMountInterface::SetInstance(mockMount);
    
    // Create simulator
    simulator = std::make_unique<GuidingHardwareSimulator>();
    simulator->SetupDefaultGuider();
    simulator->SetupDefaultStar();
    simulator->SetupDefaultMount();
}

void MockGuidingHardwareManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockHardware;
    delete mockStarDetector;
    delete mockMount;
    
    // Reset pointers
    mockHardware = nullptr;
    mockStarDetector = nullptr;
    mockMount = nullptr;
    
    // Reset static instances
    MockGuidingHardware::SetInstance(nullptr);
    MockStarDetector::SetInstance(nullptr);
    MockMountInterface::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockGuidingHardwareManager::ResetMocks() {
    if (mockHardware) {
        testing::Mock::VerifyAndClearExpectations(mockHardware);
    }
    if (mockStarDetector) {
        testing::Mock::VerifyAndClearExpectations(mockStarDetector);
    }
    if (mockMount) {
        testing::Mock::VerifyAndClearExpectations(mockMount);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockGuidingHardware* MockGuidingHardwareManager::GetMockHardware() { return mockHardware; }
MockStarDetector* MockGuidingHardwareManager::GetMockStarDetector() { return mockStarDetector; }
MockMountInterface* MockGuidingHardwareManager::GetMockMount() { return mockMount; }
GuidingHardwareSimulator* MockGuidingHardwareManager::GetSimulator() { return simulator.get(); }

void MockGuidingHardwareManager::SetupConnectedGuider() {
    if (simulator) {
        simulator->ConnectGuider();
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, Connect())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockGuidingHardwareManager::SetupLockedGuider() {
    SetupConnectedGuider();
    
    if (simulator) {
        simulator->SetLockPosition(wxPoint(500, 500));
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, IsLocked())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, GetLockPosition())
            .WillRepeatedly(::testing::Return(wxPoint(500, 500)));
    }
}

void MockGuidingHardwareManager::SetupGuidingScenario() {
    SetupLockedGuider();
    
    if (simulator) {
        simulator->StartGuiding();
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, IsGuiding())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, StartGuiding())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockGuidingHardwareManager::SetupCalibrationScenario() {
    SetupLockedGuider();
    
    if (simulator) {
        simulator->BeginCalibration();
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, IsCalibrating())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, BeginCalibration())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockGuidingHardwareManager::SetupMultiStarScenario() {
    SetupLockedGuider();
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, GetMultiStarMode())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, GetStarCount())
            .WillRepeatedly(::testing::Return(3));
    }
}

void MockGuidingHardwareManager::SimulateGuidingFailure() {
    if (simulator) {
        simulator->SetGuiderError(true);
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, StartGuiding())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, IsGuiding())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("Guiding error")));
    }
}

void MockGuidingHardwareManager::SimulateStarLoss() {
    if (simulator) {
        simulator->SetStarError(true);
    }
    
    if (mockStarDetector) {
        EXPECT_CALL(*mockStarDetector, IsStarLost(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockStarDetector, FindStar(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
}

void MockGuidingHardwareManager::SimulateCalibrationFailure() {
    SetupConnectedGuider();
    
    if (simulator) {
        simulator->SetCalibrationError(true);
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, BeginCalibration())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, IsCalibrating())
            .WillRepeatedly(::testing::Return(false));
    }
}
