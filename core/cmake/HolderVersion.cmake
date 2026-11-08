# Full checkouts use the product version. Exported core source carries a
# generated VERSION beside CMakeLists.txt instead.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../VERSION")
  set(HOLDER_CORE_VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../../VERSION")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../.git")
  message(FATAL_ERROR "Missing repository-root VERSION; refusing a stale exported version")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../VERSION")
  set(HOLDER_CORE_VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../VERSION")
else()
  message(FATAL_ERROR "Missing VERSION; prepare the standalone core source before building")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HOLDER_CORE_VERSION_FILE}")
file(READ "${HOLDER_CORE_VERSION_FILE}" HOLDER_CORE_VERSION)
string(STRIP "${HOLDER_CORE_VERSION}" HOLDER_CORE_VERSION)

if(HOLDER_CORE_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-[0-9A-Za-z-]+(\\.[0-9A-Za-z-]+)*)?(\\+[0-9A-Za-z-]+(\\.[0-9A-Za-z-]+)*)?$")
  # CMake's project(VERSION) accepts only numeric components. Keep the full
  # SemVer for Holder's public version string and expose its numeric core to
  # CMake's project/package version machinery.
  set(HOLDER_CORE_CMAKE_VERSION
      "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
  message(FATAL_ERROR
    "VERSION must contain a semantic version like 0.1.7 or 2.0.0-dev")
endif()
