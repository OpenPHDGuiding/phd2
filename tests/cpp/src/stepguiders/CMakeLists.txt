# CMakeLists.txt for PHD2 Stepguider Module Tests
# Comprehensive test suite for all stepguider components

cmake_minimum_required(VERSION 3.10)
project(PHD2_Stepguider_Tests)

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
    # Windows libraries for serial port communication
    set(PLATFORM_LIBS ole32 oleaut32 uuid comctl32 rpcrt4 winmm wsock32 wininet setupapi)
elseif(UNIX AND NOT APPLE)
    # Linux libraries
    set(PLATFORM_LIBS pthread dl m udev)
elseif(APPLE)
    # macOS libraries
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(CARBON_LIBRARY Carbon)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${COCOA_LIBRARY} 
                      ${CARBON_LIBRARY} pthread dl m)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/stepguiders)
include_directories(${CMAKE_SOURCE_DIR}/src/mounts)
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
add_definitions(-DTESTING_STEPGUIDERS=1)
add_definitions(-DwxUSE_GUI=1)
add_definitions(-D__WXDEBUG__)

# Platform-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DNOMINMAX)
endif()

# Stepguider driver definitions
add_definitions(-DSTEPGUIDER_SXAO=1)
add_definitions(-DSTEPGUIDER_SIMULATOR=1)

if(INDI_FOUND)
    add_definitions(-DSTEPGUIDER_SXAO_INDI=1)
    add_definitions(-DSTEPGUIDER_SBIGAO_INDI=1)
endif()

# Mock library for stepguider tests
add_library(stepguider_test_mocks STATIC
    mocks/mock_stepguider_hardware.cpp
    mocks/mock_serial_port.cpp
    mocks/mock_indi_stepguider.cpp
    mocks/mock_stepguider_components.cpp
)

target_link_libraries(stepguider_test_mocks
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    ${GMOCK_LIBRARIES}
)

if(INDI_FOUND)
    target_link_libraries(stepguider_test_mocks ${INDI_LIBRARIES})
endif()

# Individual test executables for each stepguider module

# Stepguider base class tests
add_executable(test_stepguider
    test_stepguider.cpp
)

target_link_libraries(test_stepguider
    stepguider_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Stepguider factory tests
add_executable(test_stepguider_factory
    test_stepguider_factory.cpp
)

target_link_libraries(test_stepguider_factory
    stepguider_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# SX AO stepguider tests
add_executable(test_stepguider_sxao
    test_stepguider_sxao.cpp
)

target_link_libraries(test_stepguider_sxao
    stepguider_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# SX AO INDI stepguider tests (if INDI available)
if(INDI_FOUND)
    add_executable(test_stepguider_sxao_indi
        test_stepguider_sxao_indi.cpp
    )
    
    target_link_libraries(test_stepguider_sxao_indi
        stepguider_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
        ${INDI_LIBRARIES}
    )
endif()

# SBIG AO INDI stepguider tests (if INDI available)
if(INDI_FOUND)
    add_executable(test_stepguider_sbigao_indi
        test_stepguider_sbigao_indi.cpp
    )
    
    target_link_libraries(test_stepguider_sbigao_indi
        stepguider_test_mocks
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${PLATFORM_LIBS}
        ${INDI_LIBRARIES}
    )
endif()

# Stepguider calibration tests
add_executable(test_stepguider_calibration
    test_stepguider_calibration.cpp
)

target_link_libraries(test_stepguider_calibration
    stepguider_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Stepguider simulator tests
add_executable(test_stepguider_simulator
    test_stepguider_simulator.cpp
)

target_link_libraries(test_stepguider_simulator
    stepguider_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Combined test executable for all stepguider tests
set(ALL_TEST_SOURCES
    test_stepguider.cpp
    test_stepguider_factory.cpp
    test_stepguider_sxao.cpp
    test_stepguider_calibration.cpp
    test_stepguider_simulator.cpp
)

# Add INDI-specific tests to combined executable
if(INDI_FOUND)
    list(APPEND ALL_TEST_SOURCES test_stepguider_sxao_indi.cpp)
    list(APPEND ALL_TEST_SOURCES test_stepguider_sbigao_indi.cpp)
endif()

add_executable(stepguider_tests_all ${ALL_TEST_SOURCES})

target_link_libraries(stepguider_tests_all
    stepguider_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

if(INDI_FOUND)
    target_link_libraries(stepguider_tests_all ${INDI_LIBRARIES})
endif()

# Add tests to CTest
enable_testing()

add_test(NAME StepguiderTests COMMAND test_stepguider)
add_test(NAME StepguiderFactoryTests COMMAND test_stepguider_factory)
add_test(NAME SXAOStepguiderTests COMMAND test_stepguider_sxao)
add_test(NAME StepguiderCalibrationTests COMMAND test_stepguider_calibration)
add_test(NAME StepguiderSimulatorTests COMMAND test_stepguider_simulator)

# INDI-specific tests
if(INDI_FOUND)
    add_test(NAME SXAOINDIStepguiderTests COMMAND test_stepguider_sxao_indi)
    add_test(NAME SBIGAOINDIStepguiderTests COMMAND test_stepguider_sbigao_indi)
endif()

add_test(NAME AllStepguiderTests COMMAND stepguider_tests_all)

# Set test properties
set_tests_properties(StepguiderTests PROPERTIES TIMEOUT 60)
set_tests_properties(StepguiderFactoryTests PROPERTIES TIMEOUT 30)
set_tests_properties(SXAOStepguiderTests PROPERTIES TIMEOUT 60)
set_tests_properties(StepguiderCalibrationTests PROPERTIES TIMEOUT 90)
set_tests_properties(StepguiderSimulatorTests PROPERTIES TIMEOUT 30)

if(INDI_FOUND)
    set_tests_properties(SXAOINDIStepguiderTests PROPERTIES TIMEOUT 60)
    set_tests_properties(SBIGAOINDIStepguiderTests PROPERTIES TIMEOUT 60)
endif()

set_tests_properties(AllStepguiderTests PROPERTIES TIMEOUT 300)

# Custom target to run all stepguider tests
add_custom_target(run_stepguider_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_stepguider test_stepguider_factory test_stepguider_sxao
            test_stepguider_calibration test_stepguider_simulator
    COMMENT "Running all stepguider module tests"
)

# Install test executables (optional)
set(TEST_EXECUTABLES
    test_stepguider test_stepguider_factory test_stepguider_sxao
    test_stepguider_calibration test_stepguider_simulator stepguider_tests_all
)

# Add INDI-specific executables
if(INDI_FOUND)
    list(APPEND TEST_EXECUTABLES test_stepguider_sxao_indi test_stepguider_sbigao_indi)
endif()

install(TARGETS ${TEST_EXECUTABLES}
    DESTINATION bin/tests
)
