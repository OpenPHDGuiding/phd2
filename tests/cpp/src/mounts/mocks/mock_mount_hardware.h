/*
 * mock_mount_hardware.h
 * PHD Guiding - Mount Module Tests
 *
 * Mock objects for mount hardware interfaces
 * Provides controllable behavior for mount communication and operations
 */

#ifndef MOCK_MOUNT_HARDWARE_H
#define MOCK_MOUNT_HARDWARE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>

// Forward declarations
class MountHardwareSimulator;

// Mock mount hardware interface
class MockMountHardware {
public:
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(GetConnectionStatus, int());
    
    // Mount capabilities
    MOCK_METHOD0(CanSlew, bool());
    MOCK_METHOD0(CanPulseGuide, bool());
    MOCK_METHOD0(CanSetTracking, bool());
    MOCK_METHOD0(CanSetPierSide, bool());
    MOCK_METHOD0(CanSetDeclinationRate, bool());
    MOCK_METHOD0(CanSetRightAscensionRate, bool());
    
    // Position and tracking
    MOCK_METHOD0(GetRightAscension, double());
    MOCK_METHOD0(GetDeclination, double());
    MOCK_METHOD0(GetAzimuth, double());
    MOCK_METHOD0(GetAltitude, double());
    MOCK_METHOD0(GetTracking, bool());
    MOCK_METHOD1(SetTracking, void(bool tracking));
    
    // Slewing operations
    MOCK_METHOD2(SlewToCoordinates, void(double ra, double dec));
    MOCK_METHOD2(SlewToCoordinatesAsync, void(double ra, double dec));
    MOCK_METHOD0(AbortSlew, void());
    MOCK_METHOD0(IsSlewing, bool());
    
    // Pulse guiding
    MOCK_METHOD2(PulseGuide, void(int direction, int duration));
    MOCK_METHOD0(IsPulseGuiding, bool());
    
    // Mount state
    MOCK_METHOD0(GetSideOfPier, int());
    MOCK_METHOD0(GetUTCDate, wxDateTime());
    MOCK_METHOD0(GetSiderealTime, double());
    MOCK_METHOD0(GetSiteLatitude, double());
    MOCK_METHOD0(GetSiteLongitude, double());
    MOCK_METHOD0(GetSiteElevation, double());
    
    // Error handling
    MOCK_METHOD0(GetLastError, wxString());
    MOCK_METHOD0(ClearError, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetPosition, void(double ra, double dec));
    MOCK_METHOD1(SetTracking, void(bool tracking));
    MOCK_METHOD1(SimulateSlew, void(bool success));
    MOCK_METHOD2(SimulatePulseGuide, void(int direction, bool success));
    
    static MockMountHardware* instance;
    static MockMountHardware* GetInstance();
    static void SetInstance(MockMountHardware* inst);
};

// Mock calibration data
class MockCalibrationData {
public:
    // Calibration state
    MOCK_METHOD0(IsValid, bool());
    MOCK_METHOD0(Clear, void());
    MOCK_METHOD0(GetCalibrationAngle, double());
    MOCK_METHOD1(SetCalibrationAngle, void(double angle));
    MOCK_METHOD0(GetCalibrationRate, double());
    MOCK_METHOD1(SetCalibrationRate, void(double rate));
    
    // Calibration steps
    MOCK_METHOD0(GetStepCount, int());
    MOCK_METHOD1(AddStep, void(const wxPoint& step));
    MOCK_METHOD1(GetStep, wxPoint(int index));
    MOCK_METHOD0(GetSteps, std::vector<wxPoint>());
    
    // Calibration calculations
    MOCK_METHOD0(CalculateAngle, double());
    MOCK_METHOD0(CalculateRate, double());
    MOCK_METHOD0(GetQuality, double());
    MOCK_METHOD0(IsGoodCalibration, bool());
    
