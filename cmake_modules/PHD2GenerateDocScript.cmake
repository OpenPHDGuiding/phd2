# This is an internal helper for generating part of the documentation of the PhD2 project

#=============================================================================
# Copyright 2017, Max Planck Society.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

# File created by Raffi Enficiaud
#=============================================================================

# Usage: cmake
#   -Dlist_of_files=list_of_files
#   -Dinput_folder=input_folder
#   -Dproject_root_dir=root_dir
#   -Doutput_folder=output_folder
#   -P PHD2GenerateDocScript.cmake

include(${project_root_dir}/cmake_modules/PHD2BuildDoc.cmake)

# Generates the hhk file of the given locale
generate_hhk(OUTPUT_FILE "${output_folder}/PHD2GuideHelp.hhk"
             INPUT_FILES "${list_of_files}")

# this is required for zip: we copy the files in the output folder to be able to zip them
# with proper path. There is no path component for this list as the ZIP command needs to be ran
# in the right folder.

file(GLOB additional_files
     ${input_folder}/*.png ${input_folder}/*.PNG ${input_folder}/*.hh[cp])
set(files_to_zip PHD2GuideHelp.hhk)
foreach(_f IN LISTS list_of_files additional_files)
  get_filename_component(filename "${_f}" NAME)
  set(files_to_zip ${files_to_zip} ${filename})
endforeach()
list(REMOVE_DUPLICATES files_to_zip)
file(COPY ${list_of_files} ${additional_files} DESTINATION ${output_folder})

message(STATUS "Adding files")
foreach(_f IN LISTS files_to_zip)
  message("\t'${_f}'")
endforeach()
message("to ${output_folder}/PHD2GuideHelp.zip")

# TODO check if the zip is available (cmake version 3.2)
# if this is not the case, we might require 7zip and plug it as explained in here:
# http://stackoverflow.com/questions/7050997/zip-files-using-cmake

execute_process(COMMAND
                ${CMAKE_COMMAND} -E tar cfv ${output_folder}/PHD2GuideHelp.zip --format=zip ${files_to_zip}
                WORKING_DIRECTORY ${output_folder}
                OUTPUT_QUIET)
