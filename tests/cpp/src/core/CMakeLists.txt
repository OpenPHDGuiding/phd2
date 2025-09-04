# CMakeLists.txt for PHD2 Core Module Tests
# Comprehensive test suite for all core components

cmake_minimum_required(VERSION 3.10)
project(PHD2_Core_Tests)

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

# Find CFITSIO for image processing tests
find_package(PkgConfig REQUIRED)
pkg_check_modules(CFITSIO cfitsio)

# Find platform-specific libraries
if(WIN32)
    # Windows libraries
    set(PLATFORM_LIBS ole32 oleaut32 uuid comctl32 rpcrt4 winmm wsock32 wininet)
elseif(UNIX AND NOT APPLE)
    # Linux libraries
    set(PLATFORM_LIBS pthread dl m)
elseif(APPLE)
    # macOS libraries
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COCOA_LIBRARY Cocoa)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${COCOA_LIBRARY} pthread dl m)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/utilities)
include_directories(${CMAKE_SOURCE_DIR}/src/communication)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

if(CFITSIO_FOUND)
    include_directories(${CFITSIO_INCLUDE_DIRS})
endif()

# Compiler definitions for testing
add_definitions(-DUNIT_TESTING=1)
add_definitions(-DTESTING_CORE=1)
add_definitions(-DwxUSE_GUI=1)
add_definitions(-D__WXDEBUG__)

# Platform-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DNOMINMAX)
endif()

# Mock library for core tests
add_library(core_test_mocks STATIC
    mocks/mock_wx_components.cpp
    mocks/mock_image_data.cpp
    mocks/mock_file_operations.cpp
    mocks/mock_threading.cpp
    mocks/mock_phd_core.cpp
)

target_link_libraries(core_test_mocks
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    ${GMOCK_LIBRARIES}
)

if(CFITSIO_FOUND)
    target_link_libraries(core_test_mocks ${CFITSIO_LIBRARIES})
endif()

# Individual test executables for each core module

# PHD Application tests
add_executable(test_phd
    test_phd.cpp
)

