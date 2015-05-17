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

# File created by Raffi Enficiaud

set(thirdparty_dir ${CMAKE_SOURCE_DIR}/thirdparty)

# the location where the archives will be deflated
set(thirdparties_deflate_directory ${CMAKE_BINARY_DIR}/external_libs_deflate)
if(NOT EXISTS ${thirdparties_deflate_directory})
  file(MAKE_DIRECTORY ${thirdparties_deflate_directory})
endif()



# custom cmake packages, should have lower priority than the ones bundled with cmake
set(CMAKE_MODULE_PATH 
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake_modules/ )



# these variables allow to specify to which the main project will link and
# to potentially copy some resources to the output directory of the main project. 
# They are used by the CMakeLists.txt calling this file.

set(PHD_LINK_EXTERNAL)          # target to which the phd2 main library will link to
set(PHD_COPY_EXTERNAL_ALL)      # copy of a file for any configuration
set(PHD_COPY_EXTERNAL_DBG)      # copy for debug only
set(PHD_COPY_EXTERNAL_REL)      # copy for release only


# this module will be used to find system installed libraries on Linux
if(UNIX AND NOT APPLE)
  find_package(PkgConfig)
endif()


#
# copies the dependency files into the target output directory
#
macro(copy_dependency_with_config target_name dependency_list_all dependency_list_dbg dependency_list_release)

  
  set(dependency_list_dbg_with_all ${${dependency_list_dbg}} ${${dependency_list_all}})
  set(dependency_list_dbg_with_all_cleaned)
  foreach(_element ${dependency_list_dbg_with_all})
    if(NOT EXISTS ${_element})
      message(FATAL_ERROR "Dependency ${_element} does not exist")
    endif()
    #message(STATUS "copyX ${_element}")
    get_filename_component(_element1 ${_element} REALPATH)
    set(dependency_list_dbg_with_all_cleaned ${dependency_list_dbg_with_all_cleaned} ${_element1})
    unset(_element1)
  endforeach()
  
  list(REMOVE_DUPLICATES dependency_list_dbg_with_all_cleaned)
  set(dependency_list_dbg_with_all ${dependency_list_dbg_with_all_cleaned})
  unset(dependency_list_dbg_with_all_cleaned)
  unset(_element)
  
  foreach(_element ${dependency_list_dbg_with_all})
    get_filename_component(_element_name ${_element} NAME)
    add_custom_command(
      TARGET ${target_name}
      PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E 
        $<$<CONFIG:Debug>:echo>
        $<$<CONFIG:Debug>:"Copy ${_element_name} to $<TARGET_FILE_DIR:${target_name}>/.">
        $<$<NOT:$<CONFIG:Debug>>:echo_append>
        $<$<NOT:$<CONFIG:Debug>>:".">
      COMMAND ${CMAKE_COMMAND} -E 
        $<$<CONFIG:Debug>:copy_if_different> 
        $<$<CONFIG:Debug>:${_element}> 
        $<$<CONFIG:Debug>:$<TARGET_FILE_DIR:${target_name}>/.>
        $<$<NOT:$<CONFIG:Debug>>:echo_append>
        $<$<NOT:$<CONFIG:Debug>>:"">
      COMMENT "Copy ${target_name} dependencies into the output folder")



      #message(STATUS "copy ${_element_name}")
    # add_custom_command(
      # TARGET ${target_name}
      # PRE_BUILD
      # COMMAND ${CMAKE_COMMAND} -E echo Copy ${_element_name} into $<TARGET_FILE_DIR:${target_name}>/.
    
      # COMMAND ${CMAKE_COMMAND} -E $<$<CONFIG:Debug>:copy_if_different>$<$<NOT:$<CONFIG:Debug>>:echo> ${_element} $<TARGET_FILE_DIR:${target_name}>/.
      # COMMENT "Copy ${target_name} dependencies into the output folder")
    
    unset(_element_name)
  endforeach()
  unset(dependency_list_dbg_with_all)
  unset(_element)
  

  set(dependency_list_release_with_all ${${dependency_list_release}} ${${dependency_list_all}})
  set(dependency_list_release_with_all_cleaned)
  foreach(_element ${dependency_list_release_with_all})
    if(NOT EXISTS ${_element})
      message(FATAL_ERROR "Dependency ${_element} does not exist")
    endif()
    get_filename_component(_element1 ${_element} REALPATH)
    set(dependency_list_release_with_all_cleaned ${dependency_list_release_with_all_cleaned} ${_element1})
    unset(_element1)
  endforeach()
  list(REMOVE_DUPLICATES dependency_list_release_with_all_cleaned)
  set(dependency_list_release_with_all ${dependency_list_release_with_all_cleaned})
  unset(dependency_list_release_with_all_cleaned)
  unset(_element)
  
  foreach(_element ${dependency_list_release_with_all})
    get_filename_component(_element_name ${_element} NAME)
    add_custom_command(
      TARGET ${target_name}
      PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E 
        $<$<CONFIG:Release>:echo> #$<$<NOT:$<CONFIG:Release>>:echo> ${_element} $<TARGET_FILE_DIR:${target_name}>/.
        $<$<CONFIG:Release>:"Copy ${_element_name} to $<TARGET_FILE_DIR:${target_name}>/.">
        $<$<NOT:$<CONFIG:Release>>:echo_append>
        $<$<NOT:$<CONFIG:Release>>:".">
      COMMAND ${CMAKE_COMMAND} -E 
        $<$<CONFIG:Release>:copy_if_different> 
        $<$<CONFIG:Release>:${_element}> 
        $<$<CONFIG:Release>:$<TARGET_FILE_DIR:${target_name}>/.>
        $<$<NOT:$<CONFIG:Release>>:echo_append>
        $<$<NOT:$<CONFIG:Release>>:"">
      COMMENT "Copy ${target_name} dependencies into the output folder")
    unset(_element_name)      
  endforeach()
  unset(dependency_list_release_with_all)
  unset(_element)

  # we can also use this thing to install external stuff
  #install( FILES ${dependency_name_debug}
  #          DESTINATION bin
  #          CONFIGURATIONS Debug)
  #install( FILES ${dependency_name_non_debug}
  #          DESTINATION bin
  #          CONFIGURATIONS Release)
            

