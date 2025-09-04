/*
 * event_server_mocks.cpp
 * PHD Guiding
 *
 * Implementation of mock classes for EventServer testing
 */

#include "event_server_mocks.h"
#include <wx/string.h>

// Global mock instances
MockCamera* g_mockCamera = nullptr;
MockMount* g_mockMount = nullptr;
MockMount* g_mockSecondaryMount = nullptr;
MockGuider* g_mockGuider = nullptr;
MockFrame* g_mockFrame = nullptr;
MockApp* g_mockApp = nullptr;

// Mock global variables that the event server depends on
MockCamera* pCamera = nullptr;
MockMount* pMount = nullptr;
MockMount* pSecondaryMount = nullptr;
MockFrame* pFrame = nullptr;

// Helper functions for setting up mocks
void SetupMockDefaults(MockCamera* camera, MockMount* mount, MockGuider* guider, MockFrame* frame) {
    if (camera) {
        camera->Connected = true;
        camera->FrameSize = wxSize(1024, 768);
        camera->CurrentDefectMap = nullptr;
    }
    
    if (mount) {
        // Set up default expectations for mount
        ON_CALL(*mount, IsConnected())
            .WillByDefault(testing::Return(true));
        ON_CALL(*mount, IsCalibrated())
            .WillByDefault(testing::Return(true));
        ON_CALL(*mount, IsStepGuider())
            .WillByDefault(testing::Return(false));
        ON_CALL(*mount, xAngle())
            .WillByDefault(testing::Return(0.0));
        ON_CALL(*mount, yAngle())
            .WillByDefault(testing::Return(90.0));
        ON_CALL(*mount, xRate())
            .WillByDefault(testing::Return(1.0));
        ON_CALL(*mount, yRate())
            .WillByDefault(testing::Return(1.0));
        ON_CALL(*mount, RAParity())
            .WillByDefault(testing::Return(1));
        ON_CALL(*mount, DecParity())
            .WillByDefault(testing::Return(1));
        ON_CALL(*mount, GetCalibrationDeclination())
            .WillByDefault(testing::Return(0.0));
        ON_CALL(*mount, GetAoMaxPos())
            .WillByDefault(testing::Return(100));
        ON_CALL(*mount, DirectionStr(testing::_))
            .WillByDefault(testing::Invoke(mount, &MockMount::DirectionStrImpl));
    }
    
    if (guider) {
        ON_CALL(*guider, IsCalibratingOrGuiding())
            .WillByDefault(testing::Return(false));
        ON_CALL(*guider, IsLocked())
            .WillByDefault(testing::Return(false));
        ON_CALL(*guider, CurrentPosition())
            .WillByDefault(testing::Return(PHD_Point(512, 384)));
        ON_CALL(*guider, LockPosition())
            .WillByDefault(testing::Return(PHD_Point()));
    }
    
    if (frame) {
        ON_CALL(*frame, LoadDarkLibrary())
            .WillByDefault(testing::Return(true));
        ON_CALL(*frame, LoadDefectMapHandler(testing::_))
            .WillByDefault(testing::Return(true));
    }
}

void SetupMockExpectations(MockCamera* camera, MockMount* mount, MockGuider* guider, MockFrame* frame) {
    // Set up default behaviors
    SetupMockDefaults(camera, mount, guider, frame);
    
    if (camera) {
        // Set up camera expectations
        EXPECT_CALL(*camera, GetDarkLibraryProperties(testing::_, testing::_, testing::_))
            .WillRepeatedly(testing::Invoke(camera, &MockCamera::GetDarkLibraryPropertiesImpl));
        EXPECT_CALL(*camera, ClearDarks())
            .WillRepeatedly(testing::Invoke(camera, &MockCamera::ClearDarksImpl));
        EXPECT_CALL(*camera, ClearDefectMap())
            .WillRepeatedly(testing::Invoke(camera, &MockCamera::ClearDefectMapImpl));
        EXPECT_CALL(*camera, SetDarkLibraryProperties(testing::_, testing::_, testing::_))
            .WillRepeatedly(testing::Invoke(camera, &MockCamera::SetDarkLibraryPropertiesImpl));
    }
    
    if (mount) {
        // Set up mount expectations - allow any number of calls
        EXPECT_CALL(*mount, IsConnected())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(*mount, IsCalibrated())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(*mount, IsStepGuider())
            .WillRepeatedly(testing::Return(false));
        EXPECT_CALL(*mount, xAngle())
            .WillRepeatedly(testing::Return(0.0));
        EXPECT_CALL(*mount, yAngle())
            .WillRepeatedly(testing::Return(90.0));
        EXPECT_CALL(*mount, xRate())
            .WillRepeatedly(testing::Return(1.0));
        EXPECT_CALL(*mount, yRate())
            .WillRepeatedly(testing::Return(1.0));
        EXPECT_CALL(*mount, RAParity())
            .WillRepeatedly(testing::Return(1));
        EXPECT_CALL(*mount, DecParity())
            .WillRepeatedly(testing::Return(1));
        EXPECT_CALL(*mount, GetCalibrationDeclination())
            .WillRepeatedly(testing::Return(0.0));
        EXPECT_CALL(*mount, GetAoMaxPos())
            .WillRepeatedly(testing::Return(100));
        EXPECT_CALL(*mount, DirectionStr(testing::_))
            .WillRepeatedly(testing::Invoke(mount, &MockMount::DirectionStrImpl));
        EXPECT_CALL(*mount, SetConnected(testing::_))
            .WillRepeatedly(testing::Invoke(mount, &MockMount::SetConnectedImpl));
        EXPECT_CALL(*mount, SetCalibrated(testing::_))
            .WillRepeatedly(testing::Invoke(mount, &MockMount::SetCalibratedImpl));
        EXPECT_CALL(*mount, SetStepGuider(testing::_))
            .WillRepeatedly(testing::Invoke(mount, &MockMount::SetStepGuiderImpl));
    }
    
    if (guider) {
        // Set up guider expectations
        EXPECT_CALL(*guider, IsCalibratingOrGuiding())
            .WillRepeatedly(testing::Return(false));
        EXPECT_CALL(*guider, IsLocked())
            .WillRepeatedly(testing::Return(false));
        EXPECT_CALL(*guider, CurrentPosition())
            .WillRepeatedly(testing::Return(PHD_Point(512, 384)));
        EXPECT_CALL(*guider, LockPosition())
            .WillRepeatedly(testing::Return(PHD_Point()));
        EXPECT_CALL(*guider, SetCalibrating(testing::_))
            .WillRepeatedly(testing::Invoke(guider, &MockGuider::SetCalibratingImpl));
        EXPECT_CALL(*guider, SetGuiding(testing::_))
            .WillRepeatedly(testing::Invoke(guider, &MockGuider::SetGuidingImpl));
        EXPECT_CALL(*guider, SetLocked(testing::_))
            .WillRepeatedly(testing::Invoke(guider, &MockGuider::SetLockedImpl));
        EXPECT_CALL(*guider, SetCurrentPosition(testing::_))
            .WillRepeatedly(testing::Invoke(guider, &MockGuider::SetCurrentPositionImpl));
        EXPECT_CALL(*guider, SetLockPosition(testing::_))
            .WillRepeatedly(testing::Invoke(guider, &MockGuider::SetLockPositionImpl));
    }
    
    if (frame) {
        // Set up frame expectations
        EXPECT_CALL(*frame, LoadDarkLibrary())
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(*frame, LoadDefectMapHandler(testing::_))
            .WillRepeatedly(testing::Return(true));
    }
    
    // Set up global pointers
    pCamera = camera;
    pMount = mount;
    pSecondaryMount = g_mockSecondaryMount;
    pFrame = frame;
}

