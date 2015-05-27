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

cmake_minimum_required(VERSION 2.8.11)


if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)

#################################################################################
#
# general defines
if(APPLE)
  # should be done *before* declaring project.
  set(CMAKE_OSX_ARCHITECTURES i386 CACHE STRING "build architecture for OSX" FORCE)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32"   CACHE STRING "Flags used by the compiler during all build types")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "Flags used by the compiler during all build types")
  set(CMAKE_MACOSX_RPATH TRUE)
endif()

# this must appear very early in the file
if(WIN32)
  set(CMAKE_GENERATOR_TOOLSET "v120_xp" CACHE STRING "Platform Toolset" FORCE)
endif()

set(USE_FOLDERS TRUE)
set(BUILD_SHARED_LIBS FALSE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


# options
option(GUIDING_GAUSSIAN_PROCESS "Includes the Gaussian Process guiding algorithm" OFF)



#################################################################################
#
# main project
# this should appear after setting the architecture
project(phd2)
enable_testing()



# root directory of the project
set(PHD_PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(phd_src_dir ${PHD_PROJECT_ROOT_DIR})


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




#################################################################################
#
# external libraries
# to be changed in add_subdirectory
include(thirdparty/thirdparty.cmake)




#################################################################################
#
# contributions
if(${GUIDING_GAUSSIAN_PROCESS})
  add_subdirectory(contributions/MPI_IS_gaussian_process tmp_gaussian_process)
  message(STATUS "[configuration] PHD2 includes the Gaussian Process guiding algorithm")
endif()



# Adding the wxWidgets include definitions. Maybe narrowed to PHD2 project only
#include_directories(${wxWidgets_INCLUDE_DIRS})





#################################################################################
#
# main project
set(cam_KWIQGuider_SRC
  ${phd_src_dir}/cam_KWIQGuider/KWIQGuider.cpp
  ${phd_src_dir}/cam_KWIQGuider/KWIQGuider.h
  ${phd_src_dir}/cam_KWIQGuider/KWIQGuider_firmware.h
  ${phd_src_dir}/cam_KWIQGuider/KWIQGuider_loader.cpp
  ${phd_src_dir}/cam_KWIQGuider/KWIQGuider_priv.h
)
source_group(Cameras\\cam_KWIQGuider FILES ${cam_KWIQGuider_SRC})

set(cam_SRC
  ${phd_src_dir}/cam_ascomlate.cpp
  ${phd_src_dir}/cam_ascomlate.h

  ${phd_src_dir}/cam_Atik16.cpp
  ${phd_src_dir}/cam_Atik16.h
  ${phd_src_dir}/cam_firewire.h
  ${phd_src_dir}/cam_firewire_IC.cpp
  ${phd_src_dir}/cam_firewire_OSX.cpp
  ${phd_src_dir}/cam_INDI.cpp
  ${phd_src_dir}/cam_INDI.h
  ${phd_src_dir}/cam_INovaPLC.cpp
  ${phd_src_dir}/cam_INovaPLC.h
  ${phd_src_dir}/cam_KWIQGuider.cpp
  ${phd_src_dir}/cam_KWIQGuider.h
  ${phd_src_dir}/cam_LEwebcam.cpp
  ${phd_src_dir}/cam_LEwebcam.h
  ${phd_src_dir}/cam_MeadeDSI.cpp
  ${phd_src_dir}/cam_MeadeDSI.h
  ${phd_src_dir}/cam_NebSBIG.cpp
  ${phd_src_dir}/cam_NebSBIG.h
  ${phd_src_dir}/cam_opencv.cpp
  ${phd_src_dir}/cam_opencv.h
  ${phd_src_dir}/cam_openssag.cpp
  ${phd_src_dir}/cam_openssag.h
  ${phd_src_dir}/cam_OSPL130.cpp
  ${phd_src_dir}/cam_OSPL130.h
  ${phd_src_dir}/cam_QGuide.cpp
  ${phd_src_dir}/cam_QGuide.h

  ${phd_src_dir}/cam_QHY5II.cpp
  ${phd_src_dir}/cam_QHY5II.h
  ${phd_src_dir}/cam_QHY5LII.cpp
  ${phd_src_dir}/cam_QHY5LII.h

  ${phd_src_dir}/cam_SAC42.cpp
  ${phd_src_dir}/cam_SAC42.h
  ${phd_src_dir}/cam_SACGuide.cpp
  ${phd_src_dir}/cam_SACGuide.h
  ${phd_src_dir}/cam_SBIG.cpp
  ${phd_src_dir}/cam_SBIG.h
  ${phd_src_dir}/cam_sbigrotator.cpp
  ${phd_src_dir}/cam_sbigrotator.h
  ${phd_src_dir}/cam_simulator.cpp
  ${phd_src_dir}/cam_simulator.h
  ${phd_src_dir}/cam_SSAG.cpp
  ${phd_src_dir}/cam_SSAG.h
  ${phd_src_dir}/cam_SSPIAG.cpp
  ${phd_src_dir}/cam_SSPIAG.h
  ${phd_src_dir}/cam_Starfish.cpp
  ${phd_src_dir}/cam_Starfish.h
  ${phd_src_dir}/cam_StarShootDSCI.cpp
  ${phd_src_dir}/cam_StarShootDSCI.h
  ${phd_src_dir}/cam_SXV.cpp
  ${phd_src_dir}/cam_SXV.h
  ${phd_src_dir}/cam_VFW.cpp
  ${phd_src_dir}/cam_VFW.h
  ${phd_src_dir}/cam_wdm.cpp
  ${phd_src_dir}/cam_WDM.h
  ${phd_src_dir}/cam_ZWO.cpp
  ${phd_src_dir}/cam_ZWO.h
  ${phd_src_dir}/camera.cpp
  ${phd_src_dir}/camera.h
  ${phd_src_dir}/cameras.h
)

# windows specific cameras
if(WIN32)
  set(cam_SRC
    ${cam_SRC}
    ${phd_src_dir}/cam_LEParallelWebcam.cpp
    ${phd_src_dir}/cam_LEParallelWebcam.h
    ${phd_src_dir}/cam_LESerialWebcam.cpp
    ${phd_src_dir}/cam_LESerialWebcam.h
    ${phd_src_dir}/cam_LELXUSBWebcam.cpp
    ${phd_src_dir}/cam_LELXUSBWebcam.h

    ${phd_src_dir}/cam_Atik16.cpp
    ${phd_src_dir}/cam_Atik16.h

    ${phd_src_dir}/cameras/ArtemisHSCAPI.h
    ${phd_src_dir}/cameras/ArtemisHSCAPI.cpp

    ${phd_src_dir}/cam_QHY5IIbase.cpp
    ${phd_src_dir}/cam_QHY5IIbase.h

    # ascom, not there in the sln project -> commented for now 
    # see also the thirdparties cmake to activate/deactivate any call the the findascom.cmake extension
    #${phd_src_dir}/cam_ascom.cpp
    #${phd_src_dir}/ascom.cpp
    #${phd_src_dir}/extras/cam_ascomlate.cpp
  )
elseif(UNIX AND (NOT APPLE))
  set(cam_SRC
      ${cam_SRC}
      #${phd_src_dir}/cam_simulator.h
      #${phd_src_dir}/cam_simulator.cpp
      
      ${phd_src_dir}/cam_qhy5.h
      ${phd_src_dir}/cam_qhy5.cpp
     )
endif()

source_group(Cameras FILES ${cam_SRC})

set(scopes_SRC
  ${phd_src_dir}/mount.cpp
  ${phd_src_dir}/mount.h
  ${phd_src_dir}/scope.cpp
  ${phd_src_dir}/scope.h
  ${phd_src_dir}/scope_ascom.cpp
  ${phd_src_dir}/scope_ascom.h
  ${phd_src_dir}/scope_eqmac.cpp
  ${phd_src_dir}/scope_eqmac.h
  ${phd_src_dir}/scope_equinox.cpp
  ${phd_src_dir}/scope_equinox.h
  ${phd_src_dir}/scope_GC_USBST4.cpp
  ${phd_src_dir}/scope_GC_USBST4.h
  ${phd_src_dir}/scope_gpint.cpp
  ${phd_src_dir}/scope_gpint.h
  ${phd_src_dir}/scope_gpusb.cpp
  ${phd_src_dir}/scope_gpusb.h
  ${phd_src_dir}/scope_onboard_st4.cpp
  ${phd_src_dir}/scope_onboard_st4.h
  ${phd_src_dir}/scope_oncamera.cpp
  ${phd_src_dir}/scope_oncamera.h
  ${phd_src_dir}/scope_onstepguider.cpp
  ${phd_src_dir}/scope_onstepguider.h
  ${phd_src_dir}/scope_voyager.cpp
  ${phd_src_dir}/scope_voyager.h
  ${phd_src_dir}/scope_INDI.h
  ${phd_src_dir}/scope_INDI.cpp
  ${phd_src_dir}/scopes.h
  ${phd_src_dir}/stepguider_sxao.cpp
  ${phd_src_dir}/stepguider_sxao.h
  ${phd_src_dir}/stepguider.cpp
  ${phd_src_dir}/stepguider.h
  ${phd_src_dir}/stepguiders.h
)
source_group(Scopes FILES ${scopes_SRC})

set(guiding_SRC
  ${phd_src_dir}/guide_algorithm_hysteresis.cpp
  ${phd_src_dir}/guide_algorithm_hysteresis.h
  ${phd_src_dir}/guide_algorithm_identity.cpp
  ${phd_src_dir}/guide_algorithm_identity.h
  ${phd_src_dir}/guide_algorithm_lowpass.cpp
  ${phd_src_dir}/guide_algorithm_lowpass.h
  ${phd_src_dir}/guide_algorithm_lowpass2.cpp
  ${phd_src_dir}/guide_algorithm_lowpass2.h
  ${phd_src_dir}/guide_algorithm_resistswitch.cpp
  ${phd_src_dir}/guide_algorithm_resistswitch.h
  ${phd_src_dir}/guide_algorithm.h
  ${phd_src_dir}/guide_algorithms.h
  ${phd_src_dir}/guider_onestar.cpp
  ${phd_src_dir}/guider_onestar.h
  ${phd_src_dir}/guider.cpp
  ${phd_src_dir}/guider.h
  ${phd_src_dir}/guiders.h
)

# gaussian process, also the main project should link to the GP project in the contrib dir
if(${GUIDING_GAUSSIAN_PROCESS})
  set(guiding_SRC 
      ${guiding_SRC}
      ${phd_src_dir}/UDPGuidingInteraction.cpp
      ${phd_src_dir}/UDPGuidingInteraction.h
      ${phd_src_dir}/guide_algorithm_gaussian_process.cpp
      ${phd_src_dir}/guide_algorithm_gaussian_process.h)
endif()

source_group(Guiding FILES ${guiding_SRC})





# it would ease if files are organised in an alphabetical order
set(phd2_SRC
  ${phd_src_dir}/about_dialog.cpp
  ${phd_src_dir}/about_dialog.h
  ${phd_src_dir}/advanced_dialog.cpp
  ${phd_src_dir}/advanced_dialog.h

  ${phd_src_dir}/calreview_dialog.cpp
  ${phd_src_dir}/calreview_dialog.h
  
  ${phd_src_dir}/calstep_dialog.cpp
  ${phd_src_dir}/calstep_dialog.h
  ${phd_src_dir}/camcal_import_dialog.cpp
  ${phd_src_dir}/camcal_import_dialog.h
  ${phd_src_dir}/circbuf.h
  
  ${phd_src_dir}/comet_tool.cpp
  ${phd_src_dir}/comet_tool.h
  
  ${phd_src_dir}/configdialog.cpp
  ${phd_src_dir}/configdialog.h
  ${phd_src_dir}/confirm_dialog.cpp
  ${phd_src_dir}/confirm_dialog.h
  ${phd_src_dir}/darks_dialog.cpp
  ${phd_src_dir}/darks_dialog.h
  ${phd_src_dir}/debuglog.cpp
  ${phd_src_dir}/debuglog.h
  ${phd_src_dir}/drift_tool.cpp
  ${phd_src_dir}/drift_tool.h
  ${phd_src_dir}/eegg.cpp
  ${phd_src_dir}/event_server.cpp
  ${phd_src_dir}/event_server.h

  ${phd_src_dir}/fitsiowrap.cpp
  ${phd_src_dir}/fitsiowrap.h
  
  ${phd_src_dir}/gear_dialog.cpp
  ${phd_src_dir}/gear_dialog.h
  ${phd_src_dir}/graph-stepguider.cpp
  ${phd_src_dir}/graph-stepguider.h
  ${phd_src_dir}/graph.cpp
  ${phd_src_dir}/graph.h
  ${phd_src_dir}/guide_algorithm.cpp
  ${phd_src_dir}/guiding_assistant.cpp
  ${phd_src_dir}/guiding_assistant.h
  ${phd_src_dir}/guidinglog.cpp
  ${phd_src_dir}/guidinglog.h
  ${phd_src_dir}/image_math.cpp
  ${phd_src_dir}/image_math.h
  ${phd_src_dir}/json_parser.cpp
  ${phd_src_dir}/json_parser.h
  ${phd_src_dir}/logger.cpp
  ${phd_src_dir}/logger.h
  ${phd_src_dir}/manualcal_dialog.cpp
  ${phd_src_dir}/manualcal_dialog.h
  ${phd_src_dir}/messagebox_proxy.cpp
  ${phd_src_dir}/messagebox_proxy.h
  ${phd_src_dir}/myframe.cpp
  ${phd_src_dir}/myframe.h
  ${phd_src_dir}/myframe_events.cpp
  ${phd_src_dir}/nudge_lock.cpp
  ${phd_src_dir}/nudge_lock.h
  ${phd_src_dir}/onboard_st4.cpp
  ${phd_src_dir}/onboard_st4.h
  ${phd_src_dir}/optionsbutton.cpp
  ${phd_src_dir}/optionsbutton.h
  ${phd_src_dir}/PHD-Info.plist
  ${phd_src_dir}/phd.cpp
  ${phd_src_dir}/phd.h
  ${phd_src_dir}/PHD2GuideHelp.zip

  ${phd_src_dir}/phdconfig.cpp
  ${phd_src_dir}/phdconfig.h

  ${phd_src_dir}/phdcontrol.cpp
  ${phd_src_dir}/phdcontrol.h
  
  ${phd_src_dir}/profile_wizard.h
  ${phd_src_dir}/profile_wizard.cpp
  
  ${phd_src_dir}/point.h

  ${phd_src_dir}/Refine_DefMap.cpp
  ${phd_src_dir}/Refine_DefMap.h
  
  # rotators
  ${phd_src_dir}/rotators.h
  
  ${phd_src_dir}/rotator.cpp
  ${phd_src_dir}/rotator.h
  
  ${phd_src_dir}/rotator_ascom.cpp
  ${phd_src_dir}/rotator_ascom.h
  
  ${phd_src_dir}/rotator_simulator.h
  ###
  
  ${phd_src_dir}/runinbg.cpp
  ${phd_src_dir}/runinbg.h
  
  ${phd_src_dir}/serialport.cpp
  ${phd_src_dir}/serialport.h
  ${phd_src_dir}/serialport_loopback.cpp
  ${phd_src_dir}/serialport_loopback.h
  ${phd_src_dir}/serialport_mac.cpp
  ${phd_src_dir}/serialport_mac.h
  ${phd_src_dir}/serialport_win32.cpp
  ${phd_src_dir}/serialport_win32.h
  ${phd_src_dir}/serialports.h
  ${phd_src_dir}/socket_server.cpp
  ${phd_src_dir}/socket_server.h
  
  ${phd_src_dir}/statswindow.cpp
  ${phd_src_dir}/statswindow.h
  
  ${phd_src_dir}/star.cpp
  ${phd_src_dir}/star.h
  ${phd_src_dir}/star_profile.cpp
  ${phd_src_dir}/star_profile.h
  ${phd_src_dir}/stepguider_simulator.h
  ${phd_src_dir}/target.cpp
  ${phd_src_dir}/target.h
  ${phd_src_dir}/testguide.cpp
  ${phd_src_dir}/testguide.h
  ${phd_src_dir}/usImage.cpp
  ${phd_src_dir}/usImage.h
  ${phd_src_dir}/worker_thread.cpp
  ${phd_src_dir}/worker_thread.h
)

if(UNIX AND NOT APPLE)
  set(phd2_SRC 
      ${phd2_SRC}
      ${phd_src_dir}/config_INDI.cpp
      ${phd_src_dir}/config_INDI.h
      
      ${phd_src_dir}/indi_gui.cpp
      ${phd_src_dir}/indi_gui.h
      
      ${phd_src_dir}/wxled.cpp
      ${phd_src_dir}/wxled.h
     )
endif()

if (WIN32)
  set(phd2_SRC
      ${phd2_SRC}
      
      # windows-only sources

      ${phd_src_dir}/parallelports.h
      ${phd_src_dir}/parallelport.h
      ${phd_src_dir}/parallelport.cpp

      ${phd_src_dir}/parallelport_win32.h
      ${phd_src_dir}/parallelport_win32.cpp

      ${phd_src_dir}/comdispatch.h
      ${phd_src_dir}/comdispatch.cpp
      ${phd_src_dir}/phd.rc   # resource file
     )
endif()

source_group(src FILES ${phd2_SRC})


include_directories(${phd_src_dir}/cam_KWIQGuider/)
include_directories(${phd_src_dir}/cameras/)

MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "\$(IntDir)/${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")

    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

if(APPLE)

  # bundle definitions
  # please edit the copyrights
  set( MACOSX_BUNDLE_BUNDLE_NAME          PHD2 )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER       "org.openphdguiding.phd2" )
  set( MACOSX_BUNDLE_ICON_FILE            PHD_OSX_icon.icns )
  set( MACOSX_BUNDLE_INFO_STRING          "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}, Copyright 2015 Open-phd-guiding team" )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}" )
  set( MACOSX_BUNDLE_LONG_VERSION_STRING  "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
  set( MACOSX_BUNDLE_BUNDLE_VERSION       "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
  set( MACOSX_BUNDLE_COPYRIGHT            "(C) 2010-2015 Open-phd-guiding team" )

  add_executable(
    phd2
    MACOSX_BUNDLE
    ${scopes_SRC}
    ${cam_SRC}
    ${cam_KWIQGuider_SRC}
    ${guiding_SRC}
    ${phd2_SRC}
    
    # resources of the bundle
    ${phd_src_dir}/${MACOSX_BUNDLE_ICON_FILE}
    ${phd_src_dir}/PHD2GuideHelp.zip
    
    # extra frameworks
    ${sbigudFramework}
    ${fcCamFramework}
  )
  set_target_properties(
    phd2 
    PROPERTIES 
      OUTPUT_NAME PHD2  # upper case on OSX
    )
  
  target_include_directories(
    phd2
    PRIVATE ${CARBON_INCLUDE_DIR}/Carbon.h
  )

  set_source_files_properties(
    ${phd_src_dir}/${MACOSX_BUNDLE_ICON_FILE} 
    ${phd_src_dir}/PHD2GuideHelp.zip
      PROPERTIES 
        MACOSX_PACKAGE_LOCATION Resources
  )
  set_source_files_properties(${fcCamFramework}  PROPERTIES MACOSX_PACKAGE_LOCATION Frameworks)  
  set_source_files_properties(${sbigudFramework} PROPERTIES MACOSX_PACKAGE_LOCATION Frameworks)

elseif(WIN32)

  set(phd2_WIN_SRC

    ${scopes_SRC}
    ${cam_SRC}
    #${cam_KWIQGuider_SRC} # not included in the win32 projects
    ${guiding_SRC}
    ${phd2_SRC}
  )

  ADD_MSVC_PRECOMPILED_HEADER("phd.h" "precompiled_header.cpp" phd2_WIN_SRC)
  source_group(src FILES precompiled_header.cpp)

  add_executable(
    phd2
    WIN32
    ${phd2_WIN_SRC}
    )

  set_target_properties(
    phd2 
    PROPERTIES 
      OUTPUT_NAME phd2  # lower case on Win32
      LINK_FLAGS "/DELAYLOAD:sbigudrv.dll /NODEFAULTLIB:libcmt.lib" #  /NODEFAULTLIB:libcmtd.lib
      LINK_FLAGS_DEBUG "/NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrt.lib"
    )
  # Delayed load for all DLLs below in the original project. Now using delay load only for the necessary stuff
  #sbigudrv.dll;ICFilterContainer.dll;TIS_DShowLib09.dll;TIS_UDSHL09_vc9.dll;FCApi.dll;OSPL130RT.dll;DLCWB.dll;cfitsio.dll;CMOSDll.dll;DSCI.dll;inpout32.dll;ShoestringGPUSB_DLL.dll;ShoestringLXUSB_DLL.dll;SXUSB.dll;AsiCamera.dll;%(DelayLoadDLLs)

  add_custom_command(
    TARGET phd2
    PRE_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${PHD_PROJECT_ROOT_DIR}/PHD2GuideHelp.zip ${phd2_BINARY_DIR})

  copy_dependency_with_config(phd2 PHD_COPY_EXTERNAL_ALL PHD_COPY_EXTERNAL_DBG PHD_COPY_EXTERNAL_REL)

elseif(UNIX)
  
  # should be linux there
  add_executable(
   phd2
   ${scopes_SRC}
   ${cam_SRC}
   ${guiding_SRC}
   ${phd2_SRC}
   )
  target_link_libraries(phd2 X11)

  set_target_properties(
    phd2 
    PROPERTIES 
      OUTPUT_NAME phd2  # lower case on linux
    )  
  
else()
  message(FATAL_ERROR "Unsupported platform")
endif()




# properties of the project common to all platforms
target_compile_definitions(phd2 PRIVATE "${wxWidgets_DEFINITIONS}" "HAVE_TYPE_TRAITS")
target_compile_options(phd2 PRIVATE "${wxWidgets_CXX_FLAGS};")
target_include_directories(phd2 PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(phd2 ${PHD_LINK_EXTERNAL})

# option configurations parts
if(${GUIDING_GAUSSIAN_PROCESS})
  target_link_libraries(phd2 MPIIS_GP)
  target_compile_definitions(phd2 PRIVATE "-DMPIIS_GAUSSIAN_PROCESS_GUIDING_ENABLED__")
endif()



# Additional files in the workspace, To improve maintainability 
add_custom_target(CmakeAdditionalFiles
  SOURCES
  ${phd_src_dir}/thirdparty/thirdparty.cmake
  ${phd_src_dir}/cmake_modules/FindASCOM_INTERFACE.cmake
  ${phd_src_dir}/cmake_modules/FindINDI.cmake
  ${phd_src_dir}/cmake_modules/FindNova.cmake)





################################################################
#
# Installation and packaging
#


if(NOT APPLE)
  install (TARGETS phd2 RUNTIME DESTINATION bin)
endif()

if(UNIX AND NOT APPLE)
  install (FILES ${phd_src_dir}/icons/phd2.png    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps/ )
  install (FILES ${phd_src_dir}/phd2.desktop      DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/ )
  install (FILES ${phd_src_dir}/PHD2GuideHelp.zip DESTINATION ${CMAKE_INSTALL_PREFIX}/share/phd2/ )


  # install language help files
  set(locales_help     de_DE es_ES fr_FR ja_JP pl_PL ru_RU uk_UA zh_CN zh_TW)
  set(locales_help_msg de    es    fr    ja    pl    ru    uk    zh_CN zh_TW)
  list(LENGTH locales_help _len)
  math(EXPR _len "${_len} - 1")
  foreach(_index RANGE ${_len})

    list(GET locales_help       ${_index} _var1)
    list(GET locales_help_msg   ${_index} _var2)

    install(FILES       ${phd_src_dir}/locale/${_var1}/PHD2GuideHelp.zip 
            DESTINATION   ${CMAKE_INSTALL_PREFIX}/share/phd2/locale/${_var1}/ 
            OPTIONAL)

    install(FILES       ${phd_src_dir}/locale/${_var1}/messages.mo 
             DESTINATION  ${CMAKE_INSTALL_PREFIX}/share/locale/${_var2}/LC_MESSAGES/ 
             RENAME "phd2.mo"
             OPTIONAL)          
            
  endforeach()
endif()





