# CMakeLists.txt for PHD2 Mount Module Tests
# Comprehensive test suite for all mount components

cmake_minimum_required(VERSION 3.10)
project(PHD2_Mount_Tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find Google Test and Google Mock
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base net adv)
include(${wxWidgets_USE_FILE})

# Find INDI (optional)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(INDI libindi)
endif()

# Find platform-specific libraries
if(WIN32)
    # Windows libraries for ASCOM and parallel port
    set(PLATFORM_LIBS ole32 oleaut32 uuid comctl32 rpcrt4 winmm wsock32 wininet)
elseif(UNIX AND NOT APPLE)
    # Linux libraries
    set(PLATFORM_LIBS pthread dl m)
elseif(APPLE)
    # macOS libraries
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(CARBON_LIBRARY Carbon)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${COCOA_LIBRARY} ${CARBON_LIBRARY} pthread dl m)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/mounts)
include_directories(${CMAKE_SOURCE_DIR}/src/mounts/drivers)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/utilities)
include_directories(${CMAKE_SOURCE_DIR}/src/communication)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

if(INDI_FOUND)
    include_directories(${INDI_INCLUDE_DIRS})
endif()

# Compiler definitions for testing
add_definitions(-DUNIT_TESTING=1)
add_definitions(-DTESTING_MOUNTS=1)
add_definitions(-DwxUSE_GUI=1)
add_definitions(-D__WXDEBUG__)

# Platform-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DNOMINMAX)
    add_definitions(-DHAS_ASCOM=1)
endif()

if(INDI_FOUND)
    add_definitions(-DHAS_INDI=1)
endif()

# Mock library for mount tests
add_library(mount_test_mocks STATIC
    mocks/mock_mount_hardware.cpp
    mocks/mock_ascom_interfaces.cpp
    mocks/mock_indi_client.cpp
    mocks/mock_parallel_port.cpp
    mocks/mock_st4_interfaces.cpp
    mocks/mock_mount_components.cpp
)

target_link_libraries(mount_test_mocks
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    ${GMOCK_LIBRARIES}
)

if(INDI_FOUND)
    target_link_libraries(mount_test_mocks ${INDI_LIBRARIES})
endif()

# Individual test executables for each mount module

# Mount base class tests
add_executable(test_mount
    test_mount.cpp
)

