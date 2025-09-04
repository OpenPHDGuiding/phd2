/*
 * mock_mount_hardware.cpp
 * PHD Guiding - Mount Module Tests
 *
 * Implementation of mock mount hardware objects
 */

#include "mock_mount_hardware.h"
#include <cmath>

// Static instance declarations
MockMountHardware* MockMountHardware::instance = nullptr;
MockCalibrationData* MockCalibrationData::instance = nullptr;
MockGuideAlgorithm* MockGuideAlgorithm::instance = nullptr;

// MockMountHardwareManager static members
MockMountHardware* MockMountHardwareManager::mockHardware = nullptr;
MockCalibrationData* MockMountHardwareManager::mockCalibration = nullptr;
MockGuideAlgorithm* MockMountHardwareManager::mockAlgorithm = nullptr;
std::unique_ptr<MountHardwareSimulator> MockMountHardwareManager::simulator = nullptr;

// MockMountHardware implementation
MockMountHardware* MockMountHardware::GetInstance() {
    return instance;
}

void MockMountHardware::SetInstance(MockMountHardware* inst) {
    instance = inst;
}

// MockCalibrationData implementation
MockCalibrationData* MockCalibrationData::GetInstance() {
    return instance;
}

void MockCalibrationData::SetInstance(MockCalibrationData* inst) {
    instance = inst;
}

// MockGuideAlgorithm implementation
MockGuideAlgorithm* MockGuideAlgorithm::GetInstance() {
    return instance;
}

void MockGuideAlgorithm::SetInstance(MockGuideAlgorithm* inst) {
    instance = inst;
}

// MountHardwareSimulator implementation
void MountHardwareSimulator::SetupMount(const MountInfo& info) {
    mountInfo = info;
}

void MountHardwareSimulator::SetupCalibration(const CalibrationInfo& info) {
    calibrationInfo = info;
}

void MountHardwareSimulator::SetupSite(const SiteInfo& info) {
    siteInfo = info;
}

MountHardwareSimulator::MountInfo MountHardwareSimulator::GetMountInfo() const {
    return mountInfo;
}

MountHardwareSimulator::CalibrationInfo MountHardwareSimulator::GetCalibrationInfo() const {
    return calibrationInfo;
}

MountHardwareSimulator::SiteInfo MountHardwareSimulator::GetSiteInfo() const {
    return siteInfo;
}

bool MountHardwareSimulator::ConnectMount() {
    if (mountInfo.shouldFail) {
        mountInfo.lastError = "Connection failed";
        return false;
    }
    
    mountInfo.isConnected = true;
    mountInfo.lastError = "";
    return true;
}

bool MountHardwareSimulator::DisconnectMount() {
    mountInfo.isConnected = false;
    mountInfo.isSlewing = false;
    mountInfo.isPulseGuiding = false;
    return true;
}

bool MountHardwareSimulator::IsConnected() const {
    return mountInfo.isConnected;
}

void MountHardwareSimulator::SetPosition(double ra, double dec) {
    mountInfo.ra = ra;
    mountInfo.dec = dec;
    
    // Update alt-az coordinates
    EquatorialToHorizontal(ra, dec, mountInfo.azimuth, mountInfo.altitude);
}

void MountHardwareSimulator::GetPosition(double& ra, double& dec) const {
    ra = mountInfo.ra;
    dec = mountInfo.dec;
}

void MountHardwareSimulator::UpdatePosition(double deltaTime) {
    if (mountInfo.isTracking && !mountInfo.isSlewing) {
        // Simulate sidereal tracking (15 arcsec/sec = 0.004167 deg/sec)
        double siderealRate = 0.004167; // degrees per second
        mountInfo.ra += siderealRate * deltaTime / 3600.0; // Convert to hours
        
        // Wrap RA around 24 hours
        while (mountInfo.ra >= 24.0) mountInfo.ra -= 24.0;
        while (mountInfo.ra < 0.0) mountInfo.ra += 24.0;
        
        // Update alt-az coordinates
        EquatorialToHorizontal(mountInfo.ra, mountInfo.dec, mountInfo.azimuth, mountInfo.altitude);
    }
}

bool MountHardwareSimulator::StartSlew(double targetRA, double targetDec) {
    if (!mountInfo.isConnected || !mountInfo.canSlew || mountInfo.shouldFail) {
        mountInfo.lastError = "Cannot slew";
        return false;
    }
    
    targetRA = targetRA;
    targetDec = targetDec;
    mountInfo.isSlewing = true;
    slewStartTime = wxDateTime::Now();
    
    return true;
}