endmacro(copy_dependency_with_config)



#
# external rules common to all platforms
#
#############################################



##############################################
# cfitsio

set(libcfitsio_root ${thirdparties_deflate_directory}/cfitsio)

if(NOT EXISTS ${libcfitsio_root})
  # untar the dependency
  message(STATUS "Untarring cfitsio")
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${thirdparty_dir}/cfitsio3370_modified.tar.bz2
                    WORKING_DIRECTORY ${thirdparties_deflate_directory})
endif()

set(BUILD_SHARED_LIBS OFF)
set(USE_PTHREADS OFF)
add_subdirectory(${libcfitsio_root} tmp_cmakecfitsio)
set_property(TARGET cfitsio PROPERTY FOLDER "Thirdparty/")

include_directories(${libcfitsio_root})
set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} cfitsio)


if(WIN32)
  target_compile_definitions(
    cfitsio
    PRIVATE FF_NO_UNISTD_H
    PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()



#############################################
# libusb / win32 / apple

set(LIBUSB libusb-1.0.9)
set(libusb_root ${thirdparties_deflate_directory}/${LIBUSB})
set(USB_build TRUE) # indicates that the USB library is part of the project. Set to FALSE if already existing on the system

if(NOT EXISTS ${libusb_root})
  # untar the dependency
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xjf ${thirdparty_dir}/${LIBUSB}.tar.bz2
    WORKING_DIRECTORY ${thirdparties_deflate_directory})
endif()  

set(libusb_dir ${libusb_root}/libusb)
# core files
set(libUSB_SRC
  ${libusb_dir}/core.c
  ${libusb_dir}/descriptor.c
  ${libusb_dir}/io.c
  ${libusb_dir}/sync.c
  ${libusb_dir}/libusb.h
  ${libusb_dir}/libusbi.h

  )

# platform dependent files
if(APPLE)
  set(libUSB_SRC ${libUSB_SRC}  
  
    # platform specific configuration file
    ${thirdparty_dir}/include/${LIBUSB}
  
    # platform specific implementation
    ${libusb_dir}/os/darwin_usb.h
    ${libusb_dir}/os/darwin_usb.c
    
    ${libusb_dir}/os/threads_posix.h 
    ${libusb_dir}/os/threads_posix.c
   )
  set(${LIBUSB}_additional_compile_definition "OS_DARWIN=1") 
  set(${LIBUSB}_additional_include_dir ${thirdparty_dir}/include/${LIBUSB})
