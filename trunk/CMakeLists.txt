cmake_minimum_required(VERSION 2.6)
project(OPENPHD)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

Include (CheckCXXSourceCompiles)
include (CheckIncludeFiles)

#v4l
include(FindPkgConfig)

pkg_check_modules(LIBUDEV libudev)
pkg_check_modules(V4L2 libv4l2)

IF(LIBUDEV_FOUND AND V4L2_FOUND)
    set(V4L_OK 1)
ELSE (LIBUDEV_FOUND AND V4L2_FOUND)
    message("\n"
            "WARNING: Missing dependencies for V4L\n"
            "WARNING: V4L cameras will not be supported\n")
ENDIF (LIBUDEV_FOUND AND V4L2_FOUND)

if(V4L_OK)
    include_directories(${LIBUDEV_INCLUDE_DIRS})
    include_directories(${V4L2_INCLUDE_DIRS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVIDEODEVICE")
endif(V4L_OK)

#wxWidgets
FIND_PACKAGE(wxWidgets REQUIRED)
INCLUDE(UsewxWidgets)

#cfitsio
find_package(CFITSIO REQUIRED)
include_directories(${CFITSIO_INCLUDE_DIR})

#zlib
find_package(ZLIB REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_WXWINDOWS_CXX_FLAGS}")

# set(CMAKE_CXX_FLAGS "-g -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGUIDE_GCUSBST4")

set(phd_SRCS
   ${CMAKE_SOURCE_DIR}/camera.cpp
   ${CMAKE_SOURCE_DIR}/cam_INDI.cpp
   ${CMAKE_SOURCE_DIR}/cam_SBIG.cpp
   ${CMAKE_SOURCE_DIR}/cam_simulator.cpp
   ${CMAKE_SOURCE_DIR}/canvas.cpp
   ${CMAKE_SOURCE_DIR}/config_INDI.cpp
   ${CMAKE_SOURCE_DIR}/eegg.cpp
   ${CMAKE_SOURCE_DIR}/frame_events.cpp
   ${CMAKE_SOURCE_DIR}/graph.cpp
   ${CMAKE_SOURCE_DIR}/guide_routines.cpp
   ${CMAKE_SOURCE_DIR}/image_math.cpp
   ${CMAKE_SOURCE_DIR}/phd.cpp
   ${CMAKE_SOURCE_DIR}/prefs.cpp
   ${CMAKE_SOURCE_DIR}/scope.cpp
   ${CMAKE_SOURCE_DIR}/socket_server.cpp
   ${CMAKE_SOURCE_DIR}/tele_INDI.cpp
   ${CMAKE_SOURCE_DIR}/usImage.cpp
   ${CMAKE_SOURCE_DIR}/GC_USBST4.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/indi.c
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indigui.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indi_io.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indi_list.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indi_config.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indisave.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/wxled.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/wxchecktreectrl.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/base64.c
   ${CMAKE_SOURCE_DIR}/libindiclient/lilxml.c

   )

if(V4L_OK)
   set(phd_SRCS ${phd_SRCS} 
      ${CMAKE_SOURCE_DIR}/cam_VIDEODEVICE.cpp
      ${CMAKE_SOURCE_DIR}/config_VIDEODEVICE.cpp
      ${CMAKE_SOURCE_DIR}/v4lcontrol.cpp
      ${CMAKE_SOURCE_DIR}/cameras/linuxvideodevice.cpp
      )
endif(V4L_OK)

#Suppress some warnings
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

add_executable(PHD ${phd_SRCS})
target_link_libraries(PHD ${CFITSIO_LIBRARIES})
target_link_libraries(PHD ${wxWidgets_LIBRARIES})
if(V4L_OK)
    target_link_libraries(PHD ${LIBUDEV_LIBRARIES})
    target_link_libraries(PHD ${V4L2_LIBRARIES})
endif(V4L_OK)
#set(phd_LIBS "${phd_LIBS} ${GTK_LIBRARIES}")
target_link_libraries(PHD ${phd_LIBS})

