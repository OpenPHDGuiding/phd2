/*
 * mock_file_operations.h
 * PHD Guiding - Core Module Tests
 *
 * Mock objects for file system operations
 * Provides controllable behavior for file I/O, directory operations, and path handling
 */

#ifndef MOCK_FILE_OPERATIONS_H
#define MOCK_FILE_OPERATIONS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declarations
class FileOperationSimulator;

// Mock file operations
class MockFileOperations {
public:
    // File existence and properties
    MOCK_METHOD1(FileExists, bool(const wxString& filename));
    MOCK_METHOD1(DirExists, bool(const wxString& dirname));
    MOCK_METHOD1(GetFileSize, wxFileOffset(const wxString& filename));
    MOCK_METHOD1(GetModificationTime, wxDateTime(const wxString& filename));
    MOCK_METHOD1(IsFileReadable, bool(const wxString& filename));
    MOCK_METHOD1(IsFileWritable, bool(const wxString& filename));
    
    // File operations
    MOCK_METHOD2(ReadFile, bool(const wxString& filename, wxString& content));
    MOCK_METHOD2(ReadBinaryFile, bool(const wxString& filename, std::vector<unsigned char>& data));
    MOCK_METHOD2(WriteFile, bool(const wxString& filename, const wxString& content));
    MOCK_METHOD2(WriteBinaryFile, bool(const wxString& filename, const std::vector<unsigned char>& data));
    MOCK_METHOD1(DeleteFile, bool(const wxString& filename));
    MOCK_METHOD2(CopyFile, bool(const wxString& src, const wxString& dest));
    MOCK_METHOD2(MoveFile, bool(const wxString& src, const wxString& dest));
    
    // Directory operations
    MOCK_METHOD1(CreateDirectory, bool(const wxString& dirname));
    MOCK_METHOD1(RemoveDirectory, bool(const wxString& dirname));
    MOCK_METHOD2(ListFiles, std::vector<wxString>(const wxString& dirname, const wxString& pattern));
    MOCK_METHOD1(ListDirectories, std::vector<wxString>(const wxString& dirname));
    
    // Path operations
    MOCK_METHOD1(GetAbsolutePath, wxString(const wxString& path));
    MOCK_METHOD1(GetRelativePath, wxString(const wxString& path));
    MOCK_METHOD1(GetFileName, wxString(const wxString& path));
    MOCK_METHOD1(GetFileExtension, wxString(const wxString& path));
    MOCK_METHOD1(GetDirectory, wxString(const wxString& path));
    MOCK_METHOD2(JoinPath, wxString(const wxString& dir, const wxString& file));
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD2(SimulateFileContent, void(const wxString& filename, const wxString& content));
    MOCK_METHOD2(SimulateFileSize, void(const wxString& filename, wxFileOffset size));
    
    static MockFileOperations* instance;
    static MockFileOperations* GetInstance();
    static void SetInstance(MockFileOperations* inst);
};

// Mock wxFileName operations
class MockWxFileName {
public:
    // Construction and assignment
    MOCK_METHOD2(Assign, void(const wxString& fullpath, wxPathFormat format));
    MOCK_METHOD3(Assign, void(const wxString& path, const wxString& name, wxPathFormat format));
    MOCK_METHOD4(Assign, void(const wxString& path, const wxString& name, const wxString& ext, wxPathFormat format));
    
    // Path components
    MOCK_METHOD0(GetPath, wxString());
    MOCK_METHOD0(GetName, wxString());
    MOCK_METHOD0(GetExt, wxString());
    MOCK_METHOD0(GetFullName, wxString());
    MOCK_METHOD0(GetFullPath, wxString());
    
    // Path manipulation
    MOCK_METHOD1(SetPath, void(const wxString& path));
    MOCK_METHOD1(SetName, void(const wxString& name));
    MOCK_METHOD1(SetExt, void(const wxString& ext));
    MOCK_METHOD1(SetFullName, void(const wxString& fullname));
    
