/*
 * event_server_mocks.h
 * PHD Guiding
 *
 * Mock classes and definitions for EventServer testing
 * Provides mock implementations of PHD2 core components
 */

#ifndef EVENT_SERVER_MOCKS_H
#define EVENT_SERVER_MOCKS_H

#include <wx/string.h>
#include <wx/socket.h>
#include <gmock/gmock.h>

// Forward declarations
class PHD_Point;
class GuideStepInfo;
class CalibrationStepInfo;
class FrameDroppedInfo;
class SingleExposure;
class Star;

// Mock PHD_Point class
class PHD_Point {
public:
    double X, Y;
    
    PHD_Point() : X(0), Y(0) {}
    PHD_Point(double x, double y) : X(x), Y(y) {}
    
    bool IsValid() const { return X != 0 || Y != 0; }
    void Invalidate() { X = Y = 0; }
};

// Mock Star class
class Star {
public:
    double Mass = 0.0;
    double SNR = 0.0;
    double HFD = 0.0;
    
    enum FindResult {
        STAR_OK = 0,
        STAR_LOWSNR = 1,
        STAR_LOWMASS = 2,
        STAR_TOO_NEAR_EDGE = 3,
        STAR_SATURATED = 4
    };
    
    int GetError() const { return m_error; }
    void SetError(int error) { m_error = error; }
    
    static bool WasFound(FindResult result) { return result == STAR_OK; }
    
private:
    int m_error = STAR_OK;
};

// Mock GuideStepInfo structure
class GuideStepInfo {
public:
    unsigned int frameNumber = 0;
    double time = 0.0;
    class MockMount* mount = nullptr;
    PHD_Point cameraOffset;
    PHD_Point mountOffset;
    double guideDistanceRA = 0.0;
    double guideDistanceDec = 0.0;
    int durationRA = 0;
    int directionRA = 0;
    int durationDec = 0;
    int directionDec = 0;
    double starMass = 0.0;
    double starSNR = 0.0;
    double starHFD = 0.0;
    int starError = 0;
    double avgDist = 0.0;
};

// Mock CalibrationStepInfo structure
class CalibrationStepInfo {
public:
    class MockMount* mount = nullptr;
    wxString phase;
    int direction = 0;
    double dist = 0.0;
    double dx = 0.0;
    double dy = 0.0;
    PHD_Point pos;
    int step = 0;
};

// Mock FrameDroppedInfo structure
class FrameDroppedInfo {
public:
    int starError = 0;
    double starMass = 0.0;
    double starSNR = 0.0;
    double starHFD = 0.0;
    wxString status;
    double avgDist = 0.0;
};

// Mock SingleExposure structure
class SingleExposure {
public:
    bool save = false;
    wxString path;
    double exposure = 0.0;
    wxString error;
};

// Mock SettleParams structure
class SettleParams {
public:
    double tolerancePx = 1.5;
    double settleTimeSec = 10.0;
    double timeoutSec = 60.0;
    int frames = 99;
};

// Mock Camera class
class MockCamera {
public:
    bool Connected = false;
    wxSize FrameSize = wxSize(1024, 768);
    void* CurrentDefectMap = nullptr;
    
    MOCK_METHOD3(GetDarkLibraryProperties, void(int* numDarks, double* minExp, double* maxExp));
    MOCK_METHOD0(ClearDarks, void());
    MOCK_METHOD0(ClearDefectMap, void());
    MOCK_METHOD3(SetDarkLibraryProperties, void(int count, double minExp, double maxExp));
    
    // Default implementations for non-mocked methods
    void GetDarkLibraryPropertiesImpl(int* numDarks, double* minExp, double* maxExp) {
        *numDarks = m_darkCount;
        *minExp = m_minExp;
        *maxExp = m_maxExp;
    }
    
    void ClearDarksImpl() { m_darkCount = 0; }
    void ClearDefectMapImpl() { CurrentDefectMap = nullptr; }
    void SetDarkLibraryPropertiesImpl(int count, double minExp, double maxExp) {
        m_darkCount = count;
        m_minExp = minExp;
        m_maxExp = maxExp;
    }
    
private:
    int m_darkCount = 0;
    double m_minExp = 0.0;
    double m_maxExp = 0.0;
};

// Mock Mount class
class MockMount {
public:
    MOCK_CONST_METHOD0(IsConnected, bool());
    MOCK_CONST_METHOD0(IsCalibrated, bool());
    MOCK_CONST_METHOD0(IsStepGuider, bool());
    MOCK_METHOD1(SetConnected, void(bool connected));
    MOCK_METHOD1(SetCalibrated, void(bool calibrated));
    MOCK_METHOD1(SetStepGuider, void(bool isStepGuider));
    
    MOCK_CONST_METHOD0(xAngle, double());
    MOCK_CONST_METHOD0(yAngle, double());
    MOCK_CONST_METHOD0(xRate, double());
    MOCK_CONST_METHOD0(yRate, double());
    MOCK_CONST_METHOD0(RAParity, int());
    MOCK_CONST_METHOD0(DecParity, int());
    MOCK_CONST_METHOD0(GetCalibrationDeclination, double());
    MOCK_CONST_METHOD0(GetAoMaxPos, int());
    MOCK_CONST_METHOD1(DirectionStr, const char*(int direction));
    
