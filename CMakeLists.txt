cmake_minimum_required(VERSION 2.6)
project(OPENPHD)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

Include (CheckCXXSourceCompiles)
include (CheckIncludeFiles)

#see /usr/local/share/cmake-2.6/Modules/Find*.cmake for more examples

#wxWidgets
FIND_PACKAGE(wxWidgets REQUIRED)
INCLUDE(UsewxWidgets)

#cfitsio
find_package(CFITSIO REQUIRED)
include_directories( ${CFITSIO_INCLUDE_DIR})

#gtk
find_package(PkgConfig REQUIRED)
pkg_check_modules (GTK REQUIRED gtk+-2.0)

include_directories( ${CMAKE_SOURCE_DIR})
include_directories( ${GTK_INCLUDE_DIRS})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_WXWINDOWS_CXX_FLAGS}")

# set(CMAKE_CXX_FLAGS "-g -Wall")
set(phd_SRCS
   ${CMAKE_SOURCE_DIR}/camera.cpp
   ${CMAKE_SOURCE_DIR}/cam_INDI.cpp
   ${CMAKE_SOURCE_DIR}/cam_SBIG.cpp
   ${CMAKE_SOURCE_DIR}/cam_simulator.cpp
   ${CMAKE_SOURCE_DIR}/canvas.cpp
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
   ${CMAKE_SOURCE_DIR}/libindiclient/indi.c
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indigui.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indi_io.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/wx/indi_list.cpp
   ${CMAKE_SOURCE_DIR}/libindiclient/base64.c
   ${CMAKE_SOURCE_DIR}/libindiclient/lilxml.c
   )

#Suppress some warnings
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


add_executable(PHD ${phd_SRCS})
target_link_libraries(PHD ${CFITSIO_LIBRARIES})
target_link_libraries(PHD ${wxWidgets_LIBRARIES})
#set(phd_LIBS "${phd_LIBS} ${GTK_LIBRARIES}")
target_link_libraries(PHD ${phd_LIBS})

