/*
 * event_server_integration_tests.cpp
 * PHD Guiding
 *
 * Integration tests for the EventServer module
 * Tests interaction with PHD2 core components and real-world scenarios
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <wx/socket.h>
#include <wx/string.h>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <atomic>

#include "event_server_mocks.h"
#include "../src/communication/network/event_server.h"
#include "../src/communication/network/json_parser.h"

// Integration test fixture
class EventServerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock objects with realistic defaults
        g_mockCamera = new MockCamera();
        g_mockMount = new MockMount();
        g_mockSecondaryMount = new MockMount();
        g_mockGuider = new MockGuider();
        g_mockFrame = new MockFrame();
        g_mockApp = new MockApp();
        
        // Set up realistic expectations
        SetupMockExpectations(g_mockCamera, g_mockMount, g_mockGuider, g_mockFrame);
        
        // Initialize wxWidgets socket system
        wxSocketBase::Initialize();
        
        // Create event server instance
        eventServer = std::make_unique<EventServer>();
    }
    
    void TearDown() override {
        if (eventServer) {
            eventServer->EventServerStop();
            eventServer.reset();
        }
        
        delete g_mockCamera;
        delete g_mockMount;
        delete g_mockSecondaryMount;
        delete g_mockGuider;
        delete g_mockFrame;
        delete g_mockApp;
        
        wxSocketBase::Shutdown();
    }
    
    // Helper to create a client connection
    std::unique_ptr<wxSocketClient> CreateClient(int timeoutSec = 5) {
        auto client = std::make_unique<wxSocketClient>();
        client->SetTimeout(timeoutSec);
        
        wxIPV4address addr;
        addr.Hostname("localhost");
        addr.Service(4400);
        
        if (client->Connect(addr, false)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return client;
        }
        return nullptr;
    }
    
    // Helper to send JSON-RPC request
    bool SendJsonRpcRequest(wxSocketClient* client, const std::string& request) {
        std::string fullRequest = request + "\r\n";
        client->Write(fullRequest.c_str(), fullRequest.length());
        return !client->Error();
    }
    
    // Helper to read JSON-RPC response
    std::string ReadJsonRpcResponse(wxSocketClient* client, int timeoutMs = 1000) {
        if (!client->WaitForRead(timeoutMs / 1000, (timeoutMs % 1000) * 1000)) {
            return "";
        }
        
        char buffer[4096];
        client->Read(buffer, sizeof(buffer) - 1);
        buffer[client->LastCount()] = '\0';
        return std::string(buffer);
    }
    
    std::unique_ptr<EventServer> eventServer;
};

// Test complete calibration workflow
TEST_F(EventServerIntegrationTest, CompleteCalibrationWorkflow) {
    // Start event server
    ASSERT_FALSE(eventServer->EventServerStart(1));
    
    // Create client connection
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Step 1: Check initial connection status
    std::string getConnectedRequest = R"({"method":"get_connected","params":{},"id":1})";
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), getConnectedRequest));
    
    std::string response = ReadJsonRpcResponse(client.get());
    EXPECT_FALSE(response.empty());
    EXPECT_TRUE(response.find("result") != std::string::npos);
    
    // Step 2: Start calibration
    std::string startCalibrationRequest = R"({
        "method":"start_guider_calibration",
        "params":{
            "force_recalibration": false,
            "settle": {
                "pixels": 1.5,
                "time": 10,
                "timeout": 60,
                "frames": 99
            }
        },
        "id":2
    })";
    
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), startCalibrationRequest));
    response = ReadJsonRpcResponse(client.get());
    EXPECT_FALSE(response.empty());
    
    // Step 3: Simulate calibration events
    CalibrationStepInfo stepInfo;
    stepInfo.mount = g_mockMount;
    stepInfo.phase = "Clearing backlash";
    stepInfo.direction = 0; // North
    stepInfo.dist = 5.0;
    stepInfo.dx = 2.5;
    stepInfo.dy = 1.8;
    stepInfo.pos = PHD_Point(512, 384);
    stepInfo.step = 1;
    
    // Send calibration step notifications
    for (int i = 0; i < 5; ++i) {
        stepInfo.step = i + 1;
        stepInfo.dist = 5.0 + i * 2.0;
        EXPECT_NO_THROW(eventServer->NotifyCalibrationStep(stepInfo));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Step 4: Complete calibration
    EXPECT_NO_THROW(eventServer->NotifyCalibrationComplete(g_mockMount));
    
    // Step 5: Check calibration status
    std::string getCalibrationRequest = R"({"method":"get_calibration_status","params":{},"id":3})";
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), getCalibrationRequest));
    response = ReadJsonRpcResponse(client.get());
    EXPECT_FALSE(response.empty());
    EXPECT_TRUE(response.find("calibrated") != std::string::npos);
}

// Test complete guiding session workflow
TEST_F(EventServerIntegrationTest, CompleteGuidingSessionWorkflow) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Step 1: Start guiding
    std::string startGuidingRequest = R"({
        "method":"guide",
        "params":{
            "settle": {
                "pixels": 1.5,
                "time": 10,
                "timeout": 60,
                "frames": 99
            },
            "recalibrate": false
        },
        "id":1
    })";
    
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), startGuidingRequest));
    std::string response = ReadJsonRpcResponse(client.get());
    EXPECT_FALSE(response.empty());
    
    // Step 2: Notify guiding started
    EXPECT_NO_THROW(eventServer->NotifyGuidingStarted());
    
    // Step 3: Simulate guide steps
    GuideStepInfo stepInfo;
    stepInfo.mount = g_mockMount;
    stepInfo.time = 1.0;
    stepInfo.cameraOffset = PHD_Point(0.5, -0.3);
    stepInfo.mountOffset = PHD_Point(0.2, -0.1);
    stepInfo.guideDistanceRA = 0.15;
    stepInfo.guideDistanceDec = 0.08;
    stepInfo.durationRA = 150;
    stepInfo.directionRA = 0; // North
    stepInfo.durationDec = 80;
    stepInfo.directionDec = 2; // East
    
    // Send multiple guide steps
    for (int i = 0; i < 10; ++i) {
        stepInfo.frameNumber = i + 1;
        stepInfo.time = 1.0 + i * 0.5;
        stepInfo.cameraOffset.X = 0.5 + (i % 3 - 1) * 0.1;
        stepInfo.cameraOffset.Y = -0.3 + (i % 2) * 0.05;
        
        EXPECT_NO_THROW(eventServer->NotifyGuideStep(stepInfo));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    // Step 4: Test dithering
    std::string ditherRequest = R"({
        "method":"dither",
        "params":{
            "amount": 5.0,
            "raOnly": false,
            "settle": {
                "pixels": 1.5,
                "time": 10,
                "timeout": 60,
                "frames": 99
            }
        },
        "id":2
    })";
    
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), ditherRequest));
    response = ReadJsonRpcResponse(client.get());
    EXPECT_FALSE(response.empty());
    
    // Simulate dither completion
    EXPECT_NO_THROW(eventServer->NotifyGuidingDithered(2.5, 1.8));
    
    // Step 5: Stop guiding
    std::string stopGuidingRequest = R"({"method":"stop_capture","params":{},"id":3})";
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), stopGuidingRequest));
    response = ReadJsonRpcResponse(client.get());
    EXPECT_FALSE(response.empty());
    
    EXPECT_NO_THROW(eventServer->NotifyGuidingStopped());
}

// Test equipment connection/disconnection scenarios
TEST_F(EventServerIntegrationTest, EquipmentConnectionScenarios) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Test camera disconnection during operation
    EXPECT_CALL(*g_mockCamera, GetDarkLibraryProperties(testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Invoke(g_mockCamera, &MockCamera::GetDarkLibraryPropertiesImpl));
    
    g_mockCamera->Connected = false;
    
    std::string getConnectedRequest = R"({"method":"get_connected","params":{},"id":1})";
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), getConnectedRequest));
    std::string response = ReadJsonRpcResponse(client.get());
    EXPECT_TRUE(response.find("false") != std::string::npos);
    
    // Test mount disconnection
    EXPECT_CALL(*g_mockMount, IsConnected())
        .WillRepeatedly(testing::Return(false));
    
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), getConnectedRequest));
    response = ReadJsonRpcResponse(client.get());
    EXPECT_TRUE(response.find("false") != std::string::npos);
    
    // Test reconnection
    g_mockCamera->Connected = true;
    EXPECT_CALL(*g_mockMount, IsConnected())
        .WillRepeatedly(testing::Return(true));
    
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), getConnectedRequest));
    response = ReadJsonRpcResponse(client.get());
    EXPECT_TRUE(response.find("true") != std::string::npos);
}

// Test error recovery scenarios
TEST_F(EventServerIntegrationTest, ErrorRecoveryScenarios) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Test star lost scenario
    FrameDroppedInfo dropInfo;
    dropInfo.starError = 1; // STAR_LOWSNR
    dropInfo.starMass = 50.0; // Low mass
    dropInfo.starSNR = 3.0;   // Low SNR
    dropInfo.status = "Star lost - low SNR";
    dropInfo.avgDist = 2.5;
    
    EXPECT_NO_THROW(eventServer->NotifyStarLost(dropInfo));
    
    // Test calibration failure
    wxString errorMsg = "Calibration failed - insufficient star movement";
    EXPECT_NO_THROW(eventServer->NotifyCalibrationFailed(g_mockMount, errorMsg));
    
    // Test alert notifications
    EXPECT_NO_THROW(eventServer->NotifyAlert("Camera disconnected", 2));
    EXPECT_NO_THROW(eventServer->NotifyAlert("Mount not responding", 3));
    
    // Test recovery - star reacquired
    PHD_Point newStarPos(256, 192);
    EXPECT_NO_THROW(eventServer->NotifyStarSelected(newStarPos));
}

// Test configuration management
TEST_F(EventServerIntegrationTest, ConfigurationManagement) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Test parameter changes
    EXPECT_NO_THROW(eventServer->NotifyGuidingParam("SearchRegion", 15));
    EXPECT_NO_THROW(eventServer->NotifyGuidingParam("MinMovePixels", 0.15));
    EXPECT_NO_THROW(eventServer->NotifyGuidingParam("CalibrationFlipRequiresDecFlip", true));
    EXPECT_NO_THROW(eventServer->NotifyGuidingParam("CameraGain", "High"));
    
    // Test configuration change notification
    EXPECT_NO_THROW(eventServer->NotifyConfigurationChange());
    
    // Test profile switching
    std::string setProfileRequest = R"({
        "method":"set_profile",
        "params":{"id": 2},
        "id":1
    })";
    
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), setProfileRequest));
    std::string response = ReadJsonRpcResponse(client.get());
    EXPECT_FALSE(response.empty());
}

// Test settle monitoring
TEST_F(EventServerIntegrationTest, SettleMonitoring) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Start settle monitoring
    EXPECT_NO_THROW(eventServer->NotifySettleBegin());
    
    // Simulate settling process
    for (int i = 0; i < 10; ++i) {
        double distance = 5.0 - i * 0.4; // Decreasing distance
        double time = i * 1.0;
        double settleTime = 10.0;
        bool starLocked = distance < 1.5;
        
        EXPECT_NO_THROW(eventServer->NotifySettling(distance, time, settleTime, starLocked));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (starLocked && distance < 1.0) {
            break; // Settled
        }
    }
    
    // Complete settling
    wxString settleError = ""; // No error
    int settleFrames = 8;
    int droppedFrames = 1;
    
    EXPECT_NO_THROW(eventServer->NotifySettleDone(settleError, settleFrames, droppedFrames));
}

// Test batch request processing
TEST_F(EventServerIntegrationTest, BatchRequestProcessing) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Send batch request
    std::string batchRequest = R"([
        {"method":"get_connected","params":{},"id":1},
        {"method":"get_exposure","params":{},"id":2},
        {"method":"get_calibration_status","params":{},"id":3}
    ])";
    
    ASSERT_TRUE(SendJsonRpcRequest(client.get(), batchRequest));
    std::string response = ReadJsonRpcResponse(client.get(), 2000); // Longer timeout for batch
    
    EXPECT_FALSE(response.empty());
    EXPECT_TRUE(response.find("[") != std::string::npos); // Should be array response
    EXPECT_TRUE(response.find("result") != std::string::npos);
}

// Test long-running session stability
TEST_F(EventServerIntegrationTest, LongRunningSessionStability) {
    ASSERT_FALSE(eventServer->EventServerStart(1));
    auto client = CreateClient();
    ASSERT_NE(client, nullptr);
    
    // Simulate a long guiding session with many events
    const int numEvents = 100;
    GuideStepInfo stepInfo;
    stepInfo.mount = g_mockMount;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numEvents; ++i) {
        stepInfo.frameNumber = i + 1;
        stepInfo.time = i * 0.5;
        stepInfo.cameraOffset = PHD_Point(
            0.1 * sin(i * 0.1), 
            0.1 * cos(i * 0.1)
        );
        
        EXPECT_NO_THROW(eventServer->NotifyGuideStep(stepInfo));
        
        // Occasionally send other events
        if (i % 20 == 0) {
            EXPECT_NO_THROW(eventServer->NotifyLooping(i, nullptr, nullptr));
        }
        
        if (i % 50 == 0) {
            EXPECT_NO_THROW(eventServer->NotifyConfigurationChange());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
    
    // Client should still be connected
    EXPECT_TRUE(client->IsConnected());
}
