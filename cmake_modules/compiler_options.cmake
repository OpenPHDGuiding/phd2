# Copyright 2017, Max Planck Society.
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

# File created by Raffi Enficiaud


if(APPLE)
  if(APPLE32)
    # should be done *before* declaring project.
    set(CMAKE_OSX_ARCHITECTURES i386 CACHE STRING "build architecture for OSX" FORCE)
  endif()
  set(CMAKE_MACOSX_RPATH TRUE)
endif()

# this must appear very early in the file
if(WIN32)
  set(CMAKE_GENERATOR_TOOLSET "v120_xp" CACHE STRING "Platform Toolset" FORCE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# these options allow to use system libraries
option(USE_SYSTEM_CFITSIO "Enable this option here or in cmake call if you want to use system's cfitsio." OFF)
option(USE_SYSTEM_LIBUSB "Enable this option here or in cmake call if you want to use system's libUSB." OFF)
option(USE_SYSTEM_EIGEN3 "Enable this option here or in cmake call if you want to use system's Eigen3." OFF)
option(USE_SYSTEM_GTEST "Enable this option here or in cmake call if you want to use system's Gtest." OFF)
option(USE_SYSTEM_LIBINDI "Enable this option here or in cmake call if you want to use system's libindi." OFF)

# build type, by default to release (with optimisations)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# compiler capabilities
include(CheckCXXCompilerFlag)
if(WIN32)
  set(FIND_LIBRARY_USE_LIB64_PATHS FALSE)
  #set(CMAKE_LIBRARY_ARCHITECTURE x86)
else()
  # c++11 options
  check_cxx_compiler_flag(-std=c++11 HAS_CXX11_FLAG)
  check_cxx_compiler_flag(-std=c++0x HAS_CXX0X_FLAG)

  if(HAS_CXX11_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif(HAS_CXX0X_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  endif()

  if(APPLE)
    check_cxx_compiler_flag(-stdlib=libc++ HAS_LIBCXX11_FLAG)

    if(HAS_LIBCXX11_FLAG)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
  endif()
endif()
