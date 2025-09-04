/*
 * mock_file_operations.cpp
 * PHD Guiding - Core Module Tests
 *
 * Implementation of mock file operation objects
 */

#include "mock_file_operations.h"
#include <algorithm>
#include <regex>

// Static instance declarations
MockFileOperations* MockFileOperations::instance = nullptr;
MockWxFileName* MockWxFileName::instance = nullptr;
MockWxStandardPaths* MockWxStandardPaths::instance = nullptr;

// MockFileOperationsManager static members
MockFileOperations* MockFileOperationsManager::mockFileOps = nullptr;
MockWxFileName* MockFileOperationsManager::mockFileName = nullptr;
MockWxStandardPaths* MockFileOperationsManager::mockStandardPaths = nullptr;
std::unique_ptr<FileOperationSimulator> MockFileOperationsManager::simulator = nullptr;

// MockFileOperations implementation
MockFileOperations* MockFileOperations::GetInstance() {
    return instance;
}

void MockFileOperations::SetInstance(MockFileOperations* inst) {
    instance = inst;
}

// MockWxFileName implementation
MockWxFileName* MockWxFileName::GetInstance() {
    return instance;
}

void MockWxFileName::SetInstance(MockWxFileName* inst) {
    instance = inst;
}

// MockWxStandardPaths implementation
MockWxStandardPaths* MockWxStandardPaths::GetInstance() {
    return instance;
}

void MockWxStandardPaths::SetInstance(MockWxStandardPaths* inst) {
    instance = inst;
}

// FileOperationSimulator implementation
void FileOperationSimulator::PathInfo::SetupDefaultPaths() {
#ifdef _WIN32
    configDir = "C:\\ProgramData\\PHD2";
    userConfigDir = "C:\\Users\\User\\AppData\\Roaming\\PHD2";
    dataDir = "C:\\Program Files\\PHD2";
    userDataDir = "C:\\Users\\User\\AppData\\Local\\PHD2";
    tempDir = "C:\\Users\\User\\AppData\\Local\\Temp";
    documentsDir = "C:\\Users\\User\\Documents";
#elif defined(__APPLE__)
    configDir = "/Library/Application Support/PHD2";
    userConfigDir = "~/Library/Application Support/PHD2";
    dataDir = "/Applications/PHD2.app/Contents/Resources";
    userDataDir = "~/Library/Application Support/PHD2";
    tempDir = "/tmp";
    documentsDir = "~/Documents";
#else
    configDir = "/etc/phd2";
    userConfigDir = "~/.config/phd2";
    dataDir = "/usr/share/phd2";
    userDataDir = "~/.local/share/phd2";
    tempDir = "/tmp";
    documentsDir = "~/Documents";
#endif
}

void FileOperationSimulator::AddFile(const wxString& filename, const wxString& content) {
    auto file = std::make_unique<FileInfo>();
    file->filename = filename;
    file->content = content;
    file->size = content.length();
    file->exists = true;
    file->modTime = wxDateTime::Now();
    files[filename] = std::move(file);
}

void FileOperationSimulator::AddBinaryFile(const wxString& filename, const std::vector<unsigned char>& data) {
    auto file = std::make_unique<FileInfo>();
    file->filename = filename;
    file->binaryData = data;
    file->size = data.size();
    file->exists = true;
    file->modTime = wxDateTime::Now();
    files[filename] = std::move(file);
}

void FileOperationSimulator::RemoveFile(const wxString& filename) {
    files.erase(filename);
}

FileOperationSimulator::FileInfo* FileOperationSimulator::GetFile(const wxString& filename) {
    auto it = files.find(filename);
    return (it != files.end()) ? it->second.get() : nullptr;
}

bool FileOperationSimulator::FileExists(const wxString& filename) const {
    auto it = files.find(filename);
    return (it != files.end()) && it->second->exists;
}

void FileOperationSimulator::AddDirectory(const wxString& dirname) {
    auto dir = std::make_unique<DirectoryInfo>();
    dir->dirname = dirname;
    dir->exists = true;
    directories[dirname] = std::move(dir);
}