target_link_libraries(test_mount
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Scope class tests
add_executable(test_scope
    test_scope.cpp
)

target_link_libraries(test_scope
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# ASCOM driver tests (Windows only)
if(WIN32)
    add_executable(test_scope_ascom
        test_scope_ascom.cpp
    )
    
    target_link_libraries(test_scope_ascom
        mount_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# INDI driver tests (if INDI available)
if(INDI_FOUND)
    add_executable(test_scope_indi
        test_scope_indi.cpp
    )
    
    target_link_libraries(test_scope_indi
        mount_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
        ${INDI_LIBRARIES}
    )
endif()

# On-camera guiding tests
add_executable(test_scope_oncamera
    test_scope_oncamera.cpp
)

target_link_libraries(test_scope_oncamera
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Step guider tests
add_executable(test_scope_onstepguider
    test_scope_onstepguider.cpp
)

target_link_libraries(test_scope_onstepguider
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Onboard ST4 tests
add_executable(test_scope_onboard_st4
    test_scope_onboard_st4.cpp
)

target_link_libraries(test_scope_onboard_st4
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Manual pointing tests
add_executable(test_scope_manual_pointing
    test_scope_manual_pointing.cpp
)

target_link_libraries(test_scope_manual_pointing
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# GPUSB tests (Windows/Linux)
if(WIN32 OR UNIX)
    add_executable(test_scope_gpusb
        test_scope_gpusb.cpp
    )
    
    target_link_libraries(test_scope_gpusb
        mount_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# Parallel port tests (Windows/Linux)
if(WIN32 OR UNIX)
    add_executable(test_scope_gpint
        test_scope_gpint.cpp
    )
    
    target_link_libraries(test_scope_gpint
        mount_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# EQMac tests (macOS only)
if(APPLE)
    add_executable(test_scope_eqmac
        test_scope_eqmac.cpp
    )
    
    target_link_libraries(test_scope_eqmac
        mount_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# Equinox tests (macOS only)
if(APPLE)
    add_executable(test_scope_equinox
        test_scope_equinox.cpp
    )
    
    target_link_libraries(test_scope_equinox
        mount_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# GC USB ST4 tests
add_executable(test_scope_gc_usbst4
    test_scope_gc_usbst4.cpp
)

target_link_libraries(test_scope_gc_usbst4
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Voyager tests
add_executable(test_scope_voyager
    test_scope_voyager.cpp
)

target_link_libraries(test_scope_voyager
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Scope factory tests
add_executable(test_scope_factory
    test_scope_factory.cpp
)

target_link_libraries(test_scope_factory
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Combined test executable for all mount tests
set(ALL_TEST_SOURCES
    test_mount.cpp
    test_scope.cpp
    test_scope_oncamera.cpp
    test_scope_onstepguider.cpp
    test_scope_onboard_st4.cpp
    test_scope_manual_pointing.cpp
    test_scope_gc_usbst4.cpp
    test_scope_voyager.cpp
    test_scope_factory.cpp
)

# Add platform-specific tests to combined executable
if(WIN32)
    list(APPEND ALL_TEST_SOURCES test_scope_ascom.cpp)
endif()

if(INDI_FOUND)
    list(APPEND ALL_TEST_SOURCES test_scope_indi.cpp)
endif()

if(WIN32 OR UNIX)
    list(APPEND ALL_TEST_SOURCES test_scope_gpusb.cpp test_scope_gpint.cpp)
endif()

if(APPLE)
    list(APPEND ALL_TEST_SOURCES test_scope_eqmac.cpp test_scope_equinox.cpp)
endif()

add_executable(mount_tests_all ${ALL_TEST_SOURCES})

target_link_libraries(mount_tests_all
    mount_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

if(INDI_FOUND)
    target_link_libraries(mount_tests_all ${INDI_LIBRARIES})
endif()

# Add tests to CTest
enable_testing()

add_test(NAME MountTests COMMAND test_mount)
add_test(NAME ScopeTests COMMAND test_scope)
add_test(NAME OnCameraTests COMMAND test_scope_oncamera)
add_test(NAME OnStepGuiderTests COMMAND test_scope_onstepguider)
add_test(NAME OnboardST4Tests COMMAND test_scope_onboard_st4)
add_test(NAME ManualPointingTests COMMAND test_scope_manual_pointing)
add_test(NAME GCUsbST4Tests COMMAND test_scope_gc_usbst4)
add_test(NAME VoyagerTests COMMAND test_scope_voyager)
add_test(NAME ScopeFactoryTests COMMAND test_scope_factory)

# Platform-specific tests
if(WIN32)
    add_test(NAME ASCOMTests COMMAND test_scope_ascom)
endif()

if(INDI_FOUND)
    add_test(NAME INDITests COMMAND test_scope_indi)
endif()

if(WIN32 OR UNIX)
    add_test(NAME GPUSBTests COMMAND test_scope_gpusb)
    add_test(NAME GPINTTests COMMAND test_scope_gpint)
endif()

if(APPLE)
    add_test(NAME EQMacTests COMMAND test_scope_eqmac)
    add_test(NAME EquinoxTests COMMAND test_scope_equinox)
endif()

add_test(NAME AllMountTests COMMAND mount_tests_all)

# Set test properties
set_tests_properties(MountTests PROPERTIES TIMEOUT 60)
set_tests_properties(ScopeTests PROPERTIES TIMEOUT 60)
set_tests_properties(OnCameraTests PROPERTIES TIMEOUT 30)
set_tests_properties(OnStepGuiderTests PROPERTIES TIMEOUT 30)
set_tests_properties(OnboardST4Tests PROPERTIES TIMEOUT 30)
set_tests_properties(ManualPointingTests PROPERTIES TIMEOUT 30)
set_tests_properties(GCUsbST4Tests PROPERTIES TIMEOUT 30)
set_tests_properties(VoyagerTests PROPERTIES TIMEOUT 30)
set_tests_properties(ScopeFactoryTests PROPERTIES TIMEOUT 30)

if(WIN32)
    set_tests_properties(ASCOMTests PROPERTIES TIMEOUT 60)
endif()

if(INDI_FOUND)
    set_tests_properties(INDITests PROPERTIES TIMEOUT 60)
endif()

if(WIN32 OR UNIX)
    set_tests_properties(GPUSBTests PROPERTIES TIMEOUT 30)
    set_tests_properties(GPINTTests PROPERTIES TIMEOUT 30)
endif()

if(APPLE)
    set_tests_properties(EQMacTests PROPERTIES TIMEOUT 30)
    set_tests_properties(EquinoxTests PROPERTIES TIMEOUT 30)
endif()

set_tests_properties(AllMountTests PROPERTIES TIMEOUT 300)

# Custom target to run all mount tests
add_custom_target(run_mount_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_mount test_scope test_scope_oncamera test_scope_onstepguider
            test_scope_onboard_st4 test_scope_manual_pointing test_scope_gc_usbst4
            test_scope_voyager test_scope_factory
    COMMENT "Running all mount module tests"
)

# Install test executables (optional)
set(TEST_EXECUTABLES
    test_mount test_scope test_scope_oncamera test_scope_onstepguider
    test_scope_onboard_st4 test_scope_manual_pointing test_scope_gc_usbst4
    test_scope_voyager test_scope_factory mount_tests_all
)

# Add platform-specific executables
if(WIN32)
    list(APPEND TEST_EXECUTABLES test_scope_ascom)
endif()

if(INDI_FOUND)
    list(APPEND TEST_EXECUTABLES test_scope_indi)
endif()

if(WIN32 OR UNIX)
    list(APPEND TEST_EXECUTABLES test_scope_gpusb test_scope_gpint)
endif()

if(APPLE)
    list(APPEND TEST_EXECUTABLES test_scope_eqmac test_scope_equinox)
endif()

install(TARGETS ${TEST_EXECUTABLES}
    DESTINATION bin/tests
)
