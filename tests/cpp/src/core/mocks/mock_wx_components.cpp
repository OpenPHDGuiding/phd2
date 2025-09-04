/*
 * mock_wx_components.cpp
 * PHD Guiding - Core Module Tests
 *
 * Implementation of mock wxWidgets components
 */

#include "mock_wx_components.h"
#include <algorithm>

// Static instance declarations
MockWxApp* MockWxApp::instance = nullptr;
MockWxFrame* MockWxFrame::instance = nullptr;
MockWxDialog* MockWxDialog::instance = nullptr;
MockWxConfig* MockWxConfig::instance = nullptr;
MockWxImage* MockWxImage::instance = nullptr;
MockWxThread* MockWxThread::instance = nullptr;
MockWxTimer* MockWxTimer::instance = nullptr;

// MockWxComponentsManager static members
MockWxApp* MockWxComponentsManager::mockApp = nullptr;
MockWxFrame* MockWxComponentsManager::mockFrame = nullptr;
MockWxDialog* MockWxComponentsManager::mockDialog = nullptr;
MockWxConfig* MockWxComponentsManager::mockConfig = nullptr;
MockWxImage* MockWxComponentsManager::mockImage = nullptr;
MockWxThread* MockWxComponentsManager::mockThread = nullptr;
MockWxTimer* MockWxComponentsManager::mockTimer = nullptr;
std::unique_ptr<WxComponentSimulator> MockWxComponentsManager::simulator = nullptr;

// MockWxApp implementation
MockWxApp* MockWxApp::GetInstance() {
    return instance;
}

void MockWxApp::SetInstance(MockWxApp* inst) {
    instance = inst;
}

// MockWxFrame implementation
MockWxFrame* MockWxFrame::GetInstance() {
    return instance;
}

void MockWxFrame::SetInstance(MockWxFrame* inst) {
    instance = inst;
}

// MockWxDialog implementation
MockWxDialog* MockWxDialog::GetInstance() {
    return instance;
}

void MockWxDialog::SetInstance(MockWxDialog* inst) {
    instance = inst;
}

// MockWxConfig implementation
MockWxConfig* MockWxConfig::GetInstance() {
    return instance;
}

void MockWxConfig::SetInstance(MockWxConfig* inst) {
    instance = inst;
}

// MockWxImage implementation
MockWxImage* MockWxImage::GetInstance() {
    return instance;
}

void MockWxImage::SetInstance(MockWxImage* inst) {
    instance = inst;
}

// MockWxThread implementation
MockWxThread* MockWxThread::GetInstance() {
    return instance;
}

void MockWxThread::SetInstance(MockWxThread* inst) {
    instance = inst;
}

// MockWxTimer implementation
MockWxTimer* MockWxTimer::GetInstance() {
    return instance;
}

void MockWxTimer::SetInstance(MockWxTimer* inst) {
    instance = inst;
}

// WxComponentSimulator implementation
void WxComponentSimulator::SetupApp(const AppInfo& info) {
    appInfo = info;
}

void WxComponentSimulator::SetupFrame(const WindowInfo& info) {
    frameInfo = info;
}

void WxComponentSimulator::SetupDialog(const WindowInfo& info) {
    dialogInfo = info;
}

void WxComponentSimulator::SetupConfig(const ConfigInfo& info) {
    configInfo = info;
}

WxComponentSimulator::AppInfo WxComponentSimulator::GetAppInfo() const {
    return appInfo;
}

WxComponentSimulator::WindowInfo WxComponentSimulator::GetFrameInfo() const {
    return frameInfo;
}

WxComponentSimulator::WindowInfo WxComponentSimulator::GetDialogInfo() const {
    return dialogInfo;
}

WxComponentSimulator::ConfigInfo WxComponentSimulator::GetConfigInfo() const {
    return configInfo;
}

void WxComponentSimulator::SimulateAppEvent(wxEventType eventType) {
    // Simulate application events
    switch (eventType) {
        case wxEVT_ACTIVATE_APP:
            // Application activation
            break;
        case wxEVT_END_SESSION:
            // Session ending
            break;
        default:
            // Other events
            break;
    }
}

