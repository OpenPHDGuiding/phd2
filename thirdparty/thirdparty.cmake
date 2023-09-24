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

if(APPLE)
  find_library(quicktimeFramework      QuickTime)
  find_library(iokitFramework          IOKit)
  find_library(carbonFramework         Carbon)
  find_library(cocoaFramework          Cocoa)
  find_library(systemFramework         System)
  find_library(webkitFramework         Webkit)
  find_library(audioToolboxFramework   AudioToolbox)
  find_library(openGLFramework         OpenGL)
  find_library(coreFoundationFramework CoreFoundation)
endif()

#
# external rules common to all platforms
#
#############################################


##############################################
# cfitsio

if(USE_SYSTEM_CFITSIO)
  find_package(CFITSIO REQUIRED)
  include_directories(${CFITSIO_INCLUDE_DIR})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${CFITSIO_LIBRARIES})
  message(STATUS "Using system's CFITSIO.")
else(USE_SYSTEM_CFITSIO)

  set(CFITSIO_MAJOR_VERSION 3)
  set(CFITSIO_MINOR_VERSION 47)
  set(CFITSIO_VERSION ${CFITSIO_MAJOR_VERSION}.${CFITSIO_MINOR_VERSION})

  set(libcfitsio_root ${thirdparties_deflate_directory}/cfitsio-${CFITSIO_VERSION})

  if(NOT EXISTS ${libcfitsio_root})
    # untar the dependency
    message(STATUS "[thirdparty] untarring cfitsio")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${thirdparty_dir}/cfitsio-${CFITSIO_VERSION}-patched.tar.gz
                    WORKING_DIRECTORY ${thirdparties_deflate_directory})
  endif()

  # copied and adapted from the CMakeLists.txt of cftsio project. The sources of the project
  # are left untouched

  file(GLOB CFTSIO_H_FILES "${libcfitsio_root}/*.h")

  # OpenPhdGuiding COMMENT HERE
  # Raffi: these should also be cleaned (link against zlib of the system)

  set(CFTSIO_SRC_FILES
    buffers.c cfileio.c checksum.c drvrfile.c drvrmem.c
    drvrnet.c drvrsmem.c drvrgsiftp.c editcol.c edithdu.c eval_l.c
    eval_y.c eval_f.c fitscore.c getcol.c getcolb.c getcold.c getcole.c
    getcoli.c getcolj.c getcolk.c getcoll.c getcols.c getcolsb.c
    getcoluk.c getcolui.c getcoluj.c getkey.c group.c grparser.c
    histo.c iraffits.c
    modkey.c putcol.c putcolb.c putcold.c putcole.c putcoli.c
    putcolj.c putcolk.c putcoluk.c putcoll.c putcols.c putcolsb.c
    putcolu.c putcolui.c putcoluj.c putkey.c region.c scalnull.c
    swapproc.c wcssub.c wcsutil.c imcompress.c quantize.c ricecomp.c
    pliocomp.c fits_hcompress.c fits_hdecompress.c zlib/zuncompress.c
    zlib/zcompress.c zlib/adler32.c zlib/crc32.c zlib/inffast.c
    zlib/inftrees.c zlib/trees.c zlib/zutil.c zlib/deflate.c
    zlib/infback.c zlib/inflate.c zlib/uncompr.c simplerng.c
    f77_wrap1.c f77_wrap2.c f77_wrap3.c f77_wrap4.c
  )

  foreach(_src_file IN LISTS CFTSIO_SRC_FILES)
    set(CFTSIO_SRC_FILES_rooted "${CFTSIO_SRC_FILES_rooted}" ${libcfitsio_root}/${_src_file})
  endforeach()

  add_library(cfitsio STATIC ${CFTSIO_H_FILES} ${CFTSIO_SRC_FILES_rooted})
  target_include_directories(cfitsio PUBLIC ${libcfitsio_root}/)

  # OpenPhdGuiding MODIFICATION HERE: we link against math library only on UNIX
  if(UNIX)
    target_link_libraries(cfitsio m)
  endif()

  # OpenPhdGuiding MODIFICATION HERE: suppress warning about unused function result
  if(UNIX AND NOT APPLE)
    set_target_properties(cfitsio PROPERTIES COMPILE_FLAGS "-Wno-unused-result")
    # Raffi: use target_compile_options ?
  endif()

  if(APPLE)
    set_target_properties(cfitsio PROPERTIES COMPILE_FLAGS "-DHAVE_UNISTD_H")
  endif()

  if(WIN32)
    target_compile_definitions(
      cfitsio
      PRIVATE BUILDING_CFITSIO
      PRIVATE FF_NO_UNISTD_H
      PRIVATE _CRT_SECURE_NO_WARNINGS
      PRIVATE _CRT_SECURE_NO_DEPRECATE)
  endif()

  set_target_properties(cfitsio PROPERTIES
                        VERSION ${CFITSIO_VERSION}
                        SOVERSION ${CFITSIO_MAJOR_VERSION}
                        FOLDER "Thirdparty/")


  # indicating the link and include directives to the main project.
  # already done by the directive target_include_directories(cfitsio PUBLIC
  # include_directories(${libcfitsio_root})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} cfitsio)

