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


#.rst:
# .. command:: get_phd_version
#
#    Extract the current version from the phd.h and populates the variables
#    `VERSION_MAJOR`, `VERSION_MINOR` and `VERSION_PATCH`.
#    Raises an error if the version cannot be extracted
function(get_phd_version)
  set(filename_to_extract_from ${PHD_PROJECT_ROOT_DIR}/phd.h)
  file(STRINGS ${filename_to_extract_from} file_content
       #REGEX "PHDVERSION[ _T\\(]+\"(.*)\""
  )

  foreach(SRC_LINE ${file_content})
    if("${SRC_LINE}" MATCHES "PHDVERSION[ _T\\(]+\"(([0-9]+)\\.([0-9]+).([0-9]+))\"")
        # message("Extracted/discovered version '${CMAKE_MATCH_1}'")
        set(VERSION_MAJOR ${CMAKE_MATCH_2} PARENT_SCOPE)
        set(VERSION_MINOR ${CMAKE_MATCH_3} PARENT_SCOPE)
        set(VERSION_PATCH ${CMAKE_MATCH_4} PARENT_SCOPE)
        return()
    endif()
  endforeach()

  message(FATAL_ERROR "Cannot extract version from file '${filename_to_extract_from}'")

endfunction()


# This file mimics the old PERL script behaviour build_help_hhk

#.rst:
# .. command:: extract_help_filenames
#
#    Extract the documentation filenames from the hhk file.
#    Raises an error if a file does not exist.
function(extract_help_filenames filename_to_extract_from base_folder file_list)
  # reads and splits all the lines
  file(STRINGS ${filename_to_extract_from} file_content)

  # file_content is a list: each string is separated by ;
  # the regex hence works as expected
  string(REGEX MATCH "\\[FILES\\](.*)\\[" subcontent "${file_content}")
  set(subcontent ${CMAKE_MATCH_1})

  set(list_of_files)
  foreach(SRC_LINE IN LISTS subcontent)

    if("${SRC_LINE}" MATCHES "(.+\\.htm.?)")
      set(file_to_add "${base_folder}/${CMAKE_MATCH_1}")
      if(NOT EXISTS "${file_to_add}")
          message(FATAL_ERROR "[DOC] the file '${file_to_add}' does not exist")
      endif()
      list(APPEND list_of_files ${file_to_add})
    endif()
  endforeach()

  set(${file_list} ${list_of_files} PARENT_SCOPE)
endfunction()

