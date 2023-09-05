# - Try to find INDI
# Once done this will define
#
#  INDI_FOUND - system has INDI
#  INDI_INCLUDE_DIR - the INDI include directory
#  For Android
#  INDI_CLIENT_ANDROID_LIBRARIES - Link to these for INDI Android client
#  For other platforms
#  INDI_LIBRARIES - Link to these for XML and INDI Common support (INDI < 1.4 ONLY)
#  INDI_CLIENT_LIBRARIES - Link to these to build INDI clients
#  INDI_CLIENT_QT_LIBRARIES - Link to these to build INDI clients with Qt5 backend

# Copyright (c) 2016, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Copyright (c) 2012, Pino Toscano <pino@kde.org>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution AND use is allowed according to the terms of the BSD license.

macro(_INDI_check_version)
  file(READ "${INDI_INCLUDE_DIR}/indiapi.h" _INDI_version_header)

  string(REGEX MATCH "#define INDI_VERSION_MAJOR[ \t]+([0-9]+)" _INDI_version_major_match "${_INDI_version_header}")
  set(INDI_VERSION_MAJOR "${CMAKE_MATCH_1}")
  string(REGEX MATCH "#define INDI_VERSION_MINOR[ \t]+([0-9]+)" _INDI_version_minor_match "${_INDI_version_header}")
  set(INDI_VERSION_MINOR "${CMAKE_MATCH_1}")
  string(REGEX MATCH "#define INDI_VERSION_RELEASE[ \t]+([0-9]+)" _INDI_version_release_match "${_INDI_version_header}")
  set(INDI_VERSION_RELEASE "${CMAKE_MATCH_1}")

  set(INDI_VERSION ${INDI_VERSION_MAJOR}.${INDI_VERSION_MINOR}.${INDI_VERSION_RELEASE})

  if(${INDI_VERSION} STREQUAL "..")
    # we may have an old version
    string(REGEX MATCH "#define INDI_LIBV[ \t]+([0-9]+).([0-9]+)" _INDI_version_major_match _INDI_version_minor_match "${_INDI_version_header}")
    set(INDI_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(INDI_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(INDI_VERSION ${INDI_VERSION_MAJOR}.${INDI_VERSION_MINOR})
    message(STATUS "Old INDI version ${INDI_VERSION} found in ${INDI_INCLUDE_DIR}, please consider upgrading")
  endif(${INDI_VERSION} STREQUAL "..")

  if(${INDI_VERSION} VERSION_LESS ${INDI_FIND_VERSION})
    set(INDI_VERSION_OK FALSE)
  else(${INDI_VERSION} VERSION_LESS ${INDI_FIND_VERSION})
    set(INDI_VERSION_OK TRUE)
  endif(${INDI_VERSION} VERSION_LESS ${INDI_FIND_VERSION})

  if(NOT INDI_VERSION_OK)
    message(STATUS "INDI version ${INDI_VERSION} found in ${INDI_INCLUDE_DIR}, "
                   "but at least version ${INDI_FIND_VERSION} is required")
  else(NOT INDI_VERSION_OK)
      mark_as_advanced(INDI_VERSION_MAJOR INDI_VERSION_MINOR INDI_VERSION_RELEASE)
  endif(NOT INDI_VERSION_OK)
endmacro(_INDI_check_version)

if (INDI_INCLUDE_DIR)
  # in cache already
  _INDI_check_version()
  if(ANDROID)
      if(INDI_CLIENT_ANDROID_LIBRARIES)
          set(INDI_FOUND ${INDI_VERSION_OK})
          message(STATUS "Found INDI: ${INDI_CLIENT_ANDROID_LIBRARIES}")
      endif(INDI_CLIENT_ANDROID_LIBRARIES)
  else(ANDROID)
      if(INDI_LIBRARIES AND (INDI_CLIENT_LIBRARIES OR INDI_CLIENT_QT_LIBRARIES))
          set(INDI_FOUND ${INDI_VERSION_OK})
          message(STATUS "Found INDI: ${INDI_LIBRARIES}, ${INDI_CLIENT_LIBRARIES}, ${INDI_INCLUDE_DIR}")
      endif(INDI_LIBRARIES AND (INDI_CLIENT_LIBRARIES OR INDI_CLIENT_QT_LIBRARIES))
  endif(ANDROID)
endif(INDI_INCLUDE_DIR)

if(NOT INDI_FOUND)
  if (NOT WIN32 AND NOT ANDROID)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(PC_INDI INDI)
    endif (PKG_CONFIG_FOUND)
  endif (NOT WIN32 AND NOT ANDROID)

  find_path(INDI_INCLUDE_DIR indidevapi.h
    PATH_SUFFIXES libindi
    ${PC_INDI_INCLUDE_DIRS}
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

if (INDI_INCLUDE_DIR)
  _INDI_check_version()

if(ANDROID)
      find_library(INDI_CLIENT_ANDROID_LIBRARIES NAMES indiclientandroid
          PATHS
          ${BUILD_KSTARSLITE_DIR}/android_libs/${ANDROID_ARCHITECTURE}/
      )
else(ANDROID)
    # Deprecated in INDI Library >= 1.4.0
    if (INDI_VERSION VERSION_LESS "1.4.0")
    find_library(INDI_LIBRARIES NAMES indi
        PATHS
        ${PC_INDI_LIBRARY_DIRS}
        ${_obLinkDir}
        ${GNUWIN32_DIR}/lib
    )
    else()
    set(INDI_LIBRARIES "Deprecated")
    endif()

    find_library(INDI_CLIENT_LIBRARIES NAMES indiclient
        PATHS
        ${PC_INDI_LIBRARY_DIRS}
        ${_obLinkDir}
        ${GNUWIN32_DIR}/lib
    )

    find_library(INDI_CLIENT_QT_LIBRARIES NAMES indiclientqt
        PATHS
            ${PC_INDI_LIBRARY_DIRS}
            ${_obLinkDir}
            ${GNUWIN32_DIR}/lib
    )
endif(ANDROID)
endif(INDI_INCLUDE_DIR)

if(ANDROID)
  if(INDI_INCLUDE_DIR AND INDI_CLIENT_ANDROID_LIBRARIES)
    set(INDI_FOUND TRUE)
  else()
    set(INDI_FOUND FALSE)
  endif()
else(ANDROID)
  if (INDI_INCLUDE_DIR AND INDI_LIBRARIES AND (INDI_CLIENT_LIBRARIES OR INDI_CLIENT_QT_LIBRARIES) AND INDI_VERSION_OK)
      # If INDI is found we need to make sure on WIN32 we have INDI Client Qt backend otherwise we can't use INDI
      if (WIN32)
          if (INDI_CLIENT_QT_LIBRARIES)
              set(INDI_FOUND TRUE)
          else(INDI_CLIENT_QT_LIBRARIES)
              set(INDI_FOUND FALSE)
          endif(INDI_CLIENT_QT_LIBRARIES)
      else (WIN32)
          set(INDI_FOUND TRUE)
      endif(WIN32)
  else (INDI_INCLUDE_DIR AND INDI_LIBRARIES AND (INDI_CLIENT_LIBRARIES OR INDI_CLIENT_QT_LIBRARIES) AND INDI_VERSION_OK)
    set(INDI_FOUND FALSE)
  endif (INDI_INCLUDE_DIR AND INDI_LIBRARIES AND (INDI_CLIENT_LIBRARIES OR INDI_CLIENT_QT_LIBRARIES) AND INDI_VERSION_OK)
endif(ANDROID)

  if (INDI_FOUND)
    if (NOT INDI_FIND_QUIETLY)
        if(ANDROID)
            message(STATUS "Found INDI Android Client: ${INDI_CLIENT_ANDROID_LIBRARIES}")
        else(ANDROID)
          message(STATUS "Found INDI: ${INDI_INCLUDE_DIR}")

          if (INDI_CLIENT_LIBRARIES)
            message(STATUS "Found INDI Client Library: ${INDI_CLIENT_LIBRARIES}")
          endif (INDI_CLIENT_LIBRARIES)
          if (INDI_CLIENT_QT_LIBRARIES)
            message(STATUS "Found INDI Qt5 Client Library: ${INDI_CLIENT_QT_LIBRARIES}")
          endif (INDI_CLIENT_QT_LIBRARIES)
      endif(ANDROID)
    endif (NOT INDI_FIND_QUIETLY)
  else (INDI_FOUND)
    if (INDI_FIND_REQUIRED)
      message(FATAL_ERROR "INDI not found. Please install INDI and try again.")
    endif (INDI_FIND_REQUIRED)
  endif (INDI_FOUND)

    if(ANDROID)
        mark_as_advanced(INDI_INCLUDE_DIR INDI_CLIENT_ANDROID_LIBRARIES)
    else(ANDROID)
        mark_as_advanced(INDI_INCLUDE_DIR INDI_LIBRARIES INDI_CLIENT_LIBRARIES INDI_CLIENT_QT_LIBRARIES)
    endif(ANDROID)
endif(NOT INDI_FOUND)
