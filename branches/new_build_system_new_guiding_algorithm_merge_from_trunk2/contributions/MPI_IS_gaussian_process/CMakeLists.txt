
# This project contains part of the Gaussian process library of the Max Planck
# for Intelligent Systems, Tuebingen, Germany.
# This project is released under the terms of the BSD 3 clauses.

project(GaussianProcess)

set(gaussian_process_root_dir ${CMAKE_CURRENT_SOURCE_DIR})



# Gaussian Process
set(gp_SRC
    ${gaussian_process_root_dir}/tools/math_tools.cpp
    ${gaussian_process_root_dir}/tools/math_tools.h
    ${gaussian_process_root_dir}/tools/circular_buffer.h
    ${gaussian_process_root_dir}/tools/circular_buffer.cpp)
add_library(MPIIS_GP STATIC ${gp_SRC})
target_include_directories(MPIIS_GP PUBLIC ${EIGEN_SRC} 
                                           ${gaussian_process_root_dir})
set_property(TARGET MPIIS_GP PROPERTY FOLDER "Contributions/")





################################################################
#
# Unit tests
#

# CircularBuffer: to remove
# this is a specific circular buffer for the MPIIS_GP project. This should be replaced by the already existing one 
# in the open-phd project.
add_executable(CircularBufferTest ${CMAKE_CURRENT_SOURCE_DIR}/tests/circular_buffer/circular_buffer_test.cpp)
target_link_libraries(CircularBufferTest MPIIS_GP gtest)
target_include_directories(CircularBufferTest PRIVATE ${gaussian_process_root_dir}/tools
                                              PRIVATE ${GTEST_HEADERS})
set_property(TARGET CircularBufferTest PROPERTY FOLDER "Unit tests/Contribution")
add_test(CircularBufferTest1 CircularBufferTest)