elseif(WIN32)
  set(libUSB_SRC ${libUSB_SRC}  
  
    # platform specific configuration files
    ${libusb_root}/msvc/stdint.h
    ${libusb_root}/msvc/inttypes.h
    ${libusb_root}/msvc/config.h
  
    # platform specific implementation
    ${libusb_dir}/os/windows_usb.h
    ${libusb_dir}/os/windows_usb.c
    
    ${libusb_dir}/os/threads_windows.h
    ${libusb_dir}/os/threads_windows.c
    
    ${libusb_dir}/os/poll_windows.h
    ${libusb_dir}/os/poll_windows.c
   )
  set(${LIBUSB}_additional_compile_definition "OS_WINDOWS=1")
  set(${LIBUSB}_additional_include_dir ${libusb_root}/msvc/)
elseif(UNIX)
  # libUSB is already an indirect requirement/dependency for phd2 (through libindi). 
  # I (Raffi) personally prefer having the same version
  # for all platforms, but it should in theory always be better to link against existing libraries
  # compiled ans shipped by skilled people.

  # this would find the libUSB module that is installed on the system. 
  # It requires "sudo apt-get install libusb-1.0-0-dev"
  pkg_check_modules(USB_pkg libusb-1.0)
  if(USB_pkg_FOUND)
    include_directories(${USB_pkg_INCLUDE_DIRS})
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${USB_pkg_LIBRARIES})
    set(USB_build FALSE)
    set(openphd_libusb ${USB_pkg_LIBRARIES})
  else()

    # in case the library is not installed on the system (as I have on my machines)
    # try by building the library ourselves
  
    set(libUSB_SRC ${libUSB_SRC}
    
     # platform specific configuration file
     ${thirdparty_dir}/include/${LIBUSB}
    
     # platform specific implementation
     ${libusb_dir}/os/linux_usbfs.c
     ${libusb_dir}/os/linux_usbfs.h
    
     ${libusb_dir}/os/threads_posix.h
     ${libusb_dir}/os/threads_posix.c
    )

    set(${LIBUSB}_additional_compile_definition "OS_LINUX=1")
    set(${LIBUSB}_additional_include_dir ${thirdparty_dir}/include/${LIBUSB})
  endif()

else()
  message(FATAL_ERROR "libUSB unsupported platform")
endif()

if(${USB_build})
  include_directories(${libusb_dir})

  # libUSB compilation if OSX or Win32 or not installed on Linux
  add_library(openphd_libusb ${libUSB_SRC})
  target_include_directories(openphd_libusb PRIVATE ${${LIBUSB}_additional_include_dir})
  target_compile_definitions(openphd_libusb PUBLIC ${${LIBUSB}_additional_compile_definition})

  if(NOT WIN32)
    target_compile_definitions(openphd_libusb PRIVATE LIBUSB_DESCRIBE "")
  else()
    # silencing the warnings on externals for win32
    target_compile_definitions(openphd_libusb PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} openphd_libusb)
  set_property(TARGET openphd_libusb PROPERTY FOLDER "Thirdparty/")
endif()



#############################################
# the Eigen library, mostly header only

set(EIGEN eigen-eigen-36bf2ceaf8f5)
set(eigen_root ${thirdparties_deflate_directory}/${EIGEN})
if(NOT EXISTS ${eigen_root})
  # untar the dependency
  execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xjf ${thirdparty_dir}/${EIGEN}.tar.bz2
    WORKING_DIRECTORY ${thirdparties_deflate_directory})
endif()

set(EIGEN_SRC ${eigen_root})





#############################################
# Google test, easily built

set(GTEST gtest-1.7.0)
set(gtest_root ${thirdparties_deflate_directory}/${GTEST})
if(NOT EXISTS ${gtest_root})
  # unzip the dependency
  execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xzf ${thirdparty_dir}/${GTEST}.zip
    WORKING_DIRECTORY ${thirdparties_deflate_directory})