void FileOperationSimulator::RemoveDirectory(const wxString& dirname) {
    directories.erase(dirname);
}

FileOperationSimulator::DirectoryInfo* FileOperationSimulator::GetDirectory(const wxString& dirname) {
    auto it = directories.find(dirname);
    return (it != directories.end()) ? it->second.get() : nullptr;
}

bool FileOperationSimulator::DirectoryExists(const wxString& dirname) const {
    auto it = directories.find(dirname);
    return (it != directories.end()) && it->second->exists;
}

bool FileOperationSimulator::ReadFile(const wxString& filename, wxString& content) {
    auto* file = GetFile(filename);
    if (file && file->exists && file->readable && !file->shouldFail) {
        content = file->content;
        return true;
    }
    return false;
}

bool FileOperationSimulator::ReadBinaryFile(const wxString& filename, std::vector<unsigned char>& data) {
    auto* file = GetFile(filename);
    if (file && file->exists && file->readable && !file->shouldFail) {
        data = file->binaryData;
        return true;
    }
    return false;
}

bool FileOperationSimulator::WriteFile(const wxString& filename, const wxString& content) {
    auto* file = GetFile(filename);
    if (!file) {
        AddFile(filename, content);
        return true;
    }
    
    if (file->writable && !file->shouldFail) {
        file->content = content;
        file->size = content.length();
        file->modTime = wxDateTime::Now();
        return true;
    }
    return false;
}

bool FileOperationSimulator::WriteBinaryFile(const wxString& filename, const std::vector<unsigned char>& data) {
    auto* file = GetFile(filename);
    if (!file) {
        AddBinaryFile(filename, data);
        return true;
    }
    
    if (file->writable && !file->shouldFail) {
        file->binaryData = data;
        file->size = data.size();
        file->modTime = wxDateTime::Now();
        return true;
    }
    return false;
}

bool FileOperationSimulator::DeleteFile(const wxString& filename) {
    auto* file = GetFile(filename);
    if (file && file->exists && file->writable && !file->shouldFail) {
        file->exists = false;
        return true;
    }
    return false;
}

bool FileOperationSimulator::CopyFile(const wxString& src, const wxString& dest) {
    auto* srcFile = GetFile(src);
    if (srcFile && srcFile->exists && srcFile->readable && !srcFile->shouldFail) {
        if (!srcFile->binaryData.empty()) {
            return WriteBinaryFile(dest, srcFile->binaryData);
        } else {
            return WriteFile(dest, srcFile->content);
        }
    }
    return false;
}

bool FileOperationSimulator::CreateDirectory(const wxString& dirname) {
    auto* dir = GetDirectory(dirname);
    if (!dir) {
        AddDirectory(dirname);
        return true;
    }
    
    if (!dir->shouldFail) {
        dir->exists = true;
        return true;
    }
    return false;
}

std::vector<wxString> FileOperationSimulator::ListFiles(const wxString& dirname, const wxString& pattern) {
    std::vector<wxString> result;
    auto* dir = GetDirectory(dirname);
    
    if (dir && dir->exists && !dir->shouldFail) {
        for (const auto& file : dir->files) {
            if (MatchesPattern(file, pattern)) {
                result.push_back(file);
            }
        }
    }
    
    return result;
}

std::vector<wxString> FileOperationSimulator::ListDirectories(const wxString& dirname) {
    std::vector<wxString> result;
    auto* dir = GetDirectory(dirname);
    
    if (dir && dir->exists && !dir->shouldFail) {
        result = dir->subdirs;
    }
    
    return result;
}

wxString FileOperationSimulator::GetAbsolutePath(const wxString& path) {
    if (path.StartsWith("/") || path.Contains(":")) {
        return path; // Already absolute
    }
    
#ifdef _WIN32
    return "C:\\" + path;
#else
    return "/" + path;
#endif
}