#.rst:
# .. command:: generate_hhk
#
#    Generates the HHK file from a set of HTML files
#
#    ::
#
#     generate_hhk(
#         OUTPUT_FILE <output_file>
#         INPUT_FILES <file1> [<file2> ...]
#         )
function(generate_hhk)

  set(options)
  set(oneValueArgs OUTPUT_FILE)
  set(multiValueArgs INPUT_FILES)
  cmake_parse_arguments(_local_vars "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if("${_local_vars_OUTPUT_FILE}" STREQUAL "")
    message(FATAL_ERROR "Incorrect output file")
  endif()

  list(LENGTH _local_vars_INPUT_FILES list_length)
  if(list_length EQUAL 0)
    message(FATAL_ERROR "Incorrect input file list")
  endif()


  set(file_out "${_local_vars_OUTPUT_FILE}")
  set(all_anchors)

  # parses the files and extract all anchors
  foreach(file_to_parse IN LISTS _local_vars_INPUT_FILES)

    message(STATUS "Parsing file ${file_to_parse}")
    get_filename_component(filename_without_directory ${file_to_parse} NAME)

    file(READ ${file_to_parse} file_content)

    string(REGEX MATCHALL "<[ \t]*a[ \t]+name[ \t]*=[ \t]*\"([^\"]+)" subcontent "${file_content}")

    foreach(m IN LISTS subcontent)
        string(REGEX MATCH "<[ \t]*a[ \t]+name[ \t]*=[ \t]*\"([^\"]+)" current_match "${m}")
        #message(STATUS "match = ${CMAKE_MATCH_1}")

        set(matched_anchor "${CMAKE_MATCH_1}")

        string(REPLACE "-" "_" string_key ${matched_anchor})
        string(TOLOWER "${string_key}" string_key)

        list(APPEND all_anchors "${string_key}")

        # simulates an associative map, see https://cmake.org/Wiki/CMake:VariablesListsStrings#Emulating_maps
        set(map_${string_key} ${filename_without_directory} ${matched_anchor})
    endforeach()
  endforeach()

  unset(filename_without_directory)
  unset(matched_anchor)

  # sorting the keys
  list(SORT all_anchors)

  file(WRITE "${file_out}"
       "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n"
       "<HTML><HEAD>\n"
       "<!-- Sitemap 1.0 -->\n"
       "</HEAD><BODY>\n"
       "<UL>\n")

  foreach(key IN LISTS all_anchors)

    #message(STATUS "matched_anchor_without_underscores = ${key}")
    #message(STATUS "    'map_${key}' = '${map_${key}}'")
    list(GET map_${key} 0 filename_without_directory)
    list(GET map_${key} 1 matched_anchor)
    string(REPLACE "_" " " matched_anchor_without_underscores ${matched_anchor})

    string(SUBSTRING "${matched_anchor_without_underscores}" 0 1 matched_anchor_without_underscores_0)
    string(SUBSTRING "${matched_anchor_without_underscores}" 1 -1 matched_anchor_without_underscores_1)
    string(TOUPPER "${matched_anchor_without_underscores_0}" matched_anchor_without_underscores_0)
    set(matched_anchor_without_underscores "${matched_anchor_without_underscores_0}${matched_anchor_without_underscores_1}")

    file(APPEND "${file_out}"
         "\t<LI><OBJECT type=\"text/sitemap\">\n"
         "\t\t<param name=\"Name\" value=\"${matched_anchor_without_underscores}\">\n"
         "\t\t<param name=\"Name\" value=\"${matched_anchor}\">\n"
         "\t\t<param name=\"Local\" value=\"${filename_without_directory}#${matched_anchor}\">\n"
         "\t\t</OBJECT>\n")

  endforeach()

  file(APPEND ${file_out}
       "</UL>\n"
       "</BODY></HTML>\n")

  message(STATUS "Generated file '${file_out}' from sources")

endfunction()


#.rst:
# .. command:: generate_single_doc_targets
#
#    Generates all the targets concerning documentation and
#    populates an output variable for each locale
#
#    ::
#
#     generate_single_doc_targets(
#        INPUT_FOLDER <folder>
#        [LOCALE <current_locale>]
#      )
#
#  * `INPUT_FOLDER` input folder. This folder should contain a `PHD2GuideHelp.hhp` file and the .html files.
#  * `LOCALE` the current locale for the documentation. Defaults to `en_EN`
#
function(generate_single_doc_targets)

  set(options)
  set(oneValueArgs LOCALE INPUT_FOLDER)
  set(multiValueArgs INPUT_FILES)
  cmake_parse_arguments(_local_vars "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if("${PHD_PROJECT_ROOT_DIR}"  STREQUAL "")
    message(FATAL_ERROR "PHD_PROJECT_ROOT_DIR should be set")
  endif()

  if("${_local_vars_INPUT_FOLDER}" STREQUAL "")
    message(FATAL_ERROR "Incorrect input folder")
  endif()

  set(input_folder "${_local_vars_INPUT_FOLDER}")

  if("${_local_vars_LOCALE}" STREQUAL "")
    set(locale "en_EN")
  else()
    set(locale "${_local_vars_LOCALE}")
  endif()

  message(STATUS "[DOC] Creating target for locale ${locale}")

  set(target_files)
  set(input_files)

  # about HTML files
  if(EXISTS "${input_folder}/help")

    if(NOT (EXISTS "${input_folder}/help/PHD2GuideHelp.hhp"))
      message(FATAL_ERROR "'${input_folder}/help/PHD2GuideHelp.hhp' file is missing")
    endif()

    # retrieves the filenames for the documentation
    list(APPEND input_files ${input_folder}/help/PHD2GuideHelp.hhp)
    extract_help_filenames(${input_folder}/help/PHD2GuideHelp.hhp
                           ${input_folder}/help
                           documentation_filelist)

    list(APPEND input_files ${documentation_filelist})

    set(current_locale_output_folder ${CMAKE_BINARY_DIR}/doc/${locale}/)
    set(generated_hhk ${current_locale_output_folder}/PHD2GuideHelp.hhk)
    set(generated_outputs
        ${generated_hhk}
        ${current_locale_output_folder}/PHD2GuideHelp.zip)

    list(APPEND target_files ${generated_outputs})

    # this command generates the hhk file and the zip command, see script PHD2GenerateDocScript.cmake
    # for more details
    add_custom_command(
        OUTPUT ${generated_outputs}
        DEPENDS ${input_files}
        COMMAND ${CMAKE_COMMAND}
          -Dproject_root_dir=${PHD_PROJECT_ROOT_DIR}
          "-Dlist_of_files='${documentation_filelist}'"
          "-Doutput_folder=${current_locale_output_folder}"
          "-Dinput_folder=${input_folder}/help/"
          -P ${PHD_PROJECT_ROOT_DIR}/cmake_modules/PHD2GenerateDocScript.cmake
        )
  endif()

  # about translation in the GUI
  if(EXISTS "${input_folder}/messages.po")

  endif()

  add_custom_target(doc_${locale}
      SOURCES
      ${input_files}
      ${target_files})
  set_target_properties(doc_${locale} PROPERTIES FOLDER "Documentation/")


endfunction()


function(generate_doc_targets)

  # global English
  generate_single_doc_targets(INPUT_FOLDER "${PHD_PROJECT_ROOT_DIR}")

  # glob all the rest
  file(GLOB all_locales "${PHD_PROJECT_ROOT_DIR}/locale/*")
  foreach(var IN LISTS all_locales)

   if(NOT (IS_DIRECTORY ${var}))
     continue()
   endif()

   get_filename_component(locale "${var}" NAME)
   message(STATUS "locale = ${var} ${locale}")
   generate_single_doc_targets(
      LOCALE ${locale}
      INPUT_FOLDER "${var}")
  endforeach()

endfunction()
