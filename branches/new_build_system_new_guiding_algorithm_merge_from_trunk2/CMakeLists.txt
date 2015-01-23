# Copyright 2014, Max Planck Society.
# Distributed under the XXX.
# (See accompanying file LICENSE.txt or copy at
# XXX)

cmake_minimum_required(VERSION 2.8.11)
project(PhD2)

enable_testing()

set(USE_FOLDERS TRUE)
set(BUILD_SHARED_LIBS FALSE)

set(VERSION_MAJOR 2)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)

# root directory of the project
set(PHD_PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})


# build type, by default to release (with optimisations)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()



# should be placed in the thirdparties cmake
# INDI
find_package(INDI REQUIRED)
include_directories(${INDI_INCLUDE_DIR})

# Nova
find_package(Nova)
if (NOVA_FOUND)
    include_directories(${NOVA_INCLUDE_DIR})
else ()
    message(WARNING "libnova not found! Considere to install libnova-dev ")
endif (NOVA_FOUND)   

if (NOVA_FOUND)
    target_link_libraries(phd2 ${NOVA_LIBRARIES} )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLIBNOVA" )
endif (NOVA_FOUND)   


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CheckCXXCompilerFlag)




# general defines
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


endif()


# to be changed in add_subdirectory
include(build/CMakeListsThirdparties.txt)

# Adding the wxWidgets include definitions. Maybe narrowed to Phd2 project only
include_directories(${wxWidgets_INCLUDE_DIRS})

# Adding the gaussian process headers
include_directories(${phd_src_dir}/gaussian_process)

set(phd_src_dir ${PHD_PROJECT_ROOT_DIR})

# Gaussian Process

set(gp_SRC
    ${phd_src_dir}/gaussian_process/tools/math_tools.cpp
    ${phd_src_dir}/gaussian_process/tools/math_tools.h
    ${phd_src_dir}/gaussian_process/tools/circular_buffer.h
    ${phd_src_dir}/gaussian_process/tools/circular_buffer.cpp)
add_library(GP STATIC ${gp_SRC})
target_include_directories(GP PUBLIC ${EIGEN_SRC})

# Test for the GP
# add_executable(GPTest ${phd_src_dir}/tests/gaussian_process/main.cpp)
# target_link_libraries(GPTest GP gtest)
# target_include_directories(GPTest PRIVATE ${phd_src_dir}/gaussian_process
#                                   PRIVATE ${GTEST_HEADERS})
# set_property(TARGET GPTest PROPERTY FOLDER "Unit tests/")
# add_test(GPTest1 GPTest)

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

  ${phd_src_dir}/cam_ascom.cpp
  ${phd_src_dir}/ascom.cpp
  ${phd_src_dir}/extras/cam_ascomlate.cpp

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
source_group(Cameras FILES ${cam_SRC})

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
  ${phd_src_dir}/scope_INDI.h
  ${phd_src_dir}/scope_INDI.cpp
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

  # gaussian process
  ${phd_src_dir}/UDPGuidingInteraction.cpp
  ${phd_src_dir}/UDPGuidingInteraction.h
  ${phd_src_dir}/guide_algorithm_gaussian_process.cpp
  ${phd_src_dir}/guide_algorithm_gaussian_process.h
)
source_group(Guiding FILES ${guiding_SRC})


set(indiClient_SRC
  ${phd_src_dir}/libindiclient/indi.c
  ${phd_src_dir}/libindiclient/wx/indigui.cpp
  ${phd_src_dir}/libindiclient/wx/indi_io.cpp
  ${phd_src_dir}/libindiclient/wx/indi_list.cpp
  ${phd_src_dir}/libindiclient/wx/indi_config.cpp
  ${phd_src_dir}/libindiclient/wx/indisave.cpp
  ${phd_src_dir}/libindiclient/wx/wxled.cpp
  ${phd_src_dir}/libindiclient/wx/wxchecktreectrl.cpp
  ${phd_src_dir}/libindiclient/base64.c
  ${phd_src_dir}/libindiclient/lilxml.c
)
source_group(IndiClient FILES ${indiClient_SRC})


set(gaussian_process_SRC
  ${phd_src_dir}/gaussian_process/tools/circular_buffer.cpp
  ${phd_src_dir}/gaussian_process/tools/circular_buffer.h
  ${phd_src_dir}/gaussian_process/tools/math_tools.cpp
  ${phd_src_dir}/gaussian_process/tools/math_tools.h
)
source_group(GaussianProcess FILES ${gaussian_process_SRC})



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
if(UNIX AND NOT APPLE)
  set(phd2_SRC 
      ${phd2_SRC}
      ${phd_src_dir}/config_INDI.cpp
     )
endif()
source_group(src FILES ${phd2_SRC})

# Test for the CircularBuffer
add_executable(CircularBufferTest ${phd_src_dir}/tests/circular_buffer/circular_buffer_test.cpp)
target_link_libraries(CircularBufferTest GP gtest)
target_include_directories(CircularBufferTest PRIVATE ${phd_src_dir}
                                              PRIVATE ${GTEST_HEADERS})
set_property(TARGET CircularBufferTest PROPERTY FOLDER "Unit tests/")
add_test(CircularBufferTest1 CircularBufferTest)




include_directories(${phd_src_dir}/cam_KWIQGuider/)
include_directories(${phd_src_dir}/cameras/)


