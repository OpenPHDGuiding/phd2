/*
 * mock_phd_components.cpp
 * PHD Guiding - Communication Module Tests
 *
 * Implementation of mock PHD2 components
 */

#include "mock_phd_components.h"

// Static instance declarations
MockCamera* MockCamera::instance = nullptr;
MockMount* MockMount::instance = nullptr;
MockStepGuider* MockStepGuider::instance = nullptr;
MockRotatorConnection* MockRotatorConnection::instance = nullptr;
MockEventServer* MockEventServer::instance = nullptr;
MockSocketServer* MockSocketServer::instance = nullptr;

// MockPHDComponentsManager static members
MockCamera* MockPHDComponentsManager::mockCamera = nullptr;
MockMount* MockPHDComponentsManager::mockMount = nullptr;
MockStepGuider* MockPHDComponentsManager::mockStepGuider = nullptr;
MockRotatorConnection* MockPHDComponentsManager::mockRotatorConnection = nullptr;
MockEventServer* MockPHDComponentsManager::mockEventServer = nullptr;
MockSocketServer* MockPHDComponentsManager::mockSocketServer = nullptr;
std::unique_ptr<PHDComponentSimulator> MockPHDComponentsManager::simulator = nullptr;

// MockCamera implementation
MockCamera* MockCamera::GetInstance() {
    return instance;
}

void MockCamera::SetInstance(MockCamera* inst) {
    instance = inst;
}

// MockMount implementation
MockMount* MockMount::GetInstance() {
    return instance;
}

void MockMount::SetInstance(MockMount* inst) {
    instance = inst;
}

// MockStepGuider implementation
MockStepGuider* MockStepGuider::GetInstance() {
    return instance;
}

void MockStepGuider::SetInstance(MockStepGuider* inst) {
    instance = inst;
}

// MockRotatorConnection implementation
MockRotatorConnection* MockRotatorConnection::GetInstance() {
    return instance;
}

void MockRotatorConnection::SetInstance(MockRotatorConnection* inst) {
    instance = inst;
}

// MockEventServer implementation
MockEventServer* MockEventServer::GetInstance() {
    return instance;
}

void MockEventServer::SetInstance(MockEventServer* inst) {
    instance = inst;
}

// MockSocketServer implementation
MockSocketServer* MockSocketServer::GetInstance() {
    return instance;
}

void MockSocketServer::SetInstance(MockSocketServer* inst) {
    instance = inst;
}

// PHDComponentSimulator implementation
void PHDComponentSimulator::SetupCamera(const CameraInfo& info) {
    cameraInfo = info;
}

void PHDComponentSimulator::SetupMount(const MountInfo& info) {
    mountInfo = info;
}

void PHDComponentSimulator::SetupEventServer(const ServerInfo& info) {
    eventServerInfo = info;
}

void PHDComponentSimulator::SetupSocketServer(const ServerInfo& info) {
    socketServerInfo = info;
}

PHDComponentSimulator::CameraInfo PHDComponentSimulator::GetCameraInfo() const {
    return cameraInfo;
}

PHDComponentSimulator::MountInfo PHDComponentSimulator::GetMountInfo() const {
    return mountInfo;
}

PHDComponentSimulator::ServerInfo PHDComponentSimulator::GetEventServerInfo() const {
    return eventServerInfo;
}

PHDComponentSimulator::ServerInfo PHDComponentSimulator::GetSocketServerInfo() const {
    return socketServerInfo;
}

void PHDComponentSimulator::SimulateCameraConnection(bool connected) {
    cameraInfo.isConnected = connected;
}

void PHDComponentSimulator::SimulateMountConnection(bool connected) {
    mountInfo.isConnected = connected;
}

void PHDComponentSimulator::SimulateServerStart(bool eventServer, unsigned int port) {
    if (eventServer) {
        eventServerInfo.isRunning = true;
        eventServerInfo.port = port;
    } else {
        socketServerInfo.isRunning = true;
        socketServerInfo.port = port;
    }
}

void PHDComponentSimulator::SimulateServerStop(bool eventServer) {
    if (eventServer) {
        eventServerInfo.isRunning = false;
        eventServerInfo.clientCount = 0;
        eventServerInfo.connectedClients.clear();
    } else {
        socketServerInfo.isRunning = false;
        socketServerInfo.clientCount = 0;
        socketServerInfo.connectedClients.clear();
    }
}

void PHDComponentSimulator::SimulateClientConnection(bool eventServer, int clientId) {
    ServerInfo& serverInfo = eventServer ? eventServerInfo : socketServerInfo;
    
    auto it = std::find(serverInfo.connectedClients.begin(), serverInfo.connectedClients.end(), clientId);
    if (it == serverInfo.connectedClients.end()) {
        serverInfo.connectedClients.push_back(clientId);
        serverInfo.clientCount = serverInfo.connectedClients.size();
    }
}

