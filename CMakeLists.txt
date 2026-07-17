cmake_minimum_required(VERSION 3.25)

project(
        mingw-vulkan
        VERSION 0.1.0
        LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(
        BUILD_SHARED_LIBS
        OFF
        CACHE BOOL ""
        FORCE)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

cpmaddpackage(
        NAME VulkanHeaders
        GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
        GIT_TAG vulkan-sdk-1.4.335.0
        OPTIONS
        "VULKAN_HEADERS_ENABLE_INSTALL OFF")

cpmaddpackage(
        NAME volk
        GITHUB_REPOSITORY zeux/volk
        GIT_TAG vulkan-sdk-1.4.335)

cpmaddpackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_DOCS OFF"
        "GLFW_INSTALL OFF")

cpmaddpackage(
        NAME spdlog
        GITHUB_REPOSITORY gabime/spdlog
        GIT_TAG v1.17.0
        OPTIONS
        "SPDLOG_NO_EXCEPTIONS ON")

cpmaddpackage(
        NAME VulkanMemoryAllocator
        GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG v3.4.0)

cpmaddpackage(
        NAME fastgltf
        GITHUB_REPOSITORY spnda/fastgltf
        GIT_TAG v0.9.0
        OPTIONS
        "FASTGLTF_ENABLE_TESTS OFF"
        "FASTGLTF_ENABLE_EXAMPLES OFF"
        "FASTGLTF_ENABLE_DOCS OFF"
        "FASTGLTF_ENABLE_CPP_MODULES OFF"
        "FASTGLTF_COMPILE_AS_CPP20 ON")

cpmaddpackage(
        NAME glm
        GITHUB_REPOSITORY g-truc/glm
        GIT_TAG 1.0.3
        OPTIONS
        "GLM_BUILD_TESTS OFF"
        "GLM_BUILD_INSTALL OFF")

# ------------------------------------------------------------------------------
# Slang
#
# The application is cross-compiled from Linux to Windows using MinGW.
#
# The official prebuilt Windows x86-64 package is downloaded. The application
# consumes the public Slang headers but deliberately does not link against the
# MSVC import library included in the package.
#
# At runtime, SlangLibrary loads slang-compiler.dll using LoadLibraryExW and
# resolves slang_createGlobalSession2 using GetProcAddress.
#
# The DLL filenames must be preserved. In particular, slang.dll must never be
# renamed to slang-compiler.dll because slang.dll may be a forwarding
# compatibility library.
# ------------------------------------------------------------------------------

set(SLANG_VERSION "2026.12.0.1")

set(
        SLANG_ARCHIVE_URL
        "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-windows-x86_64.tar.gz")

cpmaddpackage(
        NAME slang_binary
        URL "${SLANG_ARCHIVE_URL}"
        DOWNLOAD_ONLY YES)

if(NOT slang_binary_SOURCE_DIR)
  message(
          FATAL_ERROR
          "CPM did not provide slang_binary_SOURCE_DIR.")
endif()

message(
        STATUS
        "Slang extracted root: ${slang_binary_SOURCE_DIR}")

# The release archive can contain nested directories. Locate the required files
# by exact filename rather than assuming a fixed archive layout.

file(
        GLOB_RECURSE SLANG_PACKAGE_FILES
        LIST_DIRECTORIES FALSE
        "${slang_binary_SOURCE_DIR}/*")

set(SLANG_HEADER_CANDIDATES)
set(SLANG_COM_PTR_HEADER_CANDIDATES)
set(SLANG_COMPILER_DLL_CANDIDATES)
set(SLANG_GLSLANG_DLL_CANDIDATES)
set(SLANG_COMPATIBILITY_DLL_CANDIDATES)

