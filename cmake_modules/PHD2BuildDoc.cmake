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

# global overridable variable
set(translation_build_folder_names "tmp_build_translations")
set(html_build_folder_names "tmp_build_html")
set(target_string_extraction_from_sources "extract_locales_from_sources")
set(target_copy_locales_to_source_tree "update_locales_in_source_tree")
# the default language of the application.
set(_default_locale "en_EN")


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
#    populates an output variable for a specific locale.
#
#    If the HTML files are found ("help/" subfolder) a target named `<locale>_html` is created. This target
#    generates the .hhk file and then zips the documentation files.
#    A target `<locale>_translation` generating the .mo files from a translated .po file is also created. This target
#    depends on the fact that the xgettext tools are found or not. If not, only a message indicating that the target files
#    cannot be regenerated properly is emitted.
#
#    ::
#
#     generate_single_doc_targets(
#        INPUT_FOLDER <folder>
#        [LOCALE <current_locale>]
#        [OUTPUTS <output_variable>]
#      )
#
#  * `INPUT_FOLDER` input folder. This folder should contain a `PHD2GuideHelp.hhp` file and the .html files.
#  * `LOCALE` the current locale for the documentation. Defaults to `en_EN`
#  * `OUTPUT`: the prefix used to advertise all the generated files of the current locale
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
    set(locale ${_default_locale})
  else()
    set(locale "${_local_vars_LOCALE}")
  endif()

  # about HTML files
  if(EXISTS "${input_folder}/help")

    message(STATUS "[DOC] Creating HTML target for locale ${locale}")
    set(html_input_files)
    set(html_target_files)

    if(NOT (EXISTS "${input_folder}/help/PHD2GuideHelp.hhp"))
      message(FATAL_ERROR "'${input_folder}/help/PHD2GuideHelp.hhp' file is missing")
    endif()

    # retrieves the filenames for the documentation
    list(APPEND html_input_files ${input_folder}/help/PHD2GuideHelp.hhp)
    extract_help_filenames(${input_folder}/help/PHD2GuideHelp.hhp
                           ${input_folder}/help
                           documentation_filelist)

    list(APPEND html_input_files ${documentation_filelist})

    set(current_locale_output_folder ${CMAKE_BINARY_DIR}/${html_build_folder_names}/${locale}/)
    set(generated_hhk ${current_locale_output_folder}/PHD2GuideHelp.hhk)
    set(generated_outputs
        ${generated_hhk}
        ${current_locale_output_folder}/PHD2GuideHelp.zip)

    list(APPEND html_target_files ${generated_outputs})

    # this command generates the hhk file and the zip command, see script PHD2GenerateDocScript.cmake
    # for more details
    add_custom_command(
        OUTPUT ${generated_outputs}
        DEPENDS ${html_input_files}
        COMMAND ${CMAKE_COMMAND}
          -Dproject_root_dir=${PHD_PROJECT_ROOT_DIR}
          "-Dlist_of_files='${documentation_filelist}'"
          "-Doutput_folder=${current_locale_output_folder}"
          "-Dinput_folder=${input_folder}/help/"
          -P ${PHD_PROJECT_ROOT_DIR}/cmake_modules/PHD2GenerateDocScript.cmake
        )

    add_custom_target(${locale}_html
      SOURCES
      ${html_input_files}
      ${html_target_files})
    source_group(html/ FILES ${documentation_filelist})
    source_group(generated/ FILES ${html_target_files})
    set_target_properties(${locale}_html PROPERTIES FOLDER "Documentation/")
  endif()


  # about translation in the GUI
  set(translation_input_files)
  set(translation_target_files)

  set(current_translation_output_folder ${CMAKE_BINARY_DIR}/${translation_build_folder_names}/${locale})
  if(NOT EXISTS "${current_translation_output_folder}")
    file(MAKE_DIRECTORY "${current_translation_output_folder}")
  endif()

  if("${locale}" STREQUAL "${_default_locale}")
    # for the default language, we generate the .po file from the sources
    # we generate the global .po file from the sources

    # the command that generates a global .po file
    set(_extract_locales_commands)
    if(XGETTEXT)
      if((NOT MSGFMT) OR (NOT MSGMERGE))
        message(FATAL_ERROR "Some gettext tools were not found")
      endif()

      # the generation of the main messages.mo (default language) is dependant on all the source files
      # edit: this step is now manual
      #file(GLOB all_sources
      #     "${PHD_PROJECT_ROOT_DIR}/*.cpp"
      #     "${PHD_PROJECT_ROOT_DIR}/*.h")

      # extracts the messages from the sources and merges those messages with the locale/messages.pot
      # into the build folder. This is a manual step

      set(_extract_locales_commands
          COMMAND 
            ${CMAKE_COMMAND} 
              -E make_directory
              "${current_translation_output_folder}"
          COMMAND 
            ${XGETTEXT} *.cpp *.h -C
              --from-code=CP1252
              --keyword="_"
              --keyword="wxPLURAL:1,2"
              --keyword="wxTRANSLATE"
              --output="${current_translation_output_folder}/messages.po"
          # This command replaces the proper charset in the generated file
          # (see http://savannah.gnu.org/bugs/?20923)
          COMMAND 
            ${CMAKE_COMMAND}
              -Dinput_file="${current_translation_output_folder}/messages.po"
              -P ${PHD_PROJECT_ROOT_DIR}/cmake_modules/PHD2Removegettextwarning.cmake
          COMMAND 
            ${MSGMERGE} 
              -N
              -o "${current_translation_output_folder}/messages.pot"
              "${PHD_PROJECT_ROOT_DIR}/locale/messages.pot" 
              "${current_translation_output_folder}/messages.po"
          COMMAND 
            "${CMAKE_COMMAND}" 
              -E remove 
              "${current_translation_output_folder}/messages.po"
          WORKING_DIRECTORY 
            "${PHD_PROJECT_ROOT_DIR}"
      )
      list(APPEND translation_target_files
           "${current_translation_output_folder}/messages.pot")

      # This command ensures that the temporary folder messages.pot exists
      add_custom_command(
          OUTPUT
            "${current_translation_output_folder}/messages.pot"
          COMMAND 
            ${CMAKE_COMMAND} 
              -E make_directory
              "${current_translation_output_folder}"
          COMMAND 
            "${CMAKE_COMMAND}" 
              -E copy_if_different 
              "${PHD_PROJECT_ROOT_DIR}/locale/messages.pot" 
              "${current_translation_output_folder}/messages.pot" 
          COMMENT 
            "Copying './locale/messages.pot' to the build folder"
      )

    else() # if(XGETTEXT)
      set(_extract_locales_commands
          COMMAND 
            ${CMAKE_COMMAND} 
              -E echo "Cannot extract strings: the xgettext program is not found" 
      )
      list(APPEND translation_input_files 
          "${PHD_PROJECT_ROOT_DIR}/locale/messages.pot")
    endif()

    list(APPEND translation_input_files
         "${PHD_PROJECT_ROOT_DIR}/locale/messages.pot")

    # target for extracting the strings visible on an IDE
    add_custom_target(${target_string_extraction_from_sources}
      #DEPENDS
      #  "${PHD_PROJECT_ROOT_DIR}/locale/messages.pot" 
      # the command to run in order to extract the strings
      ${_extract_locales_commands}
      SOURCES
        ${translation_input_files}
        
    )
    source_group(input/ FILES ${translation_input_files})
    source_group(generated/ FILES ${translation_target_files})
    # set_target_properties(${target_string_extraction_from_sources} PROPERTIES FOLDER "Documentation/")


    # adding a target for copying back the updated messages to the source tree
    if(NOT TARGET ${target_copy_locales_to_source_tree})
      add_custom_target(${target_copy_locales_to_source_tree}
        # the command to run in order to extract the strings
        ${_extract_locales_commands}
        COMMAND 
          "${CMAKE_COMMAND}" 
            -E copy_if_different 
            "${current_translation_output_folder}/messages.pot" 
            "${PHD_PROJECT_ROOT_DIR}/locale/messages.pot" 
        COMMENT 
          "Extracting strings and updating the source tree for file 'messages.pot'"
      )
      # set_target_properties(${target_copy_locales_to_source_tree} PROPERTIES FOLDER "Documentation/")
    endif()

  elseif(EXISTS "${input_folder}/messages.po")
    message(STATUS "[DOC] Creating translation target for locale ${locale}")

    if(NOT TARGET ${target_string_extraction_from_sources})
      message(FATAL_ERROR "Target ${target_string_extraction_from_sources} should be defined first")
    endif()

    if(NOT (EXISTS "${input_folder}/messages.po"))
      message(FATAL_ERROR "'${input_folder}/messages.po' file is missing")
    endif()

    set(_merge_commands)
    if(MSGFMT)
      # generation of the messages.po (source tree) if the messages.pot (source tree) changes

      # we put the command into a variable: this is reused for the target pointed by ${target_copy_locales_to_source_tree}
      set(_merge_commands
          COMMAND 
            ${CMAKE_COMMAND} 
              -E make_directory
              "${current_translation_output_folder}"
          COMMAND 
            ${MSGMERGE} 
              -N
              -o "${current_translation_output_folder}/messages.po"
              "${input_folder}/messages.po"
              "${CMAKE_BINARY_DIR}/${translation_build_folder_names}/${_default_locale}/messages.pot"
      )

      add_custom_command(
          OUTPUT
            "${current_translation_output_folder}/messages.po"
          DEPENDS
            "${CMAKE_BINARY_DIR}/${translation_build_folder_names}/${_default_locale}/messages.pot"
            "${input_folder}/messages.po"
          ${_merge_commands}
          COMMENT 
            "Merging 'messages.po' for locale '${locale}'"
          WORKING_DIRECTORY 
            ${current_translation_output_folder}
      )

      # generation of the messages.mo (build tree) if the messages.po (source tree) changes
      add_custom_command(
          OUTPUT
            "${current_translation_output_folder}/messages.mo"
          MAIN_DEPENDENCY
            "${current_translation_output_folder}/messages.po"
          COMMAND 
            ${MSGFMT} 
              "${input_folder}/messages.po" 
              --output-file="${current_translation_output_folder}/messages.mo"
          COMMENT 
            "Generating 'messages.mo' for locale '${locale}'"
          WORKING_DIRECTORY 
            ${current_translation_output_folder}
          )

      list(APPEND translation_input_files ${input_folder}/messages.po)
      list(APPEND translation_target_files
           ${current_translation_output_folder}/messages.po
           ${current_translation_output_folder}/messages.mo)

      # This is wx widget stuff: generation of the associated wxstd.mo
      if(EXISTS "${input_folder}/wxstd.po")
        add_custom_command(
            OUTPUT 
              "${current_translation_output_folder}/wxstd.mo"
            MAIN_DEPENDENCY 
              "${input_folder}/wxstd.po"
            COMMAND 
              ${MSGFMT} "${input_folder}/wxstd.po" --output-file="${current_translation_output_folder}/wxstd.mo"
            COMMENT 
              "Generating 'wxstd.mo' for locale '${locale}'"
            WORKING_DIRECTORY 
              ${current_translation_output_folder}
            )

        list(APPEND translation_input_files "${input_folder}/wxstd.po")
        list(APPEND translation_target_files "${current_translation_output_folder}/wxstd.mo")
      endif()
    else()
      message(STATUS "[DOC] Translation for locale ${locale}: msgfmt not found on your system, cannot generate the .mo file")
      add_custom_command(
          OUTPUT 
            "${input_folder}/messages.mo"
          DEPENDS 
            "${input_folder}/messages.po"
          COMMAND 
            ${CMAKE_COMMAND} -E echo "Cannot generate the message.mo file as msgfmt is not found on your system."
          WORKING_DIRECTORY 
            ${input_folder}
          )

      list(APPEND translation_target_files ${input_folder}/messages.mo)

      # This is wx widget stuff
      if(EXISTS "${input_folder}/wxstd.po")
        add_custom_command(
            OUTPUT
              "${input_folder}/wxstd.mo"
            DEPENDS
              "${input_folder}/wxstd.po"
            COMMAND
              ${CMAKE_COMMAND} -E echo "Cannot generate the wxstd.mo file as msgfmt is not found on your system."
            WORKING_DIRECTORY
              ${input_folder}
            )

        list(APPEND translation_input_files "${input_folder}/wxstd.po")
        list(APPEND translation_target_files "${input_folder}/wxstd.mo")
      endif()

    endif()

    add_custom_target(${locale}_translation
      SOURCES
      ${translation_input_files}
      ${translation_target_files})
    source_group(input/ FILES ${translation_input_files})
    source_group(generated/ FILES ${translation_target_files})
    set_target_properties(${locale}_translation PROPERTIES FOLDER "Documentation/")

    # Copy command for updating the source tree messages.po
    add_custom_command(
        TARGET 
          ${target_copy_locales_to_source_tree}
        POST_BUILD
          #"${current_translation_output_folder}/messages.po"
        COMMAND 
          "${CMAKE_COMMAND}" 
            -E echo "Merging 'messages.po' for locale '${locale}'"
        ${_merge_commands}
        COMMAND 
          "${CMAKE_COMMAND}" 
            -E copy_if_different 
            "${current_translation_output_folder}/messages.po" 
            "${input_folder}/messages.po"
        COMMENT 
          "Updating '${input_folder}/messages.po' from build tree changes"
        )

  endif() ## elseif(EXISTS "${input_folder}/messages.po")

