# CMakeLists.txt for PHD2 Guiding Module Tests
# Comprehensive test suite for all guiding components

cmake_minimum_required(VERSION 3.10)
project(PHD2_Guiding_Tests)

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

# Find OpenCV (optional for image processing tests)
find_package(OpenCV QUIET)

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
    find_library(COCOA_LIBRARY Cocoa)
    find_library(CARBON_LIBRARY Carbon)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY} ${COCOA_LIBRARY} ${CARBON_LIBRARY} pthread dl m)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/guiding)
include_directories(${CMAKE_SOURCE_DIR}/src/guiding/algorithms)
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/cameras)
include_directories(${CMAKE_SOURCE_DIR}/src/mounts)
include_directories(${CMAKE_SOURCE_DIR}/src/utilities)
include_directories(${CMAKE_SOURCE_DIR}/src/ui)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

if(OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Compiler definitions for testing
add_definitions(-DUNIT_TESTING=1)
add_definitions(-DTESTING_GUIDING=1)
add_definitions(-DwxUSE_GUI=1)
add_definitions(-D__WXDEBUG__)

# Platform-specific definitions
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DNOMINMAX)
endif()

# Guiding algorithm definitions
add_definitions(-DGUIDE_ALGORITHM_LOWPASS=1)
add_definitions(-DGUIDE_ALGORITHM_LOWPASS2=1)
add_definitions(-DGUIDE_ALGORITHM_HYSTERESIS=1)
add_definitions(-DGUIDE_ALGORITHM_GAUSSIAN_PROCESS=1)
add_definitions(-DGUIDE_ALGORITHM_IDENTITY=1)
add_definitions(-DGUIDE_ALGORITHM_RESIST_SWITCH=1)
add_definitions(-DGUIDE_ALGORITHM_ZFILTER=1)

# OpenCV support
if(OpenCV_FOUND)
    add_definitions(-DHAVE_OPENCV=1)
endif()

# Mock library for guiding tests
add_library(guiding_test_mocks STATIC
    mocks/mock_guiding_hardware.cpp
    mocks/mock_star_detector.cpp
    mocks/mock_image_processor.cpp
    mocks/mock_mount_interface.cpp
    mocks/mock_guiding_components.cpp
)

target_link_libraries(guiding_test_mocks
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    ${GMOCK_LIBRARIES}
)

if(OpenCV_FOUND)
    target_link_libraries(guiding_test_mocks ${OpenCV_LIBS})
endif()

# Individual test executables for each guiding module

# Guider base class tests
add_executable(test_guider
    test_guider.cpp
)

target_link_libraries(test_guider
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Multi-star guider tests
add_executable(test_guider_multistar
    test_guider_multistar.cpp
)

target_link_libraries(test_guider_multistar
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Guide algorithm base class tests
add_executable(test_guide_algorithm
    test_guide_algorithm.cpp
)

target_link_libraries(test_guide_algorithm
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Guide algorithm implementation tests
add_executable(test_guide_algorithms
    test_guide_algorithms.cpp
)

target_link_libraries(test_guide_algorithms
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Backlash compensation tests
add_executable(test_backlash_comp
    test_backlash_comp.cpp
)

target_link_libraries(test_backlash_comp
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Calibration assistant tests
add_executable(test_calibration_assistant
    test_calibration_assistant.cpp
)

target_link_libraries(test_calibration_assistant
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Guiding assistant tests
add_executable(test_guiding_assistant
    test_guiding_assistant.cpp
)

target_link_libraries(test_guiding_assistant
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Z-filter factory tests
add_executable(test_zfilter_factory
    test_zfilter_factory.cpp
)

target_link_libraries(test_zfilter_factory
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

# Combined test executable for all guiding tests
set(ALL_TEST_SOURCES
    test_guider.cpp
    test_guider_multistar.cpp
    test_guide_algorithm.cpp
    test_guide_algorithms.cpp
    test_backlash_comp.cpp
    test_calibration_assistant.cpp
    test_guiding_assistant.cpp
    test_zfilter_factory.cpp
)

add_executable(guiding_tests_all ${ALL_TEST_SOURCES})

target_link_libraries(guiding_tests_all
    guiding_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
)

if(OpenCV_FOUND)
    target_link_libraries(test_guider ${OpenCV_LIBS})
    target_link_libraries(test_guider_multistar ${OpenCV_LIBS})
    target_link_libraries(test_guide_algorithm ${OpenCV_LIBS})
    target_link_libraries(test_guide_algorithms ${OpenCV_LIBS})
    target_link_libraries(test_backlash_comp ${OpenCV_LIBS})
    target_link_libraries(test_calibration_assistant ${OpenCV_LIBS})
    target_link_libraries(test_guiding_assistant ${OpenCV_LIBS})
    target_link_libraries(test_zfilter_factory ${OpenCV_LIBS})
    target_link_libraries(guiding_tests_all ${OpenCV_LIBS})
endif()

# Add tests to CTest
enable_testing()

add_test(NAME GuiderTests COMMAND test_guider)
add_test(NAME GuiderMultiStarTests COMMAND test_guider_multistar)
add_test(NAME GuideAlgorithmTests COMMAND test_guide_algorithm)
add_test(NAME GuideAlgorithmsTests COMMAND test_guide_algorithms)
add_test(NAME BacklashCompTests COMMAND test_backlash_comp)
add_test(NAME CalibrationAssistantTests COMMAND test_calibration_assistant)
add_test(NAME GuidingAssistantTests COMMAND test_guiding_assistant)
add_test(NAME ZFilterFactoryTests COMMAND test_zfilter_factory)
add_test(NAME AllGuidingTests COMMAND guiding_tests_all)

# Set test properties
set_tests_properties(GuiderTests PROPERTIES TIMEOUT 120)
set_tests_properties(GuiderMultiStarTests PROPERTIES TIMEOUT 120)
set_tests_properties(GuideAlgorithmTests PROPERTIES TIMEOUT 60)
set_tests_properties(GuideAlgorithmsTests PROPERTIES TIMEOUT 90)
set_tests_properties(BacklashCompTests PROPERTIES TIMEOUT 60)
set_tests_properties(CalibrationAssistantTests PROPERTIES TIMEOUT 90)
set_tests_properties(GuidingAssistantTests PROPERTIES TIMEOUT 90)
set_tests_properties(ZFilterFactoryTests PROPERTIES TIMEOUT 30)
set_tests_properties(AllGuidingTests PROPERTIES TIMEOUT 600)

# Custom target to run all guiding tests
add_custom_target(run_guiding_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_guider test_guider_multistar test_guide_algorithm test_guide_algorithms
            test_backlash_comp test_calibration_assistant test_guiding_assistant test_zfilter_factory
    COMMENT "Running all guiding module tests"
)

# Install test executables (optional)
set(TEST_EXECUTABLES
    test_guider test_guider_multistar test_guide_algorithm test_guide_algorithms
    test_backlash_comp test_calibration_assistant test_guiding_assistant 
    test_zfilter_factory guiding_tests_all
)

install(TARGETS ${TEST_EXECUTABLES}
    DESTINATION bin/tests
)
