
# This project contains part of the Gaussian process library of the Max Planck
# for Intelligent Systems, Tuebingen, Germany.
# This project is released under the terms of the BSD 3 clauses.

project(MPI_IS_GaussianProcess)

set(gaussian_process_root_dir ${CMAKE_CURRENT_SOURCE_DIR})



# Gaussian Process toolbox library
set(gp_tools_SRC
    ${gaussian_process_root_dir}/tools/math_tools.cpp
    ${gaussian_process_root_dir}/tools/math_tools.h
    ${gaussian_process_root_dir}/tools/circular_buffer.h
    ${gaussian_process_root_dir}/tools/circular_buffer.cpp
    ${gaussian_process_root_dir}/tools/bfgs_optimizer.cpp
    ${gaussian_process_root_dir}/tools/bfgs_optimizer.h
    ${gaussian_process_root_dir}/tools/objective_function.cpp
    ${gaussian_process_root_dir}/tools/objective_function.h    
   )
add_library(MPIIS_GP_TOOLS STATIC ${gp_tools_SRC})
target_include_directories(MPIIS_GP_TOOLS PRIVATE ${EIGEN_SRC} ${gaussian_process_root_dir}/src)
set_property(TARGET MPIIS_GP_TOOLS PROPERTY FOLDER "Contributions/")

# Gaussian Process main library
set(gp_SRC
    ${gaussian_process_root_dir}/src/gaussian_process.cpp
    ${gaussian_process_root_dir}/src/gaussian_process.h
    ${gaussian_process_root_dir}/src/covariance_functions.cpp
    ${gaussian_process_root_dir}/src/covariance_functions.h
    ${gaussian_process_root_dir}/src/parameter_priors.cpp
    ${gaussian_process_root_dir}/src/parameter_priors.h
    )
add_library(MPIIS_GP STATIC ${gp_SRC})
target_link_libraries(MPIIS_GP PUBLIC MPIIS_GP_TOOLS)
target_include_directories(MPIIS_GP PUBLIC ${EIGEN_SRC} ${gaussian_process_root_dir}/src 
                                           ${gaussian_process_root_dir}/tools)
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
target_include_directories(CircularBufferTest PRIVATE ${gaussian_process_root_dir}/tools ${GTEST_HEADERS})
set_property(TARGET CircularBufferTest PROPERTY FOLDER "Unit tests/Contribution")
add_test(CircularBufferTest1 CircularBufferTest)



# Test for the GP
add_executable(GaussianProcessTest ${gaussian_process_root_dir}/tests/gaussian_process/gaussian_process_test.cpp)
target_link_libraries(GaussianProcessTest MPIIS_GP gtest)
target_include_directories(GaussianProcessTest  PRIVATE ${gaussian_process_root_dir}/tools ${GTEST_HEADERS})
set_property(TARGET GaussianProcessTest PROPERTY FOLDER "Unit tests/Contribution")
add_test(NAME GPTest-1 COMMAND GaussianProcessTest)



# Test for the math tools
add_executable(MathToolboxTest ${gaussian_process_root_dir}/tests/gaussian_process/math_tools_test.cpp)
target_link_libraries(MathToolboxTest MPIIS_GP_TOOLS gtest)
target_include_directories(MathToolboxTest  PRIVATE ${gaussian_process_root_dir}/src ${GTEST_HEADERS} ${EIGEN_SRC})
set_property(TARGET MathToolboxTest PROPERTY FOLDER "Unit tests/Contribution")
add_test(NAME GPTest-2 COMMAND MathToolboxTest)




# Test for the BFGS implementation
add_executable(BFGSTest ${gaussian_process_root_dir}/tests/gaussian_process/bfgs_test.cpp)
target_link_libraries(BFGSTest MPIIS_GP gtest)
target_include_directories(BFGSTest PRIVATE ${gaussian_process_root_dir}/tools ${GTEST_HEADERS})
set_property(TARGET BFGSTest PROPERTY FOLDER "Unit tests/Contribution")
add_test(NAME BFGS-1 COMMAND BFGSTest)