endif(USE_SYSTEM_CFITSIO)




##############################################
# VidCapture

if(WIN32)

  set(libvidcap_root ${thirdparty_dir}/VidCapture)

  # copied and adapted from the CMakeLists.txt of cftsio project. The
  # sources of the project are left untouched

  file(GLOB VIDCAP_H_FILES "${libvidcap_root}/Source/CVCommon/*.h" "${libvidcap_root}/Source/VidCapture/*.h")

  set(VIDCAP_SRC_FILES
      Source/VidCapture/CVImage.cpp
      Source/VidCapture/CVImageGrey.cpp
      Source/VidCapture/CVImageRGB24.cpp
      Source/VidCapture/CVImageRGBFloat.cpp
      Source/VidCapture/CVVidCapture.cpp
      Source/VidCapture/CVVidCaptureDSWin32.cpp
      Source/VidCapture/CVDShowUtil.cpp
      Source/VidCapture/CVFile.cpp
      Source/VidCapture/CVPlatformWin32.cpp
      Source/VidCapture/CVTraceWin32.cpp
  )

  foreach(_src_file IN LISTS VIDCAP_SRC_FILES)
    set(VIDCAP_SRC_FILES_rooted "${VIDCAP_SRC_FILES_rooted}" ${libvidcap_root}/${_src_file})
  endforeach()

  add_library(VidCapture STATIC ${VIDCAP_H_FILES} ${VIDCAP_SRC_FILES_rooted})
  target_include_directories(VidCapture PUBLIC ${libvidcap_root}/Source/CVCommon ${libvidcap_root}/Source/VidCapture)

  target_compile_definitions(
    VidCapture
    PRIVATE FF_NO_UNISTD_H
    PRIVATE _CRT_SECURE_NO_WARNINGS
    PRIVATE _CRT_SECURE_NO_DEPRECATE)

  set_target_properties(VidCapture PROPERTIES
                         FOLDER "Thirdparty/")

  # indicating the link and include directives to the main project.
  # already done by the directive target_include_directories(vidcap PUBLIC
  # include_directories(${libvidcap_root})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} VidCapture)

endif()




#############################################
# libusb: linux / apple

if(NOT WIN32)

set(LIBUSB libusb-1.0.21)
set(libusb_root ${thirdparties_deflate_directory}/${LIBUSB})
set(USB_build TRUE) # indicates that the USB library is part of the project. Set to FALSE if already existing on the system
set(LIBUSB_static TRUE)  # true for static lib, dynamic lib otherwise

if(NOT EXISTS ${libusb_root})
  # untar the dependency
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${thirdparty_dir}/${LIBUSB}.tar.bz2
    WORKING_DIRECTORY ${thirdparties_deflate_directory})
endif()

set(libusb_dir ${libusb_root}/libusb)
# core files
set(libUSB_SRC
  ${libusb_dir}/core.c
  ${libusb_dir}/descriptor.c
  ${libusb_dir}/hotplug.c
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

    ${libusb_dir}/os/poll_posix.c
  )
  set(${LIBUSB}_additional_compile_definition "OS_DARWIN=1")
  set(${LIBUSB}_additional_include_dir ${thirdparty_dir}/include/${LIBUSB})
  # need to build a dynamic libusb since the ZWO SDK requires libusb
  set(LIBUSB_static FALSE)
elseif(WIN32)
  set(libUSB_SRC ${libUSB_SRC}

    # platform specific configuration files
    ${libusb_root}/msvc/stdint.h
    ${libusb_root}/msvc/inttypes.h
    ${libusb_root}/msvc/config.h

    # platform specific implementation
    ${libusb_dir}/os/windows_winusb.h
    ${libusb_dir}/os/windows_winusb.c

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
  # compiled and shipped by skilled people.

  # this would find the libUSB module that is installed on the system.
  # It requires "sudo apt-get install libusb-1.0-0-dev"
  if(USE_SYSTEM_LIBUSB)
    pkg_check_modules(USB_pkg libusb-1.0)
    if(0)
      message(FATAL_ERROR "libUSB not detected")
    else()
      include_directories(${USB_pkg_INCLUDE_DIRS})
      set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${USB_pkg_LIBRARIES})
      set(USB_build FALSE)
      set(usb_openphd ${USB_pkg_LIBRARIES})
      message(STATUS "Using system's libUSB.")
    endif()
  else(USE_SYSTEM_LIBUSB)

    # in case the library is not installed on the system (as I have on my machines)
    # try by building the library ourselves

    set(libUSB_SRC ${libUSB_SRC}

     # platform specific configuration file
     ${thirdparty_dir}/include/${LIBUSB}

     # platform specific implementation
     ${libusb_dir}/os/linux_usbfs.c
     ${libusb_dir}/os/linux_usbfs.h
     ${libusb_dir}/os/linux_netlink.c

     ${libusb_dir}/os/threads_posix.h
     ${libusb_dir}/os/threads_posix.c

     ${libusb_dir}/os/poll_posix.c
    )

    set(${LIBUSB}_additional_compile_definition "OS_LINUX=1")
    set(${LIBUSB}_additional_include_dir ${thirdparty_dir}/include/${LIBUSB})
  endif(USE_SYSTEM_LIBUSB)