foreach(slang_package_file IN LISTS SLANG_PACKAGE_FILES)
  get_filename_component(
          slang_package_filename
          "${slang_package_file}"
          NAME)

  if(slang_package_filename STREQUAL "slang.h")
    list(
            APPEND
            SLANG_HEADER_CANDIDATES
            "${slang_package_file}")

  elseif(slang_package_filename STREQUAL "slang-com-ptr.h")
    list(
            APPEND
            SLANG_COM_PTR_HEADER_CANDIDATES
            "${slang_package_file}")

  elseif(slang_package_filename STREQUAL "slang-compiler.dll")
    list(
            APPEND
            SLANG_COMPILER_DLL_CANDIDATES
            "${slang_package_file}")

  elseif(slang_package_filename STREQUAL "slang-glslang.dll")
    list(
            APPEND
            SLANG_GLSLANG_DLL_CANDIDATES
            "${slang_package_file}")

  elseif(slang_package_filename STREQUAL "slang.dll")
    list(
            APPEND
            SLANG_COMPATIBILITY_DLL_CANDIDATES
            "${slang_package_file}")
  endif()
endforeach()

# ------------------------------------------------------------------------------
# Slang headers
# ------------------------------------------------------------------------------

list(
        LENGTH
        SLANG_HEADER_CANDIDATES
        slang_header_candidate_count)

if(slang_header_candidate_count EQUAL 0)
  message(
          FATAL_ERROR
          "Could not find slang.h beneath: "
          "${slang_binary_SOURCE_DIR}")
endif()

if(NOT slang_header_candidate_count EQUAL 1)
  message(STATUS "Slang header candidates:")

  foreach(candidate IN LISTS SLANG_HEADER_CANDIDATES)
    message(STATUS "  ${candidate}")
  endforeach()

  message(
          FATAL_ERROR
          "Expected exactly one slang.h, found "
          "${slang_header_candidate_count}.")
endif()

list(
        GET
        SLANG_HEADER_CANDIDATES
        0
        SLANG_HEADER_FILE)

get_filename_component(
        SLANG_INCLUDE_DIRECTORY
        "${SLANG_HEADER_FILE}"
        DIRECTORY)

list(
        LENGTH
        SLANG_COM_PTR_HEADER_CANDIDATES
        slang_com_ptr_header_candidate_count)

if(slang_com_ptr_header_candidate_count EQUAL 0)
  message(
          FATAL_ERROR
          "Could not find slang-com-ptr.h beneath: "
          "${slang_binary_SOURCE_DIR}")
endif()

if(NOT EXISTS "${SLANG_INCLUDE_DIRECTORY}/slang-com-ptr.h")
  message(
          STATUS
          "slang.h was found at: ${SLANG_HEADER_FILE}")

  message(STATUS "slang-com-ptr.h candidates:")

  foreach(candidate IN LISTS SLANG_COM_PTR_HEADER_CANDIDATES)
    message(STATUS "  ${candidate}")
  endforeach()

  message(
          FATAL_ERROR
          "slang.h and slang-com-ptr.h are not in the same include directory.")
endif()

# ------------------------------------------------------------------------------
# Slang compiler DLL
# ------------------------------------------------------------------------------

list(
        LENGTH
        SLANG_COMPILER_DLL_CANDIDATES
        slang_compiler_dll_candidate_count)

if(slang_compiler_dll_candidate_count EQUAL 0)
  message(
          FATAL_ERROR
          "Could not find slang-compiler.dll beneath: "
          "${slang_binary_SOURCE_DIR}")
endif()

if(NOT slang_compiler_dll_candidate_count EQUAL 1)
  message(STATUS "Slang compiler DLL candidates:")

  foreach(candidate IN LISTS SLANG_COMPILER_DLL_CANDIDATES)
    message(STATUS "  ${candidate}")
  endforeach()

  message(
          FATAL_ERROR
          "Expected exactly one slang-compiler.dll, found "
          "${slang_compiler_dll_candidate_count}.")
endif()

list(
        GET
        SLANG_COMPILER_DLL_CANDIDATES
        0
        SLANG_COMPILER_DLL)

# ------------------------------------------------------------------------------
# Slang GLSLang DLL
# ------------------------------------------------------------------------------

list(
        LENGTH
        SLANG_GLSLANG_DLL_CANDIDATES
        slang_glslang_dll_candidate_count)