endif()

if(MSVC)
  # do not replace default things, basically this line avoids gtest to replace
  # default compilation options (which ends up with messing the link options) for the CRT
  # As the name DOES NOT suggest, it does not force the shared crt. It forces the use of default things.
  set(gtest_force_shared_crt ON CACHE INTERNAL "Gtest crt configuration" FORCE)
endif()
set(GTEST_HEADERS ${gtest_root}/include)
add_subdirectory(${gtest_root} tmp_cmakegtest)
set_property(TARGET gtest PROPERTY FOLDER "Thirdparty/")
set_property(TARGET gtest_main PROPERTY FOLDER "Thirdparty/")






#############################################
# wxWidgets
# The usage is a bit different on all the platforms. For having version >= 3.0, a version of cmake >= 3.0 should be used on Windows (on Linux/OSX it works properly this way).
if(WIN32)
  # wxWidgets
  set(wxWidgets_CONFIGURATION msw)
  
  if(NOT wxWidgets_PREFIX_DIRECTORY OR NOT EXISTS ${wxWidgets_PREFIX_DIRECTORY})
    message(FATAL_ERROR "The variable wxWidgets_PREFIX_DIRECTORY should be defined and should point to a valid wxWindows installation path. See the open-phd-guiding wiki for more information.")
  endif()
  
  set(wxWidgets_ROOT_DIR ${wxWidgets_PREFIX_DIRECTORY})
  set(wxWidgets_USE_STATIC ON)
  set(wxWidgets_USE_DEBUG ON)
  set(wxWidgets_USE_UNICODE OFF)
  find_package(wxWidgets REQUIRED COMPONENTS propgrid base core aui adv html net)
  include(${wxWidgets_USE_FILE})
  #message(${wxWidgets_USE_FILE})

  
else()
  if(wxWidgets_PREFIX_DIRECTORY)
    set(wxWidgets_CONFIG_OPTIONS --prefix=${wxWidgets_PREFIX_DIRECTORY})
  
    find_program(wxWidgets_CONFIG_EXECUTABLE NAMES "wx-config" PATHS ${wxWidgets_PREFIX_DIRECTORY}/bin NO_DEFAULT_PATH)
    if(NOT wxWidgets_CONFIG_EXECUTABLE)
      message(FATAL_ERROR "Cannot find wxWidgets_CONFIG_EXECUTABLE from the given directory ${wxWidgets_PREFIX_DIRECTORY}")
    endif()  
  endif()
  
  find_package(wxWidgets REQUIRED COMPONENTS aui core base adv html net)
  if(NOT wxWidgets_FOUND)
    message(FATAL_ERROR "WxWidget cannot be found. Please use wx-config prefix")
  endif()
  #if(APPLE)
  #  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} wx_osx_cocoau_aui-3.0)
  #endif()
  #message("wxLibraries ${wxWidgets_LIBRARIES}")
endif()

set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${wxWidgets_LIBRARIES})







#############################################
#
# Windows specific dependencies
# - OpenCV
# - Video For Windows (vfw)
# - VidCap
# - ASCOM camera stuff
#############################################