else()
  message(FATAL_ERROR "libUSB unsupported platform")
endif()

if(${USB_build})
  include_directories(${libusb_dir})

  # libUSB compilation if OSX or Win32 or not installed on Linux
  if(LIBUSB_static)
    add_library(usb_openphd ${libUSB_SRC})
  else()
    add_library(usb_openphd SHARED ${libUSB_SRC})
    # target_link_options requires newer cmake, so fall back to the old LINK_FLAGS target property
    #   target_link_options(usb_openphd -compatibility_version 2 -current_version 2)
    set_target_properties(usb_openphd PROPERTIES LINK_FLAGS "-compatibility_version 2 -current_version 2")
    if(APPLE32)
      set(libUSB_link_objc "objc")
    endif()
    target_link_libraries(usb_openphd
      ${coreFoundationFramework}
      ${iokitFramework}
      ${libUSB_link_objc}
    )
  endif()
  target_include_directories(usb_openphd PRIVATE ${${LIBUSB}_additional_include_dir})
  target_compile_definitions(usb_openphd PUBLIC ${${LIBUSB}_additional_compile_definition})
  set_property(TARGET usb_openphd PROPERTY FOLDER "Thirdparty/")

  if(WIN32)
    # silence the warnings on externals for win32
    target_compile_definitions(usb_openphd PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_definitions(usb_openphd PRIVATE LIBUSB_DESCRIBE "")
  endif()
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} usb_openphd)
endif()

endif() # NOT WIN32

#############################################
# libcurl
#############################################

if(WIN32)
  set(libcurl_root ${thirdparties_deflate_directory}/libcurl)
  set(libcurl_dir ${libcurl_root}/libcurl-7.54.0-win32)
  if(NOT EXISTS ${libcurl_root})
    # unzip the dependency
    file(MAKE_DIRECTORY ${libcurl_root})
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_SOURCE_DIR}/thirdparty/libcurl-7.54.0-win32.zip --format=zip
        WORKING_DIRECTORY ${libcurl_root})
  endif()
  include_directories(${libcurl_dir}/include)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${libcurl_dir}/lib/LIBCURL.LIB)
else()
  if(APPLE)
    # make sure to pick up the macos curl, not the mapcports curl in /opt/local/lib
    find_library(CURL_LIBRARIES
                 NAMES curl
		 PATHS /usr/lib
		 NO_DEFAULT_PATH)
    if(NOT CURL_LIBRARIES)
      message(FATAL_ERROR "libcurl not found")
    endif()
    set(CURL_FOUND True)
    set(CURL_INCLUDE_DIRS /usr/include)
  else()
    find_package(CURL REQUIRED)
  endif()
  message(STATUS "using libcurl ${CURL_LIBRARIES}")
  include_directories(${CURL_INCLUDE_DIRS})
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${CURL_LIBRARIES})
endif()

#############################################
# the Eigen library, mostly header only

if(USE_SYSTEM_EIGEN3)
  find_package(Eigen3 REQUIRED)
  set(EIGEN_SRC ${EIGEN3_INCLUDE_DIR})
  message(STATUS "Using system's Eigen3.")
else(USE_SYSTEM_EIGEN3)
  set(EIGEN eigen-eigen-67e894c6cd8f)
  set(eigen_root ${thirdparties_deflate_directory}/${EIGEN})
  if(NOT EXISTS ${eigen_root})
    # untar the dependency
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xjf ${thirdparty_dir}/${EIGEN}.tar.bz2
      WORKING_DIRECTORY ${thirdparties_deflate_directory})
  endif()

  set(EIGEN_SRC ${eigen_root})
endif(USE_SYSTEM_EIGEN3)


#############################################
# Google test, easily built

if(USE_SYSTEM_GTEST)
  find_package(GTest REQUIRED)
  set(GTEST_HEADERS ${GTEST_INCLUDE_DIRS})
  message(STATUS "Using system's Gtest.")
else(USE_SYSTEM_GTEST)
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
endif(USE_SYSTEM_GTEST)


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

elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  if(NOT DEFINED wxWidgets_PREFIX_DIRECTORY)
    set(wxWidgets_PREFIX_DIRECTORY "/usr/local")
  endif()
  set(wxWidgets_CONFIG_OPTIONS --prefix=${wxWidgets_PREFIX_DIRECTORY})

  find_program(wxWidgets_CONFIG_EXECUTABLE
    NAMES "wxgtk3u-3.1-config"
    PATHS ${wxWidgets_PREFIX_DIRECTORY}/bin NO_DEFAULT_PATH)
  if(NOT wxWidgets_CONFIG_EXECUTABLE)
    message(FATAL_ERROR "Cannot find wxWidgets_CONFIG_EXECUTABLE from the given directory ${wxWidgets_PREFIX_DIRECTORY}")
  endif()

  set(wxRequiredLibs aui core base adv html net)
  execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --libs ${wxRequiredLibs}
	  OUTPUT_VARIABLE wxWidgets_LIBRARIES
	  OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(${wxWidgets_LIBRARIES})
  execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --cflags ${wxRwxRequiredLibs}
	  OUTPUT_VARIABLE wxWidgets_CXXFLAGS
	  OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(wxWidgets_CXX_FLAGS UNIX_COMMAND "${wxWidgets_CXXFLAGS}")
  separate_arguments(wxWidgets_LDFLAGS UNIX_COMMAND "${wxWidgets_LDFLAGS}")
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
#  INDI
#
#############################################

