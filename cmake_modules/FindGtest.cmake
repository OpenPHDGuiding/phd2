# - Try to find GTEST
# Once done this will define
#
#  GTEST_FOUND - system has NOVA
#  GTEST_INCLUDE_DIR - the NOVA include directory

# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (GTEST_INCLUDE_DIR)

  # in cache already
  set(GTEST_FOUND TRUE)
  message(STATUS "Found Gtest: ${GTEST_INCLUDE_DIR}")

else (GTEST_INCLUDE_DIR)

  find_path(GTEST_INCLUDE_DIR gtest.h
    PATH_SUFFIXES gtest
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

 set(CMAKE_REQUIRED_INCLUDES ${GTEST_INCLUDE_DIR})

   if(GTEST_INCLUDE_DIR)
    set(GTEST_FOUND TRUE)
  else (GTEST_INCLUDE_DIR)
    set(GTEST_FOUND FALSE)
  endif(GTEST_INCLUDE_DIR)

  if (GTEST_FOUND)
    if (NOT Gtest_FIND_QUIETLY)
      message(STATUS "Found Gtest: ${GTEST_INCLUDE_DIR}")
    endif (NOT Gtest_FIND_QUIETLY)
  else (GTEST_FOUND)
    if (Gtest_FIND_REQUIRED)
      message(FATAL_ERROR "Gtest not found.")
    endif (Gtest_FIND_REQUIRED)
  endif (GTEST_FOUND)

  mark_as_advanced(GTEST_INCLUDE_DIR)
  
endif (GTEST_INCLUDE_DIR)
