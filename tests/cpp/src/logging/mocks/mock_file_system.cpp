/*
 * mock_file_system.cpp
 * PHD Guiding - Logging Module Tests
 *
 * Implementation of mock file system objects
 */

#include "mock_file_system.h"
#include <algorithm>
#include <sstream>

// Static instance declarations
MockFileSystem* MockFileSystem::instance = nullptr;
MockFileSystem* MockFileSystemManager::mockFileSystem = nullptr;
std::unique_ptr<FileSystemSimulator> MockFileSystemManager::simulator = nullptr;

// MockFileSystem implementation
MockFileSystem* MockFileSystem::GetInstance() {
    return instance;
}

void MockFileSystem::SetInstance(MockFileSystem* inst) {
    instance = inst;
}

// FileSystemSimulator implementation
void FileSystemSimulator::CreateFile(const wxString& path, const wxString& content) {
    wxString normalizedPath = NormalizePath(path);
    FileEntry& entry = files[normalizedPath];
    entry.content = content;
    entry.size = content.length();
    entry.modTime = wxDateTime::Now();
    entry.exists = true;
    entry.readable = true;
    entry.writable = true;
}

void FileSystemSimulator::RemoveFile(const wxString& path) {
    wxString normalizedPath = NormalizePath(path);
    auto it = files.find(normalizedPath);
    if (it != files.end()) {
        it->second.exists = false;
    }
}

bool FileSystemSimulator::FileExists(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = files.find(normalizedPath);
    return it != files.end() && it->second.exists;
}

void FileSystemSimulator::SetFileContent(const wxString& path, const wxString& content) {
    wxString normalizedPath = NormalizePath(path);
    if (files.find(normalizedPath) != files.end()) {
        files[normalizedPath].content = content;
        files[normalizedPath].size = content.length();
        files[normalizedPath].modTime = wxDateTime::Now();
    }
}

wxString FileSystemSimulator::GetFileContent(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = files.find(normalizedPath);
    if (it != files.end() && it->second.exists) {
        return it->second.content;
    }
    return wxEmptyString;
}

void FileSystemSimulator::SetFileModTime(const wxString& path, const wxDateTime& modTime) {
    wxString normalizedPath = NormalizePath(path);
    if (files.find(normalizedPath) != files.end()) {
        files[normalizedPath].modTime = modTime;
    }
}

wxDateTime FileSystemSimulator::GetFileModTime(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = files.find(normalizedPath);
    if (it != files.end() && it->second.exists) {
        return it->second.modTime;
    }
    return wxInvalidDateTime;
}

void FileSystemSimulator::SetFilePermissions(const wxString& path, int permissions) {
    wxString normalizedPath = NormalizePath(path);
    if (files.find(normalizedPath) != files.end()) {
        files[normalizedPath].permissions = permissions;
        files[normalizedPath].readable = (permissions & 0400) != 0;
        files[normalizedPath].writable = (permissions & 0200) != 0;
        files[normalizedPath].executable = (permissions & 0100) != 0;
    }
}

int FileSystemSimulator::GetFilePermissions(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = files.find(normalizedPath);
    if (it != files.end() && it->second.exists) {
        return it->second.permissions;
    }
    return 0;
}

void FileSystemSimulator::CreateDirectory(const wxString& path) {
    wxString normalizedPath = NormalizePath(path);
    DirectoryEntry& entry = directories[normalizedPath];
    entry.exists = true;
    entry.permissions = 0755;
}

void FileSystemSimulator::RemoveDirectory(const wxString& path) {
    wxString normalizedPath = NormalizePath(path);
    auto it = directories.find(normalizedPath);
    if (it != directories.end()) {
        it->second.exists = false;
    }
}

bool FileSystemSimulator::DirectoryExists(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = directories.find(normalizedPath);
    return it != directories.end() && it->second.exists;
}

void FileSystemSimulator::AddFileToDirectory(const wxString& dirPath, const wxString& fileName) {
    wxString normalizedDirPath = NormalizePath(dirPath);
    if (directories.find(normalizedDirPath) != directories.end()) {
        auto& files = directories[normalizedDirPath].files;
        if (std::find(files.begin(), files.end(), fileName) == files.end()) {
            files.push_back(fileName);
        }
    }
}

void FileSystemSimulator::AddSubdirectoryToDirectory(const wxString& dirPath, const wxString& subdirName) {
    wxString normalizedDirPath = NormalizePath(dirPath);
    if (directories.find(normalizedDirPath) != directories.end()) {
        auto& subdirs = directories[normalizedDirPath].subdirs;
        if (std::find(subdirs.begin(), subdirs.end(), subdirName) == subdirs.end()) {
            subdirs.push_back(subdirName);
        }
    }
}

std::vector<wxString> FileSystemSimulator::GetFilesInDirectory(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = directories.find(normalizedPath);
    if (it != directories.end() && it->second.exists) {
        return it->second.files;
    }
    return std::vector<wxString>();
}