if(APPLE)

  # bundle definitions
  # please edit the copyrights
  set( MACOSX_BUNDLE_BUNDLE_NAME          ${PROJECT_NAME} )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER       "com.open-phd-guiding.PHD2" )
  set( MACOSX_BUNDLE_ICON_FILE            PHD_OSX_icon.icns )
  set( MACOSX_BUNDLE_INFO_STRING          "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}, Copyright 2014 Phd2 Guiding team" )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}" )
  set( MACOSX_BUNDLE_LONG_VERSION_STRING  "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
  set( MACOSX_BUNDLE_BUNDLE_VERSION       "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" )
  set( MACOSX_BUNDLE_COPYRIGHT            "(C) 2010-2014 Phd2 Guiding team" )



  add_executable(
    Phd2
    MACOSX_BUNDLE
    ${scopes_SRC}
    ${cam_SRC}
    ${cam_KWIQGuider_SRC}
    ${guiding_SRC}
    ${gaussian_process_SRC}
    ${phd2_SRC}
    
    # ressources of the bundle
    ${phd_src_dir}/${MACOSX_BUNDLE_ICON_FILE}
    ${phd_src_dir}/PHD2GuideHelp.zip
    
    # extra frameworks
    ${sbigudFramework}
    ${fcCamFramework}
  )
  target_include_directories(
    Phd2
    PRIVATE ${CARBON_INCLUDE_DIR}/Carbon.h
  )
      
  set_source_files_properties(
    ${phd_src_dir}/${MACOSX_BUNDLE_ICON_FILE} 
    ${phd_src_dir}/PHD2GuideHelp.zip
      PROPERTIES 
        MACOSX_PACKAGE_LOCATION Resources
  )
  set_source_files_properties(
    ${fcCamFramework}
      PROPERTIES MACOSX_PACKAGE_LOCATION Frameworks
  )  
  set_source_files_properties(
    ${sbigudFramework}
      PROPERTIES MACOSX_PACKAGE_LOCATION Frameworks
  )

 elseif(WIN32)

  set(phd2_WIN_SRC

    ${phd_src_dir}/parallelports.h
    ${phd_src_dir}/parallelport.h
    ${phd_src_dir}/parallelport.cpp

    ${phd_src_dir}/parallelport_win32.h
    ${phd_src_dir}/parallelport_win32.cpp

    ${phd_src_dir}/comdispatch.h
    ${phd_src_dir}/comdispatch.cpp
    ${phd_src_dir}/phd.rc  )   # resource file



  add_executable(
    Phd2
    WIN32
    ${scopes_SRC}
    ${cam_SRC}
    #${cam_KWIQGuider_SRC} # not included in the win32 projects
    ${guiding_SRC}
    ${phd2_SRC}
    ${gaussian_process_SRC}
    ${phd2_WIN_SRC}
    )

  set_target_properties(
    Phd2
    PROPERTIES
      LINK_FLAGS "/DELAYLOAD:sbigudrv.dll"
      )
  # Delayed load for all these in the original project. Now using delay load only for the necessary stuff
  #sbigudrv.dll;ICFilterContainer.dll;TIS_DShowLib09.dll;TIS_UDSHL09_vc9.dll;FCApi.dll;OSPL130RT.dll;DLCWB.dll;cfitsio.dll;CMOSDll.dll;DSCI.dll;inpout32.dll;ShoestringGPUSB_DLL.dll;ShoestringLXUSB_DLL.dll;SXUSB.dll;AsiCamera.dll;%(DelayLoadDLLs)

  #link_directories(${phd_src_dir}/WinLibs)
  copy_dependency_with_config(Phd2 PHD_COPY_EXTERNAL_ALL PHD_COPY_EXTERNAL_DBG PHD_COPY_EXTERNAL_REL)

elseif(UNIX)
  # should be linux there
  add_executable(
   Phd2
   ${scopes_SRC}
   ${cam_SRC}
   ${indiClient_SRC}
   ${guiding_SRC}
   ${gaussian_process_SRC}
   ${phd2_SRC}
   )
  target_link_libraries(Phd2 X11)
else()
  message(FATAL_ERROR "Unsupported platform")

endif()

target_include_directories(Phd2 PUBLIC ${EIGEN_SRC})

# properties of the project common to all platforms
target_compile_definitions(Phd2 PRIVATE "${wxWidgets_DEFINITIONS};HAVE_TYPE_TRAITS")
set_target_properties(
  Phd2
  PROPERTIES
    COMPILE_OPTIONS "${wxWidgets_CXX_FLAGS};"
    )

target_link_libraries(Phd2 ${PHD_LINK_EXTERNAL})


# convenient way to edit additional files
add_custom_target(CmakeAdditionalFiles
  SOURCES
  ${phd_src_dir}/build/CMakeListsThirdparties.txt
)




install (TARGETS phd2 RUNTIME DESTINATION bin)
install (FILES "${PROJECT_SOURCE_DIR}/icons/phd2.png" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pixmaps/" )
install (FILES "${PROJECT_SOURCE_DIR}/phd2.desktop" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications/" )
install (FILES "${PROJECT_SOURCE_DIR}/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/" )
# install language help files
install (FILES "${PROJECT_SOURCE_DIR}/locale/de_DE/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/de_DE/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/es_ES/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/es_ES/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/fr_FR/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/fr_FR/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/ja_JP/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/ja_JP/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/pl_PL/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/pl_PL/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/ru_RU/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/ru_RU/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/uk_UA/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/uk_UA/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/zh_CN/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/zh_CN/" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/zh_TW/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/locale/zh_TW/" OPTIONAL)
# install translation in standard system location
install (FILES "${PROJECT_SOURCE_DIR}/locale/de_DE/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/de/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/es_ES/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/es/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/fr_FR/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/fr/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/ja_JP/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ja/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/pl_PL/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/pl/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/ru_RU/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ru/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/uk_UA/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/uk/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/zh_CN/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/zh_CN/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
install (FILES "${PROJECT_SOURCE_DIR}/locale/zh_TW/messages.mo" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/zh_TW/LC_MESSAGES/" RENAME "phd2.mo" OPTIONAL)
