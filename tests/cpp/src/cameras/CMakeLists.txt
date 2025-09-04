# CMakeLists.txt for PHD2 Camera Module Tests
# Comprehensive test suite for all camera components

cmake_minimum_required(VERSION 3.10)
project(PHD2_Camera_Tests)

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

# Find OpenCV (optional)
find_package(OpenCV QUIET)

# Find INDI (optional)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(INDI libindi)
endif()

# Find platform-specific libraries
if(WIN32)
    # Windows libraries for ASCOM and DirectShow
    set(PLATFORM_LIBS ole32 oleaut32 uuid comctl32 rpcrt4 winmm wsock32 wininet strmiids)
elseif(UNIX AND NOT APPLE)
    # Linux libraries
    set(PLATFORM_LIBS pthread dl m udev)
elseif(APPLE)
    # macOS libraries
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(CARBON_LIBRARY Carbon)
    find_library(AVFOUNDATION_LIBRARY AVFoundation)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${COCOA_LIBRARY} 
                      ${CARBON_LIBRARY} ${AVFOUNDATION_LIBRARY} pthread dl m)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/cameras)
include_directories(${CMAKE_SOURCE_DIR}/src/cameras/drivers)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/utilities)
include_directories(${CMAKE_SOURCE_DIR}/src/communication)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

