include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Common.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Durango.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/ORBIS.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/PS3.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/PSVita.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Windows.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sources/Xbox360.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/CommonSources.cmake")

include("${CMAKE_SOURCE_DIR}/cmake/Utils.cmake")

# Combine all source files into a single variable for the target
# We cant use CMAKE_CONFIGURE_PRESET here as VS doesn't set it, so just rely on the folder
set(MINECRAFT_CLIENT_SOURCES
  ${MINECRAFT_CLIENT_COMMON}
  $<$<STREQUAL:${PLATFORM_NAME},Durango>:${MINECRAFT_CLIENT_DURANGO}>
  $<$<STREQUAL:${PLATFORM_NAME},Orbis>:${MINECRAFT_CLIENT_ORBIS}>
  $<$<STREQUAL:${PLATFORM_NAME},PS3>:${MINECRAFT_CLIENT_PS3}>
  $<$<STREQUAL:${PLATFORM_NAME},PSVita>:${MINECRAFT_CLIENT_PSVITA}>
  $<$<STREQUAL:${PLATFORM_NAME},Windows64>:${MINECRAFT_CLIENT_WINDOWS}>
  $<$<STREQUAL:${PLATFORM_NAME},Xbox>:${MINECRAFT_CLIENT_XBOX360}>
  ${SOURCES_COMMON}
)

add_executable(Minecraft.Client ${MINECRAFT_CLIENT_SOURCES} )

set(MINECRAFT_CLIENT_COMPILETIME_STRINGS_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}Media/strings.h")
if(PLATFORM_NAME STREQUAL "Windows64")
  set(MINECRAFT_CLIENT_COMPILETIME_STRINGS_HEADER "${CMAKE_BINARY_DIR}/generated/Windows64Media/strings.h")
  file(GLOB_RECURSE MINECRAFT_CLIENT_WINDOWS_LOCALIZATION_XML CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Windows64Media/loc/*.xml"
  )

  add_custom_command(
    OUTPUT "${MINECRAFT_CLIENT_COMPILETIME_STRINGS_HEADER}"
    COMMAND ${CMAKE_COMMAND}
      "-DXML_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/Windows64Media/loc"
      "-DOUTPUT_FILE=${MINECRAFT_CLIENT_COMPILETIME_STRINGS_HEADER}"
      -P "${CMAKE_SOURCE_DIR}/cmake/GenerateStringsHeaderFromXml.cmake"
    DEPENDS
      ${MINECRAFT_CLIENT_WINDOWS_LOCALIZATION_XML}
      "${CMAKE_SOURCE_DIR}/cmake/GenerateStringsHeaderFromXml.cmake"
    COMMENT "Generating compile-time string IDs from XML"
    VERBATIM
  )

  add_custom_target(GenerateStringsHeader_Minecraft.Client DEPENDS "${MINECRAFT_CLIENT_COMPILETIME_STRINGS_HEADER}")
  set_property(TARGET GenerateStringsHeader_Minecraft.Client PROPERTY FOLDER "Build")
  add_dependencies(Minecraft.Client GenerateStringsHeader_Minecraft.Client)
  add_dependencies(Minecraft.Client GenerateStringIdLookup)
endif()

# Only define executable on windows
if(PLATFORM_NAME STREQUAL "Windows64")
  set_target_properties(Minecraft.Client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(Minecraft.Client PRIVATE
  "${CMAKE_BINARY_DIR}/generated" # This is for the generated BuildVer.h
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}/Iggy/include"
  "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(Minecraft.Client PRIVATE
  ${MINECRAFT_SHARED_DEFINES}
)
target_precompile_headers(Minecraft.Client PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:stdafx.h>")
set_source_files_properties(compat_shims.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON) # This redefines internal MSVC CRT symbols which will cause an issue with PCH

configure_compiler_target(Minecraft.Client)

set(MINECRAFT_CLIENT_USE_4J_DEBUG_LIBS TRUE)
if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(MINECRAFT_CLIENT_USE_4J_DEBUG_LIBS FALSE)
endif()

set_target_properties(Minecraft.Client PROPERTIES
  OUTPUT_NAME "Minecraft.Client"
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Minecraft.Client>"
)

if(MINECRAFT_CLIENT_USE_4J_DEBUG_LIBS)
  target_link_libraries(Minecraft.Client PRIVATE
    Minecraft.World
    d3d11
    dxgi
    d3dcompiler
    XInput9_1_0
    wsock32
    legacy_stdio_definitions
    4JLibs.${PLATFORM_NAME}.Input
    4JLibs.${PLATFORM_NAME}.Profile
    4JLibs.${PLATFORM_NAME}.Storage
    4JLibs.${PLATFORM_NAME}.Render
  )
else()
  target_link_libraries(Minecraft.Client PRIVATE
    Minecraft.World
    d3d11
    dxgi
    d3dcompiler
    XInput9_1_0
    wsock32
    legacy_stdio_definitions
    "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}/4JLibs/libs/4J_Input.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}/4JLibs/libs/4J_Storage.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}/4JLibs/libs/4J_Render_PC.lib"
  )
endif()

# Iggy libs
foreach(lib IN LISTS IGGY_LIBS)
  target_link_libraries(Minecraft.Client PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}/Iggy/lib/${lib}")
endforeach()

# ---
# Asset / redist copy
# ---
include("${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake")
set(ASSET_FOLDER_PAIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/music"                  "music"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/Media"           "Common/Media"
  "${CMAKE_CURRENT_SOURCE_DIR}/Common/res"             "Common/res"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM_NAME}Media"  "${PLATFORM_NAME}Media"
)
setup_asset_folder_copy(Minecraft.Client "${ASSET_FOLDER_PAIRS}")

# copy prebuilt loc folder and use it lmao
if(PLATFORM_NAME STREQUAL "Windows64")
  add_custom_target(AssetLocalizationCopy_Minecraft.Client ALL
    COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:Minecraft.Client>/Common/Localization/strings.h"
    COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:Minecraft.Client>/Common/Localization/4J_strings.h"
    COMMAND ${CMAKE_COMMAND}
      "-DCOPY_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/Windows64Media/loc"
      "-DCOPY_DEST=$<TARGET_FILE_DIR:Minecraft.Client>/Common/Localization"
      -P "${CMAKE_SOURCE_DIR}/cmake/CopyFolderScript.cmake"
    COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:Minecraft.Client>/Windows64Media/strings.h"
    COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:Minecraft.Client>/Windows64Media/4J_strings.h"
    COMMENT "Copying language files into build folder..."
    VERBATIM
  )

  add_dependencies(Minecraft.Client AssetLocalizationCopy_Minecraft.Client)
  set_property(TARGET AssetLocalizationCopy_Minecraft.Client PROPERTY FOLDER "Build")
endif()

# Copy redist files
add_copyredist_target(Minecraft.Client)

# Make sure GameHDD exists on Windows
if(PLATFORM_NAME STREQUAL "Windows64")
  add_gamehdd_target(Minecraft.Client)
endif()
