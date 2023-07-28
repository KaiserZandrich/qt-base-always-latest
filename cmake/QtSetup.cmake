# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

## Set a default build type if none was specified

# Set the QT_BUILDING_QT variable so we can verify whether we are building
# Qt from source.
# Make sure not to set it when building a standalone test, otherwise
# upon reconfiguration we get an error about qt_internal_add_test
# not being found due the if(NOT QT_BUILDING_QT) check we have
# in each standalone test.
if(NOT QT_INTERNAL_IS_STANDALONE_TEST)
    set(QT_BUILDING_QT TRUE CACHE BOOL
        "When this is present and set to true, it signals that we are building Qt from source.")
endif()

# Pre-calculate the developer_build feature if it's set by the user via INPUT_developer_build
if(NOT FEATURE_developer_build AND INPUT_developer_build
        AND NOT "${INPUT_developer_build}" STREQUAL "undefined")
    set(FEATURE_developer_build ON)
endif()

# Pre-calculate the no_prefix feature if it's set by configure via INPUT_no_prefix.
# This needs to be done before qtbase/configure.cmake is processed.
if(NOT FEATURE_no_prefix AND INPUT_no_prefix
        AND NOT "${INPUT_no_prefix}" STREQUAL "undefined")
    set(FEATURE_no_prefix ON)
endif()

set(_default_build_type "Release")
if(FEATURE_developer_build)
    set(_default_build_type "Debug")
endif()

function(qt_internal_set_message_log_level out_var)
    # Decide whether output should be verbose or not.
    # Default to verbose (--log-level=STATUS) in a developer-build and
    # non-verbose (--log-level=NOTICE) otherwise.
    # If a custom CMAKE_MESSAGE_LOG_LEVEL was specified, it takes priority.
    # Passing an explicit --log-level=Foo has the highest priority.
    if(NOT CMAKE_MESSAGE_LOG_LEVEL)
        if(FEATURE_developer_build OR QT_FEATURE_developer_build)
            set(CMAKE_MESSAGE_LOG_LEVEL "STATUS")
        else()
            set(CMAKE_MESSAGE_LOG_LEVEL "NOTICE")
        endif()
        set(${out_var} "${CMAKE_MESSAGE_LOG_LEVEL}" PARENT_SCOPE)
    endif()
endfunction()
qt_internal_set_message_log_level(CMAKE_MESSAGE_LOG_LEVEL)

# Reset content of extra build internal vars for each inclusion of QtSetup.
unset(QT_EXTRA_BUILD_INTERNALS_VARS)

