/*
 * mock_wx_components.h
 * PHD Guiding - Logging Module Tests
 *
 * Mock objects for wxWidgets components used in logging tests
 * Provides controllable behavior for file operations, threading, and UI components
 */

#ifndef MOCK_WX_COMPONENTS_H
#define MOCK_WX_COMPONENTS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/datetime.h>
#include <wx/thread.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/html/htmlwin.h>
#include <wx/hyperlink.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/clipbrd.h>
#include <string>
#include <vector>

// Mock wxFFile for file operations
class MockWxFFile {
public:
    MOCK_METHOD2(Open, bool(const wxString& filename, const wxString& mode));
    MOCK_METHOD0(Close, bool());
    MOCK_METHOD0(IsOpened, bool());
    MOCK_METHOD1(Write, size_t(const wxString& str));
    MOCK_METHOD3(Read, size_t(void* buffer, size_t size, size_t count));
    MOCK_METHOD0(Flush, bool());
    MOCK_METHOD1(Seek, bool(wxFileOffset pos));
    MOCK_METHOD0(SeekEnd, wxFileOffset());
    MOCK_METHOD0(Tell, wxFileOffset());
    MOCK_METHOD0(Length, wxFileOffset());
    MOCK_METHOD0(Eof, bool());
    MOCK_METHOD0(Error, bool());
};

// Mock wxDateTime for time operations
class MockWxDateTime {
public:
    MOCK_METHOD0(UNow, wxDateTime());
    MOCK_METHOD1(Format, wxString(const wxString& format));
    MOCK_METHOD0(GetTicks, time_t());
    MOCK_METHOD0(IsValid, bool());
    MOCK_METHOD1(ParseFormat, bool(const wxString& date));
    
    // Static mock methods
    static MockWxDateTime* instance;
    static wxDateTime MockUNow();
    static wxDateTime MockInvalidDateTime();
};

// Mock wxCriticalSection for thread safety testing
class MockWxCriticalSection {
public:
    MOCK_METHOD0(Enter, void());
    MOCK_METHOD0(Leave, void());
    MOCK_METHOD0(TryEnter, bool());
};

// Mock wxCriticalSectionLocker
class MockWxCriticalSectionLocker {
public:
    MOCK_CONSTRUCTOR(MockWxCriticalSectionLocker(wxCriticalSection& cs));
    MOCK_DESTRUCTOR(~MockWxCriticalSectionLocker());
};

// Mock wxThread for threading operations
class MockWxThread {
public:
    MOCK_METHOD0(GetCurrentId, wxThreadIdType());
    MOCK_METHOD0(Create, wxThreadError());
    MOCK_METHOD0(Run, wxThreadError());
    MOCK_METHOD0(Pause, wxThreadError());
    MOCK_METHOD0(Resume, wxThreadError());
    MOCK_METHOD0(Delete, wxThreadError());
    MOCK_METHOD0(Kill, wxThreadError());
    MOCK_METHOD0(Wait, wxThreadError());
    MOCK_METHOD0(IsRunning, bool());
    MOCK_METHOD0(IsPaused, bool());
    MOCK_METHOD0(IsDetached, bool());
    
    static MockWxThread* instance;
    static wxThreadIdType MockGetCurrentId();
};

// Mock wxDir for directory operations
class MockWxDir {
public:
    MOCK_METHOD1(Exists, bool(const wxString& dirname));
    MOCK_METHOD1(Open, bool(const wxString& dirname));
    MOCK_METHOD0(Close, void());
    MOCK_METHOD0(IsOpened, bool());
    MOCK_METHOD4(GetFirst, bool(wxString* filename, const wxString& filespec, int flags, wxDirTraverseResult* result));
    MOCK_METHOD1(GetNext, bool(wxString* filename));
    MOCK_METHOD3(Traverse, size_t(wxDirTraverser& sink, const wxString& filespec, int flags));
    
    // Static methods
    static MockWxDir* instance;
    static bool MockExists(const wxString& dirname);
    static bool MockMkdir(const wxString& dirname, int perm, int flags);
    static bool MockRmdir(const wxString& dirname, int flags);
};

// Mock wxFileName for file path operations
class MockWxFileName {
public:
    MOCK_METHOD3(Mkdir, bool(const wxString& dir, int perm, int flags));
    MOCK_METHOD1(DirExists, bool(const wxString& dir));
    MOCK_METHOD1(FileExists, bool(const wxString& file));
    MOCK_METHOD1(Remove, bool(const wxString& file));
    MOCK_METHOD2(Rename, bool(const wxString& fileOld, const wxString& fileNew));
    MOCK_METHOD0(GetFullPath, wxString());
    MOCK_METHOD0(GetPath, wxString());
    MOCK_METHOD0(GetName, wxString());
    MOCK_METHOD0(GetExt, wxString());
    
    static MockWxFileName* instance;
    static bool MockMkdir(const wxString& dir, int perm, int flags);
    static bool MockDirExists(const wxString& dir);
    static bool MockFileExists(const wxString& file);
};