bool MountHardwareSimulator::IsSlewing() const {
    return mountInfo.isSlewing;
}

void MountHardwareSimulator::UpdateSlew(double deltaTime) {
    if (!mountInfo.isSlewing) return;
    
    // Calculate distance to target
    double distance = CalculateAngularDistance(mountInfo.ra * 15.0, mountInfo.dec, 
                                              targetRA * 15.0, targetDec);
    
    // Simulate slew speed (1 degree per second)
    double slewSpeed = 1.0; // degrees per second
    double moveDistance = slewSpeed * deltaTime;
    
    if (distance <= moveDistance) {
        // Slew complete
        SetPosition(targetRA, targetDec);
        mountInfo.isSlewing = false;
    } else {
        // Continue slewing
        double ratio = moveDistance / distance;
        double newRA = mountInfo.ra + (targetRA - mountInfo.ra) * ratio;
        double newDec = mountInfo.dec + (targetDec - mountInfo.dec) * ratio;
        SetPosition(newRA, newDec);
    }
}

void MountHardwareSimulator::AbortSlew() {
    mountInfo.isSlewing = false;
}

bool MountHardwareSimulator::StartPulseGuide(GuideDirection direction, int duration) {
    if (!mountInfo.isConnected || !mountInfo.canPulseGuide || mountInfo.shouldFail) {
        mountInfo.lastError = "Cannot pulse guide";
        return false;
    }
    
    currentGuideDirection = direction;
    guideDuration = duration;
    mountInfo.isPulseGuiding = true;
    guideStartTime = wxDateTime::Now();
    
    return true;
}

bool MountHardwareSimulator::IsPulseGuiding() const {
    return mountInfo.isPulseGuiding;
}

void MountHardwareSimulator::UpdatePulseGuide(double deltaTime) {
    if (!mountInfo.isPulseGuiding) return;
    
    wxTimeSpan elapsed = wxDateTime::Now() - guideStartTime;
    if (elapsed.GetMilliseconds() >= guideDuration) {
        // Pulse guide complete
        mountInfo.isPulseGuiding = false;
        
        // Apply guide correction (simplified)
        double guideRate = 0.5; // arcsec per millisecond
        double correction = guideRate * guideDuration / 3600.0; // Convert to degrees
        
        switch (currentGuideDirection) {
            case GUIDE_NORTH:
                mountInfo.dec += correction;
                break;
            case GUIDE_SOUTH:
                mountInfo.dec -= correction;
                break;
            case GUIDE_EAST:
                mountInfo.ra += correction / 15.0; // Convert to hours
                break;
            case GUIDE_WEST:
                mountInfo.ra -= correction / 15.0; // Convert to hours
                break;
        }
        
        // Update alt-az coordinates
        EquatorialToHorizontal(mountInfo.ra, mountInfo.dec, mountInfo.azimuth, mountInfo.altitude);
    }
}

bool MountHardwareSimulator::StartCalibration() {
    if (!mountInfo.isConnected || calibrationInfo.shouldFail) {
        return false;
    }
    
    calibrationInfo.steps.clear();
    calibrationInfo.isValid = false;
    return true;
}

bool MountHardwareSimulator::AddCalibrationStep(GuideDirection direction, int duration, const wxPoint& starPos) {
    if (calibrationInfo.shouldFail) {
        return false;
    }
    
    calibrationInfo.steps.push_back(starPos);
    return true;
}

bool MountHardwareSimulator::CompleteCalibration() {
    if (calibrationInfo.shouldFail || calibrationInfo.steps.size() < 4) {
        return false;
    }
    
    // Calculate calibration angle and rate (simplified)
    if (calibrationInfo.steps.size() >= 4) {
        wxPoint north = calibrationInfo.steps[0];
        wxPoint south = calibrationInfo.steps[1];
        wxPoint east = calibrationInfo.steps[2];
        wxPoint west = calibrationInfo.steps[3];
        
        // Calculate angle from north-south vector
        double dx = south.x - north.x;
        double dy = south.y - north.y;
        calibrationInfo.angle = atan2(dy, dx) * 180.0 / M_PI;
        
        // Calculate rate from distance moved
        double distance = sqrt(dx * dx + dy * dy);
        calibrationInfo.rate = distance / 1000.0; // pixels per second to arcsec per second
        
        calibrationInfo.quality = 0.9; // Good quality
        calibrationInfo.isValid = true;
    }
    
    return calibrationInfo.isValid;
}

