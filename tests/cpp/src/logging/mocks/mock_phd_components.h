/*
 * mock_phd_components.h
 * PHD Guiding - Logging Module Tests
 *
 * Mock objects for PHD2 components used in logging tests
 * Provides controllable behavior for guider, mount, camera, and application components
 */

#ifndef MOCK_PHD_COMPONENTS_H
#define MOCK_PHD_COMPONENTS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/datetime.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class usImage;
struct FrameDroppedInfo;
struct GuideStepInfo;
struct CalibrationStepInfo;
struct LockPosShiftParams;
class PHD_Point;

// Mock usImage class for image handling
class MockUsImage {
public:
    MOCK_METHOD0(GetWidth, int());
    MOCK_METHOD0(GetHeight, int());
    MOCK_METHOD0(GetImageData, unsigned short*());
    MOCK_METHOD0(GetImageSize, size_t());
    MOCK_METHOD1(Save, bool(const wxString& filename));
    MOCK_METHOD0(FrameNum, unsigned int());
    MOCK_METHOD1(SetFrameNum, void(unsigned int frameNum));
    MOCK_METHOD0(GetTimestamp, wxDateTime());
    MOCK_METHOD1(SetTimestamp, void(const wxDateTime& timestamp));
    
    // Helper methods for testing
    MOCK_METHOD2(SetDimensions, void(int width, int height));
    MOCK_METHOD1(SetImageData, void(unsigned short* data));
    MOCK_METHOD1(SetShouldFailSave, void(bool fail));
    
    static MockUsImage* instance;
    static MockUsImage* GetInstance();
    static void SetInstance(MockUsImage* inst);
};

// Mock Mount class
class MockMount {
public:
    MOCK_METHOD1(DirectionChar, char(int direction));
    MOCK_METHOD0(GetMountClassName, wxString());
    MOCK_METHOD0(IsConnected, bool());
    MOCK_METHOD0(IsCalibrated, bool());
    MOCK_METHOD2(Guide, bool(int direction, int duration));
    MOCK_METHOD0(GetCalibrationAngle, double());
    MOCK_METHOD0(GetCalibrationRate, double());
    MOCK_METHOD0(GetCalibrationParity, int());
    
    // Helper methods for testing
    MOCK_METHOD1(SetConnected, void(bool connected));
    MOCK_METHOD1(SetCalibrated, void(bool calibrated));
    MOCK_METHOD1(SetShouldFailGuide, void(bool fail));
    
    static MockMount* instance;
    static MockMount* GetInstance();
    static void SetInstance(MockMount* inst);
};

// Mock Guider class
class MockGuider {
public:
    MOCK_METHOD0(IsGuiding, bool());
    MOCK_METHOD0(IsCalibratingOrGuiding, bool());
    MOCK_METHOD0(IsCalibrating, bool());
    MOCK_METHOD0(IsPaused, bool());
    MOCK_METHOD0(GetCurrentPosition, PHD_Point());
    MOCK_METHOD0(GetLockPosition, PHD_Point());
    MOCK_METHOD1(SetLockPosition, void(const PHD_Point& pos));
    MOCK_METHOD0(StartGuiding, bool());
    MOCK_METHOD0(StopGuiding, bool());
    MOCK_METHOD0(PauseGuiding, void());
    MOCK_METHOD0(ResumeGuiding, void());
    
    // Helper methods for testing
    MOCK_METHOD1(SetGuidingState, void(bool guiding));
    MOCK_METHOD1(SetCalibratingState, void(bool calibrating));
    MOCK_METHOD1(SetPausedState, void(bool paused));
    
    static MockGuider* instance;
    static MockGuider* GetInstance();
    static void SetInstance(MockGuider* inst);
};

// Mock PhdController class
class MockPhdController {
public:
    MOCK_METHOD0(IsSettling, bool());
    MOCK_METHOD0(GetSettlingDistance, double());
    MOCK_METHOD0(GetSettlingTime, double());
    MOCK_METHOD1(SetSettling, void(bool settling));
    
    static MockPhdController* instance;
    static MockPhdController* GetInstance();
    static void SetInstance(MockPhdController* inst);
};

// Mock wxGetApp() function and application class
class MockPhdApp {
public:
    MOCK_METHOD0(GetInstanceNumber, int());
    MOCK_METHOD0(GetLogFileTime, wxDateTime());
    MOCK_METHOD1(SetInstanceNumber, void(int number));
    MOCK_METHOD1(SetLogFileTime, void(const wxDateTime& time));
    
