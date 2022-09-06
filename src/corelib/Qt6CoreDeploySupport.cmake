# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# NOTE: This code should only ever be executed in script mode. It expects to be
#       used either as part of an install(CODE) call or called by a script
#       invoked via cmake -P as a POST_BUILD step.

cmake_minimum_required(VERSION 3.16...3.21)

# This function is currently in Technical Preview.
# Its signature and behavior might change.
function(qt6_deploy_qt_conf qt_conf_absolute_path)
    set(no_value_options "")
    set(single_value_options
        PREFIX
        DOC_DIR
        HEADERS_DIR
        LIB_DIR
        LIBEXEC_DIR
        BIN_DIR
        PLUGINS_DIR
        QML_DIR
        ARCHDATA_DIR
        DATA_DIR
        TRANSLATIONS_DIR
        EXAMPLES_DIR
        TESTS_DIR
        SETTINGS_DIR
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unparsed arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT IS_ABSOLUTE "${qt_conf_absolute_path}")
        message(FATAL_ERROR
                "Given qt.conf path is not an absolute path: '${qt_conf_absolute_path}'")
    endif()

    # Only write out locations that differ from the defaults
    set(contents "[Paths]\n")
    if(arg_PREFIX)
        string(APPEND contents "Prefix = ${arg_PREFIX}\n")
    endif()
    if(arg_DOC_DIR AND NOT arg_DOC_DIR STREQUAL "doc")
        string(APPEND contents "Documentation = ${arg_DOC_DIR}\n")
    endif()
    if(arg_HEADERS_DIR AND NOT arg_HEADERS_DIR STREQUAL "include")
        string(APPEND contents "Headers = ${arg_HEADERS_DIR}\n")
    endif()
    if(arg_LIB_DIR AND NOT arg_LIB_DIR STREQUAL "lib")
        string(APPEND contents "Libraries = ${arg_LIB_DIR}\n")
    endif()

    # This one is special, the default is platform-specific
    if(arg_LIBEXEC_DIR AND
       ((WIN32 AND NOT arg_LIBEXEC_DIR STREQUAL "bin") OR
        (NOT WIN32 AND NOT arg_LIBEXEC_DIR STREQUAL "libexec")))
        string(APPEND contents "LibraryExecutables = ${arg_LIBEXEC_DIR}\n")
    endif()

    if(arg_BIN_DIR AND NOT arg_BIN_DIR STREQUAL "bin")
        string(APPEND contents "Binaries = ${arg_BIN_DIR}\n")
    endif()
    if(arg_PLUGINS_DIR AND NOT arg_PLUGINS_DIR STREQUAL "plugins")
        string(APPEND contents "Plugins = ${arg_PLUGINS_DIR}\n")
    endif()
    if(arg_QML_DIR AND NOT arg_QML_DIR STREQUAL "qml")
        string(APPEND contents "QmlImports = ${arg_QML_DIR}\n")
    endif()
    if(arg_ARCHDATA_DIR AND NOT arg_ARCHDATA_DIR STREQUAL ".")
        string(APPEND contents "ArchData = ${arg_ARCHDATA_DIR}\n")
    endif()
    if(arg_DATA_DIR AND NOT arg_DATA_DIR STREQUAL ".")
        string(APPEND contents "Data = ${arg_DATA_DIR}\n")
    endif()
    if(arg_TRANSLATIONS_DIR AND NOT arg_TRANSLATIONS_DIR STREQUAL "translations")
        string(APPEND contents "Translations = ${arg_TRANSLATIONS_DIR}\n")
    endif()
    if(arg_EXAMPLES_DIR AND NOT arg_EXAMPLES_DIR STREQUAL "examples")
        string(APPEND contents "Examples = ${arg_EXAMPLES_DIR}\n")
    endif()
    if(arg_TESTS_DIR AND NOT arg_TESTS_DIR STREQUAL "tests")
        string(APPEND contents "Tests = ${arg_TESTS_DIR}\n")
    endif()
    if(arg_SETTINGS_DIR AND NOT arg_SETTINGS_DIR STREQUAL ".")
        string(APPEND contents "Settings = ${arg_SETTINGS_DIR}\n")
    endif()

    message(STATUS "Writing ${qt_conf_absolute_path}")
    file(WRITE "${qt_conf_absolute_path}" "${contents}")
endfunction()

