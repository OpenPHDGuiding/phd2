/*
 * mock_phd_core.cpp
 * PHD Guiding - Core Module Tests
 *
 * Implementation of mock PHD core objects
 */

#include "mock_phd_core.h"

// Static instance declarations
MockPHDApp* MockPHDApp::instance = nullptr;
MockCamera* MockCamera::instance = nullptr;
MockMount* MockMount::instance = nullptr;
MockGuidingAlgorithm* MockGuidingAlgorithm::instance = nullptr;

// MockPHDCoreManager static members
MockPHDApp* MockPHDCoreManager::mockApp = nullptr;
MockCamera* MockPHDCoreManager::mockCamera = nullptr;
MockMount* MockPHDCoreManager::mockMount = nullptr;
MockGuidingAlgorithm* MockPHDCoreManager::mockAlgorithm = nullptr;
std::unique_ptr<PHDCoreSimulator> MockPHDCoreManager::simulator = nullptr;

// MockPHDApp implementation
MockPHDApp* MockPHDApp::GetInstance() {
    return instance;
}

void MockPHDApp::SetInstance(MockPHDApp* inst) {
    instance = inst;
}

// MockCamera implementation
MockCamera* MockCamera::GetInstance() {
    return instance;
}

void MockCamera::SetInstance(MockCamera* inst) {
    instance = inst;
}

// MockMount implementation
MockMount* MockMount::GetInstance() {
    return instance;
}

void MockMount::SetInstance(MockMount* inst) {
    instance = inst;
}

// MockGuidingAlgorithm implementation
MockGuidingAlgorithm* MockGuidingAlgorithm::GetInstance() {
    return instance;
}

void MockGuidingAlgorithm::SetInstance(MockGuidingAlgorithm* inst) {
    instance = inst;
}

// PHDCoreSimulator implementation
void PHDCoreSimulator::SetupCamera(const CameraInfo& info) {
    cameraInfo = info;
}

void PHDCoreSimulator::SetupMount(const MountInfo& info) {
    mountInfo = info;
}

void PHDCoreSimulator::SetupApp(const AppInfo& info) {
    appInfo = info;
}

void PHDCoreSimulator::SetupAlgorithm(const AlgorithmInfo& info) {
    algorithmInfo = info;
}

PHDCoreSimulator::CameraInfo PHDCoreSimulator::GetCameraInfo() const {
    return cameraInfo;
}

PHDCoreSimulator::MountInfo PHDCoreSimulator::GetMountInfo() const {
    return mountInfo;
}

PHDCoreSimulator::AppInfo PHDCoreSimulator::GetAppInfo() const {
    return appInfo;
}

PHDCoreSimulator::AlgorithmInfo PHDCoreSimulator::GetAlgorithmInfo() const {
    return algorithmInfo;
}

bool PHDCoreSimulator::ConnectCamera() {
    if (cameraInfo.shouldFail) {
        return false;
    }
    
    cameraInfo.isConnected = true;
    return true;
}

bool PHDCoreSimulator::DisconnectCamera() {
    cameraInfo.isConnected = false;
    return true;
}

bool PHDCoreSimulator::CaptureImage(int duration, bool subframe) {
    if (!cameraInfo.isConnected || cameraInfo.shouldFail) {
        return false;
    }
    
    // Simulate capture time (in real implementation would be asynchronous)
    return true;
}

bool PHDCoreSimulator::ConnectMount() {
    if (mountInfo.shouldFail) {
        return false;
    }
    
    mountInfo.isConnected = true;
    return true;
}

bool PHDCoreSimulator::DisconnectMount() {
    mountInfo.isConnected = false;
    mountInfo.isCalibrated = false;
    return true;
}

int PHDCoreSimulator::GuideMount(int direction, int duration) {
    if (!mountInfo.isConnected || mountInfo.shouldFail || !mountInfo.guidingEnabled) {
        return 1; // Error
    }
    
    auto it = mountInfo.moveResults.find(direction);
    if (it != mountInfo.moveResults.end()) {
        return it->second;
    }
    
    return 0; // Success
}

bool PHDCoreSimulator::StartCalibration() {
    if (!cameraInfo.isConnected || !mountInfo.isConnected || appInfo.shouldFail) {
        return false;
    }
    
    if (!ValidateStateTransition(appInfo.state, STATE_CALIBRATING)) {
        return false;
    }
    
    appInfo.state = STATE_CALIBRATING;
    mountInfo.isCalibrated = false;
    return true;
}

bool PHDCoreSimulator::CompleteCalibration() {
    if (appInfo.state != STATE_CALIBRATING || mountInfo.shouldFail) {
        return false;
    }
    
    mountInfo.isCalibrated = true;
    appInfo.state = STATE_SELECTED;
    return true;
}

bool PHDCoreSimulator::StartGuiding() {
    if (!mountInfo.isCalibrated || appInfo.shouldFail) {
        return false;
    }
    
    if (!ValidateStateTransition(appInfo.state, STATE_GUIDING)) {
        return false;
    }
    
    appInfo.state = STATE_GUIDING;
    ResetAlgorithm();
    return true;
}