# OPENCV specific
if(WIN32)

  # openCV
  # openCV should be installed somewhere defined on the command line. If this is not the case, an error message is printed and 
  # the build is aborted.
  if(NOT OpenCVRoot)
    message(FATAL_ERROR "OpenCVRoot is not defined. OpenCVRoot should be defined with the option -DOpenCVRoot=<root-to-opencv>")
  endif()
  
  set(opencv_root ${OpenCVRoot})
  
  if(NOT EXISTS ${opencv_root}/build/include)
    message(FATAL_ERROR "Cannot find the header directory of open cv. Please ensure you have decompressed the version for windows")
  endif()

  if(NOT EXISTS ${opencv_root}/build/OpenCVConfig.cmake)
    message(FATAL_ERROR "Cannot find the header directory of open cvOpenCVConfig.cmake. Please ensure you have decompressed the version for windows")
  endif()

  # apparently this is the way cmake works... did not know, the OpenCVConfig.cmake file is enough for the configuration
  set(OpenCV_DIR ${opencv_root}/build CACHE PATH "Location of the OpenCV configuration directory")
  set(OpenCV_SHARED ON)
  set(OpenCV_STATIC OFF)
  set(BUILD_SHARED_LIBS ON)
  find_package(OpenCV REQUIRED)

  if(NOT OpenCV_INCLUDE_DIRS)
    message(FATAL_ERROR "Cannot add the OpenCV include directories")
  endif()
  
  list(REMOVE_DUPLICATES OpenCV_LIB_DIR)
  list(LENGTH OpenCV_LIB_DIR list_lenght)
  if(${list_lenght} GREATER 1)
    list(GET OpenCV_LIB_DIR 0 OpenCV_LIB_DIR)
  endif()
  set(OpenCV_BIN_DIR ${OpenCV_LIB_DIR}/../bin)
  get_filename_component(OpenCV_BIN_DIR ${OpenCV_BIN_DIR} ABSOLUTE)
  
  include_directories(${OpenCV_INCLUDE_DIRS})
  set(OPENCV_VER "2410")
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${OpenCV_LIBS}) # Raffi: maybe reduce a bit the number of libraries to link against
  set(PHD_COPY_EXTERNAL_DBG ${PHD_COPY_EXTERNAL_DBG} ${OpenCV_BIN_DIR}/opencv_imgproc${OPENCV_VER}d.dll)
  set(PHD_COPY_EXTERNAL_REL ${PHD_COPY_EXTERNAL_REL} ${OpenCV_BIN_DIR}/opencv_imgproc${OPENCV_VER}.dll)
  
  set(PHD_COPY_EXTERNAL_DBG ${PHD_COPY_EXTERNAL_DBG} ${OpenCV_BIN_DIR}/opencv_highgui${OPENCV_VER}d.dll)
  set(PHD_COPY_EXTERNAL_REL ${PHD_COPY_EXTERNAL_REL} ${OpenCV_BIN_DIR}/opencv_highgui${OPENCV_VER}.dll)
  
  set(PHD_COPY_EXTERNAL_DBG ${PHD_COPY_EXTERNAL_DBG} ${OpenCV_BIN_DIR}/opencv_core${OPENCV_VER}d.dll)
  set(PHD_COPY_EXTERNAL_REL ${PHD_COPY_EXTERNAL_REL} ${OpenCV_BIN_DIR}/opencv_core${OPENCV_VER}.dll)
endif()
  

# Various camera libraries
if(WIN32)
  # VidCapture
  set(vidcap_dir ${PHD_PROJECT_ROOT_DIR}/cameras/VidCapture)
  include_directories(${vidcap_dir})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${vidcap_dir}/VidCapLib.lib) # better compile this one


  # Video for Windows, directshow and windows media
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL} vfw32.lib Strmiids.lib Quartz.lib winmm.lib)
  
  
  # gpusb
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/ShoestringGPUSB_DLL.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/ShoestringGPUSB_DLL.dll)
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/ShoestringLXUSB_DLL.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/ShoestringLXUSB_DLL.dll)
  
  # asi cameras
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/AsiCamera2.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/ASICamera2.dll)
  
  
  # DsiDevice
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/DsiDevice.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/DSCI.dll)
  
  # SBIGUDrv
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/SBIGUDrv.lib)
  #set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL} ${PHD_PROJECT_ROOT_DIR}/WinLibs/SBIGUDrv.dll) # this is delay load, the dll does not exist in the sources
  
  # DICAMSDK
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/DICAMSDK.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/DICAMSDK.dll)
  
  # SSAGIF
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/SSAGIF.lib)
  
  # FCLib
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/FCLib.lib)
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/FcApi.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/FCAPI.dll)
  
  # SXUSB
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/SXUSB.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SXUSB.dll)
  
  # astroDLL
  #set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${PHD_PROJECT_ROOT_DIR}/cameras/astroDLLQHY5V.lib)
  
  # CMOSDLL
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/CMOSDLL.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/CMOSDLL.dll)
  
  
  # inpout32 ?
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/inpout32.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/inpout32.dll)
  
 
  # some other that are explicitly loaded at runtime
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/qhy5IIdll.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/qhy5LIIdll.dll)
  
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/astroDLLsspiag.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SSPIAGCAM.dll)
  

  # ASCOM, 
  # disabled since not used in the SLN
  #find_package(ASCOM_INTERFACE REQUIRED)
  #include_directories(${ASCOM_INTERFACE_DIR})
  
  
