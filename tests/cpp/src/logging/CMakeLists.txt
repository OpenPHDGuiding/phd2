# CMakeLists.txt for PHD2 Logging Module Tests
# Comprehensive test suite for all logging components

cmake_minimum_required(VERSION 3.10)
project(PHD2_Logging_Tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find Google Test and Google Mock
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base net)
include(${wxWidgets_USE_FILE})

# Find CURL for log uploader tests
find_package(CURL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/logging)
include_directories(${CMAKE_SOURCE_DIR}/src/utilities)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

# Compiler definitions for testing
add_definitions(-DUNIT_TESTING=1)
add_definitions(-DTESTING_LOGGING=1)
add_definitions(-DwxUSE_SOCKETS=1)

# Mock library for logging tests
add_library(logging_test_mocks STATIC
    mocks/mock_wx_components.cpp
    mocks/mock_file_system.cpp
    mocks/mock_network.cpp
    mocks/mock_phd_components.cpp
)

target_link_libraries(logging_test_mocks
    ${wxWidgets_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Individual test executables for each logging module

# DebugLog tests
add_executable(test_debuglog
    test_debuglog.cpp
)

target_link_libraries(test_debuglog
    logging_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# Logger tests
add_executable(test_logger
    test_logger.cpp
)

target_link_libraries(test_logger
    logging_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# GuidingLog tests
add_executable(test_guidinglog
    test_guidinglog.cpp
)

target_link_libraries(test_guidinglog
    logging_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# ImageLogger tests
add_executable(test_imagelogger
    test_imagelogger.cpp
)

target_link_libraries(test_imagelogger
    logging_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# LogUploader tests
add_executable(test_log_uploader
    test_log_uploader.cpp
)

target_link_libraries(test_log_uploader
    logging_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
)

# GuidingStats tests
add_executable(test_guiding_stats
    test_guiding_stats.cpp
)

target_link_libraries(test_guiding_stats
    logging_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# Combined test executable for all logging tests
add_executable(logging_tests_all
    test_debuglog.cpp
    test_logger.cpp
    test_guidinglog.cpp
    test_imagelogger.cpp
    test_log_uploader.cpp
    test_guiding_stats.cpp
)

target_link_libraries(logging_tests_all
    logging_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
)

# Add tests to CTest
enable_testing()

add_test(NAME DebugLogTests COMMAND test_debuglog)
add_test(NAME LoggerTests COMMAND test_logger)
add_test(NAME GuidingLogTests COMMAND test_guidinglog)
add_test(NAME ImageLoggerTests COMMAND test_imagelogger)
add_test(NAME LogUploaderTests COMMAND test_log_uploader)
add_test(NAME GuidingStatsTests COMMAND test_guiding_stats)
add_test(NAME AllLoggingTests COMMAND logging_tests_all)

# Set test properties
set_tests_properties(DebugLogTests PROPERTIES TIMEOUT 30)
set_tests_properties(LoggerTests PROPERTIES TIMEOUT 30)
set_tests_properties(GuidingLogTests PROPERTIES TIMEOUT 30)
set_tests_properties(ImageLoggerTests PROPERTIES TIMEOUT 30)
set_tests_properties(LogUploaderTests PROPERTIES TIMEOUT 60)  # Network tests may take longer
set_tests_properties(GuidingStatsTests PROPERTIES TIMEOUT 30)
set_tests_properties(AllLoggingTests PROPERTIES TIMEOUT 120)

# Custom target to run all logging tests
add_custom_target(run_logging_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_debuglog test_logger test_guidinglog test_imagelogger test_log_uploader test_guiding_stats
    COMMENT "Running all logging module tests"
)

# Install test executables (optional)
install(TARGETS 
    test_debuglog 
    test_logger 
    test_guidinglog 
    test_imagelogger 
    test_log_uploader 
    test_guiding_stats
    logging_tests_all
    DESTINATION bin/tests
)
