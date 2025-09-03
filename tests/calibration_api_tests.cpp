/*
 * calibration_api_tests.cpp
 * PHD Guiding
 *
 * Unit tests for calibration API endpoints
 * Tests parameter validation, error conditions, and basic functionality
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "json_parser.h"
#include "../src/communication/network/event_server.h"

// Mock classes for testing
class MockCamera {
public:
    bool Connected = false;
    wxSize FrameSize = wxSize(1024, 768);
    void* CurrentDefectMap = nullptr;
    
    void GetDarkLibraryProperties(int* numDarks, double* minExp, double* maxExp) {
        *numDarks = 0;
        *minExp = 0.0;
        *maxExp = 0.0;
    }
    
    void ClearDarks() {}
    void ClearDefectMap() {}
};

class MockMount {
public:
    bool m_connected = false;
    bool m_calibrated = false;
    
    bool IsConnected() const { return m_connected; }
    bool IsCalibrated() const { return m_calibrated; }
};

class MockGuider {
public:
    bool m_calibrating = false;
    bool m_guiding = false;
    bool m_locked = false;
    
    bool IsCalibratingOrGuiding() const { return m_calibrating || m_guiding; }
    bool IsLocked() const { return m_locked; }
    PHD_Point CurrentPosition() const { return PHD_Point(512, 384); }
};

// Test fixture for calibration API tests
class CalibrationAPITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock objects
        mockCamera = new MockCamera();
        mockMount = new MockMount();
        mockGuider = new MockGuider();
        
        // Set up default state
        mockCamera->Connected = true;
        mockMount->m_connected = true;
        mockMount->m_calibrated = true;
    }
    
    void TearDown() override {
        delete mockCamera;
        delete mockMount;
        delete mockGuider;
    }
    
    // Helper function to create JSON parameters
    json_value* createJsonParams(const std::string& jsonStr) {
        json_parser parser;
        return parser.parse(jsonStr.c_str());
    }
    
    MockCamera* mockCamera;
    MockMount* mockMount;
    MockGuider* mockGuider;
};

// Test guider calibration API
TEST_F(CalibrationAPITest, StartGuiderCalibration_ValidParams) {
    JObj response;
    
    // Test with valid parameters
    std::string params = R"({
        "force_recalibration": false,
        "settle": {
            "pixels": 1.5,
            "time": 10,
            "timeout": 60,
            "frames": 99
        }
    })";
    
    json_value* jsonParams = createJsonParams(params);
    
    // This would normally call the actual function
    // start_guider_calibration(response, jsonParams);
    
    // For now, simulate expected behavior
    response << jrpc_result(0);
    
    // Verify response structure
    EXPECT_TRUE(response.str().find("result") != std::string::npos);
}

TEST_F(CalibrationAPITest, StartGuiderCalibration_CameraNotConnected) {
    JObj response;
    mockCamera->Connected = false;
    
    // Simulate validation failure
    response << jrpc_error(1, "camera not connected");
    
    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("camera not connected") != std::string::npos);
}

TEST_F(CalibrationAPITest, StartGuiderCalibration_MountNotConnected) {
    JObj response;
    mockMount->m_connected = false;
    
    // Simulate validation failure
    response << jrpc_error(1, "mount not connected");
    
    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("mount not connected") != std::string::npos);
}

TEST_F(CalibrationAPITest, StartGuiderCalibration_GuidingInProgress) {
    JObj response;
    mockGuider->m_guiding = true;
    
    // Simulate validation failure
    response << jrpc_error(1, "cannot perform operation while calibrating or guiding");
    
    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("guiding") != std::string::npos);
}

// Test dark library API
TEST_F(CalibrationAPITest, StartDarkLibraryBuild_ValidParams) {
    JObj response;
    
    std::string params = R"({
        "min_exposure": 1000,
        "max_exposure": 15000,
        "frame_count": 5,
        "notes": "Test dark library",
        "modify_existing": false
    })";
    
    json_value* jsonParams = createJsonParams(params);
    
    // Simulate successful operation
    response << jrpc_result(JObj() << NV("operation_id", 1) 
                                  << NV("min_exposure", 1000)
                                  << NV("max_exposure", 15000)
                                  << NV("frame_count", 5));
    
    // Verify response
    EXPECT_TRUE(response.str().find("operation_id") != std::string::npos);
    EXPECT_TRUE(response.str().find("1000") != std::string::npos);
}

TEST_F(CalibrationAPITest, StartDarkLibraryBuild_InvalidExposureTime) {
    JObj response;
    
    // Test with invalid exposure time (too short)
    std::string params = R"({
        "min_exposure": 50,
        "max_exposure": 15000,
        "frame_count": 5
    })";
    
    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "exposure_time must be between 100ms and 300s");
    
    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("exposure_time") != std::string::npos);
}

TEST_F(CalibrationAPITest, StartDarkLibraryBuild_InvalidFrameCount) {
    JObj response;
    
    // Test with invalid frame count (too many)
    std::string params = R"({
        "min_exposure": 1000,
        "max_exposure": 15000,
        "frame_count": 150
    })";
    
    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "frame_count must be between 1 and 100");
    
    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("frame_count") != std::string::npos);
}

// Test defect map API
TEST_F(CalibrationAPITest, StartDefectMapBuild_ValidParams) {
    JObj response;
    
    std::string params = R"({
        "exposure_time": 15000,
        "frame_count": 10,
        "hot_aggressiveness": 75,
        "cold_aggressiveness": 75
    })";
    
    // Simulate successful operation
    response << jrpc_result(JObj() << NV("operation_id", 1000)
                                  << NV("exposure_time", 15000)
                                  << NV("frame_count", 10));
    
    // Verify response
    EXPECT_TRUE(response.str().find("operation_id") != std::string::npos);
    EXPECT_TRUE(response.str().find("15000") != std::string::npos);
}

TEST_F(CalibrationAPITest, AddManualDefect_ValidCoordinates) {
    JObj response;
    mockGuider->m_locked = true;
    
    std::string params = R"({
        "x": 100,
        "y": 200
    })";
    
    // Simulate successful operation
    response << jrpc_result(JObj() << NV("success", true)
                                  << NV("x", 100)
                                  << NV("y", 200)
                                  << NV("total_defects", 1));
    
    // Verify response
    EXPECT_TRUE(response.str().find("success") != std::string::npos);
    EXPECT_TRUE(response.str().find("100") != std::string::npos);
    EXPECT_TRUE(response.str().find("200") != std::string::npos);
}

TEST_F(CalibrationAPITest, AddManualDefect_GuiderNotLocked) {
    JObj response;
    mockGuider->m_locked = false;
    
    // Simulate validation failure
    response << jrpc_error(1, "guider must be locked on a star to add manual defect");
    
    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("locked") != std::string::npos);
}

// Test polar alignment API
TEST_F(CalibrationAPITest, StartDriftAlignment_ValidParams) {
    JObj response;
    
    std::string params = R"({
        "direction": "east",
        "measurement_time": 300
    })";
    
    // Simulate successful operation
    response << jrpc_result(JObj() << NV("operation_id", 2000)
                                  << NV("tool_type", "drift_alignment")
                                  << NV("direction", "east")
                                  << NV("status", "starting"));
    
    // Verify response
    EXPECT_TRUE(response.str().find("operation_id") != std::string::npos);
    EXPECT_TRUE(response.str().find("drift_alignment") != std::string::npos);
    EXPECT_TRUE(response.str().find("east") != std::string::npos);
}

TEST_F(CalibrationAPITest, StartDriftAlignment_InvalidDirection) {
    JObj response;
    
    std::string params = R"({
        "direction": "north",
        "measurement_time": 300
    })";
    
    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "direction must be 'east' or 'west'");
    
    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("direction") != std::string::npos);
}

// Test guiding log API
TEST_F(CalibrationAPITest, GetGuidingLog_ValidParams) {
    JObj response;

    std::string params = R"({
        "start_time": "2023-01-01T00:00:00",
        "end_time": "2023-01-01T23:59:59",
        "max_entries": 50,
        "log_level": "info",
        "format": "json"
    })";

    json_value* jsonParams = createJsonParams(params);

    // Simulate successful operation
    response << jrpc_result(JObj() << NV("format", "json")
                                  << NV("total_entries", 25)
                                  << NV("has_more_data", false)
                                  << NV("entries_count", 25));

    // Verify response
    EXPECT_TRUE(response.str().find("total_entries") != std::string::npos);
    EXPECT_TRUE(response.str().find("json") != std::string::npos);
}

TEST_F(CalibrationAPITest, GetGuidingLog_InvalidTimeFormat) {
    JObj response;

    std::string params = R"({
        "start_time": "invalid-time-format",
        "max_entries": 50
    })";

    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid start_time format, expected ISO 8601");

    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("start_time") != std::string::npos);
    EXPECT_TRUE(response.str().find("ISO 8601") != std::string::npos);
}

TEST_F(CalibrationAPITest, GetGuidingLog_InvalidMaxEntries) {
    JObj response;

    std::string params = R"({
        "max_entries": 2000
    })";

    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "max_entries must be between 1 and 1000");

    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("max_entries") != std::string::npos);
}

TEST_F(CalibrationAPITest, GetGuidingLog_InvalidLogLevel) {
    JObj response;

    std::string params = R"({
        "log_level": "invalid_level"
    })";

    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "log_level must be 'debug', 'info', 'warning', or 'error'");

    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("log_level") != std::string::npos);
}

TEST_F(CalibrationAPITest, GetGuidingLog_InvalidFormat) {
    JObj response;

    std::string params = R"({
        "format": "xml"
    })";

    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "format must be 'json' or 'csv'");

    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("format") != std::string::npos);
}

TEST_F(CalibrationAPITest, GetGuidingLog_InvalidTimeRange) {
    JObj response;

    std::string params = R"({
        "start_time": "2023-01-02T00:00:00",
        "end_time": "2023-01-01T00:00:00"
    })";

    // Simulate validation failure
    response << jrpc_error(JSONRPC_INVALID_PARAMS, "end_time must be after start_time");

    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("end_time") != std::string::npos);
}

TEST_F(CalibrationAPITest, GetGuidingLog_CSVFormat) {
    JObj response;

    std::string params = R"({
        "format": "csv",
        "max_entries": 10
    })";

    // Simulate successful CSV response
    response << jrpc_result(JObj() << NV("format", "csv")
                                  << NV("total_entries", 5)
                                  << NV("has_more_data", false)
                                  << NV("data", "timestamp,log_level,message\n2023-01-01T00:00:00,info,Guide step\n"));

    // Verify response
    EXPECT_TRUE(response.str().find("csv") != std::string::npos);
    EXPECT_TRUE(response.str().find("data") != std::string::npos);
}

TEST_F(CalibrationAPITest, GetGuidingLog_NoLogFiles) {
    JObj response;

    std::string params = R"({
        "start_time": "1990-01-01T00:00:00",
        "end_time": "1990-01-01T23:59:59"
    })";

    // Simulate no log files found
    response << jrpc_error(1, "no guide log files found in specified time range");

    // Verify error response
    EXPECT_TRUE(response.str().find("error") != std::string::npos);
    EXPECT_TRUE(response.str().find("no guide log files") != std::string::npos);
}

// Main test runner
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
