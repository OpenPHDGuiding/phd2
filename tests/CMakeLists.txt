# CMakeLists.txt for PHD2 Calibration API Tests

cmake_minimum_required(VERSION 3.10)
project(PHD2_Calibration_Tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find Google Test
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/communication/network)
include_directories(${CMAKE_SOURCE_DIR}/src/utilities)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base net)
include(${wxWidgets_USE_FILE})

# Define test executable
add_executable(calibration_api_tests
    calibration_api_tests.cpp
    # Add mock implementations or stubs as needed
    mock_phd_components.cpp
)

# Link libraries
target_link_libraries(calibration_api_tests
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# Add compiler flags
target_compile_definitions(calibration_api_tests PRIVATE
    -DUNIT_TESTING=1
)

# Enable testing
enable_testing()

# Add test
add_test(NAME CalibrationAPITests COMMAND calibration_api_tests)

# Custom target to run tests
add_custom_target(run_calibration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS calibration_api_tests
    COMMENT "Running calibration API tests"
)

# Test discovery for CTest
include(GoogleTest)
gtest_discover_tests(calibration_api_tests)