// Mock wxGrid for UI testing
class MockWxGrid {
public:
    MOCK_METHOD0(GetNumberRows, int());
    MOCK_METHOD0(GetNumberCols, int());
    MOCK_METHOD2(GetCellValue, wxString(int row, int col));
    MOCK_METHOD3(SetCellValue, void(int row, int col, const wxString& value));
    MOCK_METHOD1(AppendRows, bool(int numRows));
    MOCK_METHOD1(DeleteRows, bool(int numRows));
    MOCK_METHOD0(ClearGrid, void());
    MOCK_METHOD0(Refresh, void());
    MOCK_METHOD0(Update, void());
};

// Mock wxDialog for dialog testing
class MockWxDialog {
public:
    MOCK_METHOD0(ShowModal, int());
    MOCK_METHOD1(EndModal, void(int retCode));
    MOCK_METHOD0(IsModal, bool());
    MOCK_METHOD1(SetTitle, void(const wxString& title));
    MOCK_METHOD0(GetTitle, wxString());
};

// Mock wxClipboard for clipboard operations
class MockWxClipboard {
public:
    MOCK_METHOD0(Open, bool());
    MOCK_METHOD0(Close, void());
    MOCK_METHOD1(SetData, bool(wxDataObject* data));
    MOCK_METHOD1(GetData, bool(wxDataObject& data));
    MOCK_METHOD0(Clear, void());
    MOCK_METHOD0(IsOpened, bool());
    
    static MockWxClipboard* instance;
    static MockWxClipboard* GetInstance();
};

// Mock wxMessageBox for message display
class MockWxMessageBox {
public:
    static MockWxMessageBox* instance;
    MOCK_METHOD4(Show, int(const wxString& message, const wxString& caption, long style, wxWindow* parent));
    
    static int MockShow(const wxString& message, const wxString& caption = wxEmptyString, 
                       long style = wxOK, wxWindow* parent = nullptr);
};

// Mock wxLaunchDefaultBrowser for URL launching
class MockWxLaunchDefaultBrowser {
public:
    static MockWxLaunchDefaultBrowser* instance;
    MOCK_METHOD2(Launch, bool(const wxString& url, int flags));
    
    static bool MockLaunch(const wxString& url, int flags = 0);
};

// Helper class to manage mock instances
class MockWxComponentsManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetAllMocks();
    
    // Getters for mock instances
    static MockWxFFile* GetMockFFile();
    static MockWxDateTime* GetMockDateTime();
    static MockWxCriticalSection* GetMockCriticalSection();
    static MockWxThread* GetMockThread();
    static MockWxDir* GetMockDir();
    static MockWxFileName* GetMockFileName();
    static MockWxGrid* GetMockGrid();
    static MockWxDialog* GetMockDialog();
    static MockWxClipboard* GetMockClipboard();
    static MockWxMessageBox* GetMockMessageBox();
    static MockWxLaunchDefaultBrowser* GetMockLaunchDefaultBrowser();

private:
    static MockWxFFile* mockFFile;
    static MockWxDateTime* mockDateTime;
    static MockWxCriticalSection* mockCriticalSection;
    static MockWxThread* mockThread;
    static MockWxDir* mockDir;
    static MockWxFileName* mockFileName;
    static MockWxGrid* mockGrid;
    static MockWxDialog* mockDialog;
    static MockWxClipboard* mockClipboard;
    static MockWxMessageBox* mockMessageBox;
    static MockWxLaunchDefaultBrowser* mockLaunchDefaultBrowser;
};

// Macros for easier mock setup in tests
#define SETUP_WX_MOCKS() MockWxComponentsManager::SetupMocks()
#define TEARDOWN_WX_MOCKS() MockWxComponentsManager::TeardownMocks()
#define RESET_WX_MOCKS() MockWxComponentsManager::ResetAllMocks()

#define GET_MOCK_FFILE() MockWxComponentsManager::GetMockFFile()
#define GET_MOCK_DATETIME() MockWxComponentsManager::GetMockDateTime()
#define GET_MOCK_CRITICAL_SECTION() MockWxComponentsManager::GetMockCriticalSection()
#define GET_MOCK_THREAD() MockWxComponentsManager::GetMockThread()
#define GET_MOCK_DIR() MockWxComponentsManager::GetMockDir()
#define GET_MOCK_FILENAME() MockWxComponentsManager::GetMockFileName()
#define GET_MOCK_GRID() MockWxComponentsManager::GetMockGrid()
#define GET_MOCK_DIALOG() MockWxComponentsManager::GetMockDialog()
#define GET_MOCK_CLIPBOARD() MockWxComponentsManager::GetMockClipboard()
#define GET_MOCK_MESSAGEBOX() MockWxComponentsManager::GetMockMessageBox()
#define GET_MOCK_LAUNCH_BROWSER() MockWxComponentsManager::GetMockLaunchDefaultBrowser()

#endif // MOCK_WX_COMPONENTS_H
