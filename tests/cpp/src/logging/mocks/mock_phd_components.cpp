/*
 * mock_phd_components.cpp
 * PHD Guiding - Logging Module Tests
 *
 * Implementation of mock PHD2 components
 */

#include "mock_phd_components.h"

// Static instance declarations
MockUsImage* MockUsImage::instance = nullptr;
MockMount* MockMount::instance = nullptr;
MockGuider* MockGuider::instance = nullptr;
MockPhdController* MockPhdController::instance = nullptr;
MockPhdApp* MockPhdApp::instance = nullptr;
MockPhdConfig* MockPhdConfig::instance = nullptr;
MockPhdFrame* MockPhdFrame::instance = nullptr;

// MockPhdComponentsManager static members
MockUsImage* MockPhdComponentsManager::mockUsImage = nullptr;
MockMount* MockPhdComponentsManager::mockMount = nullptr;
MockGuider* MockPhdComponentsManager::mockGuider = nullptr;
MockPhdController* MockPhdComponentsManager::mockPhdController = nullptr;
MockPhdApp* MockPhdComponentsManager::mockPhdApp = nullptr;
MockPhdConfig* MockPhdComponentsManager::mockPhdConfig = nullptr;
MockPhdFrame* MockPhdComponentsManager::mockPhdFrame = nullptr;

// Global mock instances (to replace actual PHD2 globals)
MockPhdConfig* pConfig = nullptr;
MockPhdFrame* pFrame = nullptr;
MockGuider* pGuider = nullptr;
MockMount* pMount = nullptr;

// MockUsImage implementation
MockUsImage* MockUsImage::GetInstance() {
    return instance;
}

void MockUsImage::SetInstance(MockUsImage* inst) {
    instance = inst;
}

// MockMount implementation
MockMount* MockMount::GetInstance() {
    return instance;
}

void MockMount::SetInstance(MockMount* inst) {
    instance = inst;
}

// MockGuider implementation
MockGuider* MockGuider::GetInstance() {
    return instance;
}

void MockGuider::SetInstance(MockGuider* inst) {
    instance = inst;
}

// MockPhdController implementation
MockPhdController* MockPhdController::GetInstance() {
    return instance;
}

void MockPhdController::SetInstance(MockPhdController* inst) {
    instance = inst;
}

// MockPhdApp implementation
MockPhdApp* MockPhdApp::GetInstance() {
    return instance;
}

void MockPhdApp::SetInstance(MockPhdApp* inst) {
    instance = inst;
}

// MockPhdConfig implementation
MockPhdConfig* MockPhdConfig::GetInstance() {
    return instance;
}

void MockPhdConfig::SetInstance(MockPhdConfig* inst) {
    instance = inst;
}

// MockPhdFrame implementation
MockPhdFrame* MockPhdFrame::GetInstance() {
    return instance;
}

void MockPhdFrame::SetInstance(MockPhdFrame* inst) {
    instance = inst;
}

// MockPhdComponentsManager implementation
void MockPhdComponentsManager::SetupMocks() {
    // Create all mock instances
    mockUsImage = new MockUsImage();
    mockMount = new MockMount();
    mockGuider = new MockGuider();
    mockPhdController = new MockPhdController();
    mockPhdApp = new MockPhdApp();
    mockPhdConfig = new MockPhdConfig();
    mockPhdFrame = new MockPhdFrame();
    
    // Set static instances
    MockUsImage::SetInstance(mockUsImage);
    MockMount::SetInstance(mockMount);
    MockGuider::SetInstance(mockGuider);
    MockPhdController::SetInstance(mockPhdController);
    MockPhdApp::SetInstance(mockPhdApp);
    MockPhdConfig::SetInstance(mockPhdConfig);
    MockPhdFrame::SetInstance(mockPhdFrame);
    
    // Set global instances
    pConfig = mockPhdConfig;
    pFrame = mockPhdFrame;
    pGuider = mockGuider;
    pMount = mockMount;
    
    // Set up default relationships
    mockPhdFrame->pGuider = mockGuider;
    
    // Set up default behaviors
    SetupDefaultConfiguration();
}

void MockPhdComponentsManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockUsImage;
    delete mockMount;
    delete mockGuider;
    delete mockPhdController;
    delete mockPhdApp;
    delete mockPhdConfig;
    delete mockPhdFrame;
    
    // Reset pointers
    mockUsImage = nullptr;
    mockMount = nullptr;
    mockGuider = nullptr;
    mockPhdController = nullptr;
    mockPhdApp = nullptr;
    mockPhdConfig = nullptr;
    mockPhdFrame = nullptr;
    
    // Reset static instances
    MockUsImage::SetInstance(nullptr);
    MockMount::SetInstance(nullptr);
    MockGuider::SetInstance(nullptr);
    MockPhdController::SetInstance(nullptr);
    MockPhdApp::SetInstance(nullptr);
    MockPhdConfig::SetInstance(nullptr);
    MockPhdFrame::SetInstance(nullptr);
    
    // Reset global instances
    pConfig = nullptr;
    pFrame = nullptr;
    pGuider = nullptr;
    pMount = nullptr;
}

void MockPhdComponentsManager::ResetMocks() {
    if (mockUsImage) {
        testing::Mock::VerifyAndClearExpectations(mockUsImage);
    }
    if (mockMount) {
        testing::Mock::VerifyAndClearExpectations(mockMount);
    }
    if (mockGuider) {
        testing::Mock::VerifyAndClearExpectations(mockGuider);
    }
    if (mockPhdController) {
        testing::Mock::VerifyAndClearExpectations(mockPhdController);
    }
    if (mockPhdApp) {
        testing::Mock::VerifyAndClearExpectations(mockPhdApp);
    }
    if (mockPhdConfig) {
        testing::Mock::VerifyAndClearExpectations(mockPhdConfig);
    }
    if (mockPhdFrame) {
        testing::Mock::VerifyAndClearExpectations(mockPhdFrame);
    }
    
    // Reset to default behaviors
    SetupDefaultConfiguration();
}