bool PHDCoreSimulator::StopGuiding() {
    if (appInfo.state != STATE_GUIDING && appInfo.state != STATE_PAUSED) {
        return false;
    }
    
    appInfo.state = STATE_SELECTED;
    return true;
}

bool PHDCoreSimulator::PauseGuiding() {
    if (appInfo.state != STATE_GUIDING) {
        return false;
    }
    
    appInfo.state = STATE_PAUSED;
    return true;
}

bool PHDCoreSimulator::ResumeGuiding() {
    if (appInfo.state != STATE_PAUSED) {
        return false;
    }
    
    appInfo.state = STATE_GUIDING;
    return true;
}

double PHDCoreSimulator::CalculateGuideCorrection(double error, double dt) {
    if (algorithmInfo.shouldFail) {
        return 0.0;
    }
    
    // Simple proportional algorithm simulation
    double correction = error * (algorithmInfo.aggressiveness / 100.0);
    
    // Apply min/max limits
    if (std::abs(correction) < algorithmInfo.minMove) {
        correction = 0.0;
    } else if (std::abs(correction) > algorithmInfo.maxMove) {
        correction = (correction > 0) ? algorithmInfo.maxMove : -algorithmInfo.maxMove;
    }
    
    // Add to history
    algorithmInfo.history.push_back(correction);
    if (algorithmInfo.history.size() > 100) {
        algorithmInfo.history.erase(algorithmInfo.history.begin());
    }
    
    return correction;
}

void PHDCoreSimulator::ResetAlgorithm() {
    algorithmInfo.history.clear();
}

void PHDCoreSimulator::SetAppState(AppState newState) {
    if (ValidateStateTransition(appInfo.state, newState)) {
        appInfo.state = newState;
    }
}

void PHDCoreSimulator::SetCameraConnected(bool connected) {
    cameraInfo.isConnected = connected;
    if (!connected) {
        // Disconnect should stop guiding
        if (appInfo.state == STATE_GUIDING || appInfo.state == STATE_CALIBRATING) {
            appInfo.state = STATE_STOPPED;
        }
    }
}

void PHDCoreSimulator::SetMountConnected(bool connected) {
    mountInfo.isConnected = connected;
    if (!connected) {
        mountInfo.isCalibrated = false;
        // Disconnect should stop guiding
        if (appInfo.state == STATE_GUIDING || appInfo.state == STATE_CALIBRATING) {
            appInfo.state = STATE_STOPPED;
        }
    }
}

void PHDCoreSimulator::SetMountCalibrated(bool calibrated) {
    mountInfo.isCalibrated = calibrated;
    if (!calibrated && appInfo.state == STATE_GUIDING) {
        appInfo.state = STATE_SELECTED;
    }
}

void PHDCoreSimulator::SetCameraError(bool error) {
    cameraInfo.shouldFail = error;
}

void PHDCoreSimulator::SetMountError(bool error) {
    mountInfo.shouldFail = error;
}

void PHDCoreSimulator::SetAppError(bool error) {
    appInfo.shouldFail = error;
}

void PHDCoreSimulator::SetAlgorithmError(bool error) {
    algorithmInfo.shouldFail = error;
}

void PHDCoreSimulator::Reset() {
    cameraInfo = CameraInfo();
    mountInfo = MountInfo();
    appInfo = AppInfo();
    algorithmInfo = AlgorithmInfo();
    
    SetupDefaultComponents();
}

void PHDCoreSimulator::SetupDefaultComponents() {
    // Set up default camera
    cameraInfo.name = "Simulator";
    cameraInfo.fullSize = wxSize(1024, 768);
    cameraInfo.binning = 1;
    cameraInfo.gain = 50;
    cameraInfo.hasShutter = true;
    cameraInfo.hasGainControl = true;
    cameraInfo.hasST4 = true;
    
    // Set up default mount
    mountInfo.name = "On-camera";
    mountInfo.calibrationAngle = 0.0;
    mountInfo.calibrationRate = 1.0;
    mountInfo.guidingEnabled = true;
    
    // Set default move results
    mountInfo.moveResults[0] = 0; // North - success
    mountInfo.moveResults[1] = 0; // South - success
    mountInfo.moveResults[2] = 0; // East - success
    mountInfo.moveResults[3] = 0; // West - success
    
    // Set up default algorithm
    algorithmInfo.name = "Hysteresis";
    algorithmInfo.minMove = 0.15;
    algorithmInfo.maxMove = 5.0;
    algorithmInfo.aggressiveness = 100.0;
}

