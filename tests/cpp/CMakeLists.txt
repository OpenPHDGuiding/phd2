# CMakeLists.txt for PHD2 C++ Tests

cmake_minimum_required(VERSION 3.10)
project(PHD2_CPP_Tests)

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base net)
include(${wxWidgets_USE_FILE})

# Mock library
add_library(phd2_test_mocks
    mocks/event_server_mocks.cpp
    mocks/mock_phd_components.cpp
)

target_link_libraries(phd2_test_mocks
    ${wxWidgets_LIBRARIES}
)

# Unit Tests
add_executable(unit_tests
    unit/calibration_api_tests.cpp
    unit/event_server_tests.cpp
    unit/event_server_basic_test.cpp
    unit/simple_event_server_test.cpp
    unit/standalone_event_server_test.cpp
)

target_link_libraries(unit_tests
    phd2_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# Integration Tests
add_executable(integration_tests
    integration/calibration_integration_tests.cpp
    integration/event_server_integration_tests.cpp
    integration/event_server_performance_tests.cpp
)

target_link_libraries(integration_tests
    phd2_test_mocks
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    pthread
)

# Add compiler flags
target_compile_definitions(unit_tests PRIVATE -DUNIT_TESTING=1)
target_compile_definitions(integration_tests PRIVATE -DUNIT_TESTING=1)

# Enable testing
enable_testing()

# Add tests
add_test(NAME UnitTests COMMAND unit_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)

# Custom targets to run tests
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -R UnitTests
    DEPENDS unit_tests
    COMMENT "Running unit tests"
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -R IntegrationTests
    DEPENDS integration_tests
    COMMENT "Running integration tests"
)

add_custom_target(run_all_cpp_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS unit_tests integration_tests
    COMMENT "Running all C++ tests"
)

# Test discovery for CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)
