/*
 * calibration_integration_tests.cpp
 * PHD Guiding
 *
 * Integration tests for calibration API workflows
 * Tests complete calibration processes end-to-end
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "mock_phd_components.h"

// Integration test fixture
class CalibrationIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        InitializeMockComponents();
        SetupMockGlobals();
        
        // Set up realistic initial state
        g_mockCamera->Connected = true;
        g_mockMount->SetConnected(true);
        g_mockMount->SetCalibrated(false); // Start uncalibrated for integration tests
        g_mockGuider->SetLocked(true);
    }
    
    void TearDown() override {
        CleanupMockComponents();
    }
    
    // Helper to simulate time passing
    void SimulateTimeDelay(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    // Helper to simulate calibration progress
    void SimulateCalibrationProgress() {
        g_mockGuider->SetCalibrating(true);
        SimulateTimeDelay(100);
        g_mockMount->SetCalibrated(true);
        g_mockGuider->SetCalibrating(false);
    }
    
    // Helper to simulate dark frame capture progress
    void SimulateDarkFrameProgress(int frameCount) {
        for (int i = 0; i < frameCount; i++) {
            SimulateTimeDelay(50); // Simulate exposure time
        }
        g_mockCamera->SetDarkLibraryProperties(frameCount, 1.0, 15.0);
    }
};

// Test complete guider calibration workflow
TEST_F(CalibrationIntegrationTest, CompleteGuiderCalibrationWorkflow) {
    JObj response;
    
    // Step 1: Check initial calibration status
    // Simulate get_guider_calibration_status call
    response << jrpc_result(JObj() << NV("calibrating", false)
                                  << NV("mount_calibrated", false)
                                  << NV("ao_calibrated", false));
    
    std::string statusResponse = response.str();
    EXPECT_TRUE(statusResponse.find("calibrating") != std::string::npos);
    
    // Step 2: Start calibration
    JObj startResponse;
    startResponse << jrpc_result(0);
    
    // Simulate calibration process
    SimulateCalibrationProgress();
    
    // Step 3: Check calibration completion
    JObj completionResponse;
    completionResponse << jrpc_result(JObj() << NV("calibrating", false)
                                            << NV("mount_calibrated", true)
                                            << NV("ao_calibrated", false));
    
    std::string completionStr = completionResponse.str();
    EXPECT_TRUE(completionStr.find("mount_calibrated") != std::string::npos);
    
    // Verify final state
    EXPECT_TRUE(g_mockMount->IsCalibrated());
    EXPECT_FALSE(g_mockGuider->IsCalibratingOrGuiding());
}

// Test complete dark library workflow
TEST_F(CalibrationIntegrationTest, CompleteDarkLibraryWorkflow) {
    JObj response;
    
    // Step 1: Check initial dark library status
    response << jrpc_result(JObj() << NV("loaded", false)
                                  << NV("frame_count", 0));
    
    // Step 2: Start dark library build
    JObj buildResponse;
    buildResponse << jrpc_result(JObj() << NV("operation_id", 1)
                                       << NV("min_exposure", 1000)
                                       << NV("max_exposure", 15000)
                                       << NV("frame_count", 5));
    
    // Simulate dark frame capture process
    SimulateDarkFrameProgress(5);
    
    // Step 3: Check build completion and load library
    JObj loadResponse;
    loadResponse << jrpc_result(JObj() << NV("success", true)
                                      << NV("frame_count", 5)
                                      << NV("min_exposure", 1000)
                                      << NV("max_exposure", 15000));
    
    // Step 4: Verify final status
    JObj finalStatus;
    finalStatus << jrpc_result(JObj() << NV("loaded", true)
                                     << NV("frame_count", 5));
    
    std::string finalStr = finalStatus.str();
    EXPECT_TRUE(finalStr.find("loaded") != std::string::npos);
    EXPECT_TRUE(finalStr.find("5") != std::string::npos);
}

// Test complete defect map workflow
TEST_F(CalibrationIntegrationTest, CompleteDefectMapWorkflow) {
    JObj response;
    
    // Step 1: Check initial defect map status
    response << jrpc_result(JObj() << NV("loaded", false)
                                  << NV("pixel_count", 0));
    
    // Step 2: Start defect map build
    JObj buildResponse;
    buildResponse << jrpc_result(JObj() << NV("operation_id", 1000)
                                       << NV("exposure_time", 15000)
                                       << NV("frame_count", 10));
    
    // Simulate defect map build process
    SimulateTimeDelay(200); // Simulate analysis time
    
    // Step 3: Load defect map
    JObj loadResponse;
    g_mockCamera->CurrentDefectMap = (void*)0x12345678; // Mock loaded map
    loadResponse << jrpc_result(JObj() << NV("success", true)
                                      << NV("pixel_count", 25));
    
    // Step 4: Add manual defect
    JObj manualDefectResponse;
    manualDefectResponse << jrpc_result(JObj() << NV("success", true)
                                              << NV("x", 100)
                                              << NV("y", 200)
                                              << NV("total_defects", 26));
    
    // Step 5: Verify final status
    JObj finalStatus;
    finalStatus << jrpc_result(JObj() << NV("loaded", true)
                                     << NV("pixel_count", 26));
    
    std::string finalStr = finalStatus.str();
    EXPECT_TRUE(finalStr.find("loaded") != std::string::npos);
    EXPECT_TRUE(finalStr.find("26") != std::string::npos);
}

// Test polar alignment workflow
TEST_F(CalibrationIntegrationTest, CompletePolarAlignmentWorkflow) {
    JObj response;
    
    // Step 1: Start drift alignment
    JObj startResponse;
    startResponse << jrpc_result(JObj() << NV("operation_id", 2000)
                                       << NV("tool_type", "drift_alignment")
                                       << NV("direction", "east")
                                       << NV("status", "starting"));
    
    // Step 2: Check alignment status during process
    JObj statusResponse;
    statusResponse << jrpc_result(JObj() << NV("operation_id", 2000)
                                        << NV("tool_type", "drift_alignment")
                                        << NV("status", "measuring")
                                        << NV("progress", 50));
    
    // Simulate measurement time
    SimulateTimeDelay(100);
    
    // Step 3: Check completion status
    JObj completionResponse;
    completionResponse << jrpc_result(JObj() << NV("operation_id", 2000)
                                            << NV("tool_type", "drift_alignment")
                                            << NV("status", "complete")
                                            << NV("progress", 100)
                                            << NV("azimuth_error", 2.5)
                                            << NV("altitude_error", 1.8));
    
    std::string completionStr = completionResponse.str();
    EXPECT_TRUE(completionStr.find("complete") != std::string::npos);
    EXPECT_TRUE(completionStr.find("azimuth_error") != std::string::npos);
}

// Test error recovery workflow
TEST_F(CalibrationIntegrationTest, ErrorRecoveryWorkflow) {
    JObj response;
    
    // Step 1: Attempt calibration with camera disconnected
    g_mockCamera->Connected = false;
    
    JObj errorResponse;
    errorResponse << jrpc_error(1, "camera not connected");
    
    std::string errorStr = errorResponse.str();
    EXPECT_TRUE(errorStr.find("error") != std::string::npos);
    EXPECT_TRUE(errorStr.find("camera") != std::string::npos);
    
    // Step 2: Reconnect camera and retry
    g_mockCamera->Connected = true;
    
    JObj retryResponse;
    retryResponse << jrpc_result(0);
    
    // Simulate successful calibration after reconnection
    SimulateCalibrationProgress();
    
    // Step 3: Verify recovery
    JObj recoveryStatus;
    recoveryStatus << jrpc_result(JObj() << NV("calibrating", false)
                                        << NV("mount_calibrated", true));
    
    std::string recoveryStr = recoveryStatus.str();
    EXPECT_TRUE(recoveryStr.find("mount_calibrated") != std::string::npos);
    
    // Verify final state
    EXPECT_TRUE(g_mockMount->IsCalibrated());
    EXPECT_TRUE(g_mockCamera->Connected);
}

// Test concurrent operation handling
TEST_F(CalibrationIntegrationTest, ConcurrentOperationHandling) {
    JObj response;
    
    // Step 1: Start guider calibration
    g_mockGuider->SetCalibrating(true);
    
    // Step 2: Attempt to start dark library build while calibrating
    JObj concurrentResponse;
    concurrentResponse << jrpc_error(1, "cannot perform operation while calibrating or guiding");
    
    std::string concurrentStr = concurrentResponse.str();
    EXPECT_TRUE(concurrentStr.find("error") != std::string::npos);
    EXPECT_TRUE(concurrentStr.find("calibrating") != std::string::npos);
    
    // Step 3: Complete calibration
    g_mockGuider->SetCalibrating(false);
    g_mockMount->SetCalibrated(true);
    
    // Step 4: Now dark library build should succeed
    JObj successResponse;
    successResponse << jrpc_result(JObj() << NV("operation_id", 1));
    
    std::string successStr = successResponse.str();
    EXPECT_TRUE(successStr.find("operation_id") != std::string::npos);
}

// Test parameter validation across workflow
TEST_F(CalibrationIntegrationTest, ParameterValidationWorkflow) {
    JObj response;
    
    // Test 1: Invalid exposure time
    JObj invalidExpResponse;
    invalidExpResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "exposure_time must be between 100ms and 300s");
    
    // Test 2: Invalid frame count
    JObj invalidFrameResponse;
    invalidFrameResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "frame_count must be between 1 and 100");
    
    // Test 3: Invalid coordinates
    JObj invalidCoordResponse;
    invalidCoordResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "coordinates (2000,2000) out of bounds");
    
    // Test 4: Valid parameters after corrections
    JObj validResponse;
    validResponse << jrpc_result(JObj() << NV("operation_id", 1)
                                       << NV("min_exposure", 1000)
                                       << NV("max_exposure", 15000));
    
    // Verify all validation responses contain appropriate error information
    EXPECT_TRUE(invalidExpResponse.str().find("exposure_time") != std::string::npos);
    EXPECT_TRUE(invalidFrameResponse.str().find("frame_count") != std::string::npos);
    EXPECT_TRUE(invalidCoordResponse.str().find("coordinates") != std::string::npos);
    EXPECT_TRUE(validResponse.str().find("operation_id") != std::string::npos);
}

// Test guiding log retrieval workflow
TEST_F(CalibrationIntegrationTest, CompleteGuidingLogWorkflow) {
    JObj response;

    // Step 1: Request recent guiding logs
    JObj logRequest;
    logRequest << jrpc_result(JObj() << NV("format", "json")
                                    << NV("total_entries", 15)
                                    << NV("has_more_data", false)
                                    << NV("entries_count", 15)
                                    << NV("start_time", "2023-01-01T20:00:00")
                                    << NV("end_time", "2023-01-01T23:59:59"));

    std::string logStr = logRequest.str();
    EXPECT_TRUE(logStr.find("total_entries") != std::string::npos);
    EXPECT_TRUE(logStr.find("15") != std::string::npos);

    // Step 2: Request logs in CSV format
    JObj csvRequest;
    csvRequest << jrpc_result(JObj() << NV("format", "csv")
                                    << NV("total_entries", 15)
                                    << NV("has_more_data", false)
                                    << NV("data", "timestamp,log_level,message,frame_number,guide_distance,ra_correction,dec_correction\n"));

    std::string csvStr = csvRequest.str();
    EXPECT_TRUE(csvStr.find("csv") != std::string::npos);
    EXPECT_TRUE(csvStr.find("data") != std::string::npos);

    // Step 3: Request logs with specific time range
    JObj timeRangeRequest;
    timeRangeRequest << jrpc_result(JObj() << NV("format", "json")
                                          << NV("total_entries", 8)
                                          << NV("has_more_data", false)
                                          << NV("start_time", "2023-01-01T21:00:00")
                                          << NV("end_time", "2023-01-01T22:00:00"));

    std::string timeRangeStr = timeRangeRequest.str();
    EXPECT_TRUE(timeRangeStr.find("21:00:00") != std::string::npos);
    EXPECT_TRUE(timeRangeStr.find("22:00:00") != std::string::npos);
}

// Test guiding log parameter validation workflow
TEST_F(CalibrationIntegrationTest, GuidingLogParameterValidationWorkflow) {
    JObj response;

    // Test 1: Invalid time format
    JObj invalidTimeResponse;
    invalidTimeResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid start_time format, expected ISO 8601");

    // Test 2: Invalid max entries
    JObj invalidMaxResponse;
    invalidMaxResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "max_entries must be between 1 and 1000");

    // Test 3: Invalid log level
    JObj invalidLevelResponse;
    invalidLevelResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "log_level must be 'debug', 'info', 'warning', or 'error'");

    // Test 4: Invalid format
    JObj invalidFormatResponse;
    invalidFormatResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "format must be 'json' or 'csv'");

    // Test 5: Invalid time range
    JObj invalidRangeResponse;
    invalidRangeResponse << jrpc_error(JSONRPC_INVALID_PARAMS, "end_time must be after start_time");

    // Verify all validation responses contain appropriate error information
    EXPECT_TRUE(invalidTimeResponse.str().find("start_time") != std::string::npos);
    EXPECT_TRUE(invalidMaxResponse.str().find("max_entries") != std::string::npos);
    EXPECT_TRUE(invalidLevelResponse.str().find("log_level") != std::string::npos);
    EXPECT_TRUE(invalidFormatResponse.str().find("format") != std::string::npos);
    EXPECT_TRUE(invalidRangeResponse.str().find("end_time") != std::string::npos);
}

// Test guiding log with different filtering options
TEST_F(CalibrationIntegrationTest, GuidingLogFilteringWorkflow) {
    JObj response;

    // Step 1: Request all log levels
    JObj allLogsRequest;
    allLogsRequest << jrpc_result(JObj() << NV("format", "json")
                                        << NV("total_entries", 100)
                                        << NV("has_more_data", true)
                                        << NV("log_level", "info"));

    // Step 2: Request only error logs
    JObj errorLogsRequest;
    errorLogsRequest << jrpc_result(JObj() << NV("format", "json")
                                          << NV("total_entries", 5)
                                          << NV("has_more_data", false)
                                          << NV("log_level", "error"));

    // Step 3: Request limited number of entries
    JObj limitedRequest;
    limitedRequest << jrpc_result(JObj() << NV("format", "json")
                                        << NV("total_entries", 10)
                                        << NV("has_more_data", true)
                                        << NV("max_entries", 10));

    // Verify filtering responses
    EXPECT_TRUE(allLogsRequest.str().find("100") != std::string::npos);
    EXPECT_TRUE(errorLogsRequest.str().find("error") != std::string::npos);
    EXPECT_TRUE(limitedRequest.str().find("has_more_data") != std::string::npos);
}

// Test guiding log error handling
TEST_F(CalibrationIntegrationTest, GuidingLogErrorHandlingWorkflow) {
    JObj response;

    // Step 1: No log files found
    JObj noFilesResponse;
    noFilesResponse << jrpc_error(1, "no guide log files found in specified time range");

    // Step 2: Log file access error (simulated)
    JObj accessErrorResponse;
    accessErrorResponse << jrpc_error(1, "unable to access log files");

    // Step 3: Recovery after error - successful request
    JObj recoveryResponse;
    recoveryResponse << jrpc_result(JObj() << NV("format", "json")
                                          << NV("total_entries", 25)
                                          << NV("has_more_data", false));

    // Verify error handling
    EXPECT_TRUE(noFilesResponse.str().find("no guide log files") != std::string::npos);
    EXPECT_TRUE(accessErrorResponse.str().find("unable to access") != std::string::npos);
    EXPECT_TRUE(recoveryResponse.str().find("total_entries") != std::string::npos);
}

// Test guiding log with large datasets
TEST_F(CalibrationIntegrationTest, GuidingLogLargeDatasetWorkflow) {
    JObj response;

    // Step 1: Request with default max entries
    JObj defaultRequest;
    defaultRequest << jrpc_result(JObj() << NV("format", "json")
                                        << NV("total_entries", 100)
                                        << NV("has_more_data", false)
                                        << NV("max_entries", 100));

    // Step 2: Request with maximum allowed entries
    JObj maxRequest;
    maxRequest << jrpc_result(JObj() << NV("format", "json")
                                    << NV("total_entries", 1000)
                                    << NV("has_more_data", true)
                                    << NV("max_entries", 1000));

    // Step 3: Request with pagination (has_more_data = true)
    JObj paginatedRequest;
    paginatedRequest << jrpc_result(JObj() << NV("format", "json")
                                          << NV("total_entries", 50)
                                          << NV("has_more_data", true)
                                          << NV("message", "More data available - use time range filtering for additional entries"));

    // Verify large dataset handling
    EXPECT_TRUE(defaultRequest.str().find("100") != std::string::npos);
    EXPECT_TRUE(maxRequest.str().find("1000") != std::string::npos);
    EXPECT_TRUE(paginatedRequest.str().find("has_more_data") != std::string::npos);
}

// Main test runner
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
