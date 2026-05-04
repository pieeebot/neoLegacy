if(NOT INPUT_FILES)
  message(FATAL_ERROR "INPUT_FILES must be set.")
endif()

if(NOT OUTPUT_FILE)
  message(FATAL_ERROR "OUTPUT_FILE must be set.")
endif()

set(_entries "")

foreach(_file IN LISTS INPUT_FILES)
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "Input file does not exist: ${_file}")
  endif()

  file(READ "${_file}" _raw)
  string(REPLACE "\r\n" "\n" _raw "${_raw}")
  string(REPLACE "\r" "\n" _raw "${_raw}")
  string(REPLACE "\n" ";" _lines "${_raw}")

  foreach(_line IN LISTS _lines)
    if(_line MATCHES "static const int ([A-Za-z_][A-Za-z0-9_]*_Id)[ \t]*=[ \t]*([0-9]+)")
      set(_var "${CMAKE_MATCH_1}")
      set(_id  "${CMAKE_MATCH_2}")
      string(REGEX REPLACE "_Id$" "" _name "${_var}")
      if(_entries)
        string(APPEND _entries ",\n    { \"${_name}\", ${_id} }")
      else()
        set(_entries "    { \"${_name}\", ${_id} }")
      endif()
    endif()
  endforeach()
endforeach()

set(_tmp "${OUTPUT_FILE}.tmp")
file(WRITE "${_tmp}"
"#pragma once\n"
"\n"
"#include <string>\n"
"#include <unordered_map>\n"
"\n"
"inline const std::unordered_map<std::string, int> g_ItemNameMap =\n"
"{\n"
"${_entries}\n"
"};\n"
"\n"
"inline int GetItemIdByName(const std::string& name)\n"
"{\n"
"    auto it = g_ItemNameMap.find(name);\n"
"    return (it != g_ItemNameMap.end()) ? it->second : -1;\n"
"}\n"
"inline int GetItemIdByName(const std::wstring& name)\n"
"{\n"
"    return GetItemIdByName(std::string(name.begin(), name.end()));\n"
"}\n"
)

if(EXISTS "${OUTPUT_FILE}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files "${OUTPUT_FILE}" "${_tmp}"
    RESULT_VARIABLE _changed
  )
else()
  set(_changed 1)
endif()

if(_changed)
  file(RENAME "${_tmp}" "${OUTPUT_FILE}")
  message(STATUS "GenerateItemNameMap: wrote ${OUTPUT_FILE}")
else()
  file(REMOVE "${_tmp}")
  message(STATUS "GenerateItemNameMap: ${OUTPUT_FILE} is up-to-date")
endif()