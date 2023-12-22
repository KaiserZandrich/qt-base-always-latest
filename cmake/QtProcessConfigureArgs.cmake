# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This script reads Qt configure arguments from config.opt,
# translates the arguments to CMake arguments and calls CMake.
#
# This file is to be used in CMake script mode with the following variables set:
# OPTFILE: A text file containing the options that were passed to configure
#          with one option per line.
# MODULE_ROOT: The source directory of the module to be built.
#              If empty, qtbase/top-level is assumed.
# TOP_LEVEL: TRUE, if this is a top-level build.

include(${CMAKE_CURRENT_LIST_DIR}/QtFeatureCommon.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInformation.cmake)

cmake_policy(SET CMP0007 NEW)
cmake_policy(SET CMP0057 NEW)

set(cmake_args "")
macro(push)
    list(APPEND cmake_args ${ARGN})
endmacro()

macro(pop_path_argument)
    list(POP_FRONT configure_args path)
    string(REGEX REPLACE "^\"(.*)\"$" "\\1" path "${path}")
    file(TO_CMAKE_PATH "${path}" path)
endmacro()

function(is_non_empty_valid_arg arg value)
    if(value STREQUAL "")
        message(FATAL_ERROR "Value supplied to command line option '${arg}' is empty.")
    elseif(value MATCHES "^-.*")
        message(FATAL_ERROR
                "Value supplied to command line option '${arg}' is invalid: ${value}")
    endif()
endfunction()

function(warn_in_per_repo_build arg)
    if(NOT TOP_LEVEL)
        message(WARNING "Command line option ${arg} is only effective in top-level builds")
    endif()
endfunction()

function(is_valid_qt_hex_version arg version)
    if(NOT version MATCHES "^0x[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$")
        message(FATAL_ERROR "Incorrect version ${version} specified for ${arg}")
    endif()
endfunction()