    static MockPhdApp* instance;
    static MockPhdApp* GetInstance();
    static void SetInstance(MockPhdApp* inst);
};

// Mock pConfig global configuration object
class MockPhdConfig {
public:
    // Global configuration methods
    MOCK_METHOD2(GetString, wxString(const wxString& key, const wxString& defaultValue));
    MOCK_METHOD2(SetString, void(const wxString& key, const wxString& value));
    MOCK_METHOD2(GetInt, int(const wxString& key, int defaultValue));
    MOCK_METHOD2(SetInt, void(const wxString& key, int value));
    MOCK_METHOD2(GetBool, bool(const wxString& key, bool defaultValue));
    MOCK_METHOD2(SetBool, void(const wxString& key, bool value));
    MOCK_METHOD2(GetDouble, double(const wxString& key, double defaultValue));
    MOCK_METHOD2(SetDouble, void(const wxString& key, double value));
    
    // Profile-specific configuration methods
    MOCK_METHOD2(Profile_GetString, wxString(const wxString& key, const wxString& defaultValue));
    MOCK_METHOD2(Profile_SetString, void(const wxString& key, const wxString& value));
    MOCK_METHOD2(Profile_GetInt, int(const wxString& key, int defaultValue));
    MOCK_METHOD2(Profile_SetInt, void(const wxString& key, int value));
    MOCK_METHOD2(Profile_GetBool, bool(const wxString& key, bool defaultValue));
    MOCK_METHOD2(Profile_SetBool, void(const wxString& key, bool value));
    MOCK_METHOD2(Profile_GetDouble, double(const wxString& key, double defaultValue));
    MOCK_METHOD2(Profile_SetDouble, void(const wxString& key, double value));
    
    // Mock Global and Profile sub-objects
    struct MockGlobal {
        MockPhdConfig* parent;
        MockGlobal(MockPhdConfig* p) : parent(p) {}
        
        wxString GetString(const wxString& key, const wxString& defaultValue) {
            return parent->GetString(key, defaultValue);
        }
        void SetString(const wxString& key, const wxString& value) {
            parent->SetString(key, value);
        }
        int GetInt(const wxString& key, int defaultValue) {
            return parent->GetInt(key, defaultValue);
        }
        void SetInt(const wxString& key, int value) {
            parent->SetInt(key, value);
        }
        bool GetBool(const wxString& key, bool defaultValue) {
            return parent->GetBool(key, defaultValue);
        }
        void SetBool(const wxString& key, bool value) {
            parent->SetBool(key, value);
        }
        double GetDouble(const wxString& key, double defaultValue) {
            return parent->GetDouble(key, defaultValue);
        }
        void SetDouble(const wxString& key, double value) {
            parent->SetDouble(key, value);
        }
    };
    
    struct MockProfile {
        MockPhdConfig* parent;
        MockProfile(MockPhdConfig* p) : parent(p) {}
        
        wxString GetString(const wxString& key, const wxString& defaultValue) {
            return parent->Profile_GetString(key, defaultValue);
        }
        void SetString(const wxString& key, const wxString& value) {
            parent->Profile_SetString(key, value);
        }
        int GetInt(const wxString& key, int defaultValue) {
            return parent->Profile_GetInt(key, defaultValue);
        }
        void SetInt(const wxString& key, int value) {
            parent->Profile_SetInt(key, value);
        }
        bool GetBool(const wxString& key, bool defaultValue) {
            return parent->Profile_GetBool(key, defaultValue);
        }
        void SetBool(const wxString& key, bool value) {
            parent->Profile_SetBool(key, value);
        }
        double GetDouble(const wxString& key, double defaultValue) {
            return parent->Profile_GetDouble(key, defaultValue);
        }
        void SetDouble(const wxString& key, double value) {
            parent->Profile_SetDouble(key, value);
        }
    };
    
    MockGlobal Global;
    MockProfile Profile;
    
    MockPhdConfig() : Global(this), Profile(this) {}
    
    static MockPhdConfig* instance;
    static MockPhdConfig* GetInstance();
    static void SetInstance(MockPhdConfig* inst);
};

// Mock pFrame global frame object
class MockPhdFrame {
public:
    MockGuider* pGuider;
    
    MockPhdFrame() : pGuider(nullptr) {}
    
    MOCK_METHOD0(GetGuider, MockGuider*());
    MOCK_METHOD1(SetGuider, void(MockGuider* guider));
    
