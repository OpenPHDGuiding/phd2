cmake_minimum_required(VERSION 2.6)

Include (CheckCXXSourceCompiles)
include (CheckIncludeFiles)

#wxWidgets
FIND_PACKAGE(wxWidgets)

#gtk
find_package(PkgConfig)

set (common_SRCS
  ${CMAKE_SOURCE_DIR}/indi.c
  ${CMAKE_SOURCE_DIR}/lilxml.c
  ${CMAKE_SOURCE_DIR}/base64.c
  )
SET(CMAKE_CXX_FLAGS "-g -Wall")
SET(CMAKE_C_FLAGS "-g -Wall")

if(EXISTS "gtk/")
  #Only build gtk if the relevant files are present
  if (PKG_CONFIG_FOUND)
    pkg_check_modules (GTK gtk+-2.0)
    if (GTK_FOUND)
  
      include_directories( ${GTK_INCLUDE_DIRS})
      set (inditest_gtk_SRCS
        ${common_SRCS}
        ${CMAKE_SOURCE_DIR}/gtk/indigui.c
        ${CMAKE_SOURCE_DIR}/gtk/indi_io.c
        ${CMAKE_SOURCE_DIR}/gtk/indi_list.c
        ${CMAKE_SOURCE_DIR}/gtk/indi_config.c
        ${CMAKE_SOURCE_DIR}/gtk/indisave.c
        ${CMAKE_SOURCE_DIR}/gtk/indi_main.c
        ${CMAKE_SOURCE_DIR}/gtk/gtkled.c
        )
      add_executable(inditest_gtk ${inditest_gtk_SRCS})
      target_link_libraries(inditest_gtk ${GTK_LIBRARIES})
  
    endif (GTK_FOUND)
  endif (PKG_CONFIG_FOUND)
endif (EXISTS "gtk/")

if(EXISTS "wx/")
  #Only build gtk if the relevant files are present
  if (wxWidgets_FOUND)
    INCLUDE(UsewxWidgets)
    set (inditest_wx_SRCS
      ${common_SRCS}
      ${CMAKE_SOURCE_DIR}/wx/indigui.cpp
      ${CMAKE_SOURCE_DIR}/wx/indi_io.cpp
      ${CMAKE_SOURCE_DIR}/wx/indi_list.cpp
      ${CMAKE_SOURCE_DIR}/wx/indisave.cpp
      ${CMAKE_SOURCE_DIR}/wx/indi_config.cpp
      ${CMAKE_SOURCE_DIR}/wx/wxled.cpp
      ${CMAKE_SOURCE_DIR}/wx/wxchecktreectrl.cpp
      ${CMAKE_SOURCE_DIR}/wx/indi_main.cpp
    )
    add_executable(inditest_wx ${inditest_wx_SRCS})
    target_link_libraries(inditest_wx ${wxWidgets_LIBRARIES})
  endif(wxWidgets_FOUND)
endif (EXISTS "wx/")

