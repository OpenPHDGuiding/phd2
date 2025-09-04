# CMakeLists.txt for PHD2 Communication Module Tests
# Comprehensive test suite for all communication components

cmake_minimum_required(VERSION 3.10)
project(PHD2_Communication_Tests)

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

# Find platform-specific libraries
if(WIN32)
    # Windows COM libraries
    set(PLATFORM_LIBS ole32 oleaut32 uuid)
elseif(UNIX AND NOT APPLE)
    # Linux libraries
    set(PLATFORM_LIBS pthread)
elseif(APPLE)
    # macOS libraries
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} pthread)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/communication)
include_directories(${CMAKE_SOURCE_DIR}/src/communication/serial)
include_directories(${CMAKE_SOURCE_DIR}/src/communication/network)
include_directories(${CMAKE_SOURCE_DIR}/src/communication/parallel)
include_directories(${CMAKE_SOURCE_DIR}/src/utilities)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

# Compiler definitions for testing
add_definitions(-DUNIT_TESTING=1)
add_definitions(-DTESTING_COMMUNICATION=1)
add_definitions(-DwxUSE_SOCKETS=1)

# Platform-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Mock library for communication tests
add_library(communication_test_mocks STATIC
    mocks/mock_system_calls.cpp
    mocks/mock_wx_sockets.cpp
    mocks/mock_com_interfaces.cpp
    mocks/mock_hardware.cpp
    mocks/mock_phd_components.cpp
)

target_link_libraries(communication_test_mocks
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    ${GMOCK_LIBRARIES}
)

# Individual test executables for each communication module

# SerialPort base class tests
add_executable(test_serialport
    test_serialport.cpp
)

target_link_libraries(test_serialport
    communication_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# SerialPort POSIX implementation tests
if(UNIX)
    add_executable(test_serialport_posix
        test_serialport_posix.cpp
    )

    target_link_libraries(test_serialport_posix
        communication_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# SerialPort Win32 implementation tests
if(WIN32)
    add_executable(test_serialport_win32
        test_serialport_win32.cpp
    )

    target_link_libraries(test_serialport_win32
        communication_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# SerialPort loopback implementation tests
add_executable(test_serialport_loopback
    test_serialport_loopback.cpp
)

target_link_libraries(test_serialport_loopback
    communication_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# EventServer tests
add_executable(test_event_server
    test_event_server.cpp
)

target_link_libraries(test_event_server
    communication_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# SocketServer tests
add_executable(test_socket_server
    test_socket_server.cpp
)

target_link_libraries(test_socket_server
    communication_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# ParallelPort tests
add_executable(test_parallelport
    test_parallelport.cpp
)

target_link_libraries(test_parallelport
    communication_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# COM Dispatch tests (Windows only)
if(WIN32)
    add_executable(test_comdispatch
        test_comdispatch.cpp
    )

    target_link_libraries(test_comdispatch
        communication_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# OnboardST4 tests
add_executable(test_onboard_st4
    test_onboard_st4.cpp
)

target_link_libraries(test_onboard_st4
    communication_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Combined test executable for all communication tests
set(ALL_TEST_SOURCES
    test_serialport.cpp
    test_serialport_loopback.cpp
    test_event_server.cpp
    test_socket_server.cpp
    test_parallelport.cpp
    test_onboard_st4.cpp
)

# Add platform-specific tests to combined executable
if(UNIX)
    list(APPEND ALL_TEST_SOURCES test_serialport_posix.cpp)
endif()

if(WIN32)
    list(APPEND ALL_TEST_SOURCES test_serialport_win32.cpp test_comdispatch.cpp)
endif()

add_executable(communication_tests_all ${ALL_TEST_SOURCES})

target_link_libraries(communication_tests_all
    communication_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Add tests to CTest
enable_testing()

add_test(NAME SerialPortTests COMMAND test_serialport)
add_test(NAME SerialPortLoopbackTests COMMAND test_serialport_loopback)
add_test(NAME EventServerTests COMMAND test_event_server)
add_test(NAME SocketServerTests COMMAND test_socket_server)
add_test(NAME ParallelPortTests COMMAND test_parallelport)
add_test(NAME OnboardST4Tests COMMAND test_onboard_st4)

if(UNIX)
    add_test(NAME SerialPortPosixTests COMMAND test_serialport_posix)
endif()

if(WIN32)
    add_test(NAME SerialPortWin32Tests COMMAND test_serialport_win32)
    add_test(NAME ComDispatchTests COMMAND test_comdispatch)
endif()

add_test(NAME AllCommunicationTests COMMAND communication_tests_all)

# Set test properties
set_tests_properties(SerialPortTests PROPERTIES TIMEOUT 30)
set_tests_properties(SerialPortLoopbackTests PROPERTIES TIMEOUT 30)
set_tests_properties(EventServerTests PROPERTIES TIMEOUT 60)  # Network tests may take longer
set_tests_properties(SocketServerTests PROPERTIES TIMEOUT 60)
set_tests_properties(ParallelPortTests PROPERTIES TIMEOUT 30)
set_tests_properties(OnboardST4Tests PROPERTIES TIMEOUT 30)

if(UNIX)
    set_tests_properties(SerialPortPosixTests PROPERTIES TIMEOUT 30)
endif()

if(WIN32)
    set_tests_properties(SerialPortWin32Tests PROPERTIES TIMEOUT 30)
    set_tests_properties(ComDispatchTests PROPERTIES TIMEOUT 30)
endif()

set_tests_properties(AllCommunicationTests PROPERTIES TIMEOUT 180)

# Custom target to run all communication tests
add_custom_target(run_communication_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_serialport test_serialport_loopback test_event_server 
            test_socket_server test_parallelport test_onboard_st4
    COMMENT "Running all communication module tests"
)

# Add platform-specific dependencies
if(UNIX)
    add_dependencies(run_communication_tests test_serialport_posix)
endif()

if(WIN32)
    add_dependencies(run_communication_tests test_serialport_win32 test_comdispatch)
endif()

# Install test executables (optional)
set(TEST_EXECUTABLES
    test_serialport
    test_serialport_loopback
    test_event_server
    test_socket_server
    test_parallelport
    test_onboard_st4
    communication_tests_all
)

if(UNIX)
    list(APPEND TEST_EXECUTABLES test_serialport_posix)
endif()

if(WIN32)
    list(APPEND TEST_EXECUTABLES test_serialport_win32 test_comdispatch)
endif()

install(TARGETS ${TEST_EXECUTABLES}
    DESTINATION bin/tests
)