if(WIN32)
  set(indi_zip ${thirdparty_dir}/indiclient-2.0.3.tar.gz)
  set(indiclient_root ${thirdparties_deflate_directory})
  set(libindi_root "${indiclient_root}/indiclient-2.0.3")
  if(NOT EXISTS ${indiclient_dir})
    message(STATUS "[thirdparty] untarring indiclient")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${indi_zip}
                    WORKING_DIRECTORY ${indiclient_root})
  endif()
  include_directories(${libindi_root})
  set(PHD_LINK_EXTERNAL_RELEASE ${PHD_LINK_EXTERNAL_RELEASE} ${indiclient_dir}/lib/indiclient.lib)  
else()
  # Linux or OSX
  if(USE_SYSTEM_LIBINDI)
    message(STATUS "Using system's libindi")
    # INDI
    find_package(INDI 2.0.0 REQUIRED)
    # source files include <libindi/baseclient.h> so we need the libindi parent directory in the include directories
    get_filename_component(INDI_INCLUDE_PARENT_DIR ${INDI_INCLUDE_DIR} DIRECTORY)
    include_directories(${INDI_INCLUDE_PARENT_DIR})
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${INDI_CLIENT_LIBRARIES})

    find_package(ZLIB REQUIRED)
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${ZLIB_LIBRARIES})

    find_package(Nova REQUIRED)
    add_definitions("-DLIBNOVA")
    include_directories(${NOVA_INCLUDE_DIR})
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${NOVA_LIBRARIES})
  else()
    if(APPLE)
      # make sure to pick up the macos libz, not the mapcports libz in /opt/local/lib
      find_library(ZLIB_LIBRARIES
                   NAMES z
		   PATHS /usr/lib
		   NO_DEFAULT_PATH)
      if(NOT ZLIB_LIBRARIES)
        message(FATAL_ERROR "libz not found")
      endif()
    else()
      find_package(ZLIB REQUIRED)
    endif()
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${ZLIB_LIBRARIES})    

    set(indi_zip ${thirdparty_dir}/indiclient-2.0.3.tar.gz)
    message(STATUS "Using project provided libindi '${indi_zip}'")
    set(libindi_root "${thirdparties_deflate_directory}/indiclient-2.0.3")

    if(NOT EXISTS ${libindi_root})
      message(STATUS "[thirdparty] extracting libindi sources")
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${indi_zip}
        WORKING_DIRECTORY ${thirdparties_deflate_directory})
    endif()

    if(NOT APPLE)
      # todo: OSX build fails when Nova is found. I think it needs the include dir
      # punting for now to get the build un-broken

      # Nova is required for sidereal time computation with Indi driver that not report it.
      # This is not critical if it is not present but the hour angle computation will be wrong.
      find_package(Nova)
      if(NOVA_FOUND)
        add_definitions("-DLIBNOVA")
        set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${NOVA_LIBRARIES})
      endif()
    endif()


    ########################################  Paths  ###################################################

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    check_symbol_exists(mremap sys/mman.h HAVE_MREMAP)
    check_symbol_exists(timespec_get time.h HAVE_TIMESPEC_GET)
    check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)

    set(CMAKE_INDI_VERSION_MAJOR 2)
    set(CMAKE_INDI_VERSION_MINOR 0)
    set(CMAKE_INDI_VERSION_RELEASE 3)

    set(INDI_SOVERSION ${CMAKE_INDI_VERSION_MAJOR})
    set(CMAKE_INDI_VERSION_STRING "${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE}")
    set(INDI_VERSION ${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE})

    set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
##    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
##    set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")

    set(LIBINDI_DIRECTORY "${libindi_root}/")

    configure_file(${LIBINDI_DIRECTORY}/config.h.cmake ${LIBINDI_DIRECTORY}/config.h @ONLY)
    configure_file(${LIBINDI_DIRECTORY}/indiapi.h.in ${LIBINDI_DIRECTORY}/indiapi.h @ONLY)

    file(GLOB CLIENT_SOURCES "${LIBINDI_DIRECTORY}/*.c*")
    add_library(indiclient STATIC ${CLIENT_SOURCES})

    target_include_directories(indiclient
          PUBLIC
            ${libindi_root}
            ${ZLIB_INCLUDE_DIR}
            ${CFITSIO_INCLUDE_DIR}
            ${NOVA_INCLUDE_DIR}
        )

    target_compile_definitions(indiclient
            PUBLIC
              -DWITH_ENCLEN
              $<$<BOOL:${NOVA_FOUND}>:HAVE_LIBNOVA>
              $<$<BOOL:${HAVE_TIMESPEC_GET}>:HAVE_TIMESPEC_GET>
              $<$<BOOL:${HAVE_CLOCK_GETTIME}>:HAVE_CLOCK_GETTIME>
          )

    target_link_libraries(indiclient
          PUBLIC
            ${CMAKE_THREAD_LIBS_INIT}
            ${ZLIB_LIBRARIES}
            ${NOVA_LIBRARIES}
            )

    set_property(TARGET indiclient PROPERTY FOLDER "Thirdparty/")

    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} indiclient)
  endif()