bool PHDCoreSimulator::ValidateStateTransition(AppState fromState, AppState toState) {
    // Define valid state transitions
    switch (fromState) {
        case STATE_STOPPED:
            return (toState == STATE_SELECTED || toState == STATE_LOOPING);
        case STATE_SELECTED:
            return (toState == STATE_STOPPED || toState == STATE_CALIBRATING || 
                   toState == STATE_GUIDING || toState == STATE_LOOPING);
        case STATE_CALIBRATING:
            return (toState == STATE_SELECTED || toState == STATE_STOPPED);
        case STATE_GUIDING:
            return (toState == STATE_SELECTED || toState == STATE_STOPPED || 
                   toState == STATE_PAUSED);
        case STATE_LOOPING:
            return (toState == STATE_STOPPED || toState == STATE_SELECTED || 
                   toState == STATE_CALIBRATING || toState == STATE_GUIDING);
        case STATE_PAUSED:
            return (toState == STATE_GUIDING || toState == STATE_SELECTED || 
                   toState == STATE_STOPPED);
        default:
            return false;
    }
}

// MockPHDCoreManager implementation
void MockPHDCoreManager::SetupMocks() {
    // Create all mock instances
    mockApp = new MockPHDApp();
    mockCamera = new MockCamera();
    mockMount = new MockMount();
    mockAlgorithm = new MockGuidingAlgorithm();
    
    // Set static instances
    MockPHDApp::SetInstance(mockApp);
    MockCamera::SetInstance(mockCamera);
    MockMount::SetInstance(mockMount);
    MockGuidingAlgorithm::SetInstance(mockAlgorithm);
    
    // Create simulator
    simulator = std::make_unique<PHDCoreSimulator>();
    simulator->SetupDefaultComponents();
}

void MockPHDCoreManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockApp;
    delete mockCamera;
    delete mockMount;
    delete mockAlgorithm;
    
    // Reset pointers
    mockApp = nullptr;
    mockCamera = nullptr;
    mockMount = nullptr;
    mockAlgorithm = nullptr;
    
    // Reset static instances
    MockPHDApp::SetInstance(nullptr);
    MockCamera::SetInstance(nullptr);
    MockMount::SetInstance(nullptr);
    MockGuidingAlgorithm::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockPHDCoreManager::ResetMocks() {
    if (mockApp) {
        testing::Mock::VerifyAndClearExpectations(mockApp);
    }
    if (mockCamera) {
        testing::Mock::VerifyAndClearExpectations(mockCamera);
    }
    if (mockMount) {
        testing::Mock::VerifyAndClearExpectations(mockMount);
    }
    if (mockAlgorithm) {
        testing::Mock::VerifyAndClearExpectations(mockAlgorithm);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockPHDApp* MockPHDCoreManager::GetMockApp() { return mockApp; }
MockCamera* MockPHDCoreManager::GetMockCamera() { return mockCamera; }
MockMount* MockPHDCoreManager::GetMockMount() { return mockMount; }
MockGuidingAlgorithm* MockPHDCoreManager::GetMockAlgorithm() { return mockAlgorithm; }
PHDCoreSimulator* MockPHDCoreManager::GetSimulator() { return simulator.get(); }

void MockPHDCoreManager::SetupConnectedEquipment() {
    if (simulator) {
        simulator->ConnectCamera();
        simulator->ConnectMount();
    }
    
    if (mockCamera) {
        EXPECT_CALL(*mockCamera, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockCamera, Connect())
            .WillRepeatedly(::testing::Return(true));
    }
    
    if (mockMount) {
        EXPECT_CALL(*mockMount, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockMount, Connect())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockPHDCoreManager::SetupCalibratedMount() {
    SetupConnectedEquipment();
    
    if (simulator) {
        simulator->SetMountCalibrated(true);
    }
    
    if (mockMount) {
        EXPECT_CALL(*mockMount, IsCalibrated())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockMount, GetCalibrationAngle())
            .WillRepeatedly(::testing::Return(45.0));
        EXPECT_CALL(*mockMount, GetCalibrationRate())
            .WillRepeatedly(::testing::Return(1.0));
    }
}

void MockPHDCoreManager::SetupGuidingSession() {
    SetupCalibratedMount();
    
    if (simulator) {
        simulator->StartGuiding();
    }
    
    if (mockApp) {
        EXPECT_CALL(*mockApp, IsGuiding())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockApp, GetState())
            .WillRepeatedly(::testing::Return(static_cast<int>(PHDCoreSimulator::STATE_GUIDING)));
    }
}

void MockPHDCoreManager::SimulateEquipmentFailure() {
    if (simulator) {
        simulator->SetCameraError(true);
        simulator->SetMountError(true);
    }
    
    if (mockCamera) {
        EXPECT_CALL(*mockCamera, Connect())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockCamera, Capture(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
    
    if (mockMount) {
        EXPECT_CALL(*mockMount, Connect())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockMount, Guide(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(1)); // Error
    }
}

void MockPHDCoreManager::SimulateCalibrationFailure() {
    SetupConnectedEquipment();
    
    if (simulator) {
        simulator->SetMountError(true);
    }
    
    if (mockMount) {
        EXPECT_CALL(*mockMount, Guide(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(1)); // Calibration move failure
    }
}
