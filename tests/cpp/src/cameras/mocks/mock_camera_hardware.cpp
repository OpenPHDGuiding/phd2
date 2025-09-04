/*
 * mock_camera_hardware.cpp
 * PHD Guiding - Camera Module Tests
 *
 * Implementation of mock camera hardware objects
 */

#include "mock_camera_hardware.h"
#include <cmath>
#include <random>

// Static instance declarations
MockCameraHardware* MockCameraHardware::instance = nullptr;
MockImageProcessor* MockImageProcessor::instance = nullptr;
MockCameraConfig* MockCameraConfig::instance = nullptr;

// MockCameraHardwareManager static members
MockCameraHardware* MockCameraHardwareManager::mockHardware = nullptr;
MockImageProcessor* MockCameraHardwareManager::mockProcessor = nullptr;
MockCameraConfig* MockCameraHardwareManager::mockConfig = nullptr;
std::unique_ptr<CameraHardwareSimulator> MockCameraHardwareManager::simulator = nullptr;

// MockCameraHardware implementation
MockCameraHardware* MockCameraHardware::GetInstance() {
    return instance;
}

void MockCameraHardware::SetInstance(MockCameraHardware* inst) {
    instance = inst;
}

// MockImageProcessor implementation
MockImageProcessor* MockImageProcessor::GetInstance() {
    return instance;
}

void MockImageProcessor::SetInstance(MockImageProcessor* inst) {
    instance = inst;
}

// MockCameraConfig implementation
MockCameraConfig* MockCameraConfig::GetInstance() {
    return instance;
}

void MockCameraConfig::SetInstance(MockCameraConfig* inst) {
    instance = inst;
}

// CameraHardwareSimulator implementation
void CameraHardwareSimulator::SetupCamera(const CameraInfo& info) {
    cameraInfo = info;
}

void CameraHardwareSimulator::SetupCapture(const CaptureInfo& info) {
    captureInfo = info;
}

void CameraHardwareSimulator::SetupImage(const ImageInfo& info) {
    imageInfo = info;
}

CameraHardwareSimulator::CameraInfo CameraHardwareSimulator::GetCameraInfo() const {
    return cameraInfo;
}

CameraHardwareSimulator::CaptureInfo CameraHardwareSimulator::GetCaptureInfo() const {
    return captureInfo;
}

CameraHardwareSimulator::ImageInfo CameraHardwareSimulator::GetImageInfo() const {
    return imageInfo;
}

bool CameraHardwareSimulator::ConnectCamera(const wxString& cameraId) {
    if (cameraInfo.shouldFail) {
        cameraInfo.lastError = "Connection failed";
        return false;
    }
    
    cameraInfo.isConnected = true;
    cameraInfo.id = cameraId;
    cameraInfo.lastError = "";
    return true;
}

bool CameraHardwareSimulator::DisconnectCamera() {
    cameraInfo.isConnected = false;
    captureInfo.isCapturing = false;
    isPulseGuiding = false;
    return true;
}

bool CameraHardwareSimulator::IsConnected() const {
    return cameraInfo.isConnected;
}

bool CameraHardwareSimulator::StartCapture(int duration, int options, const wxRect& subframe) {
    if (!cameraInfo.isConnected || captureInfo.shouldFail) {
        cameraInfo.lastError = "Cannot start capture";
        return false;
    }
    
    captureInfo.isCapturing = true;
    captureInfo.exposureDuration = duration;
    captureInfo.captureOptions = options;
    captureInfo.subframe = subframe;
    captureInfo.captureStartTime = wxDateTime::Now();
    
    // Determine capture mode from options
    if (options & 0x01) { // CAPTURE_SUBTRACT_DARK
        captureInfo.mode = MODE_DARK;
    } else {
        captureInfo.mode = MODE_NORMAL;
    }
    
    return true;
}

bool CameraHardwareSimulator::IsCapturing() const {
    return captureInfo.isCapturing;
}