endif()



#############################################
#
# Windows specific dependencies
# - Visual Leak Detector (optional)
# - OpenCV
# - Video For Windows (vfw)
# - ASCOM camera stuff
#############################################

if(WIN32)

  if(NOT DISABLE_VLD)
    find_path(VLD_INCLUDE vld.h
        HINTS "C:/Program Files (x86)/Visual Leak Detector" ENV VLD_DIR
        PATH_SUFFIXES include
    )
    if (VLD_INCLUDE)
      get_filename_component(VLD_ROOT ${VLD_INCLUDE} DIRECTORY)
      add_definitions(-DHAVE_VLD=1)
      message(STATUS "Enabling VLD (${VLD_ROOT})")
    else()
      message(STATUS "Disabling VLD: VLD not found")
    endif()
  else()
    message(STATUS "Disabling VLD: DISABLE_VLD is set")
  endif()

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
  # Video for Windows, directshow and windows media
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL} vfw32.lib Strmiids.lib Quartz.lib winmm.lib)

  # gpusb
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/ShoestringGPUSB_DLL.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/ShoestringGPUSB_DLL.dll)
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/ShoestringLXUSB_DLL.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/ShoestringLXUSB_DLL.dll)

  # ASI cameras
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/ASICamera2.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/ASICamera2.dll)

  # ToupTek cameras
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/toupcam.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/toupcam.dll)

  # QHY cameras
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/qhyccd.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/qhyccd.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/tbb.dll)

  # altair cameras
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/AltairCam.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/AltairCam_legacy.dll)

  # DsiDevice
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/DsiDevice.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/DSCI.dll)

  # SBIGUDrv
  add_definitions(-DHAVE_SBIG_CAMERA=1)
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/SBIGUDrv.lib)
  #set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL} ${PHD_PROJECT_ROOT_DIR}/WinLibs/SBIGUDrv.dll) # this is delay load, the dll does not exist in the sources

  # DICAMSDK
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/DICAMSDK.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/DICAMSDK.dll)

  # SSAGIF
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/SSAGIF.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SSAGIFv2.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SSAGIFv4.dll)

  # FCLib
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/FCLib.lib)
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/FcApi.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/FCAPI.dll)

  # SXUSB
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/SXUSB.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SXUSB.dll)

  # astroDLL
  #set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${PHD_PROJECT_ROOT_DIR}/cameras/astroDLLQHY5V.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/astroDLLGeneric.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/astroDLLQHY5V.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/astroDLLsspiag.dll)

  # CMOSDLL
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/CMOSDLL.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/CMOSDLL.dll)

  # inpout32 ?
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/inpout32.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/inpout32.dll)

  # some other that are explicitly loaded at runtime
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SSPIAGCAM.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SSPIAGUSB_WIN.dll)

  # SVB cameras
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/SVBCameraSDK.lib)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/WinLibs/SVBCameraSDK.dll)

  # Moravian gX-driver cameras
#  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/moravian/win/lib/gXeth.lib)
  set(PHD_LINK_EXTERNAL     ${PHD_LINK_EXTERNAL}      ${PHD_PROJECT_ROOT_DIR}/cameras/moravian/win/lib/gXusb.lib)
#  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/cameras/moravian/win/lib/gXeth.dll)
  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${PHD_PROJECT_ROOT_DIR}/cameras/moravian/win/lib/gXusb.dll)
  include_directories(${PHD_PROJECT_ROOT_DIR}/cameras/moravian/include)

  set(PHD_COPY_EXTERNAL_ALL ${PHD_COPY_EXTERNAL_ALL}  ${libcurl_dir}/lib/LIBCURL.DLL)

  # ASCOM
  # disabled since not used in the SLN
  #find_package(ASCOM_INTERFACE REQUIRED)
  #include_directories(${ASCOM_INTERFACE_DIR})

endif()


