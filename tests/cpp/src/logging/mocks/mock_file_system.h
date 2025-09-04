/*
 * mock_file_system.h
 * PHD Guiding - Logging Module Tests
 *
 * Mock objects for file system operations used in logging tests
 * Provides controllable behavior for file I/O, directory operations, and path handling
 */

#ifndef MOCK_FILE_SYSTEM_H
#define MOCK_FILE_SYSTEM_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

// Mock file system interface
class MockFileSystem {
public:
    // File operations
    MOCK_METHOD2(FileExists, bool(const wxString& path, bool checkReadable));
    MOCK_METHOD1(FileExists, bool(const wxString& path));
    MOCK_METHOD1(RemoveFile, bool(const wxString& path));
    MOCK_METHOD2(RenameFile, bool(const wxString& oldPath, const wxString& newPath));
    MOCK_METHOD2(CopyFile, bool(const wxString& src, const wxString& dest));
    MOCK_METHOD1(GetFileSize, wxFileOffset(const wxString& path));
    MOCK_METHOD1(GetFileModificationTime, wxDateTime(const wxString& path));
    
    // Directory operations
    MOCK_METHOD1(DirExists, bool(const wxString& path));
    MOCK_METHOD3(MakeDir, bool(const wxString& path, int perm, int flags));
    MOCK_METHOD2(RemoveDir, bool(const wxString& path, int flags));
    MOCK_METHOD3(ListFiles, std::vector<wxString>(const wxString& path, const wxString& pattern, int flags));
    MOCK_METHOD3(ListDirectories, std::vector<wxString>(const wxString& path, const wxString& pattern, int flags));
    
    // Path operations
    MOCK_METHOD1(GetAbsolutePath, wxString(const wxString& path));
    MOCK_METHOD1(GetDirectoryPath, wxString(const wxString& path));
    MOCK_METHOD1(GetFileName, wxString(const wxString& path));
    MOCK_METHOD1(GetFileExtension, wxString(const wxString& path));
    MOCK_METHOD2(JoinPaths, wxString(const wxString& path1, const wxString& path2));
    MOCK_METHOD1(NormalizePath, wxString(const wxString& path));
    
    // Standard paths
    MOCK_METHOD0(GetDocumentsDir, wxString());
    MOCK_METHOD0(GetTempDir, wxString());
    MOCK_METHOD0(GetHomeDir, wxString());
    MOCK_METHOD0(GetExecutablePath, wxString());
    
    // Permissions and attributes
    MOCK_METHOD1(IsFileReadable, bool(const wxString& path));
    MOCK_METHOD1(IsFileWritable, bool(const wxString& path));
    MOCK_METHOD1(IsFileExecutable, bool(const wxString& path));
    MOCK_METHOD2(SetFilePermissions, bool(const wxString& path, int permissions));
    
    // Disk space
    MOCK_METHOD1(GetFreeDiskSpace, wxLongLong(const wxString& path));
    MOCK_METHOD1(GetTotalDiskSpace, wxLongLong(const wxString& path));
    
    static MockFileSystem* instance;
    static MockFileSystem* GetInstance();
    static void SetInstance(MockFileSystem* inst);
};

// Mock file handle for simulating file operations
class MockFileHandle {
public:
    MOCK_METHOD2(Open, bool(const wxString& path, const wxString& mode));
    MOCK_METHOD0(Close, bool());
    MOCK_METHOD0(IsOpen, bool());
    MOCK_METHOD1(Read, size_t(void* buffer, size_t size));
    MOCK_METHOD2(Write, size_t(const void* buffer, size_t size));
    MOCK_METHOD1(WriteString, size_t(const wxString& str));
    MOCK_METHOD0(Flush, bool());
    MOCK_METHOD1(Seek, bool(wxFileOffset pos));
    MOCK_METHOD0(Tell, wxFileOffset());
    MOCK_METHOD0(Length, wxFileOffset());
    MOCK_METHOD0(Eof, bool());
    MOCK_METHOD0(Error, bool());
    MOCK_METHOD0(GetLastError, int());
    
    // Additional methods for testing
    MOCK_METHOD1(SetSimulatedContent, void(const wxString& content));
    MOCK_METHOD0(GetWrittenContent, wxString());
    MOCK_METHOD1(SetShouldFail, void(bool fail));
    MOCK_METHOD1(SetErrorCode, void(int error));
    
private:
    wxString simulatedContent;
    wxString writtenContent;
    bool shouldFail = false;
    int errorCode = 0;
    wxFileOffset currentPos = 0;
};

// Mock directory traverser for directory scanning
class MockDirectoryTraverser {
public:
    MOCK_METHOD1(OnFile, wxDirTraverseResult(const wxString& filename));
    MOCK_METHOD1(OnDir, wxDirTraverseResult(const wxString& dirname));
    MOCK_METHOD1(OnOpenError, wxDirTraverseResult(const wxString& openerrorname));
    