// Getter methods
MockUsImage* MockPhdComponentsManager::GetMockUsImage() { return mockUsImage; }
MockMount* MockPhdComponentsManager::GetMockMount() { return mockMount; }
MockGuider* MockPhdComponentsManager::GetMockGuider() { return mockGuider; }
MockPhdController* MockPhdComponentsManager::GetMockPhdController() { return mockPhdController; }
MockPhdApp* MockPhdComponentsManager::GetMockPhdApp() { return mockPhdApp; }
MockPhdConfig* MockPhdComponentsManager::GetMockPhdConfig() { return mockPhdConfig; }
MockPhdFrame* MockPhdComponentsManager::GetMockPhdFrame() { return mockPhdFrame; }

void MockPhdComponentsManager::SetupGuidingState(bool guiding, bool calibrating, bool paused) {
    if (mockGuider) {
        EXPECT_CALL(*mockGuider, IsGuiding())
            .WillRepeatedly(testing::Return(guiding));
        EXPECT_CALL(*mockGuider, IsCalibrating())
            .WillRepeatedly(testing::Return(calibrating));
        EXPECT_CALL(*mockGuider, IsCalibratingOrGuiding())
            .WillRepeatedly(testing::Return(guiding || calibrating));
        EXPECT_CALL(*mockGuider, IsPaused())
            .WillRepeatedly(testing::Return(paused));
    }
}

void MockPhdComponentsManager::SetupMountState(bool connected, bool calibrated) {
    if (mockMount) {
        EXPECT_CALL(*mockMount, IsConnected())
            .WillRepeatedly(testing::Return(connected));
        EXPECT_CALL(*mockMount, IsCalibrated())
            .WillRepeatedly(testing::Return(calibrated));
        EXPECT_CALL(*mockMount, GetMountClassName())
            .WillRepeatedly(testing::Return(wxString("MockMount")));
        EXPECT_CALL(*mockMount, DirectionChar(testing::_))
            .WillRepeatedly(testing::Invoke([](int direction) -> char {
                switch (direction) {
                    case 0: return 'N'; // North
                    case 1: return 'S'; // South
                    case 2: return 'E'; // East
                    case 3: return 'W'; // West
                    default: return '?';
                }
            }));
    }
}

void MockPhdComponentsManager::SetupImageState(int width, int height, unsigned int frameNum) {
    if (mockUsImage) {
        EXPECT_CALL(*mockUsImage, GetWidth())
            .WillRepeatedly(testing::Return(width));
        EXPECT_CALL(*mockUsImage, GetHeight())
            .WillRepeatedly(testing::Return(height));
        EXPECT_CALL(*mockUsImage, FrameNum())
            .WillRepeatedly(testing::Return(frameNum));
        EXPECT_CALL(*mockUsImage, GetImageSize())
            .WillRepeatedly(testing::Return(width * height * sizeof(unsigned short)));
        EXPECT_CALL(*mockUsImage, Save(testing::_))
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(*mockUsImage, GetTimestamp())
            .WillRepeatedly(testing::Return(wxDateTime::Now()));
    }
}

void MockPhdComponentsManager::SetupDefaultConfiguration() {
    if (mockPhdConfig) {
        // Set up default configuration values
        EXPECT_CALL(*mockPhdConfig, GetString(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](const wxString& key, const wxString& defaultValue) -> wxString {
                if (key == "/frame/LogDir") {
                    return "/home/user/Documents/PHD2";
                }
                return defaultValue;
            }));
        
        EXPECT_CALL(*mockPhdConfig, GetInt(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](const wxString& key, int defaultValue) -> int {
                return defaultValue;
            }));
        
        EXPECT_CALL(*mockPhdConfig, GetBool(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](const wxString& key, bool defaultValue) -> bool {
                return defaultValue;
            }));
        
        EXPECT_CALL(*mockPhdConfig, GetDouble(testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([](const wxString& key, double defaultValue) -> double {
                return defaultValue;
            }));
        
        // Allow setting values
        EXPECT_CALL(*mockPhdConfig, SetString(testing::_, testing::_))
            .WillRepeatedly(testing::Return());
        EXPECT_CALL(*mockPhdConfig, SetInt(testing::_, testing::_))
            .WillRepeatedly(testing::Return());
        EXPECT_CALL(*mockPhdConfig, SetBool(testing::_, testing::_))
            .WillRepeatedly(testing::Return());
        EXPECT_CALL(*mockPhdConfig, SetDouble(testing::_, testing::_))
            .WillRepeatedly(testing::Return());
    }
    
    if (mockPhdApp) {
        EXPECT_CALL(*mockPhdApp, GetInstanceNumber())
            .WillRepeatedly(testing::Return(1));
        EXPECT_CALL(*mockPhdApp, GetLogFileTime())
            .WillRepeatedly(testing::Return(wxDateTime::Now()));
    }
    
    if (mockPhdController) {
        EXPECT_CALL(*mockPhdController, IsSettling())
            .WillRepeatedly(testing::Return(false));
        EXPECT_CALL(*mockPhdController, GetSettlingDistance())
            .WillRepeatedly(testing::Return(0.0));
        EXPECT_CALL(*mockPhdController, GetSettlingTime())
            .WillRepeatedly(testing::Return(0.0));
    }
    
    if (mockPhdFrame) {
        EXPECT_CALL(*mockPhdFrame, GetGuider())
            .WillRepeatedly(testing::Return(mockGuider));
    }
}
