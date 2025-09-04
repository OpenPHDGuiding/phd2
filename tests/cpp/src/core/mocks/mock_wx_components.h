/*
 * mock_wx_components.h
 * PHD Guiding - Core Module Tests
 *
 * Mock objects for wxWidgets components used in core tests
 * Provides controllable behavior for UI components, events, and system operations
 */

#ifndef MOCK_WX_COMPONENTS_H
#define MOCK_WX_COMPONENTS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/dialog.h>
#include <wx/config.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/thread.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declarations
class WxComponentSimulator;

// Mock wxApp for application testing
class MockWxApp {
public:
    // Application lifecycle
    MOCK_METHOD0(OnInit, bool());
    MOCK_METHOD0(OnExit, int());
    MOCK_METHOD0(OnInitCmdLine, void());
    MOCK_METHOD1(OnCmdLineParsed, bool(wxCmdLineParser& parser));
    
    // Event handling
    MOCK_METHOD1(ProcessEvent, bool(wxEvent& event));
    MOCK_METHOD0(Yield, bool());
    MOCK_METHOD1(YieldFor, bool(long eventsToProcess));
    
    // Application properties
    MOCK_METHOD0(GetAppName, wxString());
    MOCK_METHOD1(SetAppName, void(const wxString& name));
    MOCK_METHOD0(GetVendorName, wxString());
    MOCK_METHOD1(SetVendorName, void(const wxString& name));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateEvent, void(wxEvent& event));
    
    static MockWxApp* instance;
    static MockWxApp* GetInstance();
    static void SetInstance(MockWxApp* inst);
};

// Mock wxFrame for main window testing
class MockWxFrame {
public:
    // Window management
    MOCK_METHOD0(Show, bool());
    MOCK_METHOD1(Show, bool(bool show));
    MOCK_METHOD0(Hide, bool());
    MOCK_METHOD0(Close, bool());
    MOCK_METHOD1(Close, bool(bool force));
    MOCK_METHOD0(Destroy, bool());
    
    // Window properties
    MOCK_METHOD0(GetTitle, wxString());
    MOCK_METHOD1(SetTitle, void(const wxString& title));
    MOCK_METHOD0(GetSize, wxSize());
    MOCK_METHOD1(SetSize, void(const wxSize& size));
    MOCK_METHOD0(GetPosition, wxPoint());
    MOCK_METHOD1(SetPosition, void(const wxPoint& pos));
    
    // Status bar
    MOCK_METHOD0(CreateStatusBar, wxStatusBar*());
    MOCK_METHOD1(SetStatusText, void(const wxString& text));
    MOCK_METHOD2(SetStatusText, void(const wxString& text, int field));
    
    // Menu bar
    MOCK_METHOD1(SetMenuBar, void(wxMenuBar* menuBar));
    MOCK_METHOD0(GetMenuBar, wxMenuBar*());
    
    // Event handling
    MOCK_METHOD1(ProcessEvent, bool(wxEvent& event));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateClose, void(bool force));
    
    static MockWxFrame* instance;
    static MockWxFrame* GetInstance();
    static void SetInstance(MockWxFrame* inst);
};

// Mock wxDialog for dialog testing
class MockWxDialog {
public:
    // Dialog management
    MOCK_METHOD0(ShowModal, int());
    MOCK_METHOD1(EndModal, void(int retCode));
    MOCK_METHOD0(IsModal, bool());
    
    // Window properties
    MOCK_METHOD0(GetTitle, wxString());
    MOCK_METHOD1(SetTitle, void(const wxString& title));
    MOCK_METHOD0(GetSize, wxSize());
    MOCK_METHOD1(SetSize, void(const wxSize& size));
    
    // Button handling
    MOCK_METHOD1(SetAffirmativeId, void(int affirmativeId));
    MOCK_METHOD1(SetEscapeId, void(int escapeId));
    MOCK_METHOD0(GetAffirmativeId, int());
    MOCK_METHOD0(GetEscapeId, int());
    
    // Helper methods for testing
    MOCK_METHOD1(SetModalResult, void(int result));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockWxDialog* instance;
    static MockWxDialog* GetInstance();
    static void SetInstance(MockWxDialog* inst);
};

// Mock wxConfig for configuration testing
class MockWxConfig {
public:
    // Reading values
    MOCK_METHOD2(Read, bool(const wxString& key, wxString* str));
    MOCK_METHOD3(Read, wxString(const wxString& key, const wxString& defaultVal));
    MOCK_METHOD2(Read, bool(const wxString& key, long* l));
    MOCK_METHOD3(Read, long(const wxString& key, long defaultVal));
    MOCK_METHOD2(Read, bool(const wxString& key, double* d));
    MOCK_METHOD3(Read, double(const wxString& key, double defaultVal));
    MOCK_METHOD2(Read, bool(const wxString& key, bool* b));
    MOCK_METHOD3(Read, bool(const wxString& key, bool defaultVal));
    
    // Writing values
    MOCK_METHOD2(Write, bool(const wxString& key, const wxString& value));
    MOCK_METHOD2(Write, bool(const wxString& key, long value));
    MOCK_METHOD2(Write, bool(const wxString& key, double value));
    MOCK_METHOD2(Write, bool(const wxString& key, bool value));
    
