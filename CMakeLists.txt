cmake_minimum_required(VERSION 3.25)

project(
    mingw-vulkan
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(
    BUILD_SHARED_LIBS
    OFF
    CACHE BOOL
    "Build dependencies as shared libraries"
    FORCE
)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")

# ------------------------------------------------------------------------------
# Project options
# ------------------------------------------------------------------------------

option(
    MINGW_VULKAN_ENABLE_VALIDATION
    "Enable Vulkan validation support"
    ON
)

set(
    SLANG_VERSION
    "2026.12.0.1"
    CACHE STRING
    "Slang release version"
)

set(
    SLANG_ROOT
    ""
    CACHE PATH
    "Path to an extracted Slang installation"
)

set(
    SLANG_ARCHIVE
    ""
    CACHE FILEPATH
    "Path to a local Slang release archive"
)

# ------------------------------------------------------------------------------
# Language configuration
# ------------------------------------------------------------------------------

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

cpmaddpackage(
    NAME VulkanHeaders
    GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
    GIT_TAG vulkan-sdk-1.4.335.0
    OPTIONS
        "VULKAN_HEADERS_ENABLE_INSTALL OFF"
)

cpmaddpackage(
    NAME volk
    GITHUB_REPOSITORY zeux/volk
    GIT_TAG vulkan-sdk-1.4.335
)

cpmaddpackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.4
    OPTIONS
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_DOCS OFF"
        "GLFW_INSTALL OFF"
)

cpmaddpackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.17.0
    OPTIONS
        "SPDLOG_BUILD_ALL OFF"
        "SPDLOG_BUILD_SHARED OFF"
        "SPDLOG_BUILD_EXAMPLE OFF"
        "SPDLOG_BUILD_EXAMPLE_HO OFF"
        "SPDLOG_BUILD_TESTS OFF"
        "SPDLOG_BUILD_TESTS_HO OFF"
        "SPDLOG_BUILD_BENCH OFF"
        "SPDLOG_INSTALL OFF"
        "SPDLOG_FMT_EXTERNAL OFF"
        "SPDLOG_FMT_EXTERNAL_HO OFF"
        "SPDLOG_USE_STD_FORMAT ON"
)

cpmaddpackage(
    NAME VulkanMemoryAllocator
    GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG v3.4.0
    OPTIONS
        "VMA_BUILD_DOCUMENTATION OFF"
        "VMA_BUILD_SAMPLES OFF"
)

cpmaddpackage(
    NAME fastgltf
    GITHUB_REPOSITORY spnda/fastgltf
    GIT_TAG v0.9.0
    OPTIONS
        "FASTGLTF_ENABLE_DEPRECATED_EXT OFF"
        "FASTGLTF_ENABLE_TESTS OFF"
        "FASTGLTF_ENABLE_EXAMPLES OFF"
        "FASTGLTF_ENABLE_DOCS OFF"
)

cpmaddpackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.3
    OPTIONS
        "GLM_BUILD_TESTS OFF"
        "GLM_BUILD_INSTALL OFF"
)

# ------------------------------------------------------------------------------
# Slang binary distribution
#
# Resolution order:
#
#   1. SLANG_ROOT
#   2. SLANG_ARCHIVE
#   3. CPM network download
# ------------------------------------------------------------------------------

function(validate_slang_root root_path)
    if(NOT IS_DIRECTORY "${root_path}")
        message(
            FATAL_ERROR
            "Slang root is not a directory: ${root_path}"
        )
    endif()

    if(NOT EXISTS "${root_path}/include/slang.h")
        message(
            FATAL_ERROR
            "Slang installation does not contain include/slang.h: "
            "${root_path}"
        )
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(NOT EXISTS "${root_path}/bin/slang.dll")
            message(
                FATAL_ERROR
                "Windows Slang installation does not contain bin/slang.dll: "
                "${root_path}"
            )
        endif()
    elseif(APPLE)
        if(
            NOT EXISTS "${root_path}/lib/libslang.dylib"
            AND NOT EXISTS "${root_path}/bin/libslang.dylib"
        )
            message(
                FATAL_ERROR
                "macOS Slang installation does not contain libslang.dylib: "
                "${root_path}"
            )
        endif()
    else()
        if(
            NOT EXISTS "${root_path}/lib/libslang.so"
            AND NOT EXISTS "${root_path}/bin/libslang.so"
        )
            message(
                FATAL_ERROR
                "Linux Slang installation does not contain libslang.so: "
                "${root_path}"
            )
        endif()
    endif()