void WxComponentSimulator::SimulateWindowEvent(wxEventType eventType) {
    // Simulate window events
    switch (eventType) {
        case wxEVT_CLOSE_WINDOW:
            frameInfo.isShown = false;
            break;
        case wxEVT_SHOW:
            frameInfo.isShown = true;
            break;
        case wxEVT_SIZE:
            // Size event
            break;
        default:
            // Other events
            break;
    }
}

void WxComponentSimulator::SimulateTimerEvent() {
    // Simulate timer events
}

void WxComponentSimulator::SetAppError(bool error) {
    appInfo.shouldFail = error;
}

void WxComponentSimulator::SetWindowError(bool error) {
    frameInfo.shouldFail = error;
    dialogInfo.shouldFail = error;
}

void WxComponentSimulator::SetConfigError(bool error) {
    configInfo.shouldFail = error;
}

void WxComponentSimulator::Reset() {
    appInfo = AppInfo();
    frameInfo = WindowInfo();
    dialogInfo = WindowInfo();
    configInfo = ConfigInfo();
    
    SetupDefaultComponents();
}

void WxComponentSimulator::SetupDefaultComponents() {
    // Set up default application
    appInfo.appName = "PHD2";
    appInfo.vendorName = "PHD2";
    appInfo.isInitialized = false;
    appInfo.shouldFail = false;
    
    // Set up default frame
    frameInfo.title = "PHD2 Main Window";
    frameInfo.size = wxSize(800, 600);
    frameInfo.position = wxPoint(100, 100);
    frameInfo.isShown = false;
    frameInfo.shouldFail = false;
    
    // Set up default dialog
    dialogInfo.title = "PHD2 Dialog";
    dialogInfo.size = wxSize(400, 300);
    dialogInfo.position = wxPoint(200, 200);
    dialogInfo.isShown = false;
    dialogInfo.shouldFail = false;
    
    // Set up default configuration
    configInfo.currentPath = "/";
    configInfo.shouldFail = false;
    
    // Add some default configuration values
    configInfo.stringValues["/App/Name"] = "PHD2";
    configInfo.stringValues["/App/Version"] = "2.6.11";
    configInfo.longValues["/Window/Width"] = 800;
    configInfo.longValues["/Window/Height"] = 600;
    configInfo.boolValues["/Debug/Enabled"] = false;
}

// MockWxComponentsManager implementation
void MockWxComponentsManager::SetupMocks() {
    // Create all mock instances
    mockApp = new MockWxApp();
    mockFrame = new MockWxFrame();
    mockDialog = new MockWxDialog();
    mockConfig = new MockWxConfig();
    mockImage = new MockWxImage();
    mockThread = new MockWxThread();
    mockTimer = new MockWxTimer();
    
    // Set static instances
    MockWxApp::SetInstance(mockApp);
    MockWxFrame::SetInstance(mockFrame);
    MockWxDialog::SetInstance(mockDialog);
    MockWxConfig::SetInstance(mockConfig);
    MockWxImage::SetInstance(mockImage);
    MockWxThread::SetInstance(mockThread);
    MockWxTimer::SetInstance(mockTimer);
    
    // Create simulator
    simulator = std::make_unique<WxComponentSimulator>();
    simulator->SetupDefaultComponents();
}

void MockWxComponentsManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockApp;
    delete mockFrame;
    delete mockDialog;
    delete mockConfig;
    delete mockImage;
    delete mockThread;
    delete mockTimer;
    
    // Reset pointers
    mockApp = nullptr;
    mockFrame = nullptr;
    mockDialog = nullptr;
    mockConfig = nullptr;
    mockImage = nullptr;
    mockThread = nullptr;
    mockTimer = nullptr;
    
    // Reset static instances
    MockWxApp::SetInstance(nullptr);
    MockWxFrame::SetInstance(nullptr);
    MockWxDialog::SetInstance(nullptr);
    MockWxConfig::SetInstance(nullptr);
    MockWxImage::SetInstance(nullptr);
    MockWxThread::SetInstance(nullptr);
    MockWxTimer::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockWxComponentsManager::ResetMocks() {
    if (mockApp) {
        testing::Mock::VerifyAndClearExpectations(mockApp);
    }
    if (mockFrame) {
        testing::Mock::VerifyAndClearExpectations(mockFrame);
    }
    if (mockDialog) {
        testing::Mock::VerifyAndClearExpectations(mockDialog);
    }
    if (mockConfig) {
        testing::Mock::VerifyAndClearExpectations(mockConfig);
    }
    if (mockImage) {
        testing::Mock::VerifyAndClearExpectations(mockImage);
    }
    if (mockThread) {
        testing::Mock::VerifyAndClearExpectations(mockThread);
    }
    if (mockTimer) {
        testing::Mock::VerifyAndClearExpectations(mockTimer);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockWxApp* MockWxComponentsManager::GetMockApp() { return mockApp; }
MockWxFrame* MockWxComponentsManager::GetMockFrame() { return mockFrame; }
MockWxDialog* MockWxComponentsManager::GetMockDialog() { return mockDialog; }
MockWxConfig* MockWxComponentsManager::GetMockConfig() { return mockConfig; }
MockWxImage* MockWxComponentsManager::GetMockImage() { return mockImage; }
MockWxThread* MockWxComponentsManager::GetMockThread() { return mockThread; }
MockWxTimer* MockWxComponentsManager::GetMockTimer() { return mockTimer; }
WxComponentSimulator* MockWxComponentsManager::GetSimulator() { return simulator.get(); }

void MockWxComponentsManager::SetupApplication() {
    if (simulator) {
        WxComponentSimulator::AppInfo info;
        info.appName = "PHD2";
        info.vendorName = "PHD2";
        info.isInitialized = true;
        info.shouldFail = false;
        simulator->SetupApp(info);
    }
    
    if (mockApp) {
        EXPECT_CALL(*mockApp, OnInit())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockApp, GetAppName())
            .WillRepeatedly(::testing::Return(wxString("PHD2")));
        EXPECT_CALL(*mockApp, GetVendorName())
            .WillRepeatedly(::testing::Return(wxString("PHD2")));
    }
}

void MockWxComponentsManager::SetupMainWindow() {
    if (simulator) {
        WxComponentSimulator::WindowInfo info;
        info.title = "PHD2 Main Window";
        info.size = wxSize(800, 600);
        info.position = wxPoint(100, 100);
        info.isShown = true;
        info.shouldFail = false;
        simulator->SetupFrame(info);
    }
    
    if (mockFrame) {
        EXPECT_CALL(*mockFrame, Show(::testing::_))
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockFrame, GetTitle())
            .WillRepeatedly(::testing::Return(wxString("PHD2 Main Window")));
        EXPECT_CALL(*mockFrame, GetSize())
            .WillRepeatedly(::testing::Return(wxSize(800, 600)));
    }
}

void MockWxComponentsManager::SetupConfiguration() {
    if (simulator) {
        WxComponentSimulator::ConfigInfo info;
        info.currentPath = "/";
        info.shouldFail = false;
        // Add default configuration values
        info.stringValues["/App/Name"] = "PHD2";
        info.stringValues["/App/Version"] = "2.6.11";
        info.longValues["/Window/Width"] = 800;
        info.longValues["/Window/Height"] = 600;
        info.boolValues["/Debug/Enabled"] = false;
        simulator->SetupConfig(info);
    }
    
    if (mockConfig) {
        EXPECT_CALL(*mockConfig, Flush())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockConfig, GetPath())
            .WillRepeatedly(::testing::Return(wxString("/")));
    }
}

void MockWxComponentsManager::SimulateApplicationShutdown() {
    if (simulator) {
        simulator->SimulateAppEvent(wxEVT_END_SESSION);
    }
    
    if (mockApp) {
        EXPECT_CALL(*mockApp, OnExit())
            .WillOnce(::testing::Return(0));
    }
}

void MockWxComponentsManager::SimulateWindowClose() {
    if (simulator) {
        simulator->SimulateWindowEvent(wxEVT_CLOSE_WINDOW);
    }
    
    if (mockFrame) {
        EXPECT_CALL(*mockFrame, Close(::testing::_))
            .WillOnce(::testing::Return(true));
    }
}