    // Persistence
    MOCK_METHOD1(Save, bool(const wxString& filename));
    MOCK_METHOD1(Load, bool(const wxString& filename));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetValid, void(bool valid));
    MOCK_METHOD2(SetCalibrationData, void(double angle, double rate));
    
    static MockCalibrationData* instance;
    static MockCalibrationData* GetInstance();
    static void SetInstance(MockCalibrationData* inst);
};

// Mock guide algorithm
class MockGuideAlgorithm {
public:
    // Algorithm properties
    MOCK_METHOD0(GetName, wxString());
    MOCK_METHOD0(GetMinMove, double());
    MOCK_METHOD1(SetMinMove, void(double minMove));
    MOCK_METHOD0(GetMaxMove, double());
    MOCK_METHOD1(SetMaxMove, void(double maxMove));
    
    // Guide calculations
    MOCK_METHOD3(Calculate, double(double error, double dt, double siderealRate));
    MOCK_METHOD0(Reset, void());
    MOCK_METHOD0(GetHistory, std::vector<double>());
    
    // Configuration
    MOCK_METHOD0(LoadSettings, void());
    MOCK_METHOD0(SaveSettings, void());
    MOCK_METHOD0(GetConfigDialog, wxDialog*());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateResult, void(double result));
    
    static MockGuideAlgorithm* instance;
    static MockGuideAlgorithm* GetInstance();
    static void SetInstance(MockGuideAlgorithm* inst);
};

// Mount hardware simulator for comprehensive testing
class MountHardwareSimulator {
public:
    enum MountType {
        MOUNT_SIMULATOR = 0,
        MOUNT_ASCOM = 1,
        MOUNT_INDI = 2,
        MOUNT_ONCAMERA = 3,
        MOUNT_STEPGUIDER = 4,
        MOUNT_MANUAL = 5
    };
    
    enum GuideDirection {
        GUIDE_NORTH = 0,
        GUIDE_SOUTH = 1,
        GUIDE_EAST = 2,
        GUIDE_WEST = 3
    };
    
    struct MountInfo {
        MountType type;
        wxString name;
        bool isConnected;
        bool canSlew;
        bool canPulseGuide;
        bool canSetTracking;
        bool isTracking;
        bool isSlewing;
        bool isPulseGuiding;
        double ra, dec;  // Current position (hours, degrees)
        double azimuth, altitude;  // Alt-az coordinates
        int sideOfPier;  // 0=East, 1=West
        bool shouldFail;
        wxString lastError;
        
        MountInfo() : type(MOUNT_SIMULATOR), name("Simulator"), isConnected(false),
                     canSlew(true), canPulseGuide(true), canSetTracking(true),
                     isTracking(false), isSlewing(false), isPulseGuiding(false),
                     ra(0.0), dec(0.0), azimuth(0.0), altitude(0.0), sideOfPier(0),
                     shouldFail(false), lastError("") {}
    };
    
    struct CalibrationInfo {
        bool isValid;
        double angle;  // Calibration angle in degrees
        double rate;   // Guide rate in arcsec/sec
        std::vector<wxPoint> steps;
        double quality;
        bool shouldFail;
        
        CalibrationInfo() : isValid(false), angle(0.0), rate(1.0), quality(0.0), shouldFail(false) {}
    };
    
    struct SiteInfo {
        double latitude;   // Site latitude in degrees
        double longitude;  // Site longitude in degrees
        double elevation;  // Site elevation in meters
        wxDateTime utcTime;
        
        SiteInfo() : latitude(40.0), longitude(-75.0), elevation(100.0) {
            utcTime = wxDateTime::Now();
        }
    };
    
    // Mount management
    void SetupMount(const MountInfo& info);
    void SetupCalibration(const CalibrationInfo& info);
    void SetupSite(const SiteInfo& info);
    
    // State management
    MountInfo GetMountInfo() const;
    CalibrationInfo GetCalibrationInfo() const;
    SiteInfo GetSiteInfo() const;
    
    // Connection simulation
    bool ConnectMount();
    bool DisconnectMount();
    bool IsConnected() const;
    
