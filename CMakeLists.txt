# CMake build system

cmake_minimum_required(VERSION 2.6)
project(OPENPHD)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
#TODO propose to remove this everywhere set(CMAKE_CXX_FLAGS "-DOPENPHD")

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

include (CheckCXXSourceCompiles)
include (CheckIncludeFiles)
include (CheckCXXCompilerFlag)

# v4l
#include(FindPkgConfig)
#pkg_check_modules(LIBUDEV libudev)
#pkg_check_modules(V4L2 libv4l2 libv4lconvert)
#
#IF(LIBUDEV_FOUND AND V4L2_FOUND)
#    set(V4L_OK 1)
#ELSE (LIBUDEV_FOUND AND V4L2_FOUND)
#    message("\n"
#            "WARNING: Missing dependencies for V4L\n"
#            "WARNING: V4L cameras will not be supported\n")
#ENDIF (LIBUDEV_FOUND AND V4L2_FOUND)
#
#if(V4L_OK)
#    include_directories(${LIBUDEV_INCLUDE_DIRS})
#    include_directories(${V4L2_INCLUDE_DIRS})
#endif(V4L_OK)

# wxWidgets
FIND_PACKAGE(wxWidgets COMPONENTS core base net html aui adv REQUIRED)
INCLUDE(UsewxWidgets)

# cfitsio
find_package(CFITSIO REQUIRED)
include_directories(${CFITSIO_INCLUDE_DIR})

# zlib
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

# zwo libraries
if (UNIX AND NOT APPLE)
  find_package(ZWO REQUIRED)
endif()

include_directories(${CMAKE_SOURCE_DIR})


# we don't do the strict warnings for MSVC because the MSVC compiler
# generates > 14,000 warnings, mostly from wx. 
if (NOT MSVC)
    #Strict warnings
    CHECK_CXX_COMPILER_FLAG("-W" COMPILER_SUPPORTS_WARNINGS)
    CHECK_CXX_COMPILER_FLAG("-Wall" COMPILER_SUPPORTS_WALL)
    CHECK_CXX_COMPILER_FLAG("-Wextra" COMPILER_SUPPORTS_WEXTRA)
    CHECK_CXX_COMPILER_FLAG("-Werror" COMPILER_SUPPORTS_WERROR)
#    if(COMPILER_SUPPORTS_WARNINGS)
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W")
#        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W")
#    endif(COMPILER_SUPPORTS_WARNINGS)
    if(COMPILER_SUPPORTS_WALL)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
       set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    endif(COMPILER_SUPPORTS_WALL)
#    if(COMPILER_SUPPORTS_WEXTRA)
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
#        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
#    endif(COMPILER_SUPPORTS_WEXTRA)
    if(COMPILER_SUPPORTS_WERROR)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    endif(COMPILER_SUPPORTS_WERROR)
endif (NOT MSVC)

# Scopes to use with openPHD
if (MSVC)
    set(phd_SCOPES
        GUIDE_ASCOM
#       GUIDE_INDI
       )
    if ("${phd_SCOPES}" MATCHES ".*GUIDE_ASCOM.*")
        find_package(ASCOM_INTERFACE REQUIRED)
        include_directories(${ASCOM_INTERFACE_DIR})
    endif ("${phd_SCOPES}" MATCHES ".*GUIDE_ASCOM.*")
else (MSVC)
    set(phd_SCOPES
#        GUIDE_INDI
       )
endif (MSVC)

# Cameras to use with openPHD
if (MSVC)
    set(phd_CAMERAS
         SIMULATOR
         ASCOM_CAMERA
         ASCOM_LATECAMERA
       ) 
else (MSVC)
    set(phd_CAMERAS
        SIMULATOR
#        INDI_CAMERA
       )
endif (MSVC)

file(GLOB CPPS "*.cpp")
set(phd_SRCS ${CPPS})

if (MSVC)
    # make the headers show up in MSVC along with the source
    file(GLOB HEADERS "*.h")
    set(phd_SRCS ${phd_SRCS}
        ${HEADERS}
        ${CMAKE_SOURCE_DIR}/cam_ascom.cpp
        ${CMAKE_SOURCE_DIR}/ascom.cpp
        ${CMAKE_SOURCE_DIR}/extras/cam_ascomlate.cpp
       )
    set(phd_SCOPES ${phd_SCOPES}
        GUIDE_ASCOM
       )
else (MSVC)
	set(phd_SRCS ${phd_SRCS} 
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
ENDIF (MSVC)

#if (V4L_OK)
#    set(phd_SRCS ${phd_SRCS} 
#       ${CMAKE_SOURCE_DIR}/cam_VIDEODEVICE.cpp
#       ${CMAKE_SOURCE_DIR}/config_VIDEODEVICE.cpp
#       ${CMAKE_SOURCE_DIR}/v4lcontrol.cpp
#       ${CMAKE_SOURCE_DIR}/cameras/linuxvideodevice.cpp
#       )
#    set(phd_CAMERAS ${phd_CAMERAS}
#        V4L_CAMERA
#       )
#endif (V4L_OK)

foreach(cam ${phd_CAMERAS} )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${cam}" )
endforeach(cam ${phd_CAMERAS} )

foreach(scope ${phd_SCOPES} )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${scope}" )
endforeach(scope ${phd_SCOPES} )

if (MSVC)
    # generate a WIN32 app, not a console app
    add_executable(phd2 WIN32 ${phd_SRCS} )
    # enable exception handling
    set_target_properties(PHD PROPERTIES COMPILE_FLAGS "/EHa" )
else (MSVC)
    add_executable(phd2 ${phd_SRCS} )
endif (MSVC)


target_link_libraries(phd2 ${CFITSIO_LIBRARIES} )
target_link_libraries(phd2 ${wxWidgets_LIBRARIES} )

#if (V4L_OK)
#    target_link_libraries(phd2 ${LIBUDEV_LIBRARIES})
#    target_link_libraries(phd2 ${V4L2_LIBRARIES})
#endif (V4L_OK)

if (MSVC)
    target_link_libraries(phd2 ${phd_LIBS} z )
else (MSVC)
    target_link_libraries(phd2 ${phd_LIBS} X11 usb-1.0 z )
    if (UNIX AND NOT APPLE)
      target_link_libraries(phd2 ${ZWO_LIBRARIES} rt)
    endif(UNIX AND NOT APPLE)
endif (MSVC)

install (TARGETS phd2 RUNTIME DESTINATION bin)
install (FILES "${PROJECT_SOURCE_DIR}/PHD2GuideHelp.zip" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/phd2/" )

if (MSVC)
    STRING(REGEX REPLACE "[.]lib" ".dll" CFITSIO_DLL ${CFITSIO_LIBRARIES} )
    file(TO_NATIVE_PATH "${CFITSIO_DLL}" CFITSIO_NATIVE_PATH )
    file(TO_NATIVE_PATH "${PROJECT_SOURCE_DIR}/PHD2GuideHelp.zip" HELP_NATIVE_PATH )
    add_custom_command(TARGET phd2 POST_BUILD COMMAND copy ${CFITSIO_NATIVE_PATH} ${CMAKE_CFG_INTDIR} )
    add_custom_command(TARGET phd2 POST_BUILD COMMAND copy ${HELP_NATIVE_PATH} ${CMAKE_CFG_INTDIR} )
endif (MSVC)

