# - Try to find ZWO
# Once done this will define
#
#  ZWO_FOUND - system has ZWO Libraries
#  ZWO_INCLUDE_DIR - the ZWO Camera include directory
#  ZWO_LIBRARIES - Link these to use ZWO Camera

# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (ZWO_INCLUDE_DIR AND ZWO_LIBRARIES)

  # in cache already, be quiet
  set(ZWO_FIND_QUIETLY TRUE)

else (ZWO_INCLUDE_DIR AND ZWO_LIBRARIES)

  if (UNIX)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) 
        set(arch "x64") 
    else() 
        set(arch "x86") 
    endif() 
  endif()
  if (APPLE)
    set(arch "mac")
  endif()

  # JM: Packages from different distributions have different suffixes
  find_path(ZWO_INCLUDE_DIR ASICamera2.h
    PATHS
    ${PROJECT_SOURCE_DIR}/cameras
  )

  find_library(ZWO_LIBRARIES NAMES ASICamera2
    PATHS
    ${PROJECT_SOURCE_DIR}/cameras/zwolibs
    PATH_SUFFIXES ${arch}
  )

  if(ZWO_INCLUDE_DIR AND ZWO_LIBRARIES)
    set(ZWO_FOUND TRUE)
  else (ZWO_INCLUDE_DIR AND ZWO_LIBRARIES)
    set(ZWO_FOUND FALSE)
  endif(ZWO_INCLUDE_DIR AND ZWO_LIBRARIES)


  if (ZWO_FOUND)


    if (NOT ZWO_FIND_QUIETLY)
      message(STATUS "Found ZWO Libraries at: ${ZWO_LIBRARIES}")
    endif (NOT ZWO_FIND_QUIETLY)
  else (ZWO_FOUND)
    if (ZWO_FIND_REQUIRED)
      message(STATUS "ZWO libraries not found.")
    endif (ZWO_FIND_REQUIRED)
  endif (ZWO_FOUND)

  mark_as_advanced(ZWO_INCLUDE_DIR ZWO_LIBRARIES)

endif (ZWO_INCLUDE_DIR AND ZWO_LIBRARIES)
