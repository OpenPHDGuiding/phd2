/*
 * mock_phd_components.h
 * PHD Guiding
 *
 * Mock implementations of PHD2 components for testing
 */

#ifndef MOCK_PHD_COMPONENTS_H
#define MOCK_PHD_COMPONENTS_H

#include <wx/gdicmn.h>
#include <wx/string.h>

// Forward declarations
class PHD_Point;

// Mock PHD_Point class for testing
class PHD_Point
{
public:
    double X, Y;
    
    PHD_Point() : X(0), Y(0) {}
    PHD_Point(double x, double y) : X(x), Y(y) {}
    
    bool IsValid() const { return X >= 0 && Y >= 0; }
};

// Mock Camera class
class MockCamera
{
public:
    bool Connected;
    wxSize FrameSize;
    void* CurrentDefectMap;
    
    MockCamera();
    
    void GetDarkLibraryProperties(int* numDarks, double* minExp, double* maxExp);
    void ClearDarks();
    void ClearDefectMap();
    void SetDarkLibraryProperties(int count, double minExp, double maxExp);
    
private:
    int m_darkCount;
    double m_minExp;
    double m_maxExp;
};

// Mock Mount class
class MockMount
{
public:
    MockMount();
    
    bool IsConnected() const;
    bool IsCalibrated() const;
    void SetConnected(bool connected);
    void SetCalibrated(bool calibrated);
    
private:
    bool m_connected;
    bool m_calibrated;
};

// Mock Guider class
class MockGuider
{
public:
    MockGuider();
    
    bool IsCalibratingOrGuiding() const;
    bool IsLocked() const;
    PHD_Point CurrentPosition() const;
    
    void SetCalibrating(bool calibrating);
    void SetGuiding(bool guiding);
    void SetLocked(bool locked);
    void SetCurrentPosition(const PHD_Point& pos);
    
private:
    bool m_calibrating;
    bool m_guiding;
    bool m_locked;
    PHD_Point m_currentPosition;
};

// Mock Frame class
class MockFrame
{
public:
    MockGuider* pGuider;
    
    MockFrame();
    
    bool LoadDarkLibrary();
    bool LoadDefectMapHandler(bool enable);
};

// Mock Config class
class MockConfig
{
public:
    MockConfig();
    
    int GetCurrentProfileId() const;
    void SetCurrentProfileId(int id);
    
private:
    int m_currentProfileId;
};

// Test helper functions
void InitializeMockComponents();
void CleanupMockComponents();
void ResetMockComponentsToDefaults();
void SetupMockGlobals();

// Global mock instances
extern MockCamera* g_mockCamera;
extern MockMount* g_mockMount;
extern MockGuider* g_mockGuider;
extern MockFrame* g_mockFrame;
extern MockConfig* g_mockConfig;

// Mock global variables that would normally be defined in PHD2
extern MockCamera* pCamera;
extern MockMount* pMount;
extern MockGuider* pGuider;
extern MockFrame* pFrame;
extern MockConfig* pConfig;

// JSON-RPC constants for testing
#define JSONRPC_INVALID_PARAMS -32602

// Mock JSON classes for testing
class JObj
{
public:
    JObj() {}
    
    template<typename T>
    JObj& operator<<(const T& value) {
        // Mock implementation - just return self for chaining
        return *this;
    }
    
    std::string str() const {
        // Mock implementation - return a simple JSON string
        return "{\"mock\":\"response\"}";
    }
};

class NV
{
public:
    template<typename T>
    NV(const char* name, const T& value) {}
};

// Mock JSON-RPC helper functions
template<typename T>
JObj jrpc_result(const T& result) {
    JObj obj;
    return obj;
}

template<typename T>
JObj jrpc_error(int code, const T& message) {
    JObj obj;
    return obj;
}

// Mock JSON value types
enum json_type {
    JSON_NONE,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INT,
    JSON_FLOAT,
    JSON_BOOL,
    JSON_NULL
};

struct json_value {
    json_type type;
    const char* string_value;
    int int_value;
    double float_value;
    bool bool_value;
    
    json_value() : type(JSON_NONE), string_value(nullptr), int_value(0), float_value(0.0), bool_value(false) {}
};

// Mock parameter parsing class
class Params
{
public:
    template<typename... Args>
    Params(Args... args) {}
    
    const json_value* param(const char* name) const {
        // Mock implementation - return nullptr for now
        return nullptr;
    }
};

// Mock parameter validation functions
bool int_param(const json_value* val, int* result);
bool bool_param(const json_value* val, bool* result);
bool parse_settle(const json_value* val, void* settle, wxString& errMsg);
bool parse_roi(const json_value* val, void* roi);

#endif // MOCK_PHD_COMPONENTS_H