void MountHardwareSimulator::ClearCalibration() {
    calibrationInfo.isValid = false;
    calibrationInfo.steps.clear();
    calibrationInfo.angle = 0.0;
    calibrationInfo.rate = 1.0;
    calibrationInfo.quality = 0.0;
}

void MountHardwareSimulator::SetMountError(bool error) {
    mountInfo.shouldFail = error;
    if (error) {
        mountInfo.lastError = "Mount error simulated";
    } else {
        mountInfo.lastError = "";
    }
}

void MountHardwareSimulator::SetCalibrationError(bool error) {
    calibrationInfo.shouldFail = error;
}

void MountHardwareSimulator::SetConnectionError(bool error) {
    if (error) {
        mountInfo.isConnected = false;
        mountInfo.lastError = "Connection error";
    }
}

void MountHardwareSimulator::Reset() {
    mountInfo = MountInfo();
    calibrationInfo = CalibrationInfo();
    siteInfo = SiteInfo();
    
    SetupDefaultMount();
}

void MountHardwareSimulator::SetupDefaultMount() {
    // Set up default mount
    mountInfo.type = MOUNT_SIMULATOR;
    mountInfo.name = "Simulator";
    mountInfo.canSlew = true;
    mountInfo.canPulseGuide = true;
    mountInfo.canSetTracking = true;
    
    // Set default position (RA=12h, Dec=45°)
    SetPosition(12.0, 45.0);
    
    // Set default site (Philadelphia)
    siteInfo.latitude = 40.0;
    siteInfo.longitude = -75.0;
    siteInfo.elevation = 100.0;
    siteInfo.utcTime = wxDateTime::Now();
}

void MountHardwareSimulator::EquatorialToHorizontal(double ra, double dec, double& azimuth, double& altitude) const {
    // Simplified coordinate transformation
    double lst = GetSiderealTime();
    double ha = lst - ra; // Hour angle
    
    double lat = siteInfo.latitude * M_PI / 180.0;
    double decRad = dec * M_PI / 180.0;
    double haRad = ha * 15.0 * M_PI / 180.0; // Convert hours to radians
    
    double sinAlt = sin(decRad) * sin(lat) + cos(decRad) * cos(lat) * cos(haRad);
    altitude = asin(sinAlt) * 180.0 / M_PI;
    
    double cosAz = (sin(decRad) - sin(lat) * sinAlt) / (cos(lat) * cos(asin(sinAlt)));
    azimuth = acos(cosAz) * 180.0 / M_PI;
    
    if (sin(haRad) > 0) {
        azimuth = 360.0 - azimuth;
    }
}

void MountHardwareSimulator::HorizontalToEquatorial(double azimuth, double altitude, double& ra, double& dec) const {
    // Simplified coordinate transformation (inverse of above)
    double lat = siteInfo.latitude * M_PI / 180.0;
    double altRad = altitude * M_PI / 180.0;
    double azRad = azimuth * M_PI / 180.0;
    
    double sinDec = sin(altRad) * sin(lat) + cos(altRad) * cos(lat) * cos(azRad);
    dec = asin(sinDec) * 180.0 / M_PI;
    
    double cosHA = (sin(altRad) - sin(lat) * sinDec) / (cos(lat) * cos(asin(sinDec)));
    double ha = acos(cosHA) * 180.0 / M_PI / 15.0; // Convert to hours
    
    if (sin(azRad) > 0) {
        ha = -ha;
    }
    
    double lst = GetSiderealTime();
    ra = lst - ha;
    
    // Normalize RA
    while (ra >= 24.0) ra -= 24.0;
    while (ra < 0.0) ra += 24.0;
}

double MountHardwareSimulator::GetSiderealTime() const {
    // Simplified sidereal time calculation
    wxDateTime now = wxDateTime::Now();
    double jd = now.GetJulianDayNumber();
    double t = (jd - 2451545.0) / 36525.0;
    double gmst = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * t * t;
    
    // Convert to hours and add longitude correction
    double lst = fmod(gmst / 15.0 + siteInfo.longitude / 15.0, 24.0);
    if (lst < 0) lst += 24.0;
    
    return lst;
}