if(slang_glslang_dll_candidate_count EQUAL 0)
  message(
          FATAL_ERROR
          "Could not find slang-glslang.dll beneath: "
          "${slang_binary_SOURCE_DIR}")
endif()

if(NOT slang_glslang_dll_candidate_count EQUAL 1)
  message(STATUS "Slang GLSLang DLL candidates:")

  foreach(candidate IN LISTS SLANG_GLSLANG_DLL_CANDIDATES)
    message(STATUS "  ${candidate}")
  endforeach()

  message(
          FATAL_ERROR
          "Expected exactly one slang-glslang.dll, found "
          "${slang_glslang_dll_candidate_count}.")
endif()

list(
        GET
        SLANG_GLSLANG_DLL_CANDIDATES
        0
        SLANG_GLSLANG_DLL)

# The two required runtime DLLs should come from the same package directory.

get_filename_component(
        SLANG_COMPILER_DLL_DIRECTORY
        "${SLANG_COMPILER_DLL}"
        DIRECTORY)

get_filename_component(
        SLANG_GLSLANG_DLL_DIRECTORY
        "${SLANG_GLSLANG_DLL}"
        DIRECTORY)

if(NOT SLANG_COMPILER_DLL_DIRECTORY STREQUAL SLANG_GLSLANG_DLL_DIRECTORY)
  message(
          FATAL_ERROR
          "slang-compiler.dll and slang-glslang.dll were found in different "
          "directories.\n"
          "Compiler DLL directory: ${SLANG_COMPILER_DLL_DIRECTORY}\n"
          "GLSLang DLL directory: ${SLANG_GLSLANG_DLL_DIRECTORY}")
endif()

# ------------------------------------------------------------------------------
# Optional Slang compatibility DLL
# ------------------------------------------------------------------------------

set(SLANG_COMPATIBILITY_DLL)

list(
        LENGTH
        SLANG_COMPATIBILITY_DLL_CANDIDATES
        slang_compatibility_dll_candidate_count)

if(slang_compatibility_dll_candidate_count GREATER 1)
  message(STATUS "Slang compatibility DLL candidates:")

  foreach(candidate IN LISTS SLANG_COMPATIBILITY_DLL_CANDIDATES)
    message(STATUS "  ${candidate}")
  endforeach()

  message(
          FATAL_ERROR
          "Expected at most one slang.dll, found "
          "${slang_compatibility_dll_candidate_count}.")
endif()

if(slang_compatibility_dll_candidate_count EQUAL 1)
  list(
          GET
          SLANG_COMPATIBILITY_DLL_CANDIDATES
          0
          SLANG_COMPATIBILITY_DLL)
endif()

# ------------------------------------------------------------------------------
# Slang diagnostics and interface target
# ------------------------------------------------------------------------------

message(
        STATUS
        "Slang header: ${SLANG_HEADER_FILE}")

message(
        STATUS
        "Slang include directory: ${SLANG_INCLUDE_DIRECTORY}")

message(
        STATUS
        "Slang compiler DLL: ${SLANG_COMPILER_DLL}")

message(
        STATUS
        "Slang GLSLang DLL: ${SLANG_GLSLANG_DLL}")

if(SLANG_COMPATIBILITY_DLL)
  message(
          STATUS
          "Slang compatibility DLL: ${SLANG_COMPATIBILITY_DLL}")
endif()

add_library(slang_headers INTERFACE)

add_library(
        slang::headers
        ALIAS
        slang_headers)

target_include_directories(
        slang_headers
        INTERFACE
        "${SLANG_INCLUDE_DIRECTORY}")

# Do not define SLANG_STATIC.
#
# The application uses declarations from the public Slang headers but resolves
# slang_createGlobalSession2 manually from slang-compiler.dll. No Slang import
# library is linked.

# ------------------------------------------------------------------------------
# Common compile options
# ------------------------------------------------------------------------------

add_library(project_options INTERFACE)

target_compile_features(
        project_options
        INTERFACE
        cxx_std_23)

