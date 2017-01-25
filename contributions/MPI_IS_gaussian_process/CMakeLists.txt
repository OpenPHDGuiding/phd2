# Copyright 2014-2015, Max Planck Society.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

# This project contains part of the Gaussian process library of the Max Planck Society,
# Institute for Intelligent Systems, Tuebingen, Germany.


project(MPI_IS_GaussianProcess)

set(gaussian_process_root_dir ${CMAKE_CURRENT_SOURCE_DIR})



# Gaussian Process toolbox library
set(gp_tools_SRC
    ${gaussian_process_root_dir}/tools/math_tools.cpp
    ${gaussian_process_root_dir}/tools/math_tools.h
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
    )
add_library(MPIIS_GP STATIC ${gp_SRC})
target_link_libraries(MPIIS_GP PUBLIC MPIIS_GP_TOOLS)
target_include_directories(MPIIS_GP PUBLIC ${EIGEN_SRC} ${gaussian_process_root_dir}/src
                                           ${gaussian_process_root_dir}/tools)
set_property(TARGET MPIIS_GP PROPERTY FOLDER "Contributions/")

# Gaussian Process Guider library
set(gpg_SRC
${gaussian_process_root_dir}/src/gaussian_process_guider.cpp
${gaussian_process_root_dir}/src/gaussian_process_guider.h
)
add_library(GPGuider STATIC ${gpg_SRC})
target_link_libraries(GPGuider PUBLIC MPIIS_GP_TOOLS MPIIS_GP)
target_include_directories(GPGuider PUBLIC ${EIGEN_SRC} ${gaussian_process_root_dir}/src
${gaussian_process_root_dir}/tools ${phd_src_dir})
set_property(TARGET GPGuider PROPERTY FOLDER "Contributions/")


################################################################
#
# Unit tests
#


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

# Sandbox
add_executable(Sandbox ${gaussian_process_root_dir}/tests/gaussian_process/sandbox.cpp)
target_link_libraries(Sandbox MPIIS_GP)
target_include_directories(Sandbox  PRIVATE ${gaussian_process_root_dir}/tools ${gaussian_process_root_dir}/src ${GTEST_HEADERS} ${EIGEN_SRC})
set_property(TARGET Sandbox PROPERTY FOLDER "Unit tests/Contribution")

# Test for the GP Guider
add_executable(GPGuiderTest ${gaussian_process_root_dir}/tests/gaussian_process/gp_guider_test.cpp)
target_link_libraries(GPGuiderTest MPIIS_GP gtest GPGuider)
target_include_directories(GPGuiderTest  PRIVATE ${gaussian_process_root_dir}/tools ${GTEST_HEADERS} ${phd_src_dir})
set_property(TARGET GPGuiderTest PROPERTY FOLDER "Unit tests/Contribution")
add_test(NAME GPTest-3 COMMAND GPGuiderTest)