    // Position simulation
    void SetPosition(double ra, double dec);
    void GetPosition(double& ra, double& dec) const;
    void UpdatePosition(double deltaTime);  // Update for tracking
    
    // Slewing simulation
    bool StartSlew(double targetRA, double targetDec);
    bool IsSlewing() const;
    void UpdateSlew(double deltaTime);
    void AbortSlew();
    
    // Pulse guiding simulation
    bool StartPulseGuide(GuideDirection direction, int duration);
    bool IsPulseGuiding() const;
    void UpdatePulseGuide(double deltaTime);
    
    // Calibration simulation
    bool StartCalibration();
    bool AddCalibrationStep(GuideDirection direction, int duration, const wxPoint& starPos);
    bool CompleteCalibration();
    void ClearCalibration();
    
    // Error simulation
    void SetMountError(bool error);
    void SetCalibrationError(bool error);
    void SetConnectionError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultMount();
    
    // Coordinate transformations
    void EquatorialToHorizontal(double ra, double dec, double& azimuth, double& altitude) const;
    void HorizontalToEquatorial(double azimuth, double altitude, double& ra, double& dec) const;
    double GetSiderealTime() const;
    
private:
    MountInfo mountInfo;
    CalibrationInfo calibrationInfo;
    SiteInfo siteInfo;
    
    // Slewing state
    double targetRA, targetDec;
    wxDateTime slewStartTime;
    
    // Pulse guiding state
    GuideDirection currentGuideDirection;
    int guideDuration;
    wxDateTime guideStartTime;
    
    void InitializeDefaults();
    double CalculateAngularDistance(double ra1, double dec1, double ra2, double dec2) const;
};

// Helper class to manage all mount hardware mocks
class MockMountHardwareManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockMountHardware* GetMockHardware();
    static MockCalibrationData* GetMockCalibration();
    static MockGuideAlgorithm* GetMockAlgorithm();
    static MountHardwareSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedMount();
    static void SetupCalibratedMount();
    static void SetupGuidingSession();
    static void SimulateMountFailure();
    static void SimulateCalibrationFailure();
    
private:
    static MockMountHardware* mockHardware;
    static MockCalibrationData* mockCalibration;
    static MockGuideAlgorithm* mockAlgorithm;
    static std::unique_ptr<MountHardwareSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_MOUNT_HARDWARE_MOCKS() MockMountHardwareManager::SetupMocks()
#define TEARDOWN_MOUNT_HARDWARE_MOCKS() MockMountHardwareManager::TeardownMocks()
#define RESET_MOUNT_HARDWARE_MOCKS() MockMountHardwareManager::ResetMocks()

#define GET_MOCK_MOUNT_HARDWARE() MockMountHardwareManager::GetMockHardware()
#define GET_MOCK_CALIBRATION() MockMountHardwareManager::GetMockCalibration()
#define GET_MOCK_GUIDE_ALGORITHM() MockMountHardwareManager::GetMockAlgorithm()
#define GET_MOUNT_SIMULATOR() MockMountHardwareManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_MOUNT_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_MOUNT_HARDWARE(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_MOUNT_DISCONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_MOUNT_HARDWARE(), Disconnect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_PULSE_GUIDE_SUCCESS(direction, duration) \
    EXPECT_CALL(*GET_MOCK_MOUNT_HARDWARE(), PulseGuide(direction, duration)) \
        .WillOnce(::testing::Return())

#define EXPECT_SLEW_SUCCESS(ra, dec) \
    EXPECT_CALL(*GET_MOCK_MOUNT_HARDWARE(), SlewToCoordinates(ra, dec)) \
        .WillOnce(::testing::Return())

#define EXPECT_CALIBRATION_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CALIBRATION(), IsValid()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_GUIDE_CALCULATION(error, result) \
    EXPECT_CALL(*GET_MOCK_GUIDE_ALGORITHM(), Calculate(error, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(result))

#endif // MOCK_MOUNT_HARDWARE_H