endfunction()

set(slang_root "")

if(SLANG_ROOT)
    cmake_path(
        ABSOLUTE_PATH SLANG_ROOT
        BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        NORMALIZE
        OUTPUT_VARIABLE slang_root
    )

    message(
        STATUS
        "Using extracted local Slang installation: ${slang_root}"
    )

elseif(SLANG_ARCHIVE)
    cmake_path(
        ABSOLUTE_PATH SLANG_ARCHIVE
        BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        NORMALIZE
        OUTPUT_VARIABLE slang_archive
    )

    if(NOT EXISTS "${slang_archive}")
        message(
            FATAL_ERROR
            "SLANG_ARCHIVE does not exist: ${slang_archive}"
        )
    endif()

    message(
        STATUS
        "Using local Slang archive: ${slang_archive}"
    )

    cpmaddpackage(
        NAME slang_binary
        URL "${slang_archive}"
        DOWNLOAD_ONLY YES
    )

    set(slang_root "${slang_binary_SOURCE_DIR}")

else()
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(
            slang_archive_name
            "slang-${SLANG_VERSION}-windows-x86_64.tar.gz"
        )
    elseif(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64)$")
            set(
                slang_archive_name
                "slang-${SLANG_VERSION}-macos-aarch64.tar.gz"
            )
        else()
            set(
                slang_archive_name
                "slang-${SLANG_VERSION}-macos-x86_64.tar.gz"
            )
        endif()
    else()
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64)$")
            set(
                slang_archive_name
                "slang-${SLANG_VERSION}-linux-aarch64.tar.gz"
            )
        else()
            set(
                slang_archive_name
                "slang-${SLANG_VERSION}-linux-x86_64.tar.gz"
            )
        endif()
    endif()

    string(
        CONCAT
        slang_download_url
        "https://github.com/shader-slang/slang/releases/download/"
        "v${SLANG_VERSION}/"
        "${slang_archive_name}"
    )

    message(
        STATUS
        "Downloading Slang ${SLANG_VERSION}: ${slang_archive_name}"
    )

    cpmaddpackage(
        NAME slang_binary
        URL "${slang_download_url}"
        DOWNLOAD_ONLY YES
    )

    set(slang_root "${slang_binary_SOURCE_DIR}")
endif()

validate_slang_root("${slang_root}")

set(slang_include_dir "${slang_root}/include")
set(slang_bin_dir "${slang_root}/bin")
set(slang_lib_dir "${slang_root}/lib")

add_library(slang_binary_headers INTERFACE)
add_library(slang::headers ALIAS slang_binary_headers)

target_include_directories(
    slang_binary_headers
    INTERFACE
        "${slang_include_dir}"
)

# ------------------------------------------------------------------------------
# Application sources
# ------------------------------------------------------------------------------