    // Path queries
    MOCK_METHOD0(IsAbsolute, bool());
    MOCK_METHOD0(IsRelative, bool());
    MOCK_METHOD0(HasExt, bool());
    MOCK_METHOD0(HasName, bool());
    
    // File system operations
    MOCK_METHOD0(FileExists, bool());
    MOCK_METHOD0(DirExists, bool());
    MOCK_METHOD0(IsFileReadable, bool());
    MOCK_METHOD0(IsFileWritable, bool());
    MOCK_METHOD0(IsFileExecutable, bool());
    
    // Path normalization
    MOCK_METHOD0(Normalize, bool());
    MOCK_METHOD1(MakeRelativeTo, bool(const wxString& pathBase));
    MOCK_METHOD0(MakeAbsolute, bool());
    
    // Helper methods for testing
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockWxFileName* instance;
    static MockWxFileName* GetInstance();
    static void SetInstance(MockWxFileName* inst);
};

// Mock wxStandardPaths for system directories
class MockWxStandardPaths {
public:
    // Standard directories
    MOCK_METHOD0(GetConfigDir, wxString());
    MOCK_METHOD0(GetUserConfigDir, wxString());
    MOCK_METHOD0(GetDataDir, wxString());
    MOCK_METHOD0(GetUserDataDir, wxString());
    MOCK_METHOD0(GetLocalDataDir, wxString());
    MOCK_METHOD0(GetUserLocalDataDir, wxString());
    MOCK_METHOD0(GetPluginsDir, wxString());
    MOCK_METHOD0(GetResourcesDir, wxString());
    MOCK_METHOD0(GetDocumentsDir, wxString());
    MOCK_METHOD0(GetTempDir, wxString());
    
    // Application-specific paths
    MOCK_METHOD1(SetInstallPrefix, void(const wxString& prefix));
    MOCK_METHOD0(GetInstallPrefix, wxString());
    
    // Helper methods for testing
    MOCK_METHOD2(SetPath, void(const wxString& pathType, const wxString& path));
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    
    static MockWxStandardPaths* instance;
    static MockWxStandardPaths* GetInstance();
    static void SetInstance(MockWxStandardPaths* inst);
};

// File operation simulator for comprehensive testing
class FileOperationSimulator {
public:
    struct FileInfo {
        wxString filename;
        wxString content;
        std::vector<unsigned char> binaryData;
        wxFileOffset size;
        wxDateTime modTime;
        bool exists;
        bool readable;
        bool writable;
        bool shouldFail;
        
        FileInfo() : size(0), exists(false), readable(true), writable(true), shouldFail(false) {
            modTime = wxDateTime::Now();
        }
    };
    
    struct DirectoryInfo {
        wxString dirname;
        std::vector<wxString> files;
        std::vector<wxString> subdirs;
        bool exists;
        bool shouldFail;
        
        DirectoryInfo() : exists(false), shouldFail(false) {}
    };
    
    struct PathInfo {
        wxString configDir;
        wxString userConfigDir;
        wxString dataDir;
        wxString userDataDir;
        wxString tempDir;
        wxString documentsDir;
        bool shouldFail;
        
        PathInfo() : shouldFail(false) {
            SetupDefaultPaths();
        }
        
        void SetupDefaultPaths();
    };
    
    // File management
    void AddFile(const wxString& filename, const wxString& content = wxEmptyString);
    void AddBinaryFile(const wxString& filename, const std::vector<unsigned char>& data);
    void RemoveFile(const wxString& filename);
    FileInfo* GetFile(const wxString& filename);
    bool FileExists(const wxString& filename) const;
    
    // Directory management
    void AddDirectory(const wxString& dirname);
    void RemoveDirectory(const wxString& dirname);
    DirectoryInfo* GetDirectory(const wxString& dirname);
    bool DirectoryExists(const wxString& dirname) const;
    
    // File operations simulation
    bool ReadFile(const wxString& filename, wxString& content);
    bool ReadBinaryFile(const wxString& filename, std::vector<unsigned char>& data);
    bool WriteFile(const wxString& filename, const wxString& content);
    bool WriteBinaryFile(const wxString& filename, const std::vector<unsigned char>& data);
    bool DeleteFile(const wxString& filename);
    bool CopyFile(const wxString& src, const wxString& dest);
    
