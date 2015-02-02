
# This project contains part of the Gaussian process library of the Max Planck
# for Intelligent Systems, Tuebingen, Germany.
# This project is released under the terms of the BSD 3 clauses.

project(GaussianProcess)

# Gaussian Process
set(gp_SRC
    ${phd_src_dir}/gaussian_process/tools/math_tools.cpp
    ${phd_src_dir}/gaussian_process/tools/math_tools.h
    ${phd_src_dir}/gaussian_process/tools/circular_buffer.h
    ${phd_src_dir}/gaussian_process/tools/circular_buffer.cpp)
add_library(MPIIS_GP STATIC ${gp_SRC})
target_include_directories(MPIIS_GP PRIVATE ${EIGEN_SRC})
set_property(TARGET MPIIS_GP PROPERTY FOLDER "Contributions/")





################################################################
#
# Unit tests
#

# CircularBuffer: to remove
# this is a specific circular buffer for the MPIIS_GP project. This should be replaced by the already existing one 
# in the open-phd project.
add_executable(CircularBufferTest ${phd_src_dir}/tests/circular_buffer/circular_buffer_test.cpp)
target_link_libraries(CircularBufferTest MPIIS_GP gtest)
target_include_directories(CircularBufferTest PRIVATE ${phd_src_dir}
                                              PRIVATE ${GTEST_HEADERS})
set_property(TARGET CircularBufferTest PROPERTY FOLDER "Unit tests/")
add_test(CircularBufferTest1 CircularBufferTest)