void CameraHardwareSimulator::UpdateCapture(double deltaTime) {
    if (!captureInfo.isCapturing) return;
    
    wxTimeSpan elapsed = wxDateTime::Now() - captureInfo.captureStartTime;
    if (elapsed.GetMilliseconds() >= captureInfo.exposureDuration) {
        // Capture should be complete
        // In real implementation, this would be handled by CompleteCapture
    }
}

bool CameraHardwareSimulator::CompleteCapture(void* img) {
    if (!captureInfo.isCapturing) {
        return false;
    }
    
    captureInfo.isCapturing = false;
    
    // Generate test image data
    wxSize captureSize = captureInfo.subframe.IsEmpty() ? cameraInfo.frameSize : captureInfo.subframe.GetSize();
    
    // Simulate image generation based on exposure duration and settings
    double exposureFactor = std::min(captureInfo.exposureDuration / 1000.0, 10.0); // Cap at 10 seconds
    double baseMean = 1000.0 * exposureFactor * (cameraInfo.gain / 100.0);
    double noiseLevel = 50.0 + (cameraInfo.gain / 10.0);
    
    imageInfo.size = captureSize;
    imageInfo.data.resize(captureSize.x * captureSize.y);
    
    // Generate realistic image data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> noise(baseMean, noiseLevel);
    
    for (size_t i = 0; i < imageInfo.data.size(); i++) {
        double value = noise(gen);
        value = std::max(0.0, std::min(value, 65535.0));
        imageInfo.data[i] = static_cast<unsigned short>(value);
    }
    
    CalculateImageStats(imageInfo.data, imageInfo.size);
    imageInfo.hasValidData = true;
    
    return true;
}

bool CameraHardwareSimulator::AbortCapture() {
    captureInfo.isCapturing = false;
    return true;
}

bool CameraHardwareSimulator::SetGain(int gain) {
    if (gain < cameraInfo.minGain || gain > cameraInfo.maxGain) {
        return false;
    }
    
    cameraInfo.gain = gain;
    return true;
}

int CameraHardwareSimulator::GetGain() const {
    return cameraInfo.gain;
}

bool CameraHardwareSimulator::SetBinning(int binning) {
    if (binning < 1 || binning > cameraInfo.maxBinning) {
        return false;
    }
    
    cameraInfo.binning = binning;
    
    // Update frame size based on binning
    cameraInfo.frameSize = wxSize(
        cameraInfo.maxFrameSize.x / binning,
        cameraInfo.maxFrameSize.y / binning
    );
    
    return true;
}

int CameraHardwareSimulator::GetBinning() const {
    return cameraInfo.binning;
}

bool CameraHardwareSimulator::SetCoolerOn(bool on) {
    if (!cameraInfo.hasCooler) {
        return false;
    }
    
    cameraInfo.coolerOn = on;
    return true;
}

bool CameraHardwareSimulator::SetCoolerSetpoint(double temperature) {
    if (!cameraInfo.hasCooler) {
        return false;
    }
    
    cameraInfo.coolerSetpoint = temperature;
    return true;
}

void CameraHardwareSimulator::UpdateTemperature(double deltaTime) {
    if (!cameraInfo.hasCooler || !cameraInfo.coolerOn) {
        // Ambient temperature drift
        cameraInfo.sensorTemperature += (20.0 - cameraInfo.sensorTemperature) * deltaTime * 0.01;
        return;
    }
    
    // Simulate cooling towards setpoint
    double tempDiff = cameraInfo.coolerSetpoint - cameraInfo.sensorTemperature;
    double coolingRate = 0.1; // degrees per second
    
    if (std::abs(tempDiff) > 0.1) {
        double change = std::copysign(coolingRate * deltaTime, tempDiff);
        cameraInfo.sensorTemperature += change;
    }
}