if(OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

if(INDI_FOUND)
    include_directories(${INDI_INCLUDE_DIRS})
endif()

# Compiler definitions for testing
add_definitions(-DUNIT_TESTING=1)
add_definitions(-DTESTING_CAMERAS=1)
add_definitions(-DwxUSE_GUI=1)
add_definitions(-D__WXDEBUG__)

# Platform-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DNOMINMAX)
    add_definitions(-DHAS_ASCOM=1)
    add_definitions(-DVFW_CAMERA=1)
    add_definitions(-DWDM_CAMERA=1)
endif()

if(OpenCV_FOUND)
    add_definitions(-DOPENCV_CAMERA=1)
endif()

if(INDI_FOUND)
    add_definitions(-DHAS_INDI=1)
    add_definitions(-DINDI_CAMERA=1)
endif()

# Mock library for camera tests
add_library(camera_test_mocks STATIC
    mocks/mock_camera_hardware.cpp
    mocks/mock_ascom_camera.cpp
    mocks/mock_indi_camera.cpp
    mocks/mock_usb_camera.cpp
    mocks/mock_webcam_interfaces.cpp
    mocks/mock_camera_components.cpp
)

target_link_libraries(camera_test_mocks
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    ${GMOCK_LIBRARIES}
)

if(OpenCV_FOUND)
    target_link_libraries(camera_test_mocks ${OpenCV_LIBS})
endif()

if(INDI_FOUND)
    target_link_libraries(camera_test_mocks ${INDI_LIBRARIES})
endif()

# Individual test executables for each camera module

# Camera base class tests
add_executable(test_camera
    test_camera.cpp
)

target_link_libraries(test_camera
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Camera factory tests
add_executable(test_camera_factory
    test_camera_factory.cpp
)

target_link_libraries(test_camera_factory
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# ASCOM camera tests (Windows only)
if(WIN32)
    add_executable(test_camera_ascom
        test_camera_ascom.cpp
    )
    
    target_link_libraries(test_camera_ascom
        camera_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
    )
endif()

# INDI camera tests (if INDI available)
if(INDI_FOUND)
    add_executable(test_camera_indi
        test_camera_indi.cpp
    )
    
    target_link_libraries(test_camera_indi
        camera_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
        ${INDI_LIBRARIES}
    )
endif()

# ZWO ASI camera tests
add_executable(test_camera_zwo
    test_camera_zwo.cpp
)

target_link_libraries(test_camera_zwo
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# QHY camera tests
add_executable(test_camera_qhy
    test_camera_qhy.cpp
)

target_link_libraries(test_camera_qhy
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# OpenCV camera tests (if OpenCV available)
if(OpenCV_FOUND)
    add_executable(test_camera_opencv
        test_camera_opencv.cpp
    )
    
    target_link_libraries(test_camera_opencv
        camera_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
        ${OpenCV_LIBS}
    )
endif()

# SBIG camera tests
add_executable(test_camera_sbig
    test_camera_sbig.cpp
)

target_link_libraries(test_camera_sbig
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Camera simulator tests
add_executable(test_camera_simulator
    test_camera_simulator.cpp
)

target_link_libraries(test_camera_simulator
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Webcam interface tests
add_executable(test_camera_webcam
    test_camera_webcam.cpp
)

target_link_libraries(test_camera_webcam
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# USB camera interface tests
add_executable(test_camera_usb
    test_camera_usb.cpp
)

target_link_libraries(test_camera_usb
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Camera calibration tests
add_executable(test_camera_calibration
    test_camera_calibration.cpp
)

target_link_libraries(test_camera_calibration
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Combined test executable for all camera tests
set(ALL_TEST_SOURCES
    test_camera.cpp
    test_camera_factory.cpp
    test_camera_zwo.cpp
    test_camera_qhy.cpp
    test_camera_sbig.cpp
    test_camera_simulator.cpp
    test_camera_webcam.cpp
    test_camera_usb.cpp
    test_camera_calibration.cpp
)

# Add platform-specific tests to combined executable
if(WIN32)
    list(APPEND ALL_TEST_SOURCES test_camera_ascom.cpp)
endif()

if(INDI_FOUND)
    list(APPEND ALL_TEST_SOURCES test_camera_indi.cpp)
endif()

if(OpenCV_FOUND)
    list(APPEND ALL_TEST_SOURCES test_camera_opencv.cpp)
endif()

add_executable(camera_tests_all ${ALL_TEST_SOURCES})

target_link_libraries(camera_tests_all
    camera_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

if(OpenCV_FOUND)
    target_link_libraries(camera_tests_all ${OpenCV_LIBS})
endif()

if(INDI_FOUND)
    target_link_libraries(camera_tests_all ${INDI_LIBRARIES})
endif()

# Add tests to CTest
enable_testing()

add_test(NAME CameraTests COMMAND test_camera)
add_test(NAME CameraFactoryTests COMMAND test_camera_factory)
add_test(NAME ZWOCameraTests COMMAND test_camera_zwo)
add_test(NAME QHYCameraTests COMMAND test_camera_qhy)
add_test(NAME SBIGCameraTests COMMAND test_camera_sbig)
add_test(NAME CameraSimulatorTests COMMAND test_camera_simulator)
add_test(NAME WebcamTests COMMAND test_camera_webcam)
add_test(NAME USBCameraTests COMMAND test_camera_usb)
add_test(NAME CameraCalibrationTests COMMAND test_camera_calibration)

# Platform-specific tests
if(WIN32)
    add_test(NAME ASCOMCameraTests COMMAND test_camera_ascom)
endif()

if(INDI_FOUND)
    add_test(NAME INDICameraTests COMMAND test_camera_indi)
endif()

if(OpenCV_FOUND)
    add_test(NAME OpenCVCameraTests COMMAND test_camera_opencv)
endif()

add_test(NAME AllCameraTests COMMAND camera_tests_all)

# Set test properties
set_tests_properties(CameraTests PROPERTIES TIMEOUT 60)
set_tests_properties(CameraFactoryTests PROPERTIES TIMEOUT 30)
set_tests_properties(ZWOCameraTests PROPERTIES TIMEOUT 60)
set_tests_properties(QHYCameraTests PROPERTIES TIMEOUT 60)
set_tests_properties(SBIGCameraTests PROPERTIES TIMEOUT 60)
set_tests_properties(CameraSimulatorTests PROPERTIES TIMEOUT 30)
set_tests_properties(WebcamTests PROPERTIES TIMEOUT 45)
set_tests_properties(USBCameraTests PROPERTIES TIMEOUT 60)
set_tests_properties(CameraCalibrationTests PROPERTIES TIMEOUT 30)

if(WIN32)
    set_tests_properties(ASCOMCameraTests PROPERTIES TIMEOUT 60)
endif()

if(INDI_FOUND)
    set_tests_properties(INDICameraTests PROPERTIES TIMEOUT 60)
endif()

if(OpenCV_FOUND)
    set_tests_properties(OpenCVCameraTests PROPERTIES TIMEOUT 45)
endif()

set_tests_properties(AllCameraTests PROPERTIES TIMEOUT 300)

# Custom target to run all camera tests
add_custom_target(run_camera_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_camera test_camera_factory test_camera_zwo test_camera_qhy
            test_camera_sbig test_camera_simulator test_camera_webcam
            test_camera_usb test_camera_calibration
    COMMENT "Running all camera module tests"
)

# Install test executables (optional)
set(TEST_EXECUTABLES
    test_camera test_camera_factory test_camera_zwo test_camera_qhy
    test_camera_sbig test_camera_simulator test_camera_webcam
    test_camera_usb test_camera_calibration camera_tests_all
)

# Add platform-specific executables
if(WIN32)
    list(APPEND TEST_EXECUTABLES test_camera_ascom)
endif()

if(INDI_FOUND)
    list(APPEND TEST_EXECUTABLES test_camera_indi)
endif()

if(OpenCV_FOUND)
    list(APPEND TEST_EXECUTABLES test_camera_opencv)
endif()

install(TARGETS ${TEST_EXECUTABLES}
    DESTINATION bin/tests
)