    // Group management
    MOCK_METHOD1(SetPath, void(const wxString& strPath));
    MOCK_METHOD0(GetPath, wxString());
    MOCK_METHOD1(HasGroup, bool(const wxString& strName));
    MOCK_METHOD1(HasEntry, bool(const wxString& strName));
    
    // Entry enumeration
    MOCK_METHOD3(GetFirstGroup, bool(wxString& str, long& lIndex));
    MOCK_METHOD3(GetNextGroup, bool(wxString& str, long& lIndex));
    MOCK_METHOD3(GetFirstEntry, bool(wxString& str, long& lIndex));
    MOCK_METHOD3(GetNextEntry, bool(wxString& str, long& lIndex));
    
    // Deletion
    MOCK_METHOD1(DeleteEntry, bool(const wxString& key));
    MOCK_METHOD1(DeleteGroup, bool(const wxString& key));
    MOCK_METHOD0(DeleteAll, bool());
    
    // Persistence
    MOCK_METHOD0(Flush, bool());
    
    // Helper methods for testing
    MOCK_METHOD2(SetValue, void(const wxString& key, const wxString& value));
    MOCK_METHOD1(GetValue, wxString(const wxString& key));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockWxConfig* instance;
    static MockWxConfig* GetInstance();
    static void SetInstance(MockWxConfig* inst);
};

// Mock wxImage for image handling
class MockWxImage {
public:
    // Image creation
    MOCK_METHOD2(Create, bool(int width, int height));
    MOCK_METHOD3(Create, bool(int width, int height, bool clear));
    MOCK_METHOD0(Clear, void());
    
    // Image properties
    MOCK_METHOD0(GetWidth, int());
    MOCK_METHOD0(GetHeight, int());
    MOCK_METHOD0(GetSize, wxSize());
    MOCK_METHOD0(IsOk, bool());
    
    // Pixel access
    MOCK_METHOD2(GetRed, unsigned char(int x, int y));
    MOCK_METHOD2(GetGreen, unsigned char(int x, int y));
    MOCK_METHOD2(GetBlue, unsigned char(int x, int y));
    MOCK_METHOD5(SetRGB, void(int x, int y, unsigned char r, unsigned char g, unsigned char b));
    
    // Data access
    MOCK_METHOD0(GetData, unsigned char*());
    MOCK_METHOD1(SetData, void(unsigned char* data));
    
    // File operations
    MOCK_METHOD1(LoadFile, bool(const wxString& name));
    MOCK_METHOD2(LoadFile, bool(const wxString& name, wxBitmapType type));
    MOCK_METHOD1(SaveFile, bool(const wxString& name));
    MOCK_METHOD2(SaveFile, bool(const wxString& name, wxBitmapType type));
    
    // Transformations
    MOCK_METHOD2(Scale, wxImage(int width, int height));
    MOCK_METHOD1(Rotate, wxImage(double angle));
    MOCK_METHOD1(Mirror, wxImage(bool horizontally));
    
    // Helper methods for testing
    MOCK_METHOD2(SetSize, void(int width, int height));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateImageData, void(const std::vector<unsigned char>& data));
    
    static MockWxImage* instance;
    static MockWxImage* GetInstance();
    static void SetInstance(MockWxImage* inst);
};

// Mock wxThread for threading tests
class MockWxThread {
public:
    // Thread management
    MOCK_METHOD0(Create, wxThreadError());
    MOCK_METHOD0(Run, wxThreadError());
    MOCK_METHOD0(Pause, wxThreadError());
    MOCK_METHOD0(Resume, wxThreadError());
    MOCK_METHOD0(Delete, void());
    MOCK_METHOD0(Kill, wxThreadError());
    MOCK_METHOD1(Wait, void(wxThreadWait waitMode));
    
    // Thread state
    MOCK_METHOD0(IsRunning, bool());
    MOCK_METHOD0(IsPaused, bool());
    MOCK_METHOD0(IsDetached, bool());
    MOCK_METHOD0(GetId, unsigned long());
    
    // Thread priority
    MOCK_METHOD0(GetPriority, unsigned int());
    MOCK_METHOD1(SetPriority, void(unsigned int priority));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SimulateThreadExit, void(wxThread::ExitCode exitcode));
    
    static MockWxThread* instance;
    static MockWxThread* GetInstance();
    static void SetInstance(MockWxThread* inst);
};

// Mock wxTimer for timer tests
class MockWxTimer {
public:
    // Timer management
    MOCK_METHOD1(Start, bool(int milliseconds));
    MOCK_METHOD2(Start, bool(int milliseconds, bool oneShot));
    MOCK_METHOD0(Stop, void());
    MOCK_METHOD0(IsRunning, bool());
    
    // Timer properties
    MOCK_METHOD0(GetInterval, int());
    MOCK_METHOD0(IsOneShot, bool());
    