if("${MODULE_ROOT}" STREQUAL "")
    # If MODULE_ROOT is not set, assume that we want to build qtbase or top-level.
    get_filename_component(MODULE_ROOT ".." ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    set(qtbase_or_top_level_build TRUE)
else()
    # If MODULE_ROOT is passed without drive letter, we try to add it to the path.
    # The check is necessary; otherwise, `get_filename_component` returns an empty string.
    if(NOT MODULE_ROOT STREQUAL ".")
        get_filename_component(MODULE_ROOT "." REALPATH BASE_DIR "${MODULE_ROOT}")
    endif()
    set(qtbase_or_top_level_build FALSE)
endif()
set(configure_filename "configure.cmake")
set(commandline_filename "qt_cmdline.cmake")
if(TOP_LEVEL)
    get_filename_component(MODULE_ROOT "../.." ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    file(GLOB commandline_files "${MODULE_ROOT}/*/${commandline_filename}")
    if(EXISTS "${MODULE_ROOT}/${commandline_filename}")
        list(PREPEND commandline_files "${MODULE_ROOT}/${commandline_filename}")
    endif()
else()
    set(commandline_files "${MODULE_ROOT}/${commandline_filename}")
endif()
file(STRINGS "${OPTFILE}" configure_args)

# list(TRANSFORM ...) unexpectedly removes semicolon escaping in list items. So the list arguments
# seem to be broken. The 'bracket argument' suppresses this behavior. Right before forwarding
# command line arguments to the cmake call, 'bracket arguments' are replaced by escaped semicolons
# back.
list(TRANSFORM configure_args REPLACE ";" "[[;]]")

list(FILTER configure_args EXCLUDE REGEX "^[ \t]*$")
list(TRANSFORM configure_args STRIP)
unset(generator)
set(auto_detect_compiler TRUE)
set(auto_detect_generator ${qtbase_or_top_level_build})
unset(device_options)
unset(options_json_file)
set_property(GLOBAL PROPERTY UNHANDLED_ARGS "")
while(NOT "${configure_args}" STREQUAL "")
    list(POP_FRONT configure_args arg)
    if(arg STREQUAL "-cmake-generator")
        list(POP_FRONT configure_args generator)
    elseif(arg STREQUAL "-cmake-use-default-generator")
        set(auto_detect_generator FALSE)
    elseif(arg STREQUAL "-no-guess-compiler")
        set(auto_detect_compiler FALSE)
    elseif(arg STREQUAL "-list-features")
        set(list_features TRUE)
    elseif(arg MATCHES "^-h(elp)?$")
        set(display_module_help TRUE)
    elseif(arg STREQUAL "-write-options-for-conan")
        list(POP_FRONT configure_args options_json_file)
    elseif(arg STREQUAL "-skip")
        warn_in_per_repo_build("${arg}")
        list(POP_FRONT configure_args qtrepos)
        is_non_empty_valid_arg("${arg}" "${qtrepos}")
        list(TRANSFORM qtrepos REPLACE "," ";")
        foreach(qtrepo IN LISTS qtrepos)
            push("-DBUILD_${qtrepo}=OFF")
        endforeach()
    elseif(arg STREQUAL "-submodules")
        warn_in_per_repo_build("${arg}")
        list(POP_FRONT configure_args submodules)
        is_non_empty_valid_arg("${arg}" "${submodules}")
        list(TRANSFORM submodules REPLACE "," "[[;]]")
        push("-DQT_BUILD_SUBMODULES=${submodules}")
    elseif(arg STREQUAL "-qt-host-path")
        pop_path_argument()
        push("-DQT_HOST_PATH=${path}")
    elseif(arg STREQUAL "-hostdatadir")
        pop_path_argument()
        if(NOT path MATCHES "(^|/)mkspecs$")
            string(APPEND path "/mkspecs")
        endif()
        push("-DINSTALL_MKSPECSDIR=${path}")
    elseif(arg STREQUAL "-developer-build")
        set(developer_build TRUE)
        # Treat this argument as "unhandled" to process it further.
        set_property(GLOBAL APPEND PROPERTY UNHANDLED_ARGS "${arg}")
    elseif(arg STREQUAL "-cmake-file-api")
        set(cmake_file_api TRUE)
    elseif(arg STREQUAL "-no-cmake-file-api")
        set(cmake_file_api FALSE)
    elseif(arg STREQUAL "-verbose")
        list(APPEND cmake_args "--log-level=STATUS")
    elseif(arg STREQUAL "-disable-deprecated-up-to")
        list(POP_FRONT configure_args version)
        is_valid_qt_hex_version("${arg}" "${version}")
        push("-DQT_DISABLE_DEPRECATED_UP_TO=${version}")
    elseif(arg STREQUAL "--")
        # Everything after this argument will be passed to CMake verbatim.
        list(APPEND cmake_args "${configure_args}")
        break()
    else()
        set_property(GLOBAL APPEND PROPERTY UNHANDLED_ARGS "${arg}")
    endif()
endwhile()

# Read the specified manually generator value from CMake command line.
# The '-cmake-generator' argument has higher priority than CMake command line.
if(NOT generator)
    set(is_next_arg_generator_name FALSE)
    foreach(arg IN LISTS cmake_args)
        if(is_next_arg_generator_name)
            set(is_next_arg_generator_name FALSE)
            if(NOT arg MATCHES "^-.*")
                set(generator "${arg}")
                set(auto_detect_generator FALSE)
            endif()
        elseif(arg MATCHES "^-G(.*)")
            set(generator "${CMAKE_MATCH_1}")
            if(generator)
                set(auto_detect_generator FALSE)
            else()
                set(is_next_arg_generator_name TRUE)
            endif()
        endif()
    endforeach()
endif()

# Attempt to detect the generator type, either single or multi-config
if("${generator}" STREQUAL "Xcode"
    OR "${generator}" STREQUAL "Ninja Multi-Config"
    OR "${generator}" MATCHES "^Visual Studio")
    set(multi_config ON)
else()
    set(multi_config OFF)
endif()

# Tell the build system we are configuring via the configure script so we can act on that.
# The cache variable is unset at the end of configuration.
push("-DQT_INTERNAL_CALLED_FROM_CONFIGURE:BOOL=TRUE")

if(FRESH_REQUESTED)
    push("-DQT_INTERNAL_FRESH_REQUESTED:BOOL=TRUE")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
        push("--fresh")
    else()
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeCache.txt"
                            "${CMAKE_BINARY_DIR}/CMakeFiles")
    endif()
endif()

####################################################################################################
# Define functions/macros that are called in configure.cmake files
#
# Every function that's called in a configure.cmake file must be defined here.
# Most are empty stubs.
####################################################################################################

set_property(GLOBAL PROPERTY COMMANDLINE_KNOWN_FEATURES "")

function(qt_feature feature)
    cmake_parse_arguments(arg "" "PURPOSE;SECTION;" "" ${ARGN})
    set_property(GLOBAL APPEND PROPERTY COMMANDLINE_KNOWN_FEATURES "${feature}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_PURPOSE_${feature} "${arg_PURPOSE}")
    set_property(GLOBAL PROPERTY COMMANDLINE_FEATURE_SECTION_${feature} "${arg_SECTION}")
endfunction()

function(find_package)
    message(FATAL_ERROR "find_package must not be used directly in configure.cmake. "
        "Use qt_find_package or guard the call with an if(NOT QT_CONFIGURE_RUNNING) block.")
endfunction()

macro(defstub name)
    function(${name})
    endfunction()
endmacro()

defstub(qt_add_qmake_lib_dependency)
defstub(qt_config_compile_test)
defstub(qt_config_compile_test_machine_tuple)
defstub(qt_config_compile_test_x86simd)
defstub(qt_config_compiler_supports_flag_test)
defstub(qt_config_linker_supports_flag_test)
defstub(qt_configure_add_report_entry)
defstub(qt_configure_add_summary_build_mode)
defstub(qt_configure_add_summary_build_parts)
defstub(qt_configure_add_summary_build_type_and_config)
defstub(qt_configure_add_summary_entry)
defstub(qt_configure_add_summary_section)
defstub(qt_configure_end_summary_section)
defstub(qt_extra_definition)
defstub(qt_feature_config)
defstub(qt_feature_definition)
defstub(qt_find_package)
defstub(set_package_properties)
defstub(qt_qml_find_python)
defstub(qt_set01)
defstub(qt_internal_check_if_linker_is_available)

####################################################################################################
# Define functions/macros that are called in qt_cmdline.cmake files
####################################################################################################

unset(commandline_known_options)
unset(commandline_custom_handlers)
set(commandline_nr_of_prefixes 0)

macro(qt_commandline_subconfig subconfig)
    list(APPEND commandline_subconfigs "${subconfig}")
endmacro()

macro(qt_commandline_custom handler)
    list(APPEND commandline_custom_handlers ${handler})
endmacro()

function(qt_commandline_option name)
    set(options)
    set(oneValueArgs TYPE NAME VALUE)
    set(multiValueArgs VALUES MAPPING)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(commandline_known_options "${commandline_known_options};${name}" PARENT_SCOPE)
    set(commandline_option_${name} "${arg_TYPE}" PARENT_SCOPE)
    if(NOT "${arg_NAME}" STREQUAL "")
        set(commandline_option_${name}_variable "${arg_NAME}" PARENT_SCOPE)
    endif()
    if(NOT "${arg_VALUE}" STREQUAL "")
        set(commandline_option_${name}_value "${arg_VALUE}" PARENT_SCOPE)
    endif()
    if(arg_VALUES)
        set(commandline_option_${name}_values ${arg_VALUES} PARENT_SCOPE)
    elseif(arg_MAPPING)
        set(commandline_option_${name}_mapping ${arg_MAPPING} PARENT_SCOPE)
    endif()
endfunction()

function(qt_commandline_prefix arg var)
    set(idx ${commandline_nr_of_prefixes})
    set(commandline_prefix_${idx} "${arg}" "${var}" PARENT_SCOPE)
    math(EXPR n "${commandline_nr_of_prefixes} + 1")
    set(commandline_nr_of_prefixes ${n} PARENT_SCOPE)
endfunction()

# Check the following variable in configure.cmake files to guard code that is not covered by the
# stub functions above.
set(QT_CONFIGURE_RUNNING ON)


####################################################################################################
# Load qt_cmdline.cmake files
####################################################################################################

while(commandline_files)
    list(POP_FRONT commandline_files commandline_file)
    get_filename_component(commandline_file_directory "${commandline_file}" DIRECTORY)
    set(configure_file "${commandline_file_directory}/${configure_filename}")
    unset(commandline_subconfigs)
    if(EXISTS "${configure_file}")
        include("${configure_file}")
    endif()
    if(EXISTS "${commandline_file}")
        include("${commandline_file}")
    endif()

    if(commandline_subconfigs)
        list(TRANSFORM commandline_subconfigs PREPEND "${commandline_file_directory}/")
        list(TRANSFORM commandline_subconfigs APPEND "/${commandline_filename}")
        list(PREPEND commandline_files "${commandline_subconfigs}")
    endif()
endwhile()

get_property(commandline_known_features GLOBAL PROPERTY COMMANDLINE_KNOWN_FEATURES)

####################################################################################################
# Process the data from the qt_cmdline.cmake files
####################################################################################################

function(qtConfAddNote)
    message(${ARGV})
endfunction()

function(qtConfAddWarning)
    message(WARNING ${ARGV})
endfunction()

function(qtConfAddError)
    message(FATAL_ERROR ${ARGV})
endfunction()

set_property(GLOBAL PROPERTY CONFIG_INPUTS "")

function(qtConfCommandlineSetInput name val)
    if(NOT "${commandline_option_${name}_variable}" STREQUAL "")
        set(name "${commandline_option_${name}_variable}")
    endif()
    if(NOT "${INPUT_${name}}" STREQUAL "")
        set(oldval "${INPUT_${name}}")
        if("${oldval}" STREQUAL "${val}")
            qtConfAddNote("Option '${name}' with value '${val}' was specified twice")
        else()
            qtConfAddNote("Overriding option '${name}' with '${val}' (was: '${oldval}')")
        endif()
    endif()

    set_property(GLOBAL PROPERTY INPUT_${name} "${val}")
    set_property(GLOBAL APPEND PROPERTY CONFIG_INPUTS ${name})
endfunction()

function(qtConfCommandlineAppendInput name val)
    get_property(oldval GLOBAL PROPERTY INPUT_${name})
    if(NOT "${oldval}" STREQUAL "")
        string(PREPEND val "${oldval};")
    endif()
    qtConfCommandlineSetInput(${name} "${val}")
endfunction()

function(qtConfValidateValue opt val out_var)
    set(${out_var} TRUE PARENT_SCOPE)

    set(valid_values ${commandline_option_${arg}_values})
    list(LENGTH valid_values n)
    if(n EQUAL 0)
        return()
    endif()

    foreach(v ${valid_values})
        if(val STREQUAL v)
            return()
        endif()
    endforeach()

    set(${out_var} FALSE PARENT_SCOPE)
    list(JOIN valid_values " " valid_values_str)
    qtConfAddError("Invalid value '${val}' supplied to command line option '${opt}'."
        "\nAllowed values: ${valid_values_str}\n")
endfunction()

function(qt_commandline_mapped_enum_value opt key out_var)
    unset(result)
    set(mapping ${commandline_option_${opt}_mapping})
    while(mapping)
        list(POP_FRONT mapping mapping_key)
        list(POP_FRONT mapping mapping_value)
        if(mapping_key STREQUAL key)
            set(result "${mapping_value}")
            break()
        endif()
    endwhile()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qtConfHasNextCommandlineArg out_var)
    get_property(args GLOBAL PROPERTY UNHANDLED_ARGS)
    list(LENGTH args n)
    if(n GREATER 0)
        set(result TRUE)
    else()
        set(result FALSE)
    endif()
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

function(qtConfPeekNextCommandlineArg out_var)
    get_property(args GLOBAL PROPERTY UNHANDLED_ARGS)
    list(GET args 0 result)
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

function(qtConfGetNextCommandlineArg out_var)
    get_property(args GLOBAL PROPERTY UNHANDLED_ARGS)
    list(POP_FRONT args result)
    set_property(GLOBAL PROPERTY UNHANDLED_ARGS ${args})
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

function(qt_commandline_boolean arg val nextok)
    if("${val}" STREQUAL "")
        set(val "yes")
    endif()
    if(NOT val STREQUAL "yes" AND NOT val STREQUAL "no")
        message(FATAL_ERROR "Invalid value '${val}' given for boolean command line option '${arg}'.")
    endif()
    qtConfCommandlineSetInput("${arg}" "${val}")
endfunction()

function(qt_commandline_string arg val nextok)
    if(nextok)
        qtConfGetNextCommandlineArg(val)
        if("${val}" MATCHES "^-")
            qtConfAddError("No value supplied to command line options '${opt}'.")
        endif()
    endif()
    qtConfValidateValue("${opt}" "${val}" success)
    if(success)
        qtConfCommandlineSetInput("${opt}" "${val}")
    endif()
endfunction()

function(qt_commandline_optionalString arg val nextok)
    if("${val}" STREQUAL "")
        if(nextok)
            qtConfPeekNextCommandlineArg(val)
        endif()
        if(val MATCHES "^-.*|[A-Z0-9_+]=.*" OR val STREQUAL "")
            set(val "yes")
        else()
            qtConfGetNextCommandlineArg(val)
        endif()
    endif()
    qtConfValidateValue("${arg}" "${val}" success)
    if(success)
        qtConfCommandlineSetInput("${arg}" "${val}")
    endif()
endfunction()

function(qt_commandline_addString arg val nextok)
    if("${val}" STREQUAL "" AND nextok)
        qtConfGetNextCommandlineArg(val)
    endif()
    if(val MATCHES "^-.*" OR val STREQUAL "")
        qtConfAddError("No value supplied to command line option '${arg}'.")
    endif()
    qtConfValidateValue("${arg}" "${val}" success)
    if(success)
        if(DEFINED command_line_option_${arg}_variable)
            set(arg ${command_line_option_${arg}_variable})
        endif()
        set_property(GLOBAL APPEND PROPERTY "INPUT_${arg}" "${val}")
        set_property(GLOBAL APPEND PROPERTY CONFIG_INPUTS ${arg})
    endif()
endfunction()

function(qt_commandline_enum arg val nextok)
    if("${val}" STREQUAL "")
        set(val "yes")
    endif()
    unset(mapped)
    if(DEFINED "commandline_option_${arg}_mapping")
        qt_commandline_mapped_enum_value("${arg}" "${val}" mapped)
    elseif(DEFINED "commandline_option_${arg}_values")
        if(val IN_LIST commandline_option_${arg}_values)
            set(mapped ${val})
        endif()
    endif()
    if("${mapped}" STREQUAL "")
        qtConfAddError("Invalid value '${val}' supplied to command line option '${arg}'.")
    endif()
    qtConfCommandlineSetInput("${arg}" "${mapped}")
endfunction()

function(qt_commandline_void arg val nextok)
    if(NOT "${val}" STREQUAL "")
        qtConfAddError("Command line option '${arg}' expects no argument ('${val}' given).")
    endif()
    if(DEFINED commandline_option_${arg}_value)
        set(val ${commandline_option_${arg}_value})
    endif()
    if("${val}" STREQUAL "")
        set(val yes)
    endif()
    qtConfCommandlineSetInput("${arg}" "${val}")
endfunction()

function(qt_call_function func)
    set(call_code "${func}(")
    math(EXPR n "${ARGC} - 1")
    foreach(i RANGE 1 ${n})
        string(APPEND call_code "\"${ARGV${i}}\" ")
    endforeach()
    string(APPEND call_code ")")
    string(REPLACE "\\" "\\\\" call_code "${call_code}")
    if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
        set(incfile qt_tmp_func_call.cmake)
        file(WRITE "${incfile}" "${call_code}")
        include(${incfile})
        file(REMOVE "${incfile}")
    else()
        cmake_language(EVAL CODE "${call_code}")
    endif()
endfunction()

if(display_module_help)
    message([[
Options:
  -help, -h ............ Display this help screen

  -feature-<feature> ... Enable <feature>
  -no-feature-<feature>  Disable <feature> [none]
  -list-features ....... List available features. Note that some features
                         have dedicated command line options as well.
]])

    set(help_file "${MODULE_ROOT}/config_help.txt")
    if(EXISTS "${help_file}")
        file(READ "${help_file}" content)
        message("${content}")
    endif()

    return()
endif()

if(list_features)
    unset(lines)
    foreach(feature ${commandline_known_features})
        get_property(section GLOBAL PROPERTY COMMANDLINE_FEATURE_SECTION_${feature})
        get_property(purpose GLOBAL PROPERTY COMMANDLINE_FEATURE_PURPOSE_${feature})
        if(purpose)
            if(NOT "${section}" STREQUAL "")
                string(APPEND section ": ")
            endif()
            qt_configure_get_padded_string("${feature}" "${section}${purpose}" line
                PADDING_LENGTH 25 MIN_PADDING 1)
            list(APPEND lines "${line}")
        endif()
    endforeach()
    list(SORT lines)
    list(JOIN lines "\n" lines)
    message("${lines}")
    return()
endif()

function(write_options_json_file)
    if(qtbase_or_top_level_build)
        # Add options that are handled directly by this script.
        qt_commandline_option(qt-host-path TYPE string)
        qt_commandline_option(no-guess-compiler TYPE void)
    endif()

    set(indent "    ")
    set(content
        "{"
        "${indent}\"options\": {")
    string(APPEND indent "    ")
    list(LENGTH commandline_known_options commandline_known_options_length)
    set(i 1)
    foreach(opt ${commandline_known_options})
        list(APPEND content "${indent}\"${opt}\": {")
        string(APPEND indent "    ")
        list(APPEND content "${indent}\"type\": \"${commandline_option_${opt}}\",")
        if(NOT "${commandline_option_${opt}_values}" STREQUAL "")
            set(values "${commandline_option_${opt}_values}")
            list(TRANSFORM values PREPEND "\"")
            list(TRANSFORM values APPEND "\"")
            list(JOIN values ", " values)
            list(APPEND content "${indent}\"values\": [${values}]")
        elseif(NOT "${commandline_option_${opt}_mapping}" STREQUAL "")
            list(LENGTH commandline_option_${opt}_mapping last)
            math(EXPR last "${last} - 1")
            set(values "")
            list(APPEND content "${indent}\"values\": [")
            foreach(k RANGE 0 "${last}" 2)
                list(GET commandline_option_${opt}_mapping ${k} value)
                list(APPEND values ${value})
            endforeach()
            list(TRANSFORM values PREPEND "\"")
            list(TRANSFORM values APPEND "\"")
            list(JOIN values ", " values)
            list(APPEND content
                "${indent}    ${values}"
                "${indent}]")
        else()
            list(APPEND content "${indent}\"values\": []")
        endif()
        string(SUBSTRING "${indent}" 4 -1 indent)
        math(EXPR i "${i} + 1")
        if(i LESS commandline_known_options_length)
            list(APPEND content "${indent}},")
        else()
            list(APPEND content "${indent}}")
        endif()
    endforeach()
    string(SUBSTRING "${indent}" 4 -1 indent)

    set(features ${commandline_known_features})
    list(TRANSFORM features PREPEND "\"")
    list(TRANSFORM features APPEND "\"")
    list(JOIN features ", " features)

    list(APPEND content
        "${indent}},"
        "${indent}\"features\": [${features}]"
        "}")
    string(REPLACE ";" "\n" content "${content}")
    file(WRITE "${options_json_file}" "${content}")
endfunction()

if(options_json_file)
    write_options_json_file()
    return()
endif()

set(cmake_var_assignments)

while(1)
    qtConfHasNextCommandlineArg(has_next)
    if(NOT has_next)
        break()
    endif()
    qtConfGetNextCommandlineArg(arg)

    set(handled FALSE)
    foreach(func ${commandline_custom_handlers})
        qt_call_function("qt_commandline_${func}" handled "${arg}")
        if(handled)
            break()
        endif()
    endforeach()
    if(handled)
        continue()
    endif()

    # Handle variable assignments
    if(arg MATCHES "^([a-zA-Z0-9_][a-zA-Z0-9_-]*)=(.*)")
        list(APPEND cmake_var_assignments "${arg}")
        continue()
    endif()

    # parse out opt and val
    set(nextok FALSE)
    if(arg MATCHES "^--?enable-(.*)")
        set(opt "${CMAKE_MATCH_1}")
        set(val "yes")
    # Handle -no-prefix so it's not interpreted as the negation of -prefix
    elseif(arg MATCHES "-(no-prefix)")
        set(opt "${CMAKE_MATCH_1}")
        set(val "")
    elseif(arg MATCHES "^--?(disable|no)-(.*)")
        set(opt "${CMAKE_MATCH_2}")
        set(val "no")
    elseif(arg MATCHES "^--([^=]+)=(.*)")
        set(opt "${CMAKE_MATCH_1}")
        set(val "${CMAKE_MATCH_2}")
    elseif(arg MATCHES "^--(.*)")
        set(opt "${CMAKE_MATCH_1}")
        unset(val)
    elseif(arg MATCHES "^-(.*)")
        set(nextok TRUE)
        set(opt "${CMAKE_MATCH_1}")
        unset(val)
        if(NOT DEFINED commandline_option_${opt} AND opt MATCHES "(qt|system)-(.*)")
            set(opt "${CMAKE_MATCH_2}")
            set(val "${CMAKE_MATCH_1}")
        endif()
    else()
        qtConfAddError("Invalid command line parameter '${arg}'.")
    endif()

    set(type ${commandline_option_${opt}})
    if("${type}" STREQUAL "")
        # No match in the regular options, try matching the prefixes
        math(EXPR n "${commandline_nr_of_prefixes} - 1")
        foreach(i RANGE ${n})
            list(GET commandline_prefix_${i} 0 pfx)
            if(arg MATCHES "^-${pfx}(.*)")
                list(GET commandline_prefix_${i} 1 opt)
                set(val "${CMAKE_MATCH_1}")
                set(type addString)
                break()
            endif()
        endforeach()
    endif()

    # Handle builtin [-no]-feature-xxx
    if("${type}" STREQUAL "" AND opt MATCHES "^feature-(.*)")
        set(opt "${CMAKE_MATCH_1}")
        if(NOT opt IN_LIST commandline_known_features)
            qtConfAddError("Enabling/Disabling unknown feature '${opt}'.")
        endif()
        set(type boolean)
    endif()

    if("${type}" STREQUAL "")
        qtConfAddError("Unknown command line option '${arg}'.")
    endif()

    if(NOT COMMAND "qt_commandline_${type}")
        qtConfAddError("Unknown type '${type}' for command line option '${opt}'.")
    endif()
    qt_call_function("qt_commandline_${type}" "${opt}" "${val}" "${nextok}")
endwhile()

####################################################################################################
# Translate some of the INPUT_xxx values to CMake arguments
####################################################################################################

# Turn the global properties into proper variables
get_property(config_inputs GLOBAL PROPERTY CONFIG_INPUTS)
list(REMOVE_DUPLICATES config_inputs)
foreach(var ${config_inputs})
    get_property(INPUT_${var} GLOBAL PROPERTY INPUT_${var})
endforeach()

macro(drop_input name)
    list(REMOVE_ITEM config_inputs ${name})
endmacro()

macro(translate_boolean_input name cmake_var)
    if("${INPUT_${name}}" STREQUAL "yes")
        push("-D${cmake_var}=ON")
        drop_input(${name})
    elseif("${INPUT_${name}}" STREQUAL "no")
        push("-D${cmake_var}=OFF")
        drop_input(${name})
    endif()
endmacro()

macro(translate_string_input name cmake_var)
    if(DEFINED INPUT_${name})
        push("-D${cmake_var}=${INPUT_${name}}")
        drop_input(${name})
    endif()
endmacro()

macro(translate_path_input name cmake_var)
    if(DEFINED INPUT_${name})
        set(path "${INPUT_${name}}")
        string(REGEX REPLACE "^\"(.*)\"$" "\\1" path "${path}")
        file(TO_CMAKE_PATH "${path}" path)
        push("-D${cmake_var}=${path}")
        drop_input(${name})
    endif()
endmacro()

macro(translate_list_input name cmake_var)
    if(DEFINED INPUT_${name})
        list(JOIN INPUT_${name} "[[;]]" value)
        list(APPEND cmake_args "-D${cmake_var}=${value}")
        drop_input(${name})
    endif()
endmacro()

# Check whether to guess the compiler for the given language.
#
# Sets ${out_var} to FALSE if one of the following holds:
# - the environment variable ${env_var} is non-empty
# - the CMake variable ${cmake_var} is set on the command line
#
# Otherwise, ${out_var} is set to TRUE.
function(check_whether_to_guess_compiler out_var env_var cmake_var)
    set(result TRUE)
    if(NOT "$ENV{${env_var}}" STREQUAL "")
        set(result FALSE)
    else()
        set(filtered_args ${cmake_args})
        list(FILTER filtered_args INCLUDE REGEX "^(-D)?${cmake_var}=")
        if(NOT "${filtered_args}" STREQUAL "")
            set(result FALSE)
        endif()
    endif()
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

# Try to guess the mkspec from the -platform configure argument.
function(guess_compiler_from_mkspec)
    if(NOT auto_detect_compiler)
        return()
    endif()

    check_whether_to_guess_compiler(guess_c_compiler CC CMAKE_C_COMPILER)
    check_whether_to_guess_compiler(guess_cxx_compiler CXX CMAKE_CXX_COMPILER)
    if(NOT guess_c_compiler AND NOT guess_cxx_compiler)
        return()
    endif()

    string(REGEX MATCH "(^|;)-DQT_QMAKE_TARGET_MKSPEC=\([^;]+\)" m "${cmake_args}")
    set(mkspec ${CMAKE_MATCH_2})
    set(c_compiler "")
    set(cxx_compiler "")
    if(mkspec MATCHES "-clang-msvc$")
        set(c_compiler "clang-cl")
        set(cxx_compiler "clang-cl")
    elseif(mkspec MATCHES "-clang(-|$)" AND NOT mkspec MATCHES "android")
        set(c_compiler "clang")
        set(cxx_compiler "clang++")
    elseif(mkspec MATCHES "-msvc(-|$)")
        set(c_compiler "cl")
        set(cxx_compiler "cl")
    endif()
    if(guess_c_compiler AND NOT c_compiler STREQUAL "")
        push("-DCMAKE_C_COMPILER=${c_compiler}")
    endif()
    if(guess_cxx_compiler AND NOT cxx_compiler STREQUAL "")
        push("-DCMAKE_CXX_COMPILER=${cxx_compiler}")
    endif()
    if(mkspec MATCHES "-libc\\+\\+$")
        push("-DINPUT_stdlib_libcpp=ON")
    endif()
    set(cmake_args "${cmake_args}" PARENT_SCOPE)
endfunction()

function(check_qt_build_parts type)
    set(input "INPUT_${type}")
    set(buildFlag "TRUE")
    if("${type}" STREQUAL "nomake")
        set(buildFlag "FALSE")
    endif()

    list(APPEND knownParts "tests" "examples" "benchmarks" "manual-tests" "minimal-static-tests")

    foreach(part ${${input}})
        if(part IN_LIST knownParts)
            qt_feature_normalize_name("${part}" partUpperCase)
            string(TOUPPER "${partUpperCase}" partUpperCase)
            push("-DQT_BUILD_${partUpperCase}=${buildFlag}")
            continue()
        elseif("${part}" STREQUAL "tools" AND "${type}" STREQUAL "make")
            # default true ignored
            continue()
        endif()
        qtConfAddWarning("'-${type} ${part}' is not implemented yet.")
    endforeach()
    set(cmake_args "${cmake_args}" PARENT_SCOPE)
endfunction()

drop_input(commercial)
drop_input(confirm-license)
translate_boolean_input(precompile_header BUILD_WITH_PCH)
translate_boolean_input(unity_build QT_UNITY_BUILD)
translate_string_input(unity_build_batch_size QT_UNITY_BUILD_BATCH_SIZE)
translate_boolean_input(ccache QT_USE_CCACHE)
translate_boolean_input(vcpkg QT_USE_VCPKG)
translate_boolean_input(shared BUILD_SHARED_LIBS)
translate_boolean_input(warnings_are_errors WARNINGS_ARE_ERRORS)
translate_string_input(qt_namespace QT_NAMESPACE)
translate_string_input(qt_libinfix QT_LIBINFIX)
translate_string_input(qreal QT_COORD_TYPE)
translate_path_input(prefix CMAKE_INSTALL_PREFIX)
translate_path_input(extprefix CMAKE_STAGING_PREFIX)
foreach(kind bin lib archdata libexec qml data doc sysconf examples tests)
    string(TOUPPER ${kind} uc_kind)
    translate_path_input(${kind}dir INSTALL_${uc_kind}DIR)
endforeach()
translate_path_input(headerdir INSTALL_INCLUDEDIR)
translate_path_input(plugindir INSTALL_PLUGINSDIR)
translate_path_input(translationdir INSTALL_TRANSLATIONSDIR)

if(NOT "${INPUT_device}" STREQUAL "")
    push("-DQT_QMAKE_TARGET_MKSPEC=devices/${INPUT_device}")
    drop_input(device)
endif()
translate_string_input(platform QT_QMAKE_TARGET_MKSPEC)
translate_string_input(xplatform QT_QMAKE_TARGET_MKSPEC)
guess_compiler_from_mkspec()
translate_string_input(qpa_default_platform QT_QPA_DEFAULT_PLATFORM)

translate_path_input(android-sdk ANDROID_SDK_ROOT)
translate_path_input(android-ndk ANDROID_NDK_ROOT)
if(DEFINED INPUT_android-ndk-platform)
    drop_input(android-ndk-platform)
    push("-DANDROID_PLATFORM=${INPUT_android-ndk-platform}")
endif()
if(DEFINED INPUT_android-abis)
    if(INPUT_android-abis MATCHES ",")
        qtConfAddError("The -android-abis option cannot handle more than one ABI "
            "when building with CMake.")
    endif()
    translate_string_input(android-abis ANDROID_ABI)
endif()
translate_string_input(android-javac-source QT_ANDROID_JAVAC_SOURCE)
translate_string_input(android-javac-target QT_ANDROID_JAVAC_TARGET)

# FIXME: config_help.txt says -sdk should apply to macOS as well.
translate_string_input(sdk QT_UIKIT_SDK)
if(DEFINED INPUT_sdk OR (DEFINED INPUT_xplatform AND INPUT_xplatform STREQUAL "macx-ios-clang")
    OR (DEFINED INPUT_platform AND INPUT_platform STREQUAL "macx-ios-clang"))
    push("-DCMAKE_SYSTEM_NAME=iOS")
endif()

drop_input(make)
drop_input(nomake)
translate_boolean_input(install-examples-sources QT_INSTALL_EXAMPLES_SOURCES)

check_qt_build_parts(nomake)
check_qt_build_parts(make)

drop_input(debug)
drop_input(release)
drop_input(debug_and_release)
drop_input(force_debug_info)
unset(build_configs)
if(INPUT_debug)
    set(build_configs Debug)
elseif("${INPUT_debug}" STREQUAL "no")
    set(build_configs Release)
elseif(INPUT_debug_and_release)
    set(build_configs Release Debug)
endif()
if(INPUT_force_debug_info)
    list(TRANSFORM build_configs REPLACE "^Release$" "RelWithDebInfo")
endif()

# Code coverage handling
drop_input(gcov)
if(INPUT_gcov)
    if(NOT "${INPUT_coverage}" STREQUAL "")
        if(NOT "${INPUT_coverage}" STREQUAL "gcov")
        qtConfAddError("The -gcov argument is provided, but -coverage is set"
            " to ${INPUT_coverage}")
        endif()
    else()
        set(INPUT_coverage "gcov")
        list(APPEND config_inputs coverage)
    endif()
endif()
if(NOT "${INPUT_coverage}" STREQUAL "")
    if(build_configs)
        if(NOT "Debug" IN_LIST build_configs)
            qtConfAddError("The -coverage argument requires Qt configured with 'Debug' config.")
        endif()
    else()
        set(build_configs "Debug")
    endif()
endif()

list(LENGTH build_configs nr_of_build_configs)
if(nr_of_build_configs EQUAL 1 AND NOT multi_config)
    push("-DCMAKE_BUILD_TYPE=${build_configs}")
elseif(nr_of_build_configs GREATER 1 OR multi_config)
    set(multi_config ON)
    string(REPLACE ";" "[[;]]" escaped_build_configs "${build_configs}")
    # We must not use the push macro here to avoid variable expansion.
    # That would destroy our escaping.
    list(APPEND cmake_args "-DCMAKE_CONFIGURATION_TYPES=${escaped_build_configs}")
endif()

drop_input(ltcg)
if("${INPUT_ltcg}" STREQUAL "yes")
    foreach(config ${build_configs})
        string(TOUPPER "${config}" ucconfig)
        if(NOT ucconfig STREQUAL "DEBUG")
            push("-DCMAKE_INTERPROCEDURAL_OPTIMIZATION_${ucconfig}=ON")
        endif()
    endforeach()
endif()

translate_list_input(device-option QT_QMAKE_DEVICE_OPTIONS)
translate_list_input(defines QT_EXTRA_DEFINES)
translate_list_input(fpaths QT_EXTRA_FRAMEWORKPATHS)
translate_list_input(includes QT_EXTRA_INCLUDEPATHS)
translate_list_input(lpaths QT_EXTRA_LIBDIRS)
translate_list_input(rpaths QT_EXTRA_RPATHS)

if(cmake_file_api OR (developer_build AND NOT DEFINED cmake_file_api))
    foreach(file cache-v2 cmakeFiles-v1 codemodel-v2 toolchains-v1)
        file(WRITE "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/${file}" "")
    endforeach()
endif()

foreach(input ${config_inputs})
    qt_feature_normalize_name("${input}" cmake_input)
    push("-DINPUT_${cmake_input}=${INPUT_${input}}")
endforeach()

if(DEFINED INPUT_no-prefix AND DEFINED INPUT_prefix)
    qtConfAddError("Can't specify both -prefix and -no-prefix options at the same time.")
endif()

if(NOT generator AND auto_detect_generator)
    find_program(ninja ninja)
    if(ninja)
        set(generator Ninja)
        if(multi_config)
            string(APPEND generator " Multi-Config")
        endif()
    else()
        if(CMAKE_HOST_UNIX)
            set(generator "Unix Makefiles")
        elseif(CMAKE_HOST_WIN32)
            find_program(msvc_compiler cl.exe)
            if(msvc_compiler)
                set(generator "NMake Makefiles")
                find_program(jom jom)
                if(jom)
                    string(APPEND generator " JOM")
                endif()
            else()
                set(generator "MinGW Makefiles")
            endif()
        endif()
    endif()
endif()

if(multi_config
  AND NOT "${generator}" STREQUAL "Xcode"
  AND NOT "${generator}" STREQUAL "Ninja Multi-Config"
  AND NOT "${generator}" MATCHES "^Visual Studio")
    message(FATAL_ERROR "Multi-config build is only supported by Xcode, Ninja Multi-Config and \
Visual Studio generators. Current generator is \"${generator}\".
Note: Use '-cmake-generator <generator name>' option to specify the generator manually.")
endif()

if(generator)
    push(-G "${generator}")
endif()

# Add CMake variable assignments near the end to allow users to overwrite what configure sets.
foreach(arg IN LISTS cmake_var_assignments)
    push("-D${arg}")
endforeach()

push("${MODULE_ROOT}")

if(INPUT_sysroot)
    qtConfAddWarning("The -sysroot option is deprecated and no longer has any effect. "
                     "It is recommended to use a toolchain file instead, i.e., "
                     "-DCMAKE_TOOLCHAIN_FILE=<filename>. "
                     "Alternatively, you may use -DCMAKE_SYSROOT option "
                     "to pass the sysroot to CMake.\n")
endif()

# Restore the escaped semicolons in arguments that are lists
list(TRANSFORM cmake_args REPLACE "\\[\\[;\\]\\]" "\\\\;")

execute_process(COMMAND "${CMAKE_COMMAND}" ${cmake_args}
    COMMAND_ECHO STDOUT
    RESULT_VARIABLE exit_code)
if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR "CMake exited with code ${exit_code}.")
endif()