bool CameraHardwareSimulator::StartPulseGuide(int direction, int duration) {
    if (!cameraInfo.isConnected || cameraInfo.shouldFail) {
        cameraInfo.lastError = "Cannot pulse guide";
        return false;
    }
    
    if (direction < 0 || direction > 3 || duration <= 0) {
        cameraInfo.lastError = "Invalid guide parameters";
        return false;
    }
    
    isPulseGuiding = true;
    pulseDirection = direction;
    pulseDuration = duration;
    pulseStartTime = wxDateTime::Now();
    
    return true;
}

bool CameraHardwareSimulator::IsPulseGuiding() const {
    return isPulseGuiding;
}

void CameraHardwareSimulator::UpdatePulseGuide(double deltaTime) {
    if (!isPulseGuiding) return;
    
    wxTimeSpan elapsed = wxDateTime::Now() - pulseStartTime;
    if (elapsed.GetMilliseconds() >= pulseDuration) {
        isPulseGuiding = false;
    }
}

void CameraHardwareSimulator::SetCameraError(bool error) {
    cameraInfo.shouldFail = error;
    if (error) {
        cameraInfo.lastError = "Camera error simulated";
    } else {
        cameraInfo.lastError = "";
    }
}

void CameraHardwareSimulator::SetCaptureError(bool error) {
    captureInfo.shouldFail = error;
}

void CameraHardwareSimulator::SetConnectionError(bool error) {
    if (error) {
        cameraInfo.isConnected = false;
        cameraInfo.lastError = "Connection error";
    }
}

void CameraHardwareSimulator::Reset() {
    cameraInfo = CameraInfo();
    captureInfo = CaptureInfo();
    imageInfo = ImageInfo();
    
    availableCameraNames.Clear();
    availableCameraIds.Clear();
    
    isPulseGuiding = false;
    pulseDirection = -1;
    pulseDuration = 0;
    
    SetupDefaultCamera();
}

void CameraHardwareSimulator::SetupDefaultCamera() {
    // Set up default camera
    cameraInfo.type = CAMERA_SIMULATOR;
    cameraInfo.name = "Camera Simulator";
    cameraInfo.id = "SIM001";
    cameraInfo.hasNonGuiCapture = true;
    cameraInfo.bitsPerPixel = 16;
    cameraInfo.hasSubframes = true;
    cameraInfo.hasGainControl = true;
    cameraInfo.hasShutter = false;
    cameraInfo.hasCooler = false;
    cameraInfo.canSelectCamera = false;
    cameraInfo.frameSize = wxSize(1280, 1024);
    cameraInfo.maxFrameSize = wxSize(1280, 1024);
    cameraInfo.binning = 1;
    cameraInfo.maxBinning = 4;
    cameraInfo.gain = 50;
    cameraInfo.minGain = 0;
    cameraInfo.maxGain = 100;
    cameraInfo.defaultGain = 50;
    cameraInfo.pixelSize = 5.2;
    
    // Add default available cameras
    AddAvailableCamera("Camera Simulator", "SIM001");
    AddAvailableCamera("Test Camera 1", "TEST001");
    AddAvailableCamera("Test Camera 2", "TEST002");
}

void CameraHardwareSimulator::AddAvailableCamera(const wxString& name, const wxString& id) {
    availableCameraNames.Add(name);
    availableCameraIds.Add(id);
}

wxArrayString CameraHardwareSimulator::GetAvailableCameraNames() const {
    return availableCameraNames;
}

wxArrayString CameraHardwareSimulator::GetAvailableCameraIds() const {
    return availableCameraIds;
}

void CameraHardwareSimulator::ClearAvailableCameras() {
    availableCameraNames.Clear();
    availableCameraIds.Clear();
}

void CameraHardwareSimulator::InitializeDefaults() {
    Reset();
}