target_link_libraries(test_phd
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# PHD Configuration tests
add_executable(test_phdconfig
    test_phdconfig.cpp
)

target_link_libraries(test_phdconfig
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# PHD Control tests
add_executable(test_phdcontrol
    test_phdcontrol.cpp
)

target_link_libraries(test_phdcontrol
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# PHD Update tests
add_executable(test_phdupdate
    test_phdupdate.cpp
)

target_link_libraries(test_phdupdate
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# usImage tests
add_executable(test_usimage
    test_usimage.cpp
)

target_link_libraries(test_usimage
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

if(CFITSIO_FOUND)
    target_link_libraries(test_usimage ${CFITSIO_LIBRARIES})
endif()

# Image Math tests
add_executable(test_image_math
    test_image_math.cpp
)

target_link_libraries(test_image_math
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Star tests
add_executable(test_star
    test_star.cpp
)

target_link_libraries(test_star
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Star Profile tests
add_executable(test_star_profile
    test_star_profile.cpp
)

target_link_libraries(test_star_profile
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Target tests
add_executable(test_target
    test_target.cpp
)

target_link_libraries(test_target
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Worker Thread tests
add_executable(test_worker_thread
    test_worker_thread.cpp
)

target_link_libraries(test_worker_thread
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Test Guide tests
add_executable(test_testguide
    test_testguide.cpp
)

target_link_libraries(test_testguide
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Star Cross Test tests
add_executable(test_starcross_test
    test_starcross_test.cpp
)

target_link_libraries(test_starcross_test
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Config Dialog tests
add_executable(test_configdialog
    test_configdialog.cpp
)

target_link_libraries(test_configdialog
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Config INDI tests
add_executable(test_config_indi
    test_config_indi.cpp
)

target_link_libraries(test_config_indi
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# INDI GUI tests
add_executable(test_indi_gui
    test_indi_gui.cpp
)

target_link_libraries(test_indi_gui
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Combined test executable for all core tests
set(ALL_TEST_SOURCES
    test_phd.cpp
    test_phdconfig.cpp
    test_phdcontrol.cpp
    test_phdupdate.cpp
    test_usimage.cpp
    test_image_math.cpp
    test_star.cpp
    test_star_profile.cpp
    test_target.cpp
    test_worker_thread.cpp
    test_testguide.cpp
    test_starcross_test.cpp
    test_configdialog.cpp
    test_config_indi.cpp
    test_indi_gui.cpp
)

add_executable(core_tests_all ${ALL_TEST_SOURCES})

target_link_libraries(core_tests_all
    core_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

if(CFITSIO_FOUND)
    target_link_libraries(core_tests_all ${CFITSIO_LIBRARIES})
endif()

# Add tests to CTest
enable_testing()

add_test(NAME PHDTests COMMAND test_phd)
add_test(NAME PHDConfigTests COMMAND test_phdconfig)
add_test(NAME PHDControlTests COMMAND test_phdcontrol)
add_test(NAME PHDUpdateTests COMMAND test_phdupdate)
add_test(NAME usImageTests COMMAND test_usimage)
add_test(NAME ImageMathTests COMMAND test_image_math)
add_test(NAME StarTests COMMAND test_star)
add_test(NAME StarProfileTests COMMAND test_star_profile)
add_test(NAME TargetTests COMMAND test_target)
add_test(NAME WorkerThreadTests COMMAND test_worker_thread)
add_test(NAME TestGuideTests COMMAND test_testguide)
add_test(NAME StarCrossTestTests COMMAND test_starcross_test)
add_test(NAME ConfigDialogTests COMMAND test_configdialog)
add_test(NAME ConfigINDITests COMMAND test_config_indi)
add_test(NAME INDIGUITests COMMAND test_indi_gui)
add_test(NAME AllCoreTests COMMAND core_tests_all)

# Set test properties
set_tests_properties(PHDTests PROPERTIES TIMEOUT 60)
set_tests_properties(PHDConfigTests PROPERTIES TIMEOUT 30)
set_tests_properties(PHDControlTests PROPERTIES TIMEOUT 60)
set_tests_properties(PHDUpdateTests PROPERTIES TIMEOUT 30)
set_tests_properties(usImageTests PROPERTIES TIMEOUT 60)
set_tests_properties(ImageMathTests PROPERTIES TIMEOUT 60)
set_tests_properties(StarTests PROPERTIES TIMEOUT 30)
set_tests_properties(StarProfileTests PROPERTIES TIMEOUT 30)
set_tests_properties(TargetTests PROPERTIES TIMEOUT 30)
set_tests_properties(WorkerThreadTests PROPERTIES TIMEOUT 60)
set_tests_properties(TestGuideTests PROPERTIES TIMEOUT 30)
set_tests_properties(StarCrossTestTests PROPERTIES TIMEOUT 30)
set_tests_properties(ConfigDialogTests PROPERTIES TIMEOUT 30)
set_tests_properties(ConfigINDITests PROPERTIES TIMEOUT 30)
set_tests_properties(INDIGUITests PROPERTIES TIMEOUT 30)
set_tests_properties(AllCoreTests PROPERTIES TIMEOUT 300)

# Custom target to run all core tests
add_custom_target(run_core_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_phd test_phdconfig test_phdcontrol test_phdupdate
            test_usimage test_image_math test_star test_star_profile
            test_target test_worker_thread test_testguide test_starcross_test
            test_configdialog test_config_indi test_indi_gui
    COMMENT "Running all core module tests"
)

# Install test executables (optional)
set(TEST_EXECUTABLES
    test_phd test_phdconfig test_phdcontrol test_phdupdate
    test_usimage test_image_math test_star test_star_profile
    test_target test_worker_thread test_testguide test_starcross_test
    test_configdialog test_config_indi test_indi_gui core_tests_all
)

install(TARGETS ${TEST_EXECUTABLES}
    DESTINATION bin/tests
)