#############################################
# SBIG specific dependencies if installed part of system
#############################################
if(SBIG_SYSTEM AND UNIX)

  # Assumes SBIG's Universal driver has been loaded into the system and placed
  # in a standard path. (e.g. sbigudrv.h in /usr/include, libsbigudrv.so in /usr/lib )
  #
  # SDK for linux can be found here -> ftp://ftp.diffractionlimited.com/pub/devsw/LinuxDevKit.tar.gz
  #
  # To rebuild libSBIG
  # cd ${WORKDIR}/LinuxDevKit/x86/c/testapp
  #  local sharedlink="-shared -Wl,-soname,libSBIG-1.33.0"
  #  g++ -c ${CXXFLAGS} -c -fPIC -I /usr/include/libusb-1.0 -I ${WORKDIR}/LinuxDevKit/x86/c/testapp csbigimg.cpp csbigcam.cpp
  #  g++ -L ${WORKDIR}/LinuxDevKit/x86/c/lib64/ ${sharedlink} -o libSBIG=1.33.0 csbigimg.o csbigcam.o -lm -lsbigudrv -lusb -lcfitsio
  #  ar -cvq libSBIG.a csbigimg.o csbigcam.o

  add_definitions(-DHAVE_SBIG_CAMERA=1)
  add_definitions("-DTARGET=7")
  message(STATUS "Finding SBIG Univeral Drivers on system")
  find_path(SBIG_INCLUDE_DIR sbigudrv.h)
  find_library(SBIG_LIBRARIES NAMES SBIG)
  find_library(SBIGUDRV_LIBRARIES NAMES sbigudrv)
  include_directories(${SBIG_INCLUDE_DIR})

  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} SBIG sbigudrv)

endif()

#############################################
#
# OSX specific dependencies
#
#############################################
if(APPLE)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${QuickTime} ${IOKit} ${Carbon} ${Cocoa} ${System} ${Webkit} ${AudioToolbox} ${OpenGL})

  find_path(CARBON_INCLUDE_DIR Carbon.h)


  #############################################
  # Camera frameworks
  #
  find_library( sbigudFramework
                NAMES SBIGUDrv
                PATHS ${thirdparty_dir}/frameworks)
  add_definitions(-DHAVE_SBIG_CAMERA=1)
  if(NOT sbigudFramework)
    message(FATAL_ERROR "Cannot find the SBIGUDrv drivers")
  endif()
  include_directories(${sbigudFramework})
  add_definitions(-DHAVE_SBIG_CAMERA=1)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${sbigudFramework})
  set(phd2_OSX_FRAMEWORKS ${phd2_OSX_FRAMEWORKS} ${sbigudFramework})

  if(APPLE32)
    find_library( fcCamFramework
                  NAMES fcCamFw
                  PATHS ${thirdparty_dir}/frameworks)
    if(NOT fcCamFramework)
      message(FATAL_ERROR "Cannot find the fcCamFw drivers")
    endif()
    include_directories(${fcCamFramework})
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${fcCamFramework})
    add_definitions(-DHAVE_STARFISH_CAMERA=1)
    set(phd2_OSX_FRAMEWORKS ${phd2_OSX_FRAMEWORKS} ${fcCamFramework})
  endif()

  if(APPLE32)
    find_library( dsiMeadeLibrary
                  NAMES DsiDevice
                  PATHS ${PHD_PROJECT_ROOT_DIR}/cameras)
    if(NOT dsiMeadeLibrary)
      message(FATAL_ERROR "Cannot find the dsiMeadeLibrary drivers")
    endif()
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${dsiMeadeLibrary})
    add_definitions(-DHAVE_MEADE_DSI_CAMERA=1)
  endif()

  find_library( asiCamera2
                NAMES ASICamera2
                PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/zwolibs/mac)
  if(NOT asiCamera2)
    message(FATAL_ERROR "Cannot find the asiCamera2 drivers")
  endif()
  add_definitions(-DHAVE_ZWO_CAMERA=1)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${asiCamera2})
  set(phd2_OSX_FRAMEWORKS ${phd2_OSX_FRAMEWORKS} ${asiCamera2})

  if(APPLE32)
    find_library( SVBCameraSDK
                  NAMES SVBCameraSDK
                  PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/svblibs/mac/x86)
    # SVB SDK v1.10 crashes when the dylib loads on APPLE32 -- disable for now
    unset(SVBCameraSDK CACHE)
  else()
    find_library( SVBCameraSDK
                  NAMES SVBCameraSDK
                  PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/svblibs/mac/x64)
    if(NOT SVBCameraSDK)
      message(FATAL_ERROR "Cannot find the Svbony SDK libs")
    endif()
  endif()

  if(SVBCameraSDK)
    add_definitions(-DHAVE_SVB_CAMERA=1)
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${SVBCameraSDK})
    set(phd2_OSX_FRAMEWORKS ${phd2_OSX_FRAMEWORKS} ${SVBCameraSDK})
  endif()

  if(APPLE32)
    find_library( qhylib
                  NAMES qhyccd
                  PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/qhyccdlibs/mac/x86_32)
  else()
    find_library( qhylib
                  NAMES qhyccd
                  PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/qhyccdlibs/mac/x86_64)
  endif()
  if(NOT qhylib)
    message(FATAL_ERROR "Cannot find the qhy SDK libs")
  endif()
  add_definitions(-DHAVE_QHY_CAMERA=1)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${qhylib})
  set(phd2_OSX_FRAMEWORKS ${phd2_OSX_FRAMEWORKS} ${qhylib})

  if(APPLE32)
    find_library( mallincamFramework
                  NAMES MallincamGuider
                  PATHS ${thirdparty_dir}/frameworks)
    if(NOT mallincamFramework)
      message(FATAL_ERROR "Cannot find the Mallincam framework")
    endif()
    include_directories(${mallincamFramework})
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${mallincamFramework})
    add_definitions(-DHAVE_SKYRAIDER_CAMERA=1)
    set(phd2_OSX_FRAMEWORKS ${phd2_OSX_FRAMEWORKS} ${mallincamFramework})
  endif()

  if(NOT APPLE32)
    find_library( toupcam
                  NAMES toupcam
                  PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/toupcam/mac)
    if(NOT toupcam)
      message(FATAL_ERROR "Cannot find the toupcam drivers")
    endif()
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${toupcam})
    add_definitions(-DHAVE_TOUPTEK_CAMERA=1)
    set(phd2_OSX_FRAMEWORKS ${phd2_OSX_FRAMEWORKS} ${toupcam})
  endif()

