/*
 * mock_wx_components.cpp
 * PHD Guiding - Logging Module Tests
 *
 * Implementation of mock objects for wxWidgets components
 */

#include "mock_wx_components.h"
#include <memory>

// Static instance declarations
MockWxDateTime* MockWxDateTime::instance = nullptr;
MockWxThread* MockWxThread::instance = nullptr;
MockWxDir* MockWxDir::instance = nullptr;
MockWxFileName* MockWxFileName::instance = nullptr;
MockWxClipboard* MockWxClipboard::instance = nullptr;
MockWxMessageBox* MockWxMessageBox::instance = nullptr;
MockWxLaunchDefaultBrowser* MockWxLaunchDefaultBrowser::instance = nullptr;

// MockWxComponentsManager static members
MockWxFFile* MockWxComponentsManager::mockFFile = nullptr;
MockWxDateTime* MockWxComponentsManager::mockDateTime = nullptr;
MockWxCriticalSection* MockWxComponentsManager::mockCriticalSection = nullptr;
MockWxThread* MockWxComponentsManager::mockThread = nullptr;
MockWxDir* MockWxComponentsManager::mockDir = nullptr;
MockWxFileName* MockWxComponentsManager::mockFileName = nullptr;
MockWxGrid* MockWxComponentsManager::mockGrid = nullptr;
MockWxDialog* MockWxComponentsManager::mockDialog = nullptr;
MockWxClipboard* MockWxComponentsManager::mockClipboard = nullptr;
MockWxMessageBox* MockWxComponentsManager::mockMessageBox = nullptr;
MockWxLaunchDefaultBrowser* MockWxComponentsManager::mockLaunchDefaultBrowser = nullptr;

// MockWxDateTime static methods
wxDateTime MockWxDateTime::MockUNow() {
    if (instance) {
        return instance->UNow();
    }
    return wxDateTime::Now(); // fallback
}

wxDateTime MockWxDateTime::MockInvalidDateTime() {
    return wxInvalidDateTime;
}

// MockWxThread static methods
wxThreadIdType MockWxThread::MockGetCurrentId() {
    if (instance) {
        return instance->GetCurrentId();
    }
    return wxThread::GetCurrentId(); // fallback
}

// MockWxDir static methods
bool MockWxDir::MockExists(const wxString& dirname) {
    if (instance) {
        return instance->Exists(dirname);
    }
    return wxDir::Exists(dirname); // fallback
}

bool MockWxDir::MockMkdir(const wxString& dirname, int perm, int flags) {
    if (instance) {
        // Mock doesn't have Mkdir method, so we simulate it
        return instance->Exists(dirname); // Assume success if dir exists
    }
    return wxFileName::Mkdir(dirname, perm, flags); // fallback
}

bool MockWxDir::MockRmdir(const wxString& dirname, int flags) {
    if (instance) {
        // Mock doesn't have Rmdir method, so we simulate it
        return !instance->Exists(dirname); // Assume success if dir doesn't exist
    }
    return wxFileName::Rmdir(dirname, flags); // fallback
}

// MockWxFileName static methods
bool MockWxFileName::MockMkdir(const wxString& dir, int perm, int flags) {
    if (instance) {
        return instance->Mkdir(dir, perm, flags);
    }
    return wxFileName::Mkdir(dir, perm, flags); // fallback
}

bool MockWxFileName::MockDirExists(const wxString& dir) {
    if (instance) {
        return instance->DirExists(dir);
    }
    return wxFileName::DirExists(dir); // fallback
}

bool MockWxFileName::MockFileExists(const wxString& file) {
    if (instance) {
        return instance->FileExists(file);
    }
    return wxFileName::FileExists(file); // fallback
}

// MockWxClipboard methods
MockWxClipboard* MockWxClipboard::GetInstance() {
    return instance;
}

// MockWxMessageBox static methods
int MockWxMessageBox::MockShow(const wxString& message, const wxString& caption, 
                              long style, wxWindow* parent) {
    if (instance) {
        return instance->Show(message, caption, style, parent);
    }
    return wxOK; // fallback
}

// MockWxLaunchDefaultBrowser static methods
bool MockWxLaunchDefaultBrowser::MockLaunch(const wxString& url, int flags) {
    if (instance) {
        return instance->Launch(url, flags);
    }
    return true; // fallback - assume success
}

