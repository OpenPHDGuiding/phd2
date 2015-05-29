# - Try to find ASCOM Interface
# Once done this will define
#
#  ASCOM_INTERFACE_FOUND - system has ASCOM_INTERFACE
#  ASCOM_INTERFACE_DIR - the directory where the ASCOM Interface File can be found

# Copyright (c) 2010, Bret McKee <bretm@boneheads.us>
#
# Based on work which contained this copyright:
#
# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (ASCOM_INTERFACE_DIR)
    # in cache already, be quiet
    set(ASCOM_INTERFACE_FIND_QUIETLY TRUE)
else (ASCOM_INTERFACE_DIR)
    set(ASCOM_INTERFACE_FILE "AscomMasterInterfaces.tlb")
    
    find_path(ASCOM_INTERFACE_DIR ${ASCOM_INTERFACE_FILE}
        PATHS
            $ENV{ASCOM_INTERFACE}
            "c:/Program Files (x86)/Common Files/ASCOM/Interface"
            "c:/Program Files/Common Files/ASCOM/Interface"
    )

    if (ASCOM_INTERFACE_DIR)
        set(ASCOM_INTERFACE_FOUND TRUE)
    else (ASCOM_INTERFACE_DIR)
        set(ASCOM_INTERFACE_FOUND FALSE)
    endif (ASCOM_INTERFACE_DIR)

endif (ASCOM_INTERFACE_DIR)

if (ASCOM_INTERFACE_FOUND)
    if (NOT ASCOM_INTERFACE_FIND_QUIETLY)
        message(STATUS "Found ASCOM_INTERFACE ${ASCOM_INTERFACE_FILE} in ${ASCOM_INTERFACE_DIR}")
    endif (NOT ASCOM_INTERFACE_FIND_QUIETLY)
else (ASCOM_INTERFACE_FOUND)
    message(STATUS "Unable to find ASCOM_INTERFACE file ${ASCOM_INTERFACE_FILE}.")
endif (ASCOM_INTERFACE_FOUND)

mark_as_advanced(ASCOM_INTERFACE_DIR)