double MountHardwareSimulator::CalculateAngularDistance(double ra1, double dec1, double ra2, double dec2) const {
    // Calculate angular distance using spherical law of cosines
    double ra1Rad = ra1 * M_PI / 180.0;
    double dec1Rad = dec1 * M_PI / 180.0;
    double ra2Rad = ra2 * M_PI / 180.0;
    double dec2Rad = dec2 * M_PI / 180.0;
    
    double cosDistance = sin(dec1Rad) * sin(dec2Rad) + 
                        cos(dec1Rad) * cos(dec2Rad) * cos(ra1Rad - ra2Rad);
    
    return acos(cosDistance) * 180.0 / M_PI;
}

// MockMountHardwareManager implementation
void MockMountHardwareManager::SetupMocks() {
    // Create all mock instances
    mockHardware = new MockMountHardware();
    mockCalibration = new MockCalibrationData();
    mockAlgorithm = new MockGuideAlgorithm();
    
    // Set static instances
    MockMountHardware::SetInstance(mockHardware);
    MockCalibrationData::SetInstance(mockCalibration);
    MockGuideAlgorithm::SetInstance(mockAlgorithm);
    
    // Create simulator
    simulator = std::make_unique<MountHardwareSimulator>();
    simulator->SetupDefaultMount();
}

void MockMountHardwareManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockHardware;
    delete mockCalibration;
    delete mockAlgorithm;
    
    // Reset pointers
    mockHardware = nullptr;
    mockCalibration = nullptr;
    mockAlgorithm = nullptr;
    
    // Reset static instances
    MockMountHardware::SetInstance(nullptr);
    MockCalibrationData::SetInstance(nullptr);
    MockGuideAlgorithm::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockMountHardwareManager::ResetMocks() {
    if (mockHardware) {
        testing::Mock::VerifyAndClearExpectations(mockHardware);
    }
    if (mockCalibration) {
        testing::Mock::VerifyAndClearExpectations(mockCalibration);
    }
    if (mockAlgorithm) {
        testing::Mock::VerifyAndClearExpectations(mockAlgorithm);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockMountHardware* MockMountHardwareManager::GetMockHardware() { return mockHardware; }
MockCalibrationData* MockMountHardwareManager::GetMockCalibration() { return mockCalibration; }
MockGuideAlgorithm* MockMountHardwareManager::GetMockAlgorithm() { return mockAlgorithm; }
MountHardwareSimulator* MockMountHardwareManager::GetSimulator() { return simulator.get(); }

void MockMountHardwareManager::SetupConnectedMount() {
    if (simulator) {
        simulator->ConnectMount();
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, Connect())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockHardware, CanPulseGuide())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockMountHardwareManager::SetupCalibratedMount() {
    SetupConnectedMount();
    
    if (simulator) {
        MountHardwareSimulator::CalibrationInfo calInfo;
        calInfo.isValid = true;
        calInfo.angle = 45.0;
        calInfo.rate = 1.0;
        calInfo.quality = 0.9;
        simulator->SetupCalibration(calInfo);
    }
    
    if (mockCalibration) {
        EXPECT_CALL(*mockCalibration, IsValid())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockCalibration, GetCalibrationAngle())
            .WillRepeatedly(::testing::Return(45.0));
        EXPECT_CALL(*mockCalibration, GetCalibrationRate())
            .WillRepeatedly(::testing::Return(1.0));
    }
}

void MockMountHardwareManager::SetupGuidingSession() {
    SetupCalibratedMount();
    
    if (mockAlgorithm) {
        EXPECT_CALL(*mockAlgorithm, GetName())
            .WillRepeatedly(::testing::Return(wxString("Hysteresis")));
        EXPECT_CALL(*mockAlgorithm, GetMinMove())
            .WillRepeatedly(::testing::Return(0.15));
        EXPECT_CALL(*mockAlgorithm, GetMaxMove())
            .WillRepeatedly(::testing::Return(5.0));
    }
}

void MockMountHardwareManager::SimulateMountFailure() {
    if (simulator) {
        simulator->SetMountError(true);
    }
    
    if (mockHardware) {
        EXPECT_CALL(*mockHardware, Connect())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockHardware, PulseGuide(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return());
        EXPECT_CALL(*mockHardware, GetLastError())
            .WillRepeatedly(::testing::Return(wxString("Mount error")));
    }
}

void MockMountHardwareManager::SimulateCalibrationFailure() {
    SetupConnectedMount();
    
    if (simulator) {
        simulator->SetCalibrationError(true);
    }
    
    if (mockCalibration) {
        EXPECT_CALL(*mockCalibration, IsValid())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockCalibration, AddStep(::testing::_))
            .WillRepeatedly(::testing::Return());
    }
}