endfunction()


#.rst:
# .. command:: generate_doc_targets
#
#    Generates all the targets concerning documentation and translation. The locales
#    are advertised on the variable given after the `GENERATED_LOCALES`
#
#    ::
#
#     generate_doc_targets(
#        DEFAULT_LOCALE default_locale
#        GENERATED_LOCALES all_locales_variable
#      )
#
#  * `DEFAULT_LOCALE`: the name of the default locale
#  * `GENERATED_LOCALES` all the locales, including the default one
#
function(generate_doc_targets)

  set(options)
  set(oneValueArgs GENERATED_LOCALES DEFAULT_LOCALE)
  set(multiValueArgs)
  cmake_parse_arguments(_local_vars "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # global English
  generate_single_doc_targets(INPUT_FOLDER "${PHD_PROJECT_ROOT_DIR}")
  set(DEFAULT_LOCALE "${_default_locale}")
  set(GENERATED_LOCALES "${DEFAULT_LOCALE}")

  # glob all the rest
  file(GLOB all_locales "${PHD_PROJECT_ROOT_DIR}/locale/*")
  foreach(var IN LISTS all_locales)
    if(NOT (IS_DIRECTORY ${var}))
      continue()
    endif()

    get_filename_component(locale "${var}" NAME)
    # message(STATUS "locale = ${var} ${locale}")
    generate_single_doc_targets(
      LOCALE ${locale}
      INPUT_FOLDER "${var}")
    list(APPEND GENERATED_LOCALES "${locale}")
  endforeach()

  set(${_local_vars_DEFAULT_LOCALE} "${DEFAULT_LOCALE}" PARENT_SCOPE)
  set(${_local_vars_GENERATED_LOCALES} ${GENERATED_LOCALES} PARENT_SCOPE)

endfunction()


#.rst:
# .. command:: get_zip_file
#
#    Returns the zip file of the given locale if it exists. Otherwise the
#    provided variable is emptied.
#
#    ::
#
#     get_zip_file(
#        VAR
#        LOCALE locale
#      )
#
#  * `VAR`: the variable that is filled with the name of the zip file
#  * `LOCALE` the locale
#
function(get_zip_file)

  set(options)
  set(oneValueArgs LOCALE)
  set(multiValueArgs)
  cmake_parse_arguments(_local_vars "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(locale ${_local_vars_LOCALE})

  list(GET ARGN 0 varname)
  unset(${varname} PARENT_SCOPE)

  if(TARGET ${locale}_html)
    get_target_property(current_locale_sources ${locale}_html SOURCES)
    foreach(current_source IN LISTS current_locale_sources)
      get_source_file_property(IS_generated ${current_source} GENERATED)
      if(${IS_generated})
        get_filename_component(extension ${current_source} EXT)
        if("${extension}" STREQUAL ".zip")
          set(${varname} ${current_source} PARENT_SCOPE)
          return()
        endif()
      endif()
    endforeach()
  endif()

endfunction()



#.rst:
# .. command:: get_translation_files
#
#    Returns the generated and installable translation files of the given locale if it exists.
#
#    ::
#
#     get_zip_file(
#        VAR
#        LOCALE locale
#      )
#
#  * `VAR`: the variable that is filled
#  * `LOCALE` the locale
#
function(get_translation_files)

  set(options)
  set(oneValueArgs LOCALE)
  set(multiValueArgs)
  cmake_parse_arguments(_local_vars "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(locale ${_local_vars_LOCALE})

  list(GET ARGN 0 varname)
  unset(${varname} PARENT_SCOPE)

  set(returned_list)

  if("${locale}" STREQUAL "${_default_locale}")
    set(target_name ${target_string_extraction_from_sources})
  else()
    set(target_name ${locale}_translation)
  endif()


  if(TARGET ${target_name})
    get_target_property(current_locale_sources ${target_name} SOURCES)
    foreach(current_source IN LISTS current_locale_sources)
      get_source_file_property(IS_generated ${current_source} GENERATED)
      if(${IS_generated})
        get_filename_component(extension ${current_source} EXT)
        if("${extension}" STREQUAL ".mo" OR "${extension}" STREQUAL ".po")
          list(APPEND returned_list ${current_source})
        endif()
      endif()
    endforeach()
  endif()

  set(${varname} ${returned_list} PARENT_SCOPE)
endfunction()