    static MockPhdFrame* instance;
    static MockPhdFrame* GetInstance();
    static void SetInstance(MockPhdFrame* inst);
};

// Data structures used in logging
struct MockFrameDroppedInfo {
    unsigned int frameNumber;
    double time;
    double starMass;
    double starSNR;
    int starError;
    wxString status;
    
    MockFrameDroppedInfo() : frameNumber(0), time(0.0), starMass(0.0), starSNR(0.0), starError(0) {}
};

struct MockGuideStepInfo {
    double time;
    double dx;
    double dy;
    double distance;
    int durationRA;
    int durationDec;
    int directionRA;
    int directionDec;
    MockMount* mount;
    double starMass;
    double starSNR;
    int starError;
    
    MockGuideStepInfo() : time(0.0), dx(0.0), dy(0.0), distance(0.0), 
                         durationRA(0), durationDec(0), directionRA(0), directionDec(0),
                         mount(nullptr), starMass(0.0), starSNR(0.0), starError(0) {}
};

struct MockCalibrationStepInfo {
    double time;
    double dx;
    double dy;
    double distance;
    int direction;
    int step;
    MockMount* mount;
    
    MockCalibrationStepInfo() : time(0.0), dx(0.0), dy(0.0), distance(0.0),
                               direction(0), step(0), mount(nullptr) {}
};

struct MockLockPosShiftParams {
    bool shiftEnabled;
    bool shiftIsMountCoords;
    double shiftRate;
    int shiftUnits;
    
    MockLockPosShiftParams() : shiftEnabled(false), shiftIsMountCoords(false),
                              shiftRate(0.0), shiftUnits(0) {}
};

class MockPHD_Point {
public:
    double X;
    double Y;
    
    MockPHD_Point(double x = 0.0, double y = 0.0) : X(x), Y(y) {}
    
    MOCK_METHOD0(IsValid, bool());
    MOCK_METHOD1(SetInvalid, void(bool invalid));
    MOCK_METHOD2(SetXY, void(double x, double y));
};

// Helper class to manage all PHD component mocks
class MockPhdComponentsManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockUsImage* GetMockUsImage();
    static MockMount* GetMockMount();
    static MockGuider* GetMockGuider();
    static MockPhdController* GetMockPhdController();
    static MockPhdApp* GetMockPhdApp();
    static MockPhdConfig* GetMockPhdConfig();
    static MockPhdFrame* GetMockPhdFrame();
    
    // Convenience methods for common test scenarios
    static void SetupGuidingState(bool guiding, bool calibrating = false, bool paused = false);
    static void SetupMountState(bool connected, bool calibrated = true);
    static void SetupImageState(int width = 640, int height = 480, unsigned int frameNum = 1);
    static void SetupDefaultConfiguration();
    
private:
    static MockUsImage* mockUsImage;
    static MockMount* mockMount;
    static MockGuider* mockGuider;
    static MockPhdController* mockPhdController;
    static MockPhdApp* mockPhdApp;
    static MockPhdConfig* mockPhdConfig;
    static MockPhdFrame* mockPhdFrame;
};

// Macros for easier mock setup in tests
#define SETUP_PHD_MOCKS() MockPhdComponentsManager::SetupMocks()
#define TEARDOWN_PHD_MOCKS() MockPhdComponentsManager::TeardownMocks()
#define RESET_PHD_MOCKS() MockPhdComponentsManager::ResetMocks()

#define GET_MOCK_USIMAGE() MockPhdComponentsManager::GetMockUsImage()
#define GET_MOCK_MOUNT() MockPhdComponentsManager::GetMockMount()
#define GET_MOCK_GUIDER() MockPhdComponentsManager::GetMockGuider()
#define GET_MOCK_PHD_CONTROLLER() MockPhdComponentsManager::GetMockPhdController()
#define GET_MOCK_PHD_APP() MockPhdComponentsManager::GetMockPhdApp()
#define GET_MOCK_PHD_CONFIG() MockPhdComponentsManager::GetMockPhdConfig()
#define GET_MOCK_PHD_FRAME() MockPhdComponentsManager::GetMockPhdFrame()

// Global mock instances (to replace actual PHD2 globals)
extern MockPhdConfig* pConfig;
extern MockPhdFrame* pFrame;
extern MockGuider* pGuider;
extern MockMount* pMount;

#endif // MOCK_PHD_COMPONENTS_H
