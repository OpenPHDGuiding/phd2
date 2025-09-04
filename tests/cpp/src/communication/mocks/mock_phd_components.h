/*
 * mock_phd_components.h
 * PHD Guiding - Communication Module Tests
 *
 * Mock objects for PHD2 components used in communication tests
 * Provides controllable behavior for cameras, mounts, and other PHD2 objects
 */

#ifndef MOCK_PHD_COMPONENTS_H
#define MOCK_PHD_COMPONENTS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/event.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class PHDComponentSimulator;

// Mock Camera interface
class MockCamera {
public:
    // Camera identification
    MOCK_METHOD0(Name, wxString());
    MOCK_METHOD0(GetCameraStatusStr, wxString());
    MOCK_METHOD0(IsConnected, bool());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    
    // Camera properties
    MOCK_METHOD0(FullSize, wxSize());
    MOCK_METHOD0(HasNonGuiCapture, bool());
    MOCK_METHOD0(HasDelayParam, bool());
    MOCK_METHOD0(HasPortNum, bool());
    MOCK_METHOD0(HasGainControl, bool());
    MOCK_METHOD0(HasShutter, bool());
    MOCK_METHOD0(HasSubframes, bool());
    MOCK_METHOD0(HasCooler, bool());
    
    // ST4 interface
    MOCK_METHOD0(ST4HasGuideOutput, bool());
    MOCK_METHOD0(ST4HostConnected, bool());
    MOCK_METHOD4(ST4PulseGuideScope, bool(int direction, int duration, bool async, bool* pulsePending));
    
    // Helper methods for testing
    MOCK_METHOD1(SetConnected, void(bool connected));
    MOCK_METHOD1(SetST4Available, void(bool available));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockCamera* instance;
    static MockCamera* GetInstance();
    static void SetInstance(MockCamera* inst);
};

// Mock Mount interface
class MockMount {
public:
    // Mount identification
    MOCK_METHOD0(Name, wxString());
    MOCK_METHOD0(GetMountClassName, wxString());
    MOCK_METHOD0(IsConnected, bool());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    
    // Mount capabilities
    MOCK_METHOD0(HasSetupDialog, bool());
    MOCK_METHOD0(RequiresCamera, bool());
    MOCK_METHOD0(RequiresStepGuider, bool());
    MOCK_METHOD0(CalibrationFlipRequiresDecFlip, bool());
    MOCK_METHOD0(IsStepGuider, bool());
    
    // Guiding operations
    MOCK_METHOD2(Guide, MOVE_RESULT(GUIDE_DIRECTION direction, int duration));
    MOCK_METHOD1(CalibrationMoveSize, int(GUIDE_DIRECTION direction));
    MOCK_METHOD1(MaxMoveSize, int(GUIDE_DIRECTION direction));
    
    // Mount state
    MOCK_METHOD0(IsCalibrated, bool());
    MOCK_METHOD0(GetCalibrationAngle, double());
    MOCK_METHOD1(SetCalibrationAngle, void(double angle));
    MOCK_METHOD0(GetGuidingEnabled, bool());
    MOCK_METHOD1(SetGuidingEnabled, void(bool enabled));
    
    // Helper methods for testing
    MOCK_METHOD1(SetConnected, void(bool connected));
    MOCK_METHOD1(SetCalibrated, void(bool calibrated));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SetMoveResult, void(GUIDE_DIRECTION direction, MOVE_RESULT result));
    
    static MockMount* instance;
    static MockMount* GetInstance();
    static void SetInstance(MockMount* inst);
};

// Mock StepGuider interface
class MockStepGuider {
public:
    // StepGuider identification
    MOCK_METHOD0(Name, wxString());
    MOCK_METHOD0(IsConnected, bool());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    
    // StepGuider operations
    MOCK_METHOD2(Guide, MOVE_RESULT(GUIDE_DIRECTION direction, int steps));
    MOCK_METHOD0(MaxPosition, int(GUIDE_DIRECTION direction));
    MOCK_METHOD0(GetPosition, int(GUIDE_DIRECTION direction));
    
    // Helper methods for testing
    MOCK_METHOD1(SetConnected, void(bool connected));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockStepGuider* instance;
    static MockStepGuider* GetInstance();
    static void SetInstance(MockStepGuider* inst);
};

// Mock RotatorConnection interface
class MockRotatorConnection {
public:
    // Rotator identification
    MOCK_METHOD0(Name, wxString());
    MOCK_METHOD0(IsConnected, bool());
    
    // Connection management
    MOCK_METHOD0(Connect, bool());
    MOCK_METHOD0(Disconnect, bool());
    