#  libDC does not seem to be used by anything, disabling it (2019/11/20 - remove in next release)
if(0)
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
endif()




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
  #              PATHS ${PHD_PROJECT_ROOT_DIR}/cameras )
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
  target_include_directories(OpenSSAG PRIVATE ${thirdparty_dir}/${LIBOPENSSAG}/src)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} OpenSSAG)
  add_definitions(-DHAVE_OPENSSAG_CAMERA=1)
  set_property(TARGET OpenSSAG PROPERTY FOLDER "Thirdparty/")

endif()  # APPLE






#############################################
#
# Unix/Linux specific dependencies
# - ASI cameras
# - USB (commonly shared)
# - math (libm)
# -
#
#############################################
if(UNIX AND NOT APPLE)

  if (NOT OPENSOURCE_ONLY)

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^armv6(.*)")
      set(zwoarch "armv6")
      set(qhyarch "armv6")
      set(toupcam_arch "armel")
      set(svbony_arch "armv6")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^armv7(.*)|arm64|aarch64|^armv8(.*)")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(zwoarch "armv8")
        set(qhyarch "armv8")
        set(toupcam_arch "arm64")
        set(svbony_arch "armv8")
      else()
        set(zwoarch "armv7")
        set(qhyarch "armv7")
        set(toupcam_arch "armhf")
        set(svbony_arch "armv7")
      endif()
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86|X86|amd64|AMD64|i.86")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(zwoarch "x64")
        set(qhyarch "x86_64")
        set(toupcam_arch "x64")
        set(svbony_arch "x64")
      else()
        set(zwoarch "x86")
        set(qhyarch "x86_32")
        set(toupcam_arch "x86")
        set(svbony_arch "x86")
      endif()
    else()
      message(FATAL_ERROR "unknown system architecture")
    endif()

    find_path(ZWO_INCLUDE_DIR ASICamera2.h
      NO_DEFAULT_PATHS
      PATHS ${PHD_PROJECT_ROOT_DIR}/cameras
    )

    # The binary libraries below do not support FreeBSD, ignore them
    # when building for FreeBSD.
    if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
      find_library(asiCamera2
             NAMES ASICamera2
             NO_DEFAULT_PATHS
             PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/zwolibs/${zwoarch})

      if(NOT asiCamera2)
        message(FATAL_ERROR "Cannot find the asiCamera2 drivers")
      endif()
      message(STATUS "Found ASICamera2 lib ${asiCamera2}")
      add_definitions(-DHAVE_ZWO_CAMERA=1)
      set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${asiCamera2})

      find_library(toupcam
             NAMES toupcam
             NO_DEFAULT_PATHS
             NO_CMAKE_SYSTEM_PATH
             PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/toupcam/linux/${toupcam_arch})
      if(NOT toupcam)
        message(FATAL_ERROR "Cannot find the toupcam drivers")
      endif()
      message(STATUS "Found toupcam lib ${toupcam}")
      add_definitions(-DHAVE_TOUPTEK_CAMERA=1)
      set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${toupcam})
      set(PHD_INSTALL_LIBS ${PHD_INSTALL_LIBS} ${toupcam})

      find_library(SVBCameraSDK
            NAMES SVBCameraSDK
            NO_DEFAULT_PATHS
            PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/svblibs/linux/${svbony_arch})

      if(NOT SVBCameraSDK)
        message(FATAL_ERROR "Cannot find the SVBCameraSDK drivers")
      endif()
      message(STATUS "Found SVBCameraSDK lib ${SVBCameraSDK}")
      add_definitions(-DHAVE_SVB_CAMERA=1)
      set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${SVBCameraSDK})
      set(PHD_INSTALL_LIBS ${PHD_INSTALL_LIBS} ${SVBCameraSDK})

      if(IS_DIRECTORY ${PHD_PROJECT_ROOT_DIR}/cameras/qhyccdlibs/linux/${qhyarch})
        add_definitions(-DHAVE_QHY_CAMERA=1)

        # be careful not to pick up any other qhy lib on the system
        find_library(qhylib
               NAMES qhyccd
               NO_DEFAULT_PATH
               PATHS ${PHD_PROJECT_ROOT_DIR}/cameras/qhyccdlibs/linux/${qhyarch})
        if(NOT qhylib)
          message(FATAL_ERROR "Cannot find the qhy SDK libs")
        endif()
          set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${qhylib})
      endif()
    endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

    find_program(LSB_RELEASE_EXEC lsb_release)
    if(LSB_RELEASE_EXEC)
      execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
        OUTPUT_VARIABLE LSB_RELEASE_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        )
      execute_process(COMMAND ${LSB_RELEASE_EXEC} -rs
        OUTPUT_VARIABLE LSB_RELEASE_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        )
      # temporarily disable qhy camera pending fix for link error on Ubuntu Trusty
      if((${LSB_RELEASE_ID} STREQUAL "Ubuntu") AND (${LSB_RELEASE_RELEASE} STREQUAL "14.04"))
        message(STATUS "Disabling QHY camera support on this platform")
        remove_definitions(-DHAVE_QHY_CAMERA=1)
      endif()
    endif()

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
    target_include_directories(OpenSSAG PRIVATE ${thirdparty_dir}/${LIBOPENSSAG}/src)
    add_definitions(-DHAVE_OPENSSAG_CAMERA=1)
    set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} OpenSSAG)
    set_property(TARGET OpenSSAG PROPERTY FOLDER "Thirdparty/")

  endif()  # OPENSOURCE_ONLY

  # math library is needed, and should be one of the last things to link to here
  find_library(mathlib NAMES m)
  set(PHD_LINK_EXTERNAL ${PHD_LINK_EXTERNAL} ${mathlib})