void CameraHardwareSimulator::CalculateImageStats(const std::vector<unsigned short>& data, const wxSize& size) {
    if (data.empty()) {
        imageInfo.mean = 0.0;
        imageInfo.stddev = 0.0;
        imageInfo.minValue = 0;
        imageInfo.maxValue = 0;
        return;
    }
    
    // Calculate mean
    double sum = 0.0;
    unsigned short minVal = 65535;
    unsigned short maxVal = 0;
    
    for (unsigned short value : data) {
        sum += value;
        minVal = std::min(minVal, value);
        maxVal = std::max(maxVal, value);
    }
    
    imageInfo.mean = sum / data.size();
    imageInfo.minValue = minVal;
    imageInfo.maxValue = maxVal;
    
    // Calculate standard deviation
    double sumSquares = 0.0;
    for (unsigned short value : data) {
        double diff = value - imageInfo.mean;
        sumSquares += diff * diff;
    }
    
    imageInfo.stddev = std::sqrt(sumSquares / data.size());
}

// MockCameraHardwareManager implementation
void MockCameraHardwareManager::SetupMocks() {
    // Create all mock instances
    mockHardware = new MockCameraHardware();
    mockProcessor = new MockImageProcessor();
    mockConfig = new MockCameraConfig();
    
    // Set static instances
    MockCameraHardware::SetInstance(mockHardware);
    MockImageProcessor::SetInstance(mockProcessor);
    MockCameraConfig::SetInstance(mockConfig);
    
    // Create simulator
    simulator = std::make_unique<CameraHardwareSimulator>();
    simulator->SetupDefaultCamera();
}

void MockCameraHardwareManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockHardware;
    delete mockProcessor;
    delete mockConfig;
    
    // Reset pointers
    mockHardware = nullptr;
    mockProcessor = nullptr;
    mockConfig = nullptr;
    
    // Reset static instances
    MockCameraHardware::SetInstance(nullptr);
    MockImageProcessor::SetInstance(nullptr);
    MockCameraConfig::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockCameraHardwareManager::ResetMocks() {
    if (mockHardware) {
        testing::Mock::VerifyAndClearExpectations(mockHardware);
    }
    if (mockProcessor) {
        testing::Mock::VerifyAndClearExpectations(mockProcessor);
    }
    if (mockConfig) {
        testing::Mock::VerifyAndClearExpectations(mockConfig);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockCameraHardware* MockCameraHardwareManager::GetMockHardware() { return mockHardware; }
MockImageProcessor* MockCameraHardwareManager::GetMockProcessor() { return mockProcessor; }
MockCameraConfig* MockCameraHardwareManager::GetMockConfig() { return mockConfig; }
CameraHardwareSimulator* MockCameraHardwareManager::GetSimulator() { return simulator.get(); }

void MockCameraHardwareManager::SetupConnectedCamera() {
    if (simulator) {
        simulator->ConnectCamera("SIM001");
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, Connect(::testing::_))
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockCameraHardwareManager::SetupCameraWithCapabilities() {
    SetupConnectedCamera();
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, HasNonGuiCapture())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, BitsPerPixel())
            .WillRepeatedly(::testing::Return(16));
        EXPECT_CALL(*mockHardware, HasSubframes())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, HasGainControl())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, GetFrameSize())
            .WillRepeatedly(::testing::Return(wxSize(1280, 1024)));
        EXPECT_CALL(*mockHardware, GetMaxFrameSize())
            .WillRepeatedly(::testing::Return(wxSize(1280, 1024)));
    }
}

void MockCameraHardwareManager::SetupImageCapture() {
    SetupCameraWithCapabilities();
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, Capture(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false)); // false = success
    }
}

void MockCameraHardwareManager::SimulateCameraFailure() {
    if (simulator) {
        simulator->SetCameraError(true);
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, Connect(::testing::_))
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, IsConnected())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("Camera error")));
    }
}

void MockCameraHardwareManager::SimulateCaptureFailure() {
    SetupConnectedCamera();
    
    if (simulator) {
        simulator->SetCaptureError(true);
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, Capture(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(true)); // true = failure
        EXPECT_CALL(*mockHardware, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("Capture failed")));
    }
}