if(NOT __QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_deploy_qt_conf)
        if(__QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_deploy_qt_conf(${ARGV})
        else()
            message(FATAL_ERROR "qt_deploy_qt_conf() is only available in Qt 6.")
        endif()
    endfunction()
endif()

# Copied from QtCMakeHelpers.cmake
function(_qt_internal_re_escape out_var str)
    string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" regex "${str}")
    set(${out_var} ${regex} PARENT_SCOPE)
endfunction()

function(_qt_internal_generic_deployqt)
    set(no_value_options
        NO_TRANSLATIONS
        VERBOSE
    )
    set(single_value_options
        EXECUTABLE
        LIB_DIR
        PLUGINS_DIR
    )
    set(multi_value_options
        ADDITIONAL_EXECUTABLES
        ADDITIONAL_LIBRARIES
        ADDITIONAL_MODULES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(arg_VERBOSE OR __QT_DEPLOY_VERBOSE)
        set(verbose TRUE)
    endif()

    # Make input file paths absolute
    foreach(var IN ITEMS EXECUTABLE ADDITIONAL_EXECUTABLES ADDITIONAL_LIBRARIES ADDITIONAL_MODULES)
        string(PREPEND var arg_)
        set(abspaths "")
        foreach(path IN LISTS ${var})
            get_filename_component(abspath "${path}" REALPATH BASE_DIR "${QT_DEPLOY_PREFIX}")
            list(APPEND abspaths "${abspath}")
        endforeach()
        set(${var} "${abspaths}")
    endforeach()

    # We need to get the runtime dependencies of plugins too.
    list(APPEND arg_ADDITIONAL_MODULES ${__QT_DEPLOY_PLUGINS})

    set(file_args "")
    if(arg_EXECUTABLE OR arg_ADDITIONAL_EXECUTABLES)
        list(APPEND file_args EXECUTABLES ${arg_EXECUTABLE} ${arg_ADDITIONAL_EXECUTABLES})
    endif()
    if(arg_ADDITIONAL_LIBRARIES)
        list(APPEND file_args LIBRARIES ${arg_ADDITIONAL_LIBRARIES})
    endif()
    if(arg_ADDITIONAL_MODULES)
        list(APPEND file_args MODULES ${arg_ADDITIONAL_MODULES})
    endif()

    # Compile a list of regular expressions that represent the Qt installation prefixes.
    set(prefix_regexes)
    foreach(path IN LISTS __QT_DEPLOY_QT_INSTALL_PREFIX
            __QT_DEPLOY_QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
        _qt_internal_re_escape(path_rex "${path}")
        list(APPEND prefix_regexes "^${path_rex}")
    endforeach()

    # Get the runtime dependencies recursively, restricted to Qt's installation prefix.
    file(GET_RUNTIME_DEPENDENCIES
        ${file_args}
        POST_INCLUDE_REGEXES ${prefix_regexes}
        POST_EXCLUDE_REGEXES ".*"
        RESOLVED_DEPENDENCIES_VAR resolved
        UNRESOLVED_DEPENDENCIES_VAR unresolved
        CONFLICTING_DEPENDENCIES_PREFIX conflicting
    )
    if(verbose)
        message("file(GET_RUNTIME_DEPENDENCIES ${file_args})")
        foreach(file IN LISTS resolved)
            message("    resolved: ${file}")
        endforeach()
        foreach(file IN LISTS unresolved)
            message("    unresolved: ${file}")
        endforeach()
        foreach(file IN LISTS conflicting_FILENAMES)
            message("    conflicting: ${file}")
            message("    with ${conflicting_${file}}")
        endforeach()
    endif()

    # Deploy the Qt libraries.
    file(INSTALL ${resolved}
        DESTINATION "${QT_DEPLOY_PREFIX}/${arg_LIB_DIR}"
        FOLLOW_SYMLINK_CHAIN
    )

    # Deploy the Qt plugins.
    foreach(file_path IN LISTS __QT_DEPLOY_PLUGINS)
        file(RELATIVE_PATH destination
            "${__QT_DEPLOY_QT_INSTALL_PREFIX}/${__QT_DEPLOY_QT_INSTALL_PLUGINS}"
            "${file_path}"
        )
        get_filename_component(destination "${destination}" DIRECTORY)
        string(PREPEND destination "${QT_DEPLOY_PREFIX}/${arg_PLUGINS_DIR}/")
        file(INSTALL ${file_path} DESTINATION ${destination})
    endforeach()

    # Deploy translations.
    if(NOT arg_NO_TRANSLATIONS)
        qt6_deploy_translations()
    endif()
endfunction()

# This function is currently in Technical Preview.
# Its signature and behavior might change.
function(qt6_deploy_runtime_dependencies)

    if(NOT __QT_DEPLOY_TOOL)
        message(FATAL_ERROR "No Qt deploy tool available for this target platform")
    endif()

    set(no_value_options
        GENERATE_QT_CONF
        VERBOSE
        NO_OVERWRITE
        NO_APP_STORE_COMPLIANCE   # TODO: Might want a better name
        NO_TRANSLATIONS
    )
    set(single_value_options
        EXECUTABLE
        BIN_DIR
        LIB_DIR
        PLUGINS_DIR
        QML_DIR
    )
    set(multi_value_options
        # These ADDITIONAL_... options are based on what file(GET_RUNTIME_DEPENDENCIES)
        # supports. We differentiate between the types of binaries so that we keep
        # open the possibility of switching to a purely CMake implementation of
        # the deploy tool based on file(GET_RUNTIME_DEPENDENCIES) instead of the
        # individual platform-specific tools (macdeployqt, windeployqt, etc.).
        ADDITIONAL_EXECUTABLES
        ADDITIONAL_LIBRARIES
        ADDITIONAL_MODULES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unparsed arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT arg_EXECUTABLE)
        message(FATAL_ERROR "EXECUTABLE must be specified")
    endif()

    # None of these are used if the executable is a macOS app bundle
    if(NOT arg_BIN_DIR)
        set(arg_BIN_DIR "${QT_DEPLOY_BIN_DIR}")
    endif()
    if(NOT arg_LIB_DIR)
        set(arg_LIB_DIR "${QT_DEPLOY_LIB_DIR}")
    endif()
    if(NOT arg_QML_DIR)
        set(arg_QML_DIR "${QT_DEPLOY_QML_DIR}")
    endif()
    if(NOT arg_PLUGINS_DIR)
        set(arg_PLUGINS_DIR "${QT_DEPLOY_PLUGINS_DIR}")
    endif()

    # macdeployqt always writes out a qt.conf file. It will complain if one
    # already exists, so leave it to create it for us if we will be running it.
    if(__QT_DEPLOY_SYSTEM_NAME STREQUAL Darwin)
        # We might get EXECUTABLE pointing to either the actual binary under the
        # Contents/MacOS directory, or it might be pointing to the top of the
        # app bundle (i.e. the <appname>.app directory). We want the latter to
        # pass to macdeployqt.
        if(arg_EXECUTABLE MATCHES "^((.*/)?(.*).app)/Contents/MacOS/(.*)$")
            set(arg_EXECUTABLE "${CMAKE_MATCH_1}")
        endif()
    elseif(arg_GENERATE_QT_CONF)
        set(exe_dir "${QT_DEPLOY_BIN_DIR}")
        if(exe_dir STREQUAL "" OR exe_dir STREQUAL ".")
            set(exe_dir ".")
            set(prefix  ".")
        else()
            string(REPLACE "/" ";" path "${exe_dir}")
            list(LENGTH path path_count)
            string(REPEAT "../" ${path_count} rel_path)
            string(REGEX REPLACE "/+$" "" prefix "${rel_path}")
        endif()
        qt6_deploy_qt_conf("${QT_DEPLOY_PREFIX}/${exe_dir}/qt.conf"
            PREFIX "${prefix}"
            BIN_DIR "${arg_BIN_DIR}"
            LIB_DIR "${arg_LIB_DIR}"
            PLUGINS_DIR "${arg_PLUGINS_DIR}"
            QML_DIR "${arg_QML_DIR}"
        )
    endif()

    set(extra_binaries_option "")
    set(tool_options "")

    if(arg_VERBOSE OR __QT_DEPLOY_VERBOSE)
        # macdeployqt supports 0-3: 0=no output, 1=error/warn (default), 2=normal, 3=debug
        # windeployqt supports 0-2: 0=error/warn (default), 1=verbose, 2=full_verbose
        if(__QT_DEPLOY_SYSTEM_NAME STREQUAL Windows)
            list(APPEND tool_options --verbose 2)
        elseif(__QT_DEPLOY_SYSTEM_NAME STREQUAL Darwin)
            list(APPEND tool_options -verbose=3)
        else()
            list(APPEND tool_options VERBOSE)
        endif()
    endif()

    if(__QT_DEPLOY_SYSTEM_NAME STREQUAL Windows)
        list(APPEND tool_options
            --dir       .
            --libdir    "${arg_BIN_DIR}"     # NOTE: Deliberately not arg_LIB_DIR
            --plugindir "${arg_PLUGINS_DIR}"
        )
        if(NOT arg_NO_OVERWRITE)
            list(APPEND tool_options --force)
        endif()
        if(arg_NO_TRANSLATIONS)
            list(APPEND tool_options --no-translations)
        endif()
    elseif(__QT_DEPLOY_SYSTEM_NAME STREQUAL Darwin)
        set(extra_binaries_option "-executable=")
        if(NOT arg_NO_APP_STORE_COMPLIANCE)
            list(APPEND tool_options -appstore-compliant)
        endif()
        if(NOT arg_NO_OVERWRITE)
            list(APPEND tool_options -always-overwrite)
        endif()
    endif()

    # This is an internal variable. It is normally unset and is only intended
    # for debugging purposes. It may be removed at any time without warning.
    list(APPEND tool_options ${__qt_deploy_tool_extra_options})

    if(__QT_DEPLOY_TOOL STREQUAL "GRD")
        message(STATUS "Running generic Qt deploy tool on ${arg_EXECUTABLE}")

        # Forward the ADDITIONAL_* arguments.
        foreach(file_type EXECUTABLES LIBRARIES MODULES)
            if("${arg_ADDITIONAL_${file_type}}" STREQUAL "")
                continue()
            endif()
            list(APPEND tool_options ADDITIONAL_${file_type} ${arg_ADDITIONAL_${file_type}})
        endforeach()

        if(arg_NO_TRANSLATIONS)
            list(APPEND tool_options NO_TRANSLATIONS)
        endif()

        _qt_internal_generic_deployqt(
            EXECUTABLE "${arg_EXECUTABLE}"
            LIB_DIR "${arg_LIB_DIR}"
            PLUGINS_DIR "${arg_PLUGINS_DIR}"
            ${tool_options}
        )
        return()
    endif()

    # Both windeployqt and macdeployqt don't differentiate between the different
    # types of binaries, so we merge the lists and treat them all the same.
    set(additional_binaries
        ${arg_ADDITIONAL_EXECUTABLES}
        ${arg_ADDITIONAL_LIBRARIES}
        ${arg_ADDITIONAL_MODULES}
    )
    foreach(extra_binary IN LISTS additional_binaries)
        list(APPEND tool_options "${extra_binaries_option}${extra_binary}")
    endforeach()

    message(STATUS
        "Running Qt deploy tool for ${arg_EXECUTABLE} in working directory '${QT_DEPLOY_PREFIX}'")
    execute_process(
        COMMAND_ECHO STDOUT
        COMMAND "${__QT_DEPLOY_TOOL}" "${arg_EXECUTABLE}" ${tool_options}
        WORKING_DIRECTORY "${QT_DEPLOY_PREFIX}"
        RESULT_VARIABLE result
    )
    if(result)
        message(FATAL_ERROR "Executing ${__QT_DEPLOY_TOOL} failed: ${result}")
    endif()

endfunction()

if(NOT __QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_deploy_runtime_dependencies)
        if(__QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_deploy_runtime_dependencies(${ARGV})
        else()
            message(FATAL_ERROR "qt_deploy_runtime_dependencies() is only available in Qt 6.")
        endif()
    endfunction()
endif()

function(_qt_internal_show_skip_runtime_deploy_message qt_build_type_string)
    message(STATUS
        "Skipping runtime deployment steps. "
        "Support for installing runtime dependencies is not implemented for "
        "this target platform (${__QT_DEPLOY_SYSTEM_NAME}, ${qt_build_type_string})."
    )
endfunction()

# This function is currently in Technical Preview.
# Its signature and behavior might change.
function(qt6_deploy_translations)
    set(no_value_options VERBOSE)
    set(single_value_options
        LCONVERT
    )
    set(multi_value_options
        CATALOGS
        LOCALES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(verbose OFF)
    if(arg_VERBOSE OR __QT_DEPLOY_VERBOSE)
        set(verbose ON)
    endif()

    if(arg_CATALOGS)
        set(catalogs ${arg_CATALOGS})
    else()
        set(catalogs qt qtbase)

        # Find the translations that belong to the Qt modules that are used by the project.
        # "Used by the project" means just all modules that are pulled in via find_package for now.
        set(modules ${__QT_DEPLOY_ALL_MODULES_FOUND_VIA_FIND_PACKAGE})

        set(module_catalog_mapping
            "Bluetooth|Nfc" qtconnectivity
            "Help" qt_help
            "Multimedia(Widgets|QuickPrivate)?" qtmultimedia
            "Qml|Quick" qtdeclarative
            "SerialPort" qtserialport
            "WebEngine" qtwebengine
            "WebSockets" qtwebsockets
        )
        list(LENGTH module_catalog_mapping max_i)
        math(EXPR max_i "${max_i} - 1")
        foreach(module IN LISTS modules)
            foreach(i RANGE 0 ${max_i} 2)
                list(GET module_catalog_mapping ${i} module_rex)
                if(NOT module MATCHES "^${module_rex}")
                    continue()
                endif()
                math(EXPR k "${i} + 1")
                list(GET module_catalog_mapping ${k} catalog)
                list(APPEND catalogs ${catalog})
            endforeach()
        endforeach()
    endif()

    get_filename_component(qt_translations_dir "${__QT_DEPLOY_QT_INSTALL_TRANSLATIONS}" ABSOLUTE
        BASE_DIR "${__QT_DEPLOY_QT_INSTALL_PREFIX}"
    )
    set(locales ${arg_LOCALES})
    if(NOT locales)
        file(GLOB locales RELATIVE "${qt_translations_dir}" "${qt_translations_dir}/*.qm")
        list(TRANSFORM locales REPLACE "\\.qm$" "")
        list(TRANSFORM locales REPLACE "^qt_help" "qt-help")
        list(TRANSFORM locales REPLACE "^[^_]+" "")
        list(TRANSFORM locales REPLACE "^_" "")
        list(REMOVE_DUPLICATES locales)
    endif()

    # Ensure existence of the output directory.
    set(output_dir "${QT_DEPLOY_PREFIX}/${QT_DEPLOY_TRANSLATIONS_DIR}")
    if(NOT EXISTS "${output_dir}")
        file(MAKE_DIRECTORY "${output_dir}")
    endif()

    # Locate lconvert.
    if(arg_LCONVERT)
        set(lconvert "${arg_LCONVERT}")
    else()
        set(lconvert "${__QT_DEPLOY_QT_INSTALL_PREFIX}/${__QT_DEPLOY_QT_INSTALL_BINS}/lconvert")
        if(CMAKE_HOST_WIN32)
            string(APPEND lconvert ".exe")
        endif()
        if(NOT EXISTS ${lconvert})
            message(STATUS "lconvert was not found. Skipping deployment of translations.")
            return()
        endif()
    endif()

    # Find the .qm files for the selected locales
    if(verbose)
        message(STATUS "Looking for translations in ${qt_translations_dir}")
    endif()
    foreach(locale IN LISTS locales)
        set(qm_files "")
        foreach(catalog IN LISTS catalogs)
            set(qm_file "${catalog}_${locale}.qm")
            if(EXISTS "${qt_translations_dir}/${qm_file}")
                list(APPEND qm_files ${qm_file})
            endif()
        endforeach()

        if(NOT qm_files)
            message(WARNING "No translations found for requested locale '${locale}'.")
            continue()
        endif()

        if(verbose)
            foreach(qm_file IN LISTS qm_files)
                message(STATUS "found translation file: ${qm_file}")
            endforeach()
        endif()

        # Merge the .qm files into one qt_${locale}.qm file.
        set(output_file_name "qt_${locale}.qm")
        set(output_file_path "${output_dir}/${output_file_name}")
        message(STATUS "Creating: ${output_file_path}")
        set(extra_options)
        if(verbose)
            list(APPEND extra_options COMMAND_ECHO STDOUT)
        endif()
        execute_process(
            COMMAND ${lconvert} -if qm -o ${output_file_path} ${qm_files}
            WORKING_DIRECTORY ${qt_translations_dir}
            RESULT_VARIABLE process_result
            ${extra_options}
        )
        if(NOT process_result EQUAL "0")
            if(process_result MATCHES "^[0-9]+$")
                message(WARNING "lconvert failed with exit code ${process_result}.")
            else()
                message(WARNING "lconvert failed: ${process_result}.")
            endif()
        endif()
    endforeach()
endfunction()

if(NOT __QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_deploy_translations)
        if(__QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_deploy_translations(${ARGV})
        else()
            message(FATAL_ERROR "qt_deploy_translations() is only available in Qt 6.")
        endif()
    endfunction()
endif()