    // Helper methods for testing
    MOCK_METHOD1(SetFilesToFind, void(const std::vector<wxString>& files));
    MOCK_METHOD1(SetDirectoriesToFind, void(const std::vector<wxString>& dirs));
    MOCK_METHOD1(SetShouldFailOnFile, void(const wxString& filename));
    MOCK_METHOD1(SetShouldFailOnDir, void(const wxString& dirname));
};

// File system simulator for comprehensive testing
class FileSystemSimulator {
public:
    struct FileEntry {
        wxString content;
        wxDateTime modTime;
        wxFileOffset size;
        int permissions;
        bool exists;
        bool readable;
        bool writable;
        bool executable;
        
        FileEntry() : size(0), permissions(0644), exists(false), 
                     readable(true), writable(true), executable(false) {}
    };
    
    struct DirectoryEntry {
        bool exists;
        int permissions;
        std::vector<wxString> files;
        std::vector<wxString> subdirs;
        
        DirectoryEntry() : exists(false), permissions(0755) {}
    };
    
    // File operations
    void CreateFile(const wxString& path, const wxString& content = wxEmptyString);
    void RemoveFile(const wxString& path);
    bool FileExists(const wxString& path) const;
    void SetFileContent(const wxString& path, const wxString& content);
    wxString GetFileContent(const wxString& path) const;
    void SetFileModTime(const wxString& path, const wxDateTime& modTime);
    wxDateTime GetFileModTime(const wxString& path) const;
    void SetFilePermissions(const wxString& path, int permissions);
    int GetFilePermissions(const wxString& path) const;
    
    // Directory operations
    void CreateDirectory(const wxString& path);
    void RemoveDirectory(const wxString& path);
    bool DirectoryExists(const wxString& path) const;
    void AddFileToDirectory(const wxString& dirPath, const wxString& fileName);
    void AddSubdirectoryToDirectory(const wxString& dirPath, const wxString& subdirName);
    std::vector<wxString> GetFilesInDirectory(const wxString& path) const;
    std::vector<wxString> GetSubdirectoriesInDirectory(const wxString& path) const;
    
    // Utility methods
    void Clear();
    void SetDefaultDirectories();
    void SimulateDiskFull(bool full);
    void SimulatePermissionDenied(const wxString& path, bool denied);
    
    // Error simulation
    void SetShouldFailFileOperation(const wxString& path, bool shouldFail);
    void SetShouldFailDirectoryOperation(const wxString& path, bool shouldFail);
    bool ShouldFailFileOperation(const wxString& path) const;
    bool ShouldFailDirectoryOperation(const wxString& path) const;
    
private:
    std::map<wxString, FileEntry> files;
    std::map<wxString, DirectoryEntry> directories;
    std::map<wxString, bool> fileOperationFailures;
    std::map<wxString, bool> directoryOperationFailures;
    bool diskFull = false;
    
    wxString NormalizePath(const wxString& path) const;
};

// Helper class to manage file system mocking
class MockFileSystemManager {
public:
    static void SetupMocks();
    static void TeardownMocks();
    static void ResetMocks();
    
    static MockFileSystem* GetMockFileSystem();
    static FileSystemSimulator* GetSimulator();
    
    // Convenience methods for common test scenarios
    static void SetupStandardDirectories();
    static void SetupLogDirectories();
    static void SimulateFileSystemError(const wxString& path, bool error = true);
    static void SimulateDiskFull(bool full = true);
    static void SimulatePermissionDenied(const wxString& path, bool denied = true);
    
private:
    static MockFileSystem* mockFileSystem;
    static std::unique_ptr<FileSystemSimulator> simulator;
};

// Macros for easier mock setup in tests
#define SETUP_FILESYSTEM_MOCKS() MockFileSystemManager::SetupMocks()
#define TEARDOWN_FILESYSTEM_MOCKS() MockFileSystemManager::TeardownMocks()
#define RESET_FILESYSTEM_MOCKS() MockFileSystemManager::ResetMocks()

#define GET_MOCK_FILESYSTEM() MockFileSystemManager::GetMockFileSystem()
#define GET_FILESYSTEM_SIMULATOR() MockFileSystemManager::GetSimulator()

// Helper macros for common expectations
#define EXPECT_FILE_EXISTS(path, result) \
    EXPECT_CALL(*GET_MOCK_FILESYSTEM(), FileExists(path)) \
        .WillOnce(::testing::Return(result))

#define EXPECT_DIR_EXISTS(path, result) \
    EXPECT_CALL(*GET_MOCK_FILESYSTEM(), DirExists(path)) \
        .WillOnce(::testing::Return(result))

#define EXPECT_FILE_REMOVE(path, result) \
    EXPECT_CALL(*GET_MOCK_FILESYSTEM(), RemoveFile(path)) \
        .WillOnce(::testing::Return(result))

#define EXPECT_DIR_CREATE(path, result) \
    EXPECT_CALL(*GET_MOCK_FILESYSTEM(), MakeDir(path, ::testing::_, ::testing::_)) \
        .WillOnce(::testing::Return(result))

#endif // MOCK_FILE_SYSTEM_H