target_compile_definitions(
        project_options
        INTERFACE
        NOMINMAX
        WIN32_LEAN_AND_MEAN)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(
          project_options
          INTERFACE
          $<$<COMPILE_LANGUAGE:CXX>:-Wall>
          $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
          $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
          $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
          $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif()

if(MSVC)
  target_compile_options(
          project_options
          INTERFACE
          /W4
          /permissive-
          /EHs-c-
          /GR-)

  target_compile_definitions(
          project_options
          INTERFACE
          _HAS_EXCEPTIONS=0)
endif()

# ------------------------------------------------------------------------------
# Application
# ------------------------------------------------------------------------------

add_executable(
        mingw-vulkan
        include/allocator.hxx
        include/buffer.hxx
        include/config.hxx
        include/context.hxx
        include/device_error.hxx
        include/fly_string.hxx
        include/forward.hxx
        include/forward_target.hxx
        include/geometry_arena.hxx
        include/image.hxx
        include/image_storage.hxx
        include/load_model.hxx
        include/material_storage.hxx
        include/pipeline.hxx
        include/pipeline_storage.hxx
        include/renderer.hxx
        include/slang_compiler.hxx
        include/slang_library.hxx
        src/buffer.cxx
        src/forward_target.cxx
        src/geometry_arena.cxx
        src/image.cxx
        src/image_storage.cxx
        src/load_model.cxx
        src/logger.cxx
        src/main.cxx
        src/material_storage.cxx
        src/pipeline.cxx
        src/pipeline_storage.cxx
        src/renderer.cxx
        src/slang_compiler.cxx
        src/slang_library.cxx
        src/swapchain.cxx
        src/vma.cxx
        include/sampler_storage.hxx
        include/gpu_resource_table.hxx
        src/gpu_resource_table.cxx
        src/sampler_storage.cxx)

target_include_directories(
        mingw-vulkan
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(
        mingw-vulkan
        PRIVATE
        project_options
        volk::volk
        Vulkan::Headers
        glfw
        spdlog::spdlog
        GPUOpen::VulkanMemoryAllocator
        fastgltf::fastgltf
        glm::glm
        slang::headers)

# ------------------------------------------------------------------------------
# MinGW configuration
# ------------------------------------------------------------------------------

if(MINGW)
  target_link_options(
          mingw-vulkan
          PRIVATE
          -static-libgcc
          -static-libstdc++)

  target_link_libraries(
          mingw-vulkan
          PRIVATE
          stdc++exp)
endif()

# ------------------------------------------------------------------------------
# Runtime deployment
#
# Preserve every DLL's original filename.
#
# SlangLibrary should load:
#
#   slang-compiler.dll
#
# It must not load a renamed copy of slang.dll.
# ------------------------------------------------------------------------------

add_custom_command(
        TARGET mingw-vulkan
        POST_BUILD
        COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${SLANG_COMPILER_DLL}"
        "$<TARGET_FILE_DIR:mingw-vulkan>/slang-compiler.dll"
        COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${SLANG_GLSLANG_DLL}"
        "$<TARGET_FILE_DIR:mingw-vulkan>/slang-glslang.dll"
        COMMENT
        "Copying Slang compiler runtime"
        VERBATIM)

if(SLANG_COMPATIBILITY_DLL)
  add_custom_command(
          TARGET mingw-vulkan
          POST_BUILD
          COMMAND
          "${CMAKE_COMMAND}" -E copy_if_different
          "${SLANG_COMPATIBILITY_DLL}"
          "$<TARGET_FILE_DIR:mingw-vulkan>/slang.dll"
          COMMENT
          "Copying Slang compatibility runtime"
          VERBATIM)
endif()

# ------------------------------------------------------------------------------
# Installation
# ------------------------------------------------------------------------------

include(GNUInstallDirs)

install(
        TARGETS mingw-vulkan
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(
        FILES
        "${SLANG_COMPILER_DLL}"
        "${SLANG_GLSLANG_DLL}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(SLANG_COMPATIBILITY_DLL)
  install(
          FILES
          "${SLANG_COMPATIBILITY_DLL}"
          DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()