// Mock wxGetHostName function
wxString wxGetHostName() {
    return wxT("test-host");
}

// Mock wxGetUTCTimeMillis function
wxLongLong wxGetUTCTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return wxLongLong(millis);
}

// Mock wxGetApp function
MockApp& wxGetApp() {
    static MockApp app;
    return app;
}

// Mock Debug class for logging
class MockDebug {
public:
    void Write(const wxString& msg) {
        // For testing, we can just ignore debug output
        // or optionally store it for verification
    }
    
    void AddLine(const wxString& msg) {
        Write(msg);
    }
};

MockDebug Debug;

// Mock PhdController class methods
class PhdController {
public:
    static bool CanGuide(wxString* error) {
        if (error) {
            *error = wxT("");
        }
        return true; // Assume we can always guide in tests
    }
    
    static bool Guide(unsigned int options, const SettleParams& settle, const wxRect& roi, wxString* error) {
        if (error) {
            *error = wxT("");
        }
        return false; // Return false for success (PHD2 convention)
    }
};

// Mock Guider class for exposed state
class Guider {
public:
    enum EXPOSED_STATE {
        EXPOSED_STATE_NONE,
        EXPOSED_STATE_SELECTED,
        EXPOSED_STATE_CALIBRATING_PRIMARY,
        EXPOSED_STATE_CALIBRATING_SECONDARY,
        EXPOSED_STATE_CALIBRATED,
        EXPOSED_STATE_GUIDING_LOCKED,
        EXPOSED_STATE_GUIDING_LOST
    };
    
    static EXPOSED_STATE GetExposedState() {
        return EXPOSED_STATE_CALIBRATED; // Default state for testing
    }
};

// Mock constants
const int JSONRPC_INVALID_PARAMS = -32602;
const int JSONRPC_INVALID_REQUEST = -32600;
const int JSONRPC_PARSE_ERROR = -32700;
const int MSG_PROTOCOL_VERSION = 1;
const char* PHDVERSION = "2.6.11";
const char* PHDSUBVER = "test";

// Mock guide options
const unsigned int GUIDEOPT_USE_STICKY_LOCK = 0x01;
const unsigned int GUIDEOPT_FORCE_RECAL = 0x02;

// Mock JSON helper functions
class JObj {
public:
    JObj() {}
    
    template<typename T>
    JObj& operator<<(const T& value) {
        // For testing, we just build a simple string representation
        return *this;
    }
    
    wxString str() const {
        return m_content;
    }
    
private:
    wxString m_content;
};

class JAry {
public:
    template<typename T>
    JAry& operator<<(const T& value) {
        return *this;
    }
};

template<typename T>
class NV {
public:
    NV(const wxString& name, const T& value, int precision = -1) {}
};

// Mock JSON-RPC helper functions
JObj jrpc_result(const JObj& result) {
    JObj response;
    return response;
}

template<typename T>
JObj jrpc_result(const T& result) {
    JObj response;
    return response;
}

JObj jrpc_error(int code, const wxString& message) {
    JObj response;
    return response;
}

JObj jrpc_id(int id) {
    JObj response;
    return response;
}

// Mock NVMount function
NV<JObj> NVMount(const MockMount* mount) {
    return NV<JObj>("Mount", JObj());
}