    // Rotator operations
    MOCK_METHOD0(Position, float());
    MOCK_METHOD1(SetPosition, bool(float position));
    MOCK_METHOD0(IsMoving, bool());
    MOCK_METHOD0(CanReverse, bool());
    MOCK_METHOD0(IsReversed, bool());
    MOCK_METHOD1(SetReversed, void(bool reversed));
    
    // Helper methods for testing
    MOCK_METHOD1(SetConnected, void(bool connected));
    MOCK_METHOD1(SetPosition, void(float position));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockRotatorConnection* instance;
    static MockRotatorConnection* GetInstance();
    static void SetInstance(MockRotatorConnection* inst);
};

// Mock EventServer for testing network communication
class MockEventServer {
public:
    // Server management
    MOCK_METHOD1(EventServerStart, bool(unsigned int instanceId));
    MOCK_METHOD0(EventServerStop, void());
    MOCK_METHOD0(IsEventServerRunning, bool());
    MOCK_METHOD0(GetEventServerPort, unsigned int());
    
    // Event handling
    MOCK_METHOD2(NotifyStartCalibration, void(Mount* mount, const wxString& msg));
    MOCK_METHOD2(NotifyCalibrationComplete, void(Mount* mount, const wxString& msg));
    MOCK_METHOD2(NotifyCalibrationFailed, void(Mount* mount, const wxString& msg));
    MOCK_METHOD0(NotifyStartGuiding, void());
    MOCK_METHOD0(NotifyGuidingStopped, void());
    MOCK_METHOD0(NotifyPaused, void());
    MOCK_METHOD0(NotifyResumed, void());
    MOCK_METHOD2(NotifyGuidingDithered, void(double dx, double dy));
    MOCK_METHOD1(NotifySettlingStateChange, void(const wxString& msg));
    MOCK_METHOD1(NotifyAlert, void(const wxString& msg));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetPort, void(unsigned int port));
    MOCK_METHOD1(SimulateClientConnection, void(bool connected));
    
    static MockEventServer* instance;
    static MockEventServer* GetInstance();
    static void SetInstance(MockEventServer* inst);
};

// Mock SocketServer for testing legacy socket communication
class MockSocketServer {
public:
    // Server management
    MOCK_METHOD1(SocketServerStart, bool(unsigned int instanceId));
    MOCK_METHOD0(SocketServerStop, void());
    MOCK_METHOD0(IsSocketServerRunning, bool());
    MOCK_METHOD0(GetSocketServerPort, unsigned int());
    
    // Client management
    MOCK_METHOD0(GetClientCount, int());
    MOCK_METHOD1(SendToAllClients, void(const wxString& message));
    MOCK_METHOD2(SendToClient, void(int clientId, const wxString& message));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetPort, void(unsigned int port));
    MOCK_METHOD1(SimulateClientConnection, void(int clientId));
    MOCK_METHOD1(SimulateClientDisconnection, void(int clientId));
    MOCK_METHOD2(SimulateClientMessage, void(int clientId, const wxString& message));
    
    static MockSocketServer* instance;
    static MockSocketServer* GetInstance();
    static void SetInstance(MockSocketServer* inst);
};

// PHD component simulator for comprehensive testing
class PHDComponentSimulator {
public:
    struct CameraInfo {
        wxString name;
        bool isConnected;
        bool hasST4;
        bool hasShutter;
        bool hasGainControl;
        wxSize fullSize;
        bool shouldFail;
        
        CameraInfo() : name("Mock Camera"), isConnected(false), hasST4(true),
                      hasShutter(true), hasGainControl(true), fullSize(1024, 768),
                      shouldFail(false) {}
    };
    
    struct MountInfo {
        wxString name;
        wxString className;
        bool isConnected;
        bool isCalibrated;
        bool isStepGuider;
        double calibrationAngle;
        bool guidingEnabled;
        bool shouldFail;
        std::map<int, int> moveResults; // direction -> MOVE_RESULT
        
        MountInfo() : name("Mock Mount"), className("MockMount"), isConnected(false),
                     isCalibrated(false), isStepGuider(false), calibrationAngle(0.0),
                     guidingEnabled(true), shouldFail(false) {}
    };
    
    struct ServerInfo {
        bool isRunning;
        unsigned int port;
        int clientCount;
        bool shouldFail;
        std::vector<int> connectedClients;
        
        ServerInfo() : isRunning(false), port(0), clientCount(0), shouldFail(false) {}
    };
    
    // Component management
    void SetupCamera(const CameraInfo& info);
    void SetupMount(const MountInfo& info);
    void SetupEventServer(const ServerInfo& info);
    void SetupSocketServer(const ServerInfo& info);
    