// MockWxComponentsManager implementation
void MockWxComponentsManager::SetupMocks() {
    // Create all mock instances
    mockFFile = new MockWxFFile();
    mockDateTime = new MockWxDateTime();
    mockCriticalSection = new MockWxCriticalSection();
    mockThread = new MockWxThread();
    mockDir = new MockWxDir();
    mockFileName = new MockWxFileName();
    mockGrid = new MockWxGrid();
    mockDialog = new MockWxDialog();
    mockClipboard = new MockWxClipboard();
    mockMessageBox = new MockWxMessageBox();
    mockLaunchDefaultBrowser = new MockWxLaunchDefaultBrowser();
    
    // Set static instances
    MockWxDateTime::instance = mockDateTime;
    MockWxThread::instance = mockThread;
    MockWxDir::instance = mockDir;
    MockWxFileName::instance = mockFileName;
    MockWxClipboard::instance = mockClipboard;
    MockWxMessageBox::instance = mockMessageBox;
    MockWxLaunchDefaultBrowser::instance = mockLaunchDefaultBrowser;
}

void MockWxComponentsManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockFFile;
    delete mockDateTime;
    delete mockCriticalSection;
    delete mockThread;
    delete mockDir;
    delete mockFileName;
    delete mockGrid;
    delete mockDialog;
    delete mockClipboard;
    delete mockMessageBox;
    delete mockLaunchDefaultBrowser;
    
    // Reset pointers
    mockFFile = nullptr;
    mockDateTime = nullptr;
    mockCriticalSection = nullptr;
    mockThread = nullptr;
    mockDir = nullptr;
    mockFileName = nullptr;
    mockGrid = nullptr;
    mockDialog = nullptr;
    mockClipboard = nullptr;
    mockMessageBox = nullptr;
    mockLaunchDefaultBrowser = nullptr;
    
    // Reset static instances
    MockWxDateTime::instance = nullptr;
    MockWxThread::instance = nullptr;
    MockWxDir::instance = nullptr;
    MockWxFileName::instance = nullptr;
    MockWxClipboard::instance = nullptr;
    MockWxMessageBox::instance = nullptr;
    MockWxLaunchDefaultBrowser::instance = nullptr;
}

void MockWxComponentsManager::ResetAllMocks() {
    if (mockFFile) {
        testing::Mock::VerifyAndClearExpectations(mockFFile);
    }
    if (mockDateTime) {
        testing::Mock::VerifyAndClearExpectations(mockDateTime);
    }
    if (mockCriticalSection) {
        testing::Mock::VerifyAndClearExpectations(mockCriticalSection);
    }
    if (mockThread) {
        testing::Mock::VerifyAndClearExpectations(mockThread);
    }
    if (mockDir) {
        testing::Mock::VerifyAndClearExpectations(mockDir);
    }
    if (mockFileName) {
        testing::Mock::VerifyAndClearExpectations(mockFileName);
    }
    if (mockGrid) {
        testing::Mock::VerifyAndClearExpectations(mockGrid);
    }
    if (mockDialog) {
        testing::Mock::VerifyAndClearExpectations(mockDialog);
    }
    if (mockClipboard) {
        testing::Mock::VerifyAndClearExpectations(mockClipboard);
    }
    if (mockMessageBox) {
        testing::Mock::VerifyAndClearExpectations(mockMessageBox);
    }
    if (mockLaunchDefaultBrowser) {
        testing::Mock::VerifyAndClearExpectations(mockLaunchDefaultBrowser);
    }
}

// Getter methods
MockWxFFile* MockWxComponentsManager::GetMockFFile() { return mockFFile; }
MockWxDateTime* MockWxComponentsManager::GetMockDateTime() { return mockDateTime; }
MockWxCriticalSection* MockWxComponentsManager::GetMockCriticalSection() { return mockCriticalSection; }
MockWxThread* MockWxComponentsManager::GetMockThread() { return mockThread; }
MockWxDir* MockWxComponentsManager::GetMockDir() { return mockDir; }
MockWxFileName* MockWxComponentsManager::GetMockFileName() { return mockFileName; }
MockWxGrid* MockWxComponentsManager::GetMockGrid() { return mockGrid; }
MockWxDialog* MockWxComponentsManager::GetMockDialog() { return mockDialog; }
MockWxClipboard* MockWxComponentsManager::GetMockClipboard() { return mockClipboard; }
MockWxMessageBox* MockWxComponentsManager::GetMockMessageBox() { return mockMessageBox; }
MockWxLaunchDefaultBrowser* MockWxComponentsManager::GetMockLaunchDefaultBrowser() { return mockLaunchDefaultBrowser; }
