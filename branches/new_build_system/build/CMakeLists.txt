# Copyright 2014, Max Planck Society.
# Distributed under the XXX.
# (See accompanying file LICENSE.txt or copy at
# XXX)

cmake_minimum_required(VERSION 2.8.11)
project(PhD2)


set(VERSION_MAJOR 2)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)

# root directory of the project
set(PHD_PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR}/..)


# build type, by default to release (with optimisations) 
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CheckCXXCompilerFlag)


# general defines
if(WIN32)
else()
  # c++11 options
  check_cxx_compiler_flag(-std=c++11 HAS_CXX11_FLAG)
  check_cxx_compiler_flag(-std=c++0x HAS_CXX0X_FLAG)

  if(HAS_CXX11_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif(HAS_CXX0X_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  endif()


endif()





# to be changed in add_subdirectory
include(CMakeListsThirdparties.txt)


include_directories(${wxWidgets_INCLUDE_DIRS})


set(phd_src_dir ${PHD_PROJECT_ROOT_DIR})


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
  ${phd_src_dir}/cam_indi.cpp
  ${phd_src_dir}/cam_indi.h
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
source_group(Cameras FILES ${cam_SRC})

set(scopes_SRC
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
  ${phd_src_dir}/scopes.h
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
  ${phd_src_dir}/guidinglog.cpp
  ${phd_src_dir}/guidinglog.h
  ${phd_src_dir}/stepguider_sxao.cpp
  ${phd_src_dir}/stepguider_sxao.h
  ${phd_src_dir}/stepguider.cpp
  ${phd_src_dir}/stepguider.h
  ${phd_src_dir}/stepguiders.h
)
source_group(Guiding FILES ${guiding_SRC})

set(phd2_SRC
  ${phd_src_dir}/about_dialog.cpp
  ${phd_src_dir}/about_dialog.h
  ${phd_src_dir}/advanced_dialog.cpp
  ${phd_src_dir}/advanced_dialog.h
  ${phd_src_dir}/ascom_common.cpp
  ${phd_src_dir}/ascom_common.h
  ${phd_src_dir}/calrestore_dialog.cpp
  ${phd_src_dir}/calrestore_dialog.h
  ${phd_src_dir}/calstep_dialog.cpp
  ${phd_src_dir}/calstep_dialog.h
  ${phd_src_dir}/circbuf.h
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

  ${phd_src_dir}/gear_dialog.cpp
  ${phd_src_dir}/gear_dialog.h
  ${phd_src_dir}/graph-stepguider.cpp
  ${phd_src_dir}/graph-stepguider.h
  ${phd_src_dir}/graph.cpp
  ${phd_src_dir}/graph.h
  ${phd_src_dir}/guide_algorithm.cpp
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
  ${phd_src_dir}/mount.cpp
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
  ${phd_src_dir}/point.h
  ${phd_src_dir}/precompiled_header.cpp
  ${phd_src_dir}/Refine_DefMap.cpp
  ${phd_src_dir}/Refine_DefMap.h
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
source_group(src FILES ${phd2_SRC})


message("compilation definitions ${wxWidgets_DEFINITIONS}")
message("compilation options ${wxWidgets_CXX_FLAGS}")


include_directories(${phd_src_dir}/cam_KWIQGuider/)
include_directories(${phd_src_dir}/cameras/)


if(APPLE)

  # bundle definitions
  # please edit the copyrights
  set( MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME} )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER "com.open-phd-guiding.PHD2" )
  set( MACOSX_BUNDLE_ICON_FILE PHD_OSX_icon.icns )
  set( MACOSX_BUNDLE_INFO_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}, Copyright 2014 Phd2 Guiding team" )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}" )
  set( MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
  set( MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
  set( MACOSX_BUNDLE_COPYRIGHT "(C) 2010-2014 Phd2 Guiding team" )



  add_executable(
    Phd2
    MACOSX_BUNDLE
    ${scopes_SRC}
    ${cam_SRC}
    ${cam_KWIQGuider_SRC}
    ${guiding_SRC}
    ${phd2_SRC})
  set_target_properties(
    Phd2
    PROPERTIES 
      #MACOSX_BUNDLE_INFO_PLIST ${phd_src_dir}/PHD-Info.plist
      COMPILE_OPTIONS "-include;${CARBON_INCLUDE_DIR}/Carbon.h"
      )
      
      
  # Allows for bundle re-creation just by running "make". Also installs bundle icon
  add_custom_target( osx_bundle_dirs
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/Phd2.app/Contents/Resources
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/Phd2.app/Contents/MacOS
    COMMAND cp ${phd_src_dir}/${MACOSX_BUNDLE_ICON_FILE} ${CMAKE_CURRENT_BINARY_DIR}/Phd2.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE}
   )
  add_dependencies(Phd2 osx_bundle_dirs)
  
  
  # This tells cmake where to place files inside the bundle
  #set_source_files_properties( ${ProjectName_RESOURCES} ${ProjectName_TRANSLATIONS}
  #  PROPERTIES MACOSX_PACKAGE_LOCATION Resources )      

elseif(WIN32)
  add_executable(
    Phd2
    WIN32 
    ${scopes_SRC}
    ${cam_SRC}
    #${cam_KWIQGuider_SRC} # not included in the win32 projects
    ${guiding_SRC}
    ${phd2_SRC}
    ${phd_src_dir}/phd.rc  # resource file
    )
endif()

# properties of the project common to all platforms
target_compile_definitions(Phd2 PRIVATE "${wxWidgets_DEFINITIONS};HAVE_TYPE_TRAITS")
set_target_properties(
  Phd2
  PROPERTIES 
    COMPILE_OPTIONS "${wxWidgets_CXX_FLAGS};"
    )

target_link_libraries(Phd2 ${PHD_LINK_EXTERNAL})
    
  


# link dependencies on OSX
#-lz
#-lpthread
#-lusb-1.0
#-liconv