    // Directory operations simulation
    bool CreateDirectory(const wxString& dirname);
    std::vector<wxString> ListFiles(const wxString& dirname, const wxString& pattern = "*");
    std::vector<wxString> ListDirectories(const wxString& dirname);
    
    // Path operations simulation
    wxString GetAbsolutePath(const wxString& path);
    wxString GetFileName(const wxString& path);
    wxString GetFileExtension(const wxString& path);
    wxString GetDirectory(const wxString& path);
    wxString JoinPath(const wxString& dir, const wxString& file);
    
    // Standard paths simulation
    void SetupPaths(const PathInfo& info);
    PathInfo GetPathInfo() const;
    wxString GetStandardPath(const wxString& pathType);
    
    // Error simulation
    void SetFileError(const wxString& filename, bool error);
    void SetDirectoryError(const wxString& dirname, bool error);
    void SetPathError(bool error);
    
    // Utility methods
    void Reset();
    void SetupDefaultFileSystem();
    
private:
    std::map<wxString, std::unique_ptr<FileInfo>> files;
    std::map<wxString, std::unique_ptr<DirectoryInfo>> directories;
    PathInfo pathInfo;
    
    void InitializeDefaults();
    wxString NormalizePath(const wxString& path);
    bool MatchesPattern(const wxString& filename, const wxString& pattern);
};

// Helper class to manage all file operation mocks
class MockFileOperationsManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    // Getters for mock instances
    static MockFileOperations* GetMockFileOps();
    static MockWxFileName* GetMockFileName();
    static MockWxStandardPaths* GetMockStandardPaths();
    static FileOperationSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupFileSystem();
    static void SetupConfigFiles();
    static void SetupTempDirectory();
    static void SimulateFileSystemError();
    static void SimulatePermissionDenied();
    
private:
    static MockFileOperations* mockFileOps;
    static MockWxFileName* mockFileName;
    static MockWxStandardPaths* mockStandardPaths;
    static std::unique_ptr<FileOperationSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_FILE_OPERATION_MOCKS() MockFileOperationsManager::SetupMocks()
#define TEARDOWN_FILE_OPERATION_MOCKS() MockFileOperationsManager::TeardownMocks()
#define RESET_FILE_OPERATION_MOCKS() MockFileOperationsManager::ResetMocks()

#define GET_MOCK_FILE_OPS() MockFileOperationsManager::GetMockFileOps()
#define GET_MOCK_FILENAME() MockFileOperationsManager::GetMockFileName()
#define GET_MOCK_STANDARD_PATHS() MockFileOperationsManager::GetMockStandardPaths()
#define GET_FILE_SIMULATOR() MockFileOperationsManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_FILE_EXISTS(filename) \
    EXPECT_CALL(*GET_MOCK_FILE_OPS(), FileExists(filename)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_FILE_NOT_EXISTS(filename) \
    EXPECT_CALL(*GET_MOCK_FILE_OPS(), FileExists(filename)) \
        .WillOnce(::testing::Return(false))

#define EXPECT_FILE_READ_SUCCESS(filename, content) \
    EXPECT_CALL(*GET_MOCK_FILE_OPS(), ReadFile(filename, ::testing::_)) \
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>(content), ::testing::Return(true)))

#define EXPECT_FILE_WRITE_SUCCESS(filename, content) \
    EXPECT_CALL(*GET_MOCK_FILE_OPS(), WriteFile(filename, content)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_DIR_CREATE_SUCCESS(dirname) \
    EXPECT_CALL(*GET_MOCK_FILE_OPS(), CreateDirectory(dirname)) \
        .WillOnce(::testing::Return(true))

#define EXPECT_PATH_GET_SUCCESS(pathType, path) \
    EXPECT_CALL(*GET_MOCK_STANDARD_PATHS(), Get##pathType##Dir()) \
        .WillOnce(::testing::Return(path))

#endif // MOCK_FILE_OPERATIONS_H