    // State management
    CameraInfo GetCameraInfo() const;
    MountInfo GetMountInfo() const;
    ServerInfo GetEventServerInfo() const;
    ServerInfo GetSocketServerInfo() const;
    
    // Connection simulation
    void SimulateCameraConnection(bool connected);
    void SimulateMountConnection(bool connected);
    void SimulateServerStart(bool eventServer, unsigned int port);
    void SimulateServerStop(bool eventServer);
    void SimulateClientConnection(bool eventServer, int clientId);
    
    // Error simulation
    void SetCameraError(bool error);
    void SetMountError(bool error);
    void SetServerError(bool eventServer, bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultComponents();
    
private:
    CameraInfo cameraInfo;
    MountInfo mountInfo;
    ServerInfo eventServerInfo;
    ServerInfo socketServerInfo;
    
    void InitializeDefaults();
};

// Helper class to manage all PHD component mocks
class MockPHDComponentsManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockCamera* GetMockCamera();
    static MockMount* GetMockMount();
    static MockStepGuider* GetMockStepGuider();
    static MockRotatorConnection* GetMockRotatorConnection();
    static MockEventServer* GetMockEventServer();
    static MockSocketServer* GetMockSocketServer();
    static PHDComponentSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupConnectedCamera();
    static void SetupConnectedMount();
    static void SetupRunningServers();
    static void SimulateEquipmentFailure();
    static void SimulateNetworkFailure();
    
private:
    static MockCamera* mockCamera;
    static MockMount* mockMount;
    static MockStepGuider* mockStepGuider;
    static MockRotatorConnection* mockRotatorConnection;
    static MockEventServer* mockEventServer;
    static MockSocketServer* mockSocketServer;
    static std::unique_ptr<PHDComponentSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_PHD_COMPONENT_MOCKS() MockPHDComponentsManager::SetupMocks()
#define TEARDOWN_PHD_COMPONENT_MOCKS() MockPHDComponentsManager::TeardownMocks()
#define RESET_PHD_COMPONENT_MOCKS() MockPHDComponentsManager::ResetMocks()

#define GET_MOCK_CAMERA() MockPHDComponentsManager::GetMockCamera()
#define GET_MOCK_MOUNT() MockPHDComponentsManager::GetMockMount()
#define GET_MOCK_STEP_GUIDER() MockPHDComponentsManager::GetMockStepGuider()
#define GET_MOCK_ROTATOR() MockPHDComponentsManager::GetMockRotatorConnection()
#define GET_MOCK_EVENT_SERVER() MockPHDComponentsManager::GetMockEventServer()
#define GET_MOCK_SOCKET_SERVER() MockPHDComponentsManager::GetMockSocketServer()
#define GET_PHD_SIMULATOR() MockPHDComponentsManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_CAMERA_CONNECT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_CAMERA(), Connect()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CAMERA_CONNECT_FAILURE() \
    EXPECT_CALL(*GET_MOCK_CAMERA(), Connect()) \
        .WillOnce(::testing::Return(false))

#define EXPECT_MOUNT_GUIDE_SUCCESS(direction, duration) \
    EXPECT_CALL(*GET_MOCK_MOUNT(), Guide(direction, duration)) \
        .WillOnce(::testing::Return(MOVE_OK))

#define EXPECT_MOUNT_GUIDE_FAILURE(direction, duration) \
    EXPECT_CALL(*GET_MOCK_MOUNT(), Guide(direction, duration)) \
        .WillOnce(::testing::Return(MOVE_ERROR))

#define EXPECT_ST4_PULSE_SUCCESS(direction, duration) \
    EXPECT_CALL(*GET_MOCK_CAMERA(), ST4PulseGuideScope(direction, duration, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERVER_START_SUCCESS(port) \
    EXPECT_CALL(*GET_MOCK_EVENT_SERVER(), EventServerStart(::testing::_)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_SERVER_START_FAILURE() \
    EXPECT_CALL(*GET_MOCK_EVENT_SERVER(), EventServerStart(::testing::_)) \
        .WillOnce(::testing::Return(false))

// Define common enums used in PHD2 (simplified for testing)
enum GUIDE_DIRECTION {
    NORTH = 0,
    SOUTH = 1,
    EAST = 2,
    WEST = 3
};

enum MOVE_RESULT {
    MOVE_OK = 0,
    MOVE_ERROR = 1,
    MOVE_STOP_GUIDING = 2
};

#endif // MOCK_PHD_COMPONENTS_H