wxString FileOperationSimulator::GetFileName(const wxString& path) {
    wxString normalized = NormalizePath(path);
    size_t pos = normalized.find_last_of("/\\");
    if (pos != wxString::npos) {
        return normalized.substr(pos + 1);
    }
    return normalized;
}

wxString FileOperationSimulator::GetFileExtension(const wxString& path) {
    wxString filename = GetFileName(path);
    size_t pos = filename.find_last_of('.');
    if (pos != wxString::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return wxEmptyString;
}

wxString FileOperationSimulator::GetDirectory(const wxString& path) {
    wxString normalized = NormalizePath(path);
    size_t pos = normalized.find_last_of("/\\");
    if (pos != wxString::npos) {
        return normalized.substr(0, pos);
    }
    return wxEmptyString;
}

wxString FileOperationSimulator::JoinPath(const wxString& dir, const wxString& file) {
    if (dir.empty()) return file;
    if (file.empty()) return dir;
    
    wxString result = dir;
    if (!result.EndsWith("/") && !result.EndsWith("\\")) {
#ifdef _WIN32
        result += "\\";
#else
        result += "/";
#endif
    }
    result += file;
    return result;
}

void FileOperationSimulator::SetupPaths(const PathInfo& info) {
    pathInfo = info;
}

FileOperationSimulator::PathInfo FileOperationSimulator::GetPathInfo() const {
    return pathInfo;
}

wxString FileOperationSimulator::GetStandardPath(const wxString& pathType) {
    if (pathInfo.shouldFail) {
        return wxEmptyString;
    }
    
    if (pathType == "Config") return pathInfo.configDir;
    if (pathType == "UserConfig") return pathInfo.userConfigDir;
    if (pathType == "Data") return pathInfo.dataDir;
    if (pathType == "UserData") return pathInfo.userDataDir;
    if (pathType == "Temp") return pathInfo.tempDir;
    if (pathType == "Documents") return pathInfo.documentsDir;
    
    return wxEmptyString;
}

void FileOperationSimulator::SetFileError(const wxString& filename, bool error) {
    auto* file = GetFile(filename);
    if (file) {
        file->shouldFail = error;
    }
}

void FileOperationSimulator::SetDirectoryError(const wxString& dirname, bool error) {
    auto* dir = GetDirectory(dirname);
    if (dir) {
        dir->shouldFail = error;
    }
}

void FileOperationSimulator::SetPathError(bool error) {
    pathInfo.shouldFail = error;
}

void FileOperationSimulator::Reset() {
    files.clear();
    directories.clear();
    pathInfo = PathInfo();
    
    SetupDefaultFileSystem();
}

void FileOperationSimulator::SetupDefaultFileSystem() {
    // Create default directories
    AddDirectory(pathInfo.configDir);
    AddDirectory(pathInfo.userConfigDir);
    AddDirectory(pathInfo.dataDir);
    AddDirectory(pathInfo.userDataDir);
    AddDirectory(pathInfo.tempDir);
    AddDirectory(pathInfo.documentsDir);
    
    // Add some default files
    AddFile(JoinPath(pathInfo.userConfigDir, "phd2.cfg"), "[General]\nVersion=2.6.11\n");
    AddFile(JoinPath(pathInfo.userDataDir, "guide.log"), "# PHD2 Guide Log\n");
    AddFile(JoinPath(pathInfo.tempDir, "temp.txt"), "Temporary file");
}

wxString FileOperationSimulator::NormalizePath(const wxString& path) {
    wxString result = path;
    result.Replace("\\", "/");
    return result;
}

bool FileOperationSimulator::MatchesPattern(const wxString& filename, const wxString& pattern) {
    if (pattern == "*" || pattern.empty()) {
        return true;
    }
    
    // Simple pattern matching - convert * to .* for regex
    wxString regexPattern = pattern;
    regexPattern.Replace("*", ".*");
    regexPattern.Replace("?", ".");
    
    try {
        std::regex regex(regexPattern.ToStdString());
        return std::regex_match(filename.ToStdString(), regex);
    } catch (...) {
        return false;
    }
}

// MockFileOperationsManager implementation
void MockFileOperationsManager::SetupMocks() {
    // Create all mock instances
    mockFileOps = new MockFileOperations();
    mockFileName = new MockWxFileName();
    mockStandardPaths = new MockWxStandardPaths();
    
    // Set static instances
    MockFileOperations::SetInstance(mockFileOps);
    MockWxFileName::SetInstance(mockFileName);
    MockWxStandardPaths::SetInstance(mockStandardPaths);
    
    // Create simulator
    simulator = std::make_unique<FileOperationSimulator>();
    simulator->SetupDefaultFileSystem();
}

void MockFileOperationsManager::TeardownMocks() {
    // Clean up all mock instances
    delete mockFileOps;
    delete mockFileName;
    delete mockStandardPaths;
    
    // Reset pointers
    mockFileOps = nullptr;
    mockFileName = nullptr;
    mockStandardPaths = nullptr;
    
    // Reset static instances
    MockFileOperations::SetInstance(nullptr);
    MockWxFileName::SetInstance(nullptr);
    MockWxStandardPaths::SetInstance(nullptr);
    
    // Clean up simulator
    simulator.reset();
}

void MockFileOperationsManager::ResetMocks() {
    if (mockFileOps) {
        testing::Mock::VerifyAndClearExpectations(mockFileOps);
    }
    if (mockFileName) {
        testing::Mock::VerifyAndClearExpectations(mockFileName);
    }
    if (mockStandardPaths) {
        testing::Mock::VerifyAndClearExpectations(mockStandardPaths);
    }
    
    if (simulator) {
        simulator->Reset();
    }
}

// Getter methods
MockFileOperations* MockFileOperationsManager::GetMockFileOps() { return mockFileOps; }
MockWxFileName* MockFileOperationsManager::GetMockFileName() { return mockFileName; }
MockWxStandardPaths* MockFileOperationsManager::GetMockStandardPaths() { return mockStandardPaths; }
FileOperationSimulator* MockFileOperationsManager::GetSimulator() { return simulator.get(); }

void MockFileOperationsManager::SetupFileSystem() {
    if (simulator) {
        simulator->SetupDefaultFileSystem();
    }
    
    if (mockFileOps) {
        EXPECT_CALL(*mockFileOps, FileExists(::testing::_))
            .WillRepeatedly(::testing::Invoke([](const wxString& filename) {
                return GET_FILE_SIMULATOR()->FileExists(filename);
            }));
        
        EXPECT_CALL(*mockFileOps, DirExists(::testing::_))
            .WillRepeatedly(::testing::Invoke([](const wxString& dirname) {
                return GET_FILE_SIMULATOR()->DirectoryExists(dirname);
            }));
    }
}

void MockFileOperationsManager::SetupConfigFiles() {
    if (simulator) {
        auto pathInfo = simulator->GetPathInfo();
        simulator->AddFile(simulator->JoinPath(pathInfo.userConfigDir, "phd2.cfg"), 
                          "[General]\nVersion=2.6.11\nDebug=false\n");
        simulator->AddFile(simulator->JoinPath(pathInfo.userConfigDir, "profiles.cfg"), 
                          "[Profiles]\nDefault=1\n");
    }
}

void MockFileOperationsManager::SetupTempDirectory() {
    if (simulator) {
        auto pathInfo = simulator->GetPathInfo();
        simulator->AddDirectory(pathInfo.tempDir);
    }
}

void MockFileOperationsManager::SimulateFileSystemError() {
    if (mockFileOps) {
        EXPECT_CALL(*mockFileOps, ReadFile(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(*mockFileOps, WriteFile(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(false));
    }
}

void MockFileOperationsManager::SimulatePermissionDenied() {
    if (simulator) {
        auto pathInfo = simulator->GetPathInfo();
        simulator->SetDirectoryError(pathInfo.userConfigDir, true);
        simulator->SetFileError(simulator->JoinPath(pathInfo.userConfigDir, "phd2.cfg"), true);
    }
}