std::vector<wxString> FileSystemSimulator::GetSubdirectoriesInDirectory(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = directories.find(normalizedPath);
    if (it != directories.end() && it->second.exists) {
        return it->second.subdirs;
    }
    return std::vector<wxString>();
}

void FileSystemSimulator::Clear() {
    files.clear();
    directories.clear();
    fileOperationFailures.clear();
    directoryOperationFailures.clear();
    diskFull = false;
}

void FileSystemSimulator::SetDefaultDirectories() {
    CreateDirectory("/home/user/Documents");
    CreateDirectory("/tmp");
    CreateDirectory("/home/user");
    CreateDirectory("/usr/bin");
}

void FileSystemSimulator::SimulateDiskFull(bool full) {
    diskFull = full;
}

void FileSystemSimulator::SimulatePermissionDenied(const wxString& path, bool denied) {
    wxString normalizedPath = NormalizePath(path);
    if (denied) {
        fileOperationFailures[normalizedPath] = true;
        directoryOperationFailures[normalizedPath] = true;
    } else {
        fileOperationFailures.erase(normalizedPath);
        directoryOperationFailures.erase(normalizedPath);
    }
}

void FileSystemSimulator::SetShouldFailFileOperation(const wxString& path, bool shouldFail) {
    wxString normalizedPath = NormalizePath(path);
    if (shouldFail) {
        fileOperationFailures[normalizedPath] = true;
    } else {
        fileOperationFailures.erase(normalizedPath);
    }
}

void FileSystemSimulator::SetShouldFailDirectoryOperation(const wxString& path, bool shouldFail) {
    wxString normalizedPath = NormalizePath(path);
    if (shouldFail) {
        directoryOperationFailures[normalizedPath] = true;
    } else {
        directoryOperationFailures.erase(normalizedPath);
    }
}

bool FileSystemSimulator::ShouldFailFileOperation(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = fileOperationFailures.find(normalizedPath);
    return it != fileOperationFailures.end() && it->second;
}

bool FileSystemSimulator::ShouldFailDirectoryOperation(const wxString& path) const {
    wxString normalizedPath = NormalizePath(path);
    auto it = directoryOperationFailures.find(normalizedPath);
    return it != directoryOperationFailures.end() && it->second;
}

wxString FileSystemSimulator::NormalizePath(const wxString& path) const {
    wxString normalized = path;
    normalized.Replace("\\", "/");
    
    // Remove duplicate slashes
    while (normalized.Contains("//")) {
        normalized.Replace("//", "/");
    }
    
    // Remove trailing slash (except for root)
    if (normalized.length() > 1 && normalized.EndsWith("/")) {
        normalized = normalized.Left(normalized.length() - 1);
    }
    
    return normalized;
}

// MockFileSystemManager implementation
void MockFileSystemManager::SetupMocks() {
    mockFileSystem = new MockFileSystem();
    simulator = std::make_unique<FileSystemSimulator>();
    MockFileSystem::SetInstance(mockFileSystem);
    
    // Set up default simulator state
    simulator->SetDefaultDirectories();
}

void MockFileSystemManager::TeardownMocks() {
    delete mockFileSystem;
    mockFileSystem = nullptr;
    simulator.reset();
    MockFileSystem::SetInstance(nullptr);
}

void MockFileSystemManager::ResetMocks() {
    if (mockFileSystem) {
        testing::Mock::VerifyAndClearExpectations(mockFileSystem);
    }
    if (simulator) {
        simulator->Clear();
        simulator->SetDefaultDirectories();
    }
}

MockFileSystem* MockFileSystemManager::GetMockFileSystem() {
    return mockFileSystem;
}

FileSystemSimulator* MockFileSystemManager::GetSimulator() {
    return simulator.get();
}

void MockFileSystemManager::SetupStandardDirectories() {
    if (simulator) {
        simulator->CreateDirectory("/home/user/Documents/PHD2");
        simulator->CreateDirectory("/tmp/phd2_test");
        simulator->CreateDirectory("/var/log");
    }
}

void MockFileSystemManager::SetupLogDirectories() {
    if (simulator) {
        simulator->CreateDirectory("/home/user/Documents/PHD2");
        simulator->CreateDirectory("/home/user/Documents/PHD2/logs");
        simulator->CreateDirectory("/home/user/Documents/PHD2/PHD2_CameraFrames_2023-01-01-120000");
    }
}

void MockFileSystemManager::SimulateFileSystemError(const wxString& path, bool error) {
    if (simulator) {
        simulator->SetShouldFailFileOperation(path, error);
        simulator->SetShouldFailDirectoryOperation(path, error);
    }
}

void MockFileSystemManager::SimulateDiskFull(bool full) {
    if (simulator) {
        simulator->SimulateDiskFull(full);
    }
}

void MockFileSystemManager::SimulatePermissionDenied(const wxString& path, bool denied) {
    if (simulator) {
        simulator->SimulatePermissionDenied(path, denied);
    }
}