# Save the global property in a variable to make it available to feature conditions.
get_property(QT_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Try to detect if an explicit CMAKE_BUILD_TYPE was set by the user.
# CMake sets CMAKE_BUILD_TYPE_INIT to Debug on most Windows platforms and doesn't set
# anything for UNIXes. CMake assigns CMAKE_BUILD_TYPE_INIT to CMAKE_BUILD_TYPE during
# first project() if CMAKE_BUILD_TYPE has no previous value.
# We use extra information about the state of CMAKE_BUILD_TYPE before the first
# project() call that's set in QtAutodetect.
# STREQUAL check needs to have expanded variables because an undefined var is not equal
# to an empty defined var.
# See also qt_internal_force_set_cmake_build_type_conditionally which is used
# to set the build type when building other repos or tests.
if("${CMAKE_BUILD_TYPE}" STREQUAL "${CMAKE_BUILD_TYPE_INIT}"
    AND NOT __qt_auto_detect_cmake_build_type_before_project_call
    AND NOT __qt_build_internals_cmake_build_type
    AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${_default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${_default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE
      PROPERTY STRINGS
      "Debug" "Release" "MinSizeRel" "RelWithDebInfo") # Set the possible values for cmake-gui.
elseif(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Building for multiple configurations: ${CMAKE_CONFIGURATION_TYPES}.")
    message(STATUS "Main configuration is: ${QT_MULTI_CONFIG_FIRST_CONFIG}.")
    if(CMAKE_NINJA_MULTI_DEFAULT_BUILD_TYPE)
        message(STATUS
            "Default build configuration set to '${CMAKE_NINJA_MULTI_DEFAULT_BUILD_TYPE}'.")
    endif()
    if(CMAKE_GENERATOR STREQUAL "Ninja")
        message(FATAL_ERROR
            "It's not possible to build multiple configurations with the single config Ninja "
            "generator. Consider configuring with -G\"Ninja Multi-Config\" instead of -GNinja."
        )
    endif()
else()
    message(STATUS "CMAKE_BUILD_TYPE was set to: '${CMAKE_BUILD_TYPE}'")
endif()

# Append a config-specific postfix to library names to ensure distinct names
# in a multi-config build.
# e.g. lib/libQt6DBus_relwithdebinfo.6.3.0.dylib
# Don't apply the postfix to the first encountered release-like config, so we have at least one
# config without a postifx.
# If postfixes are set by user warn about potential issues.
function(qt_internal_setup_cmake_config_postfix)
    # Collect configuration that require postfix in Qt library names.
    if(QT_GENERATOR_IS_MULTI_CONFIG)
        set(postfix_configurations ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(postfix_configurations ${CMAKE_BUILD_TYPE})

        # Set the default postfix to empty by default for single-config builds.
        string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower)
        set(default_cmake_${build_type_lower}_postfix "")
    endif()

    # Override the generic debug postfixes above with custom debug postfixes (even in a single
    # config build) to follow the conventions we had since Qt 5.
    # e.g. lib/libQt6DBus_debug.6.3.0.dylib
    if(WIN32)
        if(MINGW)
            # On MinGW we don't have "d" suffix for debug libraries like on Linux,
            # unless we're building debug and release libraries in one go.
            if(QT_GENERATOR_IS_MULTI_CONFIG)
                set(default_cmake_debug_postfix "d")
            endif()
        else()
            set(default_cmake_debug_postfix "d")
        endif()
    elseif(APPLE)
        set(default_cmake_debug_postfix "_debug")
    endif()

    set(custom_postfix_vars "")
    set(release_configs Release RelWithDebInfo MinSizeRel)
    set(found_first_release_config FALSE)
    foreach(config_type IN LISTS postfix_configurations)
        string(TOLOWER "${config_type}" config_type_lower)
        string(TOUPPER "${config_type}" config_type_upper)
        set(postfix_var CMAKE_${config_type_upper}_POSTFIX)

        # Skip assigning postfix for the first release-like config.
        if(NOT found_first_release_config
                AND config_type IN_LIST release_configs)
            set(found_first_release_config TRUE)
            if(NOT "${${postfix_var}}" STREQUAL "")
                list(APPEND custom_postfix_vars ${postfix_var})
            endif()
            continue()
        endif()

        # Check if the default postfix is set, use '_<config_type_lower>' otherwise.
        set(default_postfix_var
            default_cmake_${config_type_lower}_postfix)
        if(NOT DEFINED ${default_postfix_var})
            set(${default_postfix_var}
                "_${config_type_lower}")
        endif()

        # If postfix is set by user avoid changing it, but save postfix variable that has
        # a non-default value for further warning.
        if("${${postfix_var}}" STREQUAL "")
            set(${postfix_var} "${${default_postfix_var}}" PARENT_SCOPE)
        elseif(NOT "${${postfix_var}}" STREQUAL "${${default_postfix_var}}")
            list(APPEND custom_postfix_vars ${postfix_var})
        endif()

        # Adjust framework postfixes accordingly
        if(APPLE)
            set(CMAKE_FRAMEWORK_MULTI_CONFIG_POSTFIX_${config_type_upper}
                "${${postfix_var}}"  PARENT_SCOPE)
        endif()
    endforeach()
    if(custom_postfix_vars)
        list(REMOVE_DUPLICATES custom_postfix_vars)
        list(JOIN custom_postfix_vars ", " postfix_vars_string)

        message(WARNING "You are using custom library postfixes: '${postfix_vars_string}' which are"
            " considered experimental and are not officially supported by Qt."
            " Expect unforeseen issues and user projects built with qmake to be broken."
        )
    endif()
endfunction()
qt_internal_setup_cmake_config_postfix()

## Position independent code:
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Does the linker support position independent code?
include(CheckPIESupported)
check_pie_supported()

# Do not relink dependent libraries when no header has changed:
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# Detect non-prefix builds: either when the qtbase install prefix is set to the binary dir
# or when a developer build is explicitly enabled and no install prefix (or staging prefix)
# is specified.
# This detection only happens when building qtbase, and later is propagated via the generated
# QtBuildInternalsExtra.cmake file.
if (PROJECT_NAME STREQUAL "QtBase" AND NOT QT_BUILD_STANDALONE_TESTS)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        # Handle both FEATURE_ and QT_FEATURE_ cases when they are specified on the command line
        # explicitly. It's possible for one to be set, but not the other, because
        # qtbase/configure.cmake is not processed by this point.
        if((FEATURE_developer_build
            OR QT_FEATURE_developer_build
            OR FEATURE_no_prefix
            OR QT_FEATURE_no_prefix
            )
            AND NOT CMAKE_STAGING_PREFIX)
            # Handle non-prefix builds by setting the CMake install prefix to point to qtbase's
            # build dir. While building another repo (like qtsvg) the CMAKE_PREFIX_PATH should be
            # set on the command line to point to the qtbase build dir.
            set(__qt_default_prefix "${QtBase_BINARY_DIR}")
        else()
            if(CMAKE_HOST_WIN32)
                set(__qt_default_prefix "C:/Qt/")
            else()
                set(__qt_default_prefix "/usr/local/")
            endif()
            string(APPEND __qt_default_prefix
                "Qt-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
        endif()
        set(CMAKE_INSTALL_PREFIX ${__qt_default_prefix} CACHE PATH
            "Install path prefix, prepended onto install directories." FORCE)
        unset(__qt_default_prefix)
    endif()
    if(CMAKE_STAGING_PREFIX)
        set(__qt_prefix "${CMAKE_STAGING_PREFIX}")
    else()
        set(__qt_prefix "${CMAKE_INSTALL_PREFIX}")
    endif()
    if(__qt_prefix STREQUAL QtBase_BINARY_DIR)
        set(__qt_will_install_value OFF)
    else()
        set(__qt_will_install_value ON)
    endif()
    set(QT_WILL_INSTALL ${__qt_will_install_value} CACHE BOOL
        "Boolean indicating if doing a Qt prefix build (vs non-prefix build)." FORCE)
    unset(__qt_prefix)
    unset(__qt_will_install_value)
endif()

# Specify the QT_SOURCE_TREE only when building qtbase. Needed by some tests when the tests are
# built as part of the project, and not standalone. For standalone tests, the value is set in
# QtBuildInternalsExtra.cmake.
if(PROJECT_NAME STREQUAL "QtBase")
    set(QT_SOURCE_TREE "${QtBase_SOURCE_DIR}" CACHE PATH
        "A path to the source tree of the previously configured QtBase project." FORCE)
endif()

# QT_INTERNAL_CONFIGURE_FROM_IDE is set to TRUE for the following known IDE applications:
# - Qt Creator, detected by QTC_RUN environment variable
# - CLion, detected by CLION_IDE environment variable
# - Visual Studio Code, detected by VSCODE_CLI environment variable
if("$ENV{QTC_RUN}" OR "$ENV{CLION_IDE}" OR "$ENV{VSCODE_CLI}")
    set(QT_INTERNAL_CONFIGURE_FROM_IDE TRUE CACHE INTERNAL "Configuring Qt Project from IDE")
else()
    set(QT_INTERNAL_CONFIGURE_FROM_IDE FALSE CACHE INTERNAL "Configuring Qt Project from IDE")
endif()

set(_qt_sync_headers_at_configure_time_default ${QT_INTERNAL_CONFIGURE_FROM_IDE})

if(FEATURE_developer_build)
    if(DEFINED QT_CMAKE_EXPORT_COMPILE_COMMANDS)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ${QT_CMAKE_EXPORT_COMPILE_COMMANDS})
    else()
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    endif()
    set(_qt_build_tests_default ON)
    set(__build_benchmarks ON)

    # Tests are not built by default with qmake for iOS and friends, and thus the overall build
    # tends to fail. Disable them by default when targeting uikit.
    if(UIKIT OR ANDROID)
        set(_qt_build_tests_default OFF)
    endif()

    # Disable benchmarks for single configuration generators which do not build
    # with release configuration.
    if (CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL Debug)
        set(__build_benchmarks OFF)
    endif()

    # Sync headers during the initial configuration of a -developer-build to facilitate code
    # navigation for code editors that use an LSP-based code model.
    set(_qt_sync_headers_at_configure_time_default TRUE)
else()
    set(_qt_build_tests_default OFF)
    set(__build_benchmarks OFF)
endif()

# Sync Qt header files at configure time
option(QT_SYNC_HEADERS_AT_CONFIGURE_TIME "Run syncqt at configure time already"
    ${_qt_sync_headers_at_configure_time_default})
unset(_qt_sync_headers_at_configure_time_default)

# In static Ninja Multi-Config builds the sync_headers dependencies(and other autogen dependencies
# are not added to '_autogen/timestamp' targets. See QTBUG-113974.
if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND NOT QT_BUILD_SHARED_LIBS)
    set(QT_SYNC_HEADERS_AT_CONFIGURE_TIME TRUE CACHE BOOL "" FORCE)
endif()

# Build Benchmarks
option(QT_BUILD_BENCHMARKS "Build Qt Benchmarks" ${__build_benchmarks})
if(QT_BUILD_BENCHMARKS)
    set(_qt_build_tests_default ON)
endif()

## Set up testing
option(QT_BUILD_TESTS "Build the testing tree." ${_qt_build_tests_default})
unset(_qt_build_tests_default)
option(QT_BUILD_TESTS_BY_DEFAULT "Should tests be built as part of the default 'all' target." ON)
if(QT_BUILD_STANDALONE_TESTS)
    # BuildInternals might have set it to OFF on initial configuration. So force it to ON when
    # building standalone tests.
    set(QT_BUILD_TESTS ON CACHE BOOL "Build the testing tree." FORCE)

    # Also force the tests to be built as part of the default build target.
    set(QT_BUILD_TESTS_BY_DEFAULT ON CACHE BOOL
        "Should tests be built as part of the default 'all' target." FORCE)
endif()
set(BUILD_TESTING ${QT_BUILD_TESTS} CACHE INTERNAL "")

if (WASM)
    set(_qt_batch_tests ON)
else()
    set(_qt_batch_tests OFF)
endif()

if(DEFINED INPUT_batch_tests)
    if (${INPUT_batch_tests})
        set(_qt_batch_tests ON)
    else()
        set(_qt_batch_tests OFF)
    endif()
endif()

option(QT_BUILD_TESTS_BATCHED "Link all tests into a single binary." ${_qt_batch_tests})

if(QT_BUILD_TESTS AND QT_BUILD_TESTS_BATCHED AND CMAKE_VERSION VERSION_LESS "3.19")
    message(FATAL_ERROR
        "Test batching requires at least CMake 3.19, due to requiring per-source "
        "TARGET_DIRECTORY assignments and DEFER calls.")
endif()

# QT_BUILD_TOOLS_WHEN_CROSSCOMPILING -> QT_FORCE_BUILD_TOOLS
# pre-6.4 compatibility flag (remove sometime in the future)
if(CMAKE_CROSSCOMPILING AND QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
    message(WARNING "QT_BUILD_TOOLS_WHEN_CROSSCOMPILING is deprecated. "
        "Please use QT_FORCE_BUILD_TOOLS instead.")
    set(QT_FORCE_BUILD_TOOLS TRUE CACHE INTERNAL "" FORCE)
endif()

# When cross-building, we don't build tools by default. Sometimes this also covers Qt apps as well.
# Like in qttools/assistant/assistant.pro, load(qt_app), which is guarded by a qtNomakeTools() call.

set(_qt_build_tools_by_default_default ON)
if(CMAKE_CROSSCOMPILING AND NOT QT_FORCE_BUILD_TOOLS)
    set(_qt_build_tools_by_default_default OFF)
endif()
option(QT_BUILD_TOOLS_BY_DEFAULT "Should tools be built as part of the default 'all' target."
       "${_qt_build_tools_by_default_default}")
unset(_qt_build_tools_by_default_default)

include(CTest)
enable_testing()

option(QT_BUILD_EXAMPLES "Build Qt examples" OFF)
option(QT_BUILD_EXAMPLES_BY_DEFAULT "Should examples be built as part of the default 'all' target." ON)

# FIXME: Support prefix builds as well QTBUG-96232
if(QT_WILL_INSTALL)
    set(_qt_build_examples_as_external OFF)
else()
    set(_qt_build_examples_as_external ON)
endif()
option(QT_BUILD_EXAMPLES_AS_EXTERNAL "Should examples be built as ExternalProjects."
       ${_qt_build_examples_as_external})
unset(_qt_build_examples_as_external)

option(QT_BUILD_MANUAL_TESTS "Build Qt manual tests" OFF)

if(WASM)
    option(QT_BUILD_MINIMAL_STATIC_TESTS "Build minimal subset of tests for static Qt builds" ON)
else()
    option(QT_BUILD_MINIMAL_STATIC_TESTS "Build minimal subset of tests for static Qt builds" OFF)
endif()

option(QT_BUILD_MINIMAL_ANDROID_MULTI_ABI_TESTS
    "Build minimal subset of tests for Android multi-ABI Qt builds" OFF)

## Path used to find host tools, either when cross-compiling or just when using the tools from
## a different host build.
set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH
    "Installed Qt host directory path, used for cross compiling.")

## Android platform settings
if(ANDROID)
    include(QtPlatformAndroid)
endif()

## qt_add_module and co.:
include(QtBuild)

## Qt Feature support:
include(QtBuildInformation)
include(QtFeature)

## Compiler optimization flags:
include(QtCompilerOptimization)

## Compiler flags:
include(QtCompilerFlags)

qt_set_language_standards()

option(QT_USE_CCACHE "Enable the use of ccache")
if(QT_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    else()
        message(FATAL_ERROR "Ccache use was requested, but the program was not found.")
    endif()
endif()

option(QT_UNITY_BUILD "Enable unity (jumbo) build")
set(QT_UNITY_BUILD_BATCH_SIZE "32" CACHE STRING "Unity build batch size")
if(QT_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE "${QT_UNITY_BUILD_BATCH_SIZE}")
endif()

option(QT_ALLOW_SYMLINK_IN_PATHS "Allows symlinks in paths." OFF)

# We need to clean up QT_FEATURE_*, but only once per configuration cycle
get_property(qt_feature_clean GLOBAL PROPERTY _qt_feature_clean)
if(NOT qt_feature_clean)
    message(STATUS "Check for feature set changes")
    set_property(GLOBAL PROPERTY _qt_feature_clean TRUE)
    foreach(feature ${QT_KNOWN_FEATURES})
        if(DEFINED "FEATURE_${feature}" AND
            NOT "${QT_FEATURE_${feature}}" STREQUAL "${FEATURE_${feature}}")
            message("    '${feature}' is changed from ${QT_FEATURE_${feature}} \
to ${FEATURE_${feature}}")
            set(dirty_build TRUE)
        endif()
        unset("QT_FEATURE_${feature}" CACHE)
    endforeach()

    set(QT_KNOWN_FEATURES "" CACHE INTERNAL "" FORCE)

    if(dirty_build)
        set_property(GLOBAL PROPERTY _qt_dirty_build TRUE)
        message(WARNING "Re-configuring in existing build folder. \
Some features will be re-evaluated automatically.")
    endif()
endif()

if(NOT QT_BUILD_EXAMPLES)
    # Disable deployment setup to avoid warnings about missing patchelf with CMake < 3.21.
    set(QT_SKIP_SETUP_DEPLOYMENT ON)
endif()

option(QT_ALLOW_DOWNLOAD "Allows files to be downloaded when building Qt." OFF)