    // Helper methods for testing
    MOCK_METHOD0(SimulateTimeout, void());
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockWxTimer* instance;
    static MockWxTimer* GetInstance();
    static void SetInstance(MockWxTimer* inst);
};

// wxWidgets component simulator for comprehensive testing
class WxComponentSimulator {
public:
    struct AppInfo {
        wxString appName;
        wxString vendorName;
        bool isInitialized;
        bool shouldFail;
        
        AppInfo() : appName("PHD2"), vendorName("PHD2"), isInitialized(false), shouldFail(false) {}
    };
    
    struct WindowInfo {
        wxString title;
        wxSize size;
        wxPoint position;
        bool isShown;
        bool shouldFail;
        
        WindowInfo() : title("Test Window"), size(800, 600), position(100, 100), 
                      isShown(false), shouldFail(false) {}
    };
    
    struct ConfigInfo {
        std::map<wxString, wxString> stringValues;
        std::map<wxString, long> longValues;
        std::map<wxString, double> doubleValues;
        std::map<wxString, bool> boolValues;
        wxString currentPath;
        bool shouldFail;
        
        ConfigInfo() : currentPath("/"), shouldFail(false) {}
    };
    
    // Component management
    void SetupApp(const AppInfo& info);
    void SetupFrame(const WindowInfo& info);
    void SetupDialog(const WindowInfo& info);
    void SetupConfig(const ConfigInfo& info);
    
    // State management
    AppInfo GetAppInfo() const;
    WindowInfo GetFrameInfo() const;
    WindowInfo GetDialogInfo() const;
    ConfigInfo GetConfigInfo() const;
    
    // Event simulation
    void SimulateAppEvent(wxEventType eventType);
    void SimulateWindowEvent(wxEventType eventType);
    void SimulateTimerEvent();
    
    // Error simulation
    void SetAppError(bool error);
    void SetWindowError(bool error);
    void SetConfigError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultComponents();
    
private:
    AppInfo appInfo;
    WindowInfo frameInfo;
    WindowInfo dialogInfo;
    ConfigInfo configInfo;
    
    void InitializeDefaults();
};

// Helper class to manage all wxWidgets mocks
class MockWxComponentsManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockWxApp* GetMockApp();
    static MockWxFrame* GetMockFrame();
    static MockWxDialog* GetMockDialog();
    static MockWxConfig* GetMockConfig();
    static MockWxImage* GetMockImage();
    static MockWxThread* GetMockThread();
    static MockWxTimer* GetMockTimer();
    static WxComponentSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupApplication();
    static void SetupMainWindow();
    static void SetupConfiguration();
    static void SimulateApplicationShutdown();
    static void SimulateWindowClose();
    
private:
    static MockWxApp* mockApp;
    static MockWxFrame* mockFrame;
    static MockWxDialog* mockDialog;
    static MockWxConfig* mockConfig;
    static MockWxImage* mockImage;
    static MockWxThread* mockThread;
    static MockWxTimer* mockTimer;
    static std::unique_ptr<WxComponentSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_WX_COMPONENT_MOCKS() MockWxComponentsManager::SetupMocks()
#define TEARDOWN_WX_COMPONENT_MOCKS() MockWxComponentsManager::TeardownMocks()
#define RESET_WX_COMPONENT_MOCKS() MockWxComponentsManager::ResetMocks()

#define GET_MOCK_APP() MockWxComponentsManager::GetMockApp()
#define GET_MOCK_FRAME() MockWxComponentsManager::GetMockFrame()
#define GET_MOCK_DIALOG() MockWxComponentsManager::GetMockDialog()
#define GET_MOCK_CONFIG() MockWxComponentsManager::GetMockConfig()
#define GET_MOCK_IMAGE() MockWxComponentsManager::GetMockImage()
#define GET_MOCK_THREAD() MockWxComponentsManager::GetMockThread()
#define GET_MOCK_TIMER() MockWxComponentsManager::GetMockTimer()
#define GET_WX_SIMULATOR() MockWxComponentsManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_APP_INIT_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_APP(), OnInit()) \
        .WillOnce(::testing::Return(true))

#define EXPECT_APP_INIT_FAILURE() \
    EXPECT_CALL(*GET_MOCK_APP(), OnInit()) \
        .WillOnce(::testing::Return(false))

#define EXPECT_WINDOW_SHOW_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_FRAME(), Show(true)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_CONFIG_READ_SUCCESS(key, value) \
    EXPECT_CALL(*GET_MOCK_CONFIG(), Read(key, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(value), ::testing::Return(true)))

#define EXPECT_CONFIG_WRITE_SUCCESS(key, value) \
    EXPECT_CALL(*GET_MOCK_CONFIG(), Write(key, value)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_THREAD_CREATE_SUCCESS() \
    EXPECT_CALL(*GET_MOCK_THREAD(), Create()) \
        .WillOnce(::testing::Return(wxTHREAD_NO_ERROR))

#define EXPECT_TIMER_START_SUCCESS(interval) \
    EXPECT_CALL(*GET_MOCK_TIMER(), Start(interval, ::testing::_)) \
        .WillOnce(::testing::Return(true))

#endif // MOCK_WX_COMPONENTS_H
