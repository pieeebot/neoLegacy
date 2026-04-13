cmake_minimum_required(VERSION 3.24)

project(LCE-Revelations LANGUAGES C CXX RC ASM_MASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32 AND NOT CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "This CMake build currently supports Windows only. For cross-compilation from Linux, use the clang-cl toolchain.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Use a 64-bit generator/toolchain (x64).")
endif()

set(LCE_XWIN_CROSS_DEBUG_COMPAT OFF)
if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(LCE_XWIN_CROSS_DEBUG_COMPAT ON)
endif()

set(CMAKE_CONFIGURATION_TYPES
  "Debug"
  "Release"
  CACHE STRING "" FORCE
)

if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

function(configure_compiler_target target)
  # MSVC and compatible compilers (like Clang-cl)
  if (MSVC)
    target_compile_options(${target} PRIVATE
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:/W3>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/W0>
      $<$<COMPILE_LANGUAGE:C,CXX>:/MP>
      $<$<COMPILE_LANGUAGE:C,CXX>:/FS>
      $<$<COMPILE_LANGUAGE:C,CXX>:/GS>
      $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
      $<$<COMPILE_LANGUAGE:CXX>:/GR>
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:/Z7>
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:/Od>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/O2 /Oi /GT /GF>
    )
    target_link_options(${target} PRIVATE
      $<$<CONFIG:Debug>:/DEBUG:FULL /INCREMENTAL:NO>
    )
  endif()

  # MSVC
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${target} PRIVATE
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/GL>
    )
    target_link_options(${target} PRIVATE
      $<$<CONFIG:Release>:/LTCG:incremental>
    )
  endif()

  # Clang
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${target} PRIVATE
      $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C,CXX>>:-O0 -Wall>
      $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:-O2 -w -flto>
    )
    target_link_options(${target} PRIVATE
      $<$<CONFIG:Release>:-flto>
    )
  endif()
endfunction()


# ---
# Configuration
# ---
set(MINECRAFT_SHARED_DEFINES
  _LARGE_WORLDS
  _DEBUG_MENUS_ENABLED
  $<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:${LCE_XWIN_CROSS_DEBUG_COMPAT}>>>:_DEBUG>
  $<$<AND:$<CONFIG:Debug>,$<BOOL:${LCE_XWIN_CROSS_DEBUG_COMPAT}>>:_ITERATOR_DEBUG_LEVEL=0>
  $<$<AND:$<CONFIG:Debug>,$<BOOL:${LCE_XWIN_CROSS_DEBUG_COMPAT}>>:_HAS_ITERATOR_DEBUGGING=0>
  _CRT_NON_CONFORMING_SWPRINTFS
  _CRT_SECURE_NO_WARNINGS
  _HAS_STD_BYTE=0
)

# Add platform-specific defines
if(PLATFORM_NAME STREQUAL "Windows64")
  list(APPEND MINECRAFT_SHARED_DEFINES _WINDOWS64)
  set(IGGY_LIBS iggy_w64.lib)
endif()
list(APPEND MINECRAFT_SHARED_DEFINES ${PLATFORM_DEFINES})

# ---
# Sources
# ---
add_subdirectory(Minecraft.World)
add_subdirectory(Minecraft.Client)
if(PLATFORM_NAME STREQUAL "Windows64") # Server is only supported on Windows for now
  add_subdirectory(Minecraft.Server.FourKit)
  add_subdirectory(Minecraft.Server)
endif()

if(TARGET GenerateStringsHeader_Minecraft.Client)
  add_dependencies(Minecraft.World GenerateStringsHeader_Minecraft.Client)
endif()

# ---
# String ID lookup generation
# ---

set(STRING_ID_LOOKUP_OUTPUT
  "${CMAKE_CURRENT_BINARY_DIR}/generated/StringIdLookup.generated.inc"
)

set(STRING_ID_XML_ROOT
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Windows64Media/loc"
)

file(GLOB_RECURSE STRING_ID_XML_FILES CONFIGURE_DEPENDS
  "${STRING_ID_XML_ROOT}/*.xml"
)

set(GENERATED_STRINGS_HEADER
  "${CMAKE_BINARY_DIR}/generated/Windows64Media/strings.h"
)

add_custom_command(
  OUTPUT "${STRING_ID_LOOKUP_OUTPUT}"
  COMMAND ${CMAKE_COMMAND}
    "-DHEADER_LIST=${GENERATED_STRINGS_HEADER}"
    "-DOUTPUT_FILE=${STRING_ID_LOOKUP_OUTPUT}"
    -P "${CMAKE_SOURCE_DIR}/cmake/GenerateStringIdLookup.cmake"
  DEPENDS
    ${STRING_ID_XML_FILES}
    "${GENERATED_STRINGS_HEADER}"
    "${CMAKE_SOURCE_DIR}/cmake/GenerateStringIdLookup.cmake"
  COMMENT "Generating StringIdLookup.generated.inc"
  VERBATIM
)

add_custom_target(GenerateStringIdLookup ALL
  DEPENDS "${STRING_ID_LOOKUP_OUTPUT}"
)

if(TARGET GenerateStringsHeader_Minecraft.Client)
  add_dependencies(GenerateStringIdLookup GenerateStringsHeader_Minecraft.Client)
endif()

set_property(TARGET GenerateStringIdLookup PROPERTY FOLDER "Build")

# ---
# Build versioning
# ---

set(BUILDVER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVer.cmake")
set(BUILDVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/Common/BuildVer.h")

add_custom_target(GenerateBuildVer
  COMMAND ${CMAKE_COMMAND}
    "-DOUTPUT_FILE=${BUILDVER_OUTPUT}"
    -P "${BUILDVER_SCRIPT}"
  COMMENT "Generating BuildVer.h..."
  VERBATIM
)

add_dependencies(Minecraft.World GenerateBuildVer)
add_dependencies(Minecraft.Client GenerateBuildVer)
if(PLATFORM_NAME STREQUAL "Windows64")
  add_dependencies(Minecraft.Server GenerateBuildVer)
endif()

add_dependencies(Minecraft.Client GenerateStringIdLookup)

if(TARGET Minecraft.Server)
  add_dependencies(Minecraft.Server GenerateStringIdLookup)
endif()

target_include_directories(Minecraft.Client PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_include_directories(Minecraft.Server PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# ---
# Project organisation
# ---
# Set the startup project for Visual Studio

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Minecraft.Client)

# Setup folders for Visual Studio, just hides the build targets under a sub folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET GenerateBuildVer PROPERTY FOLDER "Build")