void PHDComponentSimulator::SetCameraError(bool error) {
    cameraInfo.shouldFail = error;
}

void PHDComponentSimulator::SetMountError(bool error) {
    mountInfo.shouldFail = error;
}

void PHDComponentSimulator::SetServerError(bool eventServer, bool error) {
    if (eventServer) {
        eventServerInfo.shouldFail = error;
    } else {
        socketServerInfo.shouldFail = error;
    }
}

void PHDComponentSimulator::Reset() {
    cameraInfo = CameraInfo();
    mountInfo = MountInfo();
    eventServerInfo = ServerInfo();
    socketServerInfo = ServerInfo();
    
    SetupDefaultComponents();
}

void PHDComponentSimulator::SetupDefaultComponents() {
    // Set up default camera
    cameraInfo.name = "Mock Camera";
    cameraInfo.isConnected = false;
    cameraInfo.hasST4 = true;
    cameraInfo.hasShutter = true;
    cameraInfo.hasGainControl = true;
    cameraInfo.fullSize = wxSize(1024, 768);
    cameraInfo.shouldFail = false;
    
    // Set up default mount
    mountInfo.name = "Mock Mount";
    mountInfo.className = "MockMount";
    mountInfo.isConnected = false;
    mountInfo.isCalibrated = false;
    mountInfo.isStepGuider = false;
    mountInfo.calibrationAngle = 0.0;
    mountInfo.guidingEnabled = true;
    mountInfo.shouldFail = false;
    
    // Set default move results
    mountInfo.moveResults[NORTH] = MOVE_OK;
    mountInfo.moveResults[SOUTH] = MOVE_OK;
    mountInfo.moveResults[EAST] = MOVE_OK;
    mountInfo.moveResults[WEST] = MOVE_OK;
    
    // Set up default servers
    eventServerInfo.isRunning = false;
    eventServerInfo.port = 4400;
    eventServerInfo.clientCount = 0;
    eventServerInfo.shouldFail = false;
    
    socketServerInfo.isRunning = false;
    socketServerInfo.port = 4300;
    socketServerInfo.clientCount = 0;
    socketServerInfo.shouldFail = false;
}

// MockPHDComponentsManager implementation
void MockPHDComponentsManager::SetupMocks() {
    // Create all mock instances
    mockCamera = new MockCamera();
    mockMount = new MockMount();
    mockStepGuider = new MockStepGuider();
    mockRotatorConnection = new MockRotatorConnection();
    mockEventServer = new MockEventServer();
    mockSocketServer = new MockSocketServer();
    
    // Set static instances
    MockCamera::SetInstance(mockCamera);
    MockMount::SetInstance(mockMount);
    MockStepGuider::SetInstance(mockStepGuider);
    MockRotatorConnection::SetInstance(mockRotatorConnection);
    MockEventServer::SetInstance(mockEventServer);
    MockSocketServer::SetInstance(mockSocketServer);
    
    // Create simulator
    simulator = std::make_unique<PHDComponentSimulator>();
    simulator->SetupDefaultComponents();
}

void MockPHDComponentsManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockCamera;
    delete mockMount;
    delete mockStepGuider;
    delete mockRotatorConnection;
    delete mockEventServer;
    delete mockSocketServer;
    
    // Reset pointers
    mockCamera = nullptr;
    mockMount = nullptr;
    mockStepGuider = nullptr;
    mockRotatorConnection = nullptr;
    mockEventServer = nullptr;
    mockSocketServer = nullptr;
    
    // Reset static instances
    MockCamera::SetInstance(nullptr);
    MockMount::SetInstance(nullptr);
    MockStepGuider::SetInstance(nullptr);
    MockRotatorConnection::SetInstance(nullptr);
    MockEventServer::SetInstance(nullptr);
    MockSocketServer::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockPHDComponentsManager::ResetMocks() {
    if (mockCamera) {
        testing::Mock::VerifyAndClearExpectations(mockCamera);
    }
    if (mockMount) {
        testing::Mock::VerifyAndClearExpectations(mockMount);
    }
    if (mockStepGuider) {
        testing::Mock::VerifyAndClearExpectations(mockStepGuider);
    }
    if (mockRotatorConnection) {
        testing::Mock::VerifyAndClearExpectations(mockRotatorConnection);
    }
    if (mockEventServer) {
        testing::Mock::VerifyAndClearExpectations(mockEventServer);
    }
    if (mockSocketServer) {
        testing::Mock::VerifyAndClearExpectations(mockSocketServer);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockCamera* MockPHDComponentsManager::GetMockCamera() { return mockCamera; }
MockMount* MockPHDComponentsManager::GetMockMount() { return mockMount; }
MockStepGuider* MockPHDComponentsManager::GetMockStepGuider() { return mockStepGuider; }
MockRotatorConnection* MockPHDComponentsManager::GetMockRotatorConnection() { return mockRotatorConnection; }
MockEventServer* MockPHDComponentsManager::GetMockEventServer() { return mockEventServer; }
MockSocketServer* MockPHDComponentsManager::GetMockSocketServer() { return mockSocketServer; }
PHDComponentSimulator* MockPHDComponentsManager::GetSimulator() { return simulator.get(); }

void MockPHDComponentsManager::SetupConnectedCamera() {
    if (simulator) {
        PHDComponentSimulator::CameraInfo info;
        info.name = "Connected Mock Camera";
        info.isConnected = true;
        info.hasST4 = true;
        info.hasShutter = true;
        info.hasGainControl = true;
        info.fullSize = wxSize(1024, 768);
        info.shouldFail = false;
        simulator->SetupCamera(info);
    }
    
    if (mockCamera) {
        EXPECT_CALL(*mockCamera, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockCamera, Name())
            .WillRepeatedly(::testing::Return(wxString("Connected Mock Camera")));
        EXPECT_CALL(*mockCamera, ST4HasGuideOutput())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockCamera, ST4HostConnected())
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockPHDComponentsManager::SetupConnectedMount() {
    if (simulator) {
        PHDComponentSimulator::MountInfo info;
        info.name = "Connected Mock Mount";
        info.className = "MockMount";
        info.isConnected = true;
        info.isCalibrated = true;
        info.isStepGuider = false;
        info.calibrationAngle = 45.0;
        info.guidingEnabled = true;
        info.shouldFail = false;
        simulator->SetupMount(info);
    }
    
    if (mockMount) {
        EXPECT_CALL(*mockMount, IsConnected())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockMount, Name())
            .WillRepeatedly(::testing::Return(wxString("Connected Mock Mount")));
        EXPECT_CALL(*mockMount, IsCalibrated())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockMount, GetGuidingEnabled())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockMount, Guide(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(MOVE_OK));
    }
}

void MockPHDComponentsManager::SetupRunningServers() {
    if (simulator) {
        PHDComponentSimulator::ServerInfo eventInfo;
        eventInfo.isRunning = true;
        eventInfo.port = 4400;
        eventInfo.clientCount = 0;
        eventInfo.shouldFail = false;
        simulator->SetupEventServer(eventInfo);
        
        PHDComponentSimulator::ServerInfo socketInfo;
        socketInfo.isRunning = true;
        socketInfo.port = 4300;
        socketInfo.clientCount = 0;
        socketInfo.shouldFail = false;
        simulator->SetupSocketServer(socketInfo);
    }
    
    if (mockEventServer) {
        EXPECT_CALL(*mockEventServer, IsEventServerRunning())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockEventServer, GetEventServerPort())
            .WillRepeatedly(::testing::Return(4400));
        EXPECT_CALL(*mockEventServer, EventServerStart(::testing::_))
            .WillRepeatedly(::testing::Return(true));
    }
    
    if (mockSocketServer) {
        EXPECT_CALL(*mockSocketServer, IsSocketServerRunning())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockSocketServer, GetSocketServerPort())
            .WillRepeatedly(::testing::Return(4300));
        EXPECT_CALL(*mockSocketServer, SocketServerStart(::testing::_))
            .WillRepeatedly(::testing::Return(true));
    }
}

void MockPHDComponentsManager::SimulateEquipmentFailure() {
    if (simulator) {
        simulator->SetCameraError(true);
        simulator->SetMountError(true);
    }
    
    if (mockCamera) {
        EXPECT_CALL(*mockCamera, Connect())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockCamera, ST4PulseGuideScope(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
    
    if (mockMount) {
        EXPECT_CALL(*mockMount, Connect())
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockMount, Guide(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(MOVE_ERROR));
    }
}

void MockPHDComponentsManager::SimulateNetworkFailure() {
    if (simulator) {
        simulator->SetServerError(true, true);  // Event server
        simulator->SetServerError(false, true); // Socket server
    }
    
    if (mockEventServer) {
        EXPECT_CALL(*mockEventServer, EventServerStart(::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
    
    if (mockSocketServer) {
        EXPECT_CALL(*mockSocketServer, SocketServerStart(::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
}
