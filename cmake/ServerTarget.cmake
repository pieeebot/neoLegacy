# Shared CMake helper for the dedicated server executable targets.
#
# Both server flavours (vanilla "Minecraft.Server" and FourKit-enabled
# "Minecraft.Server.FourKit") share the same compile/link/asset/debugger
# settings. This helper applies them to a target after add_executable.
#
# The two targets are defined in different source subdirectories so that
# the Visual Studio generator emits their .vcxproj files into matching
# binary subdirectories (build/<dir>/Minecraft.Server/ and
# build/<dir>/Minecraft.Server.FourKit/), keeping the variant identity
# consistent across source dir, project file, and runtime output dir.

include_guard(GLOBAL)

include("${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake")

# Apply shared compile/link/asset/debugger settings to a server executable target.
function(configure_lce_server_target target)
  # compat_shims.cpp redefines internal MSVC CRT symbols which would explode
  # if included via the precompiled header. CMake source-file properties are
  # directory-scoped, so we have to apply this for the directory of every
  # target that consumes the file (one per server flavour), not just once
  # at module-include time.
  set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/Minecraft.Client/compat_shims.cpp"
    TARGET_DIRECTORY ${target}
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
  )

  # Asset / redist source paths used by setup_asset_*_copy(). Defined inside
  # the function so they are in scope when this helper is called from a
  # different CMakeLists.txt. (CMake variables set at module file scope are
  # not visible to function bodies invoked from other files.)
  set(_asset_folder_pairs
    "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/res"         "Common/res"
    "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/Media/MediaWindows64" "Common/Media/MediaWindows64"
  )
#  set(_asset_files_pairs
#    "${CMAKE_SOURCE_DIR}/Minecraft.Client/Common/Media/MediaWindows64.arc" "Common/Media/"
#  )

  set(_use_debug_4j_libs TRUE)
  if(CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(_use_debug_4j_libs FALSE)
  endif()

  target_include_directories(${target} PRIVATE
    "${CMAKE_BINARY_DIR}/generated/" # Generated BuildVer.h
    "${CMAKE_SOURCE_DIR}/Minecraft.Client/"
    "${CMAKE_SOURCE_DIR}/Minecraft.Client/${PLATFORM_NAME}/Iggy/include"
    "${CMAKE_SOURCE_DIR}/Minecraft.Server"
    "${CMAKE_SOURCE_DIR}/include/"
  )
  target_compile_definitions(${target} PRIVATE
    ${MINECRAFT_SHARED_DEFINES}
    MINECRAFT_SERVER_BUILD
  )
  target_precompile_headers(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:stdafx.h>")

  configure_compiler_target(${target})

  # Both flavours produce a binary literally named "Minecraft.Server.exe".
  # The variant identity lives in the build directory name, not the file name,
  # so end users can ship either one as a drop-in replacement. Override the
  # per-config RUNTIME_OUTPUT_DIRECTORY explicitly because the default would
  # otherwise put both targets in the same Minecraft.Server/<config>/ dir
  # (the source dir of whichever CMakeLists.txt called add_executable) and
  # ninja would complain about duplicate output rules.
  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "Minecraft.Server"
    RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_BINARY_DIR}/${target}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/${target}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${target}/Release"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
    VS_DEBUGGER_COMMAND_ARGUMENTS "-port 25565 -bind 0.0.0.0 -name DedicatedServer"
  )

  if(_use_debug_4j_libs)
    target_link_libraries(${target} PRIVATE
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
    target_link_libraries(${target} PRIVATE
      Minecraft.World
      d3d11
      dxgi
      d3dcompiler
      XInput9_1_0
      wsock32
      legacy_stdio_definitions
      "${CMAKE_SOURCE_DIR}/Minecraft.Client/${PLATFORM_NAME}/4JLibs/libs/4J_Input.lib"
      "${CMAKE_SOURCE_DIR}/Minecraft.Client/${PLATFORM_NAME}/4JLibs/libs/4J_Storage.lib"
      "${CMAKE_SOURCE_DIR}/Minecraft.Client/${PLATFORM_NAME}/4JLibs/libs/4J_Render_PC.lib"
    )
  endif()

  foreach(lib IN LISTS IGGY_LIBS)
    target_link_libraries(${target} PRIVATE "${CMAKE_SOURCE_DIR}/Minecraft.Client/${PLATFORM_NAME}/Iggy/lib/${lib}")
  endforeach()

  # Per-target asset / redist copy steps. These create helper sub-targets
  # named after the parent (e.g. AssetFileCopy_${target}) which is why each
  # server flavour has its own set in the VS Solution Explorer.
  setup_asset_folder_copy(${target} "${_asset_folder_pairs}")
  #setup_asset_file_copy(${target} "${_asset_files_pairs}")

  if(PLATFORM_NAME STREQUAL "Windows64")
    add_custom_target(AssetLocalizationCopy_${target} ALL
      COMMAND ${CMAKE_COMMAND}
        "-DCOPY_SOURCE=${CMAKE_SOURCE_DIR}/Minecraft.Client/Windows64Media/loc"
        "-DCOPY_DEST=$<TARGET_FILE_DIR:${target}>/Windows64Media/loc"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyFolderScript.cmake"
      VERBATIM
    )

    add_dependencies(${target} AssetLocalizationCopy_${target})
    set_property(TARGET AssetLocalizationCopy_${target} PROPERTY FOLDER "Build")
  endif()

  add_copyredist_target(${target})
  if(PLATFORM_NAME STREQUAL "Windows64")
    add_gamehdd_target(${target})
  endif()
endfunction()