endif()

#############################################
# Starlight Xpress
#############################################

if(WIN32)
  add_definitions(-DHAVE_SXV_CAMERA=1)
elseif(UNIX OR APPLE)
  add_definitions(-DHAVE_SXV_CAMERA=1)
  set(SXV_PLATFORM_SRC
      ${phd_src_dir}/cameras/SXMacLib.h
      ${phd_src_dir}/cameras/SXMacLib.c)
endif()


#############################################
# KwiqGuider
#############################################

if(APPLE)
  add_definitions(-DHAVE_KWIQGUIDER_CAMERA=1)
  include_directories(${phd_src_dir}/cam_KWIQGuider/)
  set(KWIQGuider_PLATFORM_SRC
    ${phd_src_dir}/cam_KWIQGuider/KWIQGuider.cpp
    ${phd_src_dir}/cam_KWIQGuider/KWIQGuider.h
    ${phd_src_dir}/cam_KWIQGuider/KWIQGuider_firmware.h
    ${phd_src_dir}/cam_KWIQGuider/KWIQGuider_loader.cpp
    ${phd_src_dir}/cam_KWIQGuider/KWIQGuider_priv.h
  )
endif()


#############################################
#
# gettext and msgmerge tools for documentation/internationalization
#
#############################################

# zip file support integrated in cmake 3.2+
if(WIN32 AND ("${CMAKE_VERSION}" VERSION_GREATER "3.2")
         AND ("${GETTEXT_ROOT}" STREQUAL ""))

  # GETTEXT_ROOT not given from the command line: deflating our own

  set(GETTEXTTOOLS gettext-0.14.4)
  set(GETTEXT_ROOT ${thirdparties_deflate_directory}/${GETTEXTTOOLS})

  # deflate
  if(NOT EXISTS ${GETTEXT_ROOT})

    message(STATUS "Deflating gettexttools from thirdparties to ${GETTEXT_ROOT}")
    # create directory
    if(NOT EXISTS ${GETTEXT_ROOT})
      file(MAKE_DIRECTORY ${GETTEXT_ROOT})
    endif()

    # untar the dependency
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${thirdparty_dir}/${GETTEXTTOOLS}-bin.zip
      WORKING_DIRECTORY ${GETTEXT_ROOT})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${thirdparty_dir}/${GETTEXTTOOLS}-dep.zip
      WORKING_DIRECTORY ${GETTEXT_ROOT})
  endif()

endif()

set(GETTEXT_FINDPROGRAM_OPTIONS)
if(NOT ("${GETTEXT_ROOT}" STREQUAL ""))
  set(GETTEXT_FINDPROGRAM_OPTIONS
      PATHS ${GETTEXT_ROOT}
               PATH_SUFFIXES bin
               DOC "gettext program deflated from the thirdparties"
               NO_DEFAULT_PATH)
endif()

find_program(XGETTEXT
             NAMES xgettext
             ${GETTEXT_FINDPROGRAM_OPTIONS})

find_program(MSGFMT
              NAMES msgfmt
             ${GETTEXT_FINDPROGRAM_OPTIONS})

find_program(MSGMERGE
              NAMES msgmerge
             ${GETTEXT_FINDPROGRAM_OPTIONS})

if(NOT XGETTEXT)
  message(STATUS "'xgettext' program not found")
else()
  message(STATUS "'xgettext' program found at '${XGETTEXT}'")
endif()

if(NOT MSGFMT)
  message(STATUS "'msgfmt' program not found")
else()
  message(STATUS "'msgfmt' program found at '${MSGFMT}'")
endif()

if(NOT MSGMERGE)
  message(STATUS "'msgmerge' program not found")
else()
  message(STATUS "'msgmerge' program found at '${MSGMERGE}'")
endif()