    // Default implementations
    bool IsConnectedImpl() const { return m_connected; }
    bool IsCalibratedImpl() const { return m_calibrated; }
    bool IsStepGuiderImpl() const { return m_isStepGuider; }
    void SetConnectedImpl(bool connected) { m_connected = connected; }
    void SetCalibratedImpl(bool calibrated) { m_calibrated = calibrated; }
    void SetStepGuiderImpl(bool isStepGuider) { m_isStepGuider = isStepGuider; }
    
    double xAngleImpl() const { return m_xAngle; }
    double yAngleImpl() const { return m_yAngle; }
    double xRateImpl() const { return m_xRate; }
    double yRateImpl() const { return m_yRate; }
    int RAParityImpl() const { return m_raParity; }
    int DecParityImpl() const { return m_decParity; }
    double GetCalibrationDeclinationImpl() const { return m_declination; }
    int GetAoMaxPosImpl() const { return m_aoMaxPos; }
    
    const char* DirectionStrImpl(int direction) const {
        switch(direction) {
            case 0: return "North";
            case 1: return "South"; 
            case 2: return "East";
            case 3: return "West";
            default: return "Unknown";
        }
    }
    
private:
    bool m_connected = false;
    bool m_calibrated = false;
    bool m_isStepGuider = false;
    double m_xAngle = 0.0;
    double m_yAngle = 90.0;
    double m_xRate = 1.0;
    double m_yRate = 1.0;
    int m_raParity = 1;
    int m_decParity = 1;
    double m_declination = 0.0;
    int m_aoMaxPos = 100;
};

// Mock Guider class
class MockGuider {
public:
    MOCK_CONST_METHOD0(IsCalibratingOrGuiding, bool());
    MOCK_CONST_METHOD0(IsLocked, bool());
    MOCK_CONST_METHOD0(CurrentPosition, PHD_Point());
    MOCK_CONST_METHOD0(LockPosition, PHD_Point());
    
    MOCK_METHOD1(SetCalibrating, void(bool calibrating));
    MOCK_METHOD1(SetGuiding, void(bool guiding));
    MOCK_METHOD1(SetLocked, void(bool locked));
    MOCK_METHOD1(SetCurrentPosition, void(const PHD_Point& pos));
    MOCK_METHOD1(SetLockPosition, void(const PHD_Point& pos));
    
    // Default implementations
    bool IsCalibratingOrGuidingImpl() const { return m_calibrating || m_guiding; }
    bool IsLockedImpl() const { return m_locked; }
    PHD_Point CurrentPositionImpl() const { return m_currentPosition; }
    PHD_Point LockPositionImpl() const { return m_lockPosition; }
    
    void SetCalibratingImpl(bool calibrating) { m_calibrating = calibrating; }
    void SetGuidingImpl(bool guiding) { m_guiding = guiding; }
    void SetLockedImpl(bool locked) { m_locked = locked; }
    void SetCurrentPositionImpl(const PHD_Point& pos) { m_currentPosition = pos; }
    void SetLockPositionImpl(const PHD_Point& pos) { m_lockPosition = pos; }
    
private:
    bool m_calibrating = false;
    bool m_guiding = false;
    bool m_locked = false;
    PHD_Point m_currentPosition;
    PHD_Point m_lockPosition;
};

// Mock Frame class
class MockFrame {
public:
    MockGuider* pGuider = nullptr;
    
    MockFrame() {
        pGuider = new MockGuider();
    }
    
    ~MockFrame() {
        delete pGuider;
    }
    
    MOCK_METHOD0(LoadDarkLibrary, bool());
    MOCK_METHOD1(LoadDefectMapHandler, bool(bool enable));
    
    // Default implementations
    bool LoadDarkLibraryImpl() { return true; }
    bool LoadDefectMapHandlerImpl(bool enable) { return true; }
};

// Mock App class for instance number
class MockApp {
public:
    MOCK_METHOD0(GetInstanceNumber, unsigned int());
    
    unsigned int GetInstanceNumberImpl() { return m_instanceNumber; }
    void SetInstanceNumber(unsigned int instanceNumber) { m_instanceNumber = instanceNumber; }
    
private:
    unsigned int m_instanceNumber = 1;
};

// Helper functions for setting up mocks
void SetupMockDefaults(MockCamera* camera, MockMount* mount, MockGuider* guider, MockFrame* frame);
void SetupMockExpectations(MockCamera* camera, MockMount* mount, MockGuider* guider, MockFrame* frame);

// Global mock instances (declared extern, defined in test file)
extern MockCamera* g_mockCamera;
extern MockMount* g_mockMount;
extern MockMount* g_mockSecondaryMount;
extern MockGuider* g_mockGuider;
extern MockFrame* g_mockFrame;
extern MockApp* g_mockApp;

#endif // EVENT_SERVER_MOCKS_H