file(
    GLOB_RECURSE mingw_vulkan_sources
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

file(
    GLOB_RECURSE mingw_vulkan_headers
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx"
)

add_executable(
    mingw-vulkan
    ${mingw_vulkan_sources}
    ${mingw_vulkan_headers}
)

target_include_directories(
    mingw-vulkan
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(
    mingw-vulkan
    PRIVATE
        Vulkan::Headers
        volk::volk
        glfw
        spdlog::spdlog
        GPUOpen::VulkanMemoryAllocator
        fastgltf::fastgltf
        glm::glm
        slang::headers
)

# ------------------------------------------------------------------------------
# Compile definitions
# ------------------------------------------------------------------------------

target_compile_definitions(
    mingw-vulkan
    PRIVATE
        VK_NO_PROTOTYPES
        GLFW_INCLUDE_NONE
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
        SLANG_ROOT_PATH="${slang_root}"
)

if(MINGW_VULKAN_ENABLE_VALIDATION)
    target_compile_definitions(
        mingw-vulkan
        PRIVATE
            MINGW_VULKAN_ENABLE_VALIDATION=1
    )
else()
    target_compile_definitions(
        mingw-vulkan
        PRIVATE
            MINGW_VULKAN_ENABLE_VALIDATION=0
    )
endif()

# ------------------------------------------------------------------------------
# Compiler options
# ------------------------------------------------------------------------------

if(MSVC)
    target_compile_options(
        mingw-vulkan
        PRIVATE
            /W4
            /permissive-
            /Zc:preprocessor
            /Zc:__cplusplus
            /EHsc
            /GR-
    )

    target_compile_definitions(
        mingw-vulkan
        PRIVATE
            _HAS_EXCEPTIONS=1
    )
elseif(
    CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"
)
    target_compile_options(
        mingw-vulkan
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -fno-exceptions
            -fno-rtti
    )

    target_compile_definitions(
        mingw-vulkan
        PRIVATE
            _HAS_EXCEPTIONS=0
    )
endif()

if(MINGW)
    target_link_options(
        mingw-vulkan
        PRIVATE
            -static-libgcc
            -static-libstdc++
    )
endif()

# ------------------------------------------------------------------------------
# Third-party warning suppression
# ------------------------------------------------------------------------------

set(vma_source "${CMAKE_CURRENT_SOURCE_DIR}/src/vma.cxx")

if(EXISTS "${vma_source}")
    if(MSVC)
        set_source_files_properties(
            "${vma_source}"
            PROPERTIES
                COMPILE_OPTIONS "/W0"
        )
    elseif(
        CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    )
        set_source_files_properties(
            "${vma_source}"
            PROPERTIES
                COMPILE_OPTIONS "-w"
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Slang runtime deployment
# ------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    file(
        GLOB slang_runtime_files
        CONFIGURE_DEPENDS
        "${slang_bin_dir}/*.dll"
    )

    if(NOT slang_runtime_files)
        message(
            FATAL_ERROR
            "No Slang DLL files found in ${slang_bin_dir}"
        )
    endif()

    foreach(slang_runtime_file IN LISTS slang_runtime_files)
        add_custom_command(
            TARGET mingw-vulkan
            POST_BUILD
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "${slang_runtime_file}"
                "$<TARGET_FILE_DIR:mingw-vulkan>"
            VERBATIM
        )
    endforeach()

elseif(APPLE)
    file(
        GLOB slang_runtime_files
        CONFIGURE_DEPENDS
        "${slang_lib_dir}/*.dylib"
        "${slang_bin_dir}/*.dylib"
    )

    foreach(slang_runtime_file IN LISTS slang_runtime_files)
        add_custom_command(
            TARGET mingw-vulkan
            POST_BUILD
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "${slang_runtime_file}"
                "$<TARGET_FILE_DIR:mingw-vulkan>"
            VERBATIM
        )
    endforeach()

else()
    file(
        GLOB slang_runtime_files
        CONFIGURE_DEPENDS
        "${slang_lib_dir}/*.so"
        "${slang_lib_dir}/*.so.*"
        "${slang_bin_dir}/*.so"
        "${slang_bin_dir}/*.so.*"
    )

    foreach(slang_runtime_file IN LISTS slang_runtime_files)
        add_custom_command(
            TARGET mingw-vulkan
            POST_BUILD
            COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different
                "${slang_runtime_file}"
                "$<TARGET_FILE_DIR:mingw-vulkan>"
            VERBATIM
        )
    endforeach()
endif()

# ------------------------------------------------------------------------------
# Asset deployment
# ------------------------------------------------------------------------------

set(
    project_asset_directories
    assets
)

foreach(asset_directory IN LISTS project_asset_directories)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${asset_directory}")
        add_custom_command(
            TARGET mingw-vulkan
            POST_BUILD
            COMMAND
                "${CMAKE_COMMAND}" -E copy_directory_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${asset_directory}"
                "$<TARGET_FILE_DIR:mingw-vulkan>/${asset_directory}"
            VERBATIM
        )
    endif()
endforeach()

# ------------------------------------------------------------------------------
# Output configuration
# ------------------------------------------------------------------------------

set_target_properties(
    mingw-vulkan
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:mingw-vulkan>"
)

# ------------------------------------------------------------------------------
# Configuration summary
# ------------------------------------------------------------------------------

message(STATUS "Slang root: ${slang_root}")
message(STATUS "Slang include directory: ${slang_include_dir}")
message(STATUS "Slang binary directory: ${slang_bin_dir}")

if(SLANG_ROOT)
    message(STATUS "Slang source: extracted local installation")
elseif(SLANG_ARCHIVE)
    message(STATUS "Slang source: local archive")
else()
    message(STATUS "Slang source: CPM download")
endif()