endif()





#############################################
#
# OSX specific dependencies
#
#############################################
if(APPLE)
  find_library(quicktimeFramework QuickTime)
  find_library(iokitFramework     IOKit)
  find_library(carbonFramework    Carbon)
  find_library(cocoaFramework     Cocoa)
  find_library(systemFramework    System)
  find_library(webkitFramework    Webkit)
  find_library(audioToolboxFramework AudioToolbox)
  find_library(openGLFramework    OpenGL)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${QuickTime} ${IOKit} ${Carbon} ${Cocoa} ${System} ${Webkit} ${AudioToolbox} ${OpenGL})

  find_path(CARBON_INCLUDE_DIR Carbon.h)
  
  
  #############################################
  # Camera frameworks
  #
  find_library( sbigudFramework
                NAMES SBIGUDrv
                PATHS ${thirdparty_dir}/frameworks)
  if(NOT sbigudFramework)
    message(FATAL_ERROR "Cannot find the SBIGUDrv drivers")
  endif()
  include_directories(${sbigudFramework})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${sbigudFramework})

  ### 
  find_library( fcCamFramework
                NAMES fcCamFw
                PATHS ${thirdparty_dir}/frameworks)
  if(NOT fcCamFramework)
    message(FATAL_ERROR "Cannot find the fcCamFw drivers")
  endif()
  include_directories(${fcCamFramework})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${fcCamFramework})

  find_library( dsiMeadeLibrary
                NAMES DsiDevice
                PATHS ${PHD_PROJECT_ROOT_DIR}/cameras)
  if(NOT dsiMeadeLibrary)
    message(FATAL_ERROR "Cannot find the dsiMeadeLibrary drivers")
  endif()
  #include_directories(${fcCamFramework})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${dsiMeadeLibrary})

  find_library( sxMac
                NAMES SXMacLib
                PATHS ${PHD_PROJECT_ROOT_DIR}/cameras)
  if(NOT sxMac)
    message(FATAL_ERROR "Cannot find the sxMac drivers")
  endif()
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${sxMac})

  find_library( asiCamera2
                NAMES ASICamera2
                PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/zwolibs/x86)
  if(NOT asiCamera2)
    message(FATAL_ERROR "Cannot find the asiCamera2 drivers")
  endif()
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${asiCamera2})
  
  


  #############################################
  # libDC
  #
  
  set(LIBDC libdc1394-2.2.2)
  set(libdc_root ${thirdparties_deflate_directory}/${LIBDC})
  if(NOT EXISTS ${libdc_root})
    # untar the dependency
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xzf ${thirdparty_dir}/${LIBDC}.tar.gz
      WORKING_DIRECTORY ${thirdparties_deflate_directory})
  endif()  
  

  set(libdc_include_dir ${libdc_root})
  set(libdc_dir ${libdc_include_dir}/dc1394)
  include_directories(${libdc_dir})
  include_directories(${libdc_include_dir})
  
  set(libDC_SRC
    ${libdc_dir}/dc1394.h

    ${libdc_dir}/bayer.c
    ${libdc_dir}/camera.h

    ${libdc_dir}/capture.c
    ${libdc_dir}/capture.h

    ${libdc_dir}/control.c
    ${libdc_dir}/control.h
    
    ${libdc_dir}/conversions.c
    ${libdc_dir}/conversions.h
        
    ${libdc_dir}/enumeration.c
    
    ${libdc_dir}/format7.c
    ${libdc_dir}/format7.h
    
    ${libdc_dir}/internal.c
    ${libdc_dir}/internal.h
    
    ${libdc_dir}/iso.c
    ${libdc_dir}/iso.h
    
    ${libdc_dir}/log.c
    ${libdc_dir}/log.h
    
    ${libdc_dir}/offsets.h
    ${libdc_dir}/platform.h
    
    ${libdc_dir}/register.c
    ${libdc_dir}/register.h
    ${libdc_dir}/types.h
    
    ${libdc_dir}/utils.c
    ${libdc_dir}/utils.h
    
    ${libdc_dir}/video.h
    
    # USB backend
    ${libdc_dir}/usb/capture.c
    ${libdc_dir}/usb/control.c
    ${libdc_dir}/usb/usb.h

    # mac specific
    ${libdc_dir}/macosx.c
    ${libdc_dir}/macosx.h
    ${libdc_dir}/macosx/control.c
    ${libdc_dir}/macosx/capture.c
    ${libdc_dir}/macosx/macosx.h

  )
  
  add_library(dc ${libDC_SRC})
  target_include_directories(dc PRIVATE ${thirdparty_dir}/include/${LIBDC})
  set_property(TARGET dc PROPERTY FOLDER "Thirdparty/")
  # the following line generated too many warnings. The macros are already defined in the config.h of this library
  # target_compile_definitions(dc PRIVATE HAVE_LIBUSB HAVE_MACOSX) 
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} dc)




  #############################################
  # HID Utils
  #
  #if(NOT EXISTS "${thirdparty_dir}/HID Utilities Source")
  #  # untar the dependency
  #  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${thirdparty_dir}/HID Utilities Source.zip")
  #endif()  

  # library removed
  
  
  ### does not work on x64
  #find_library( openssag
  #              NAMES openssag
  #              PATHS ${PHD_PROJECT_ROOT_DIR}/cameras	)
  #if(NOT openssag)
  #  message(FATAL_ERROR "Cannot find the openssag drivers")
  #endif()
  #set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${openssag})

  set(LIBOPENSSAG openssag)
  set(libopenssag_dir ${thirdparty_dir}/${LIBOPENSSAG}/src)
  include_directories(${libopenssag_dir})
  set(libOPENSSAG_SRC
    ${libopenssag_dir}/firmware.h
    ${libopenssag_dir}/loader.cpp
    ${libopenssag_dir}/openssag_priv.h
    ${libopenssag_dir}/openssag.cpp
    ${libopenssag_dir}/openssag.h
    )
  add_library(OpenSSAG ${libOPENSSAG_SRC})
  target_link_libraries(OpenSSAG openphd_libusb)
  target_include_directories(OpenSSAG PRIVATE ${thirdparty_dir}/${LIBOPENSSAG}/src)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} OpenSSAG)
  set_property(TARGET OpenSSAG PROPERTY FOLDER "Thirdparty/")
    
endif()  # APPLE






#############################################
#
# Unix/Linux specific dependencies
# - ASI cameras
# - INDI
# - Nova (optional)
# - USB (commonly shared)
# - math (libm)
# - 
#
#############################################
if(UNIX AND NOT APPLE)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) 
    set(arch "x64") 
  else() 
    set(arch "x86") 
  endif()
  
  
  find_path(ZWO_INCLUDE_DIR ASICamera2.h
    PATHS
    ${PHD_PROJECT_ROOT_DIR}/cameras
  )  
  
  find_library(asiCamera2
                NAMES ASICamera2
                PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/zwolibs/${arch})
  if(NOT asiCamera2)
    message(FATAL_ERROR "Cannot find the asiCamera2 drivers")
  endif()
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${asiCamera2})


  # math library is needed, and should be one of the last things to link to here
  find_library(mathlib NAMES m)  
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${mathlib})

  # INDI
  # some features for indi >= 0.9 are used apparently
  find_package(INDI 0.9 REQUIRED)
  include_directories(${INDI_INCLUDE_DIR})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${INDI_CLIENT_LIBRARIES} ${INDI_LIBRARIES})
  if(PC_INDI_VERSION VERSION_LESS "1.1")
    add_definitions("-DINDI_PRE_1_1_0")
  endif()
  if(PC_INDI_VERSION VERSION_LESS "1.0")
    add_definitions("-DINDI_PRE_1_0_0")
  endif()
  
  # INDI depends on libz
  find_package(ZLIB REQUIRED)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${ZLIB_LIBRARIES})

  
  # Nova
  find_package(Nova)
  if(NOVA_FOUND)
      include_directories(${NOVA_INCLUDE_DIR})
      set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${NOVA_LIBRARIES})
      add_definitions("-DLIBNOVA" )
  else()
      message(WARNING "libnova not found! Considere to install libnova-dev ")
  endif() 


endif()
