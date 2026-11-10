option(HOLDER_CORE_CATCH_DISCOVER_TESTS "Discover Catch2 tests as individual CTest tests" ON)
option(HOLDER_CORE_TSAN_USE_SETARCH "Run tests through setarch -R for ThreadSanitizer on Linux" OFF)

find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
  find_package(Catch2 REQUIRED)
endif()

# Exercise unconditional C ABI callback ownership even when make_shared fails.
# Linker wrapping keeps allocation fault injection out of the main test process.
# _Znwm is operator new(size_t) in the 64-bit Itanium C++ ABI.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8
   AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_executable(holder_core_allocation_tests CApiAllocation_test.cpp)
  target_link_libraries(holder_core_allocation_tests PRIVATE Holder::Core)
  target_link_options(holder_core_allocation_tests PRIVATE "LINKER:--wrap=_Znwm")
  if(HOLDER_CORE_TSAN_USE_SETARCH)
    find_program(SETARCH_EXECUTABLE setarch REQUIRED)
    add_test(NAME "C API allocation failures release callback data"
      COMMAND ${SETARCH_EXECUTABLE} ${CMAKE_SYSTEM_PROCESSOR} -R $<TARGET_FILE:holder_core_allocation_tests>)
  else()
    add_test(NAME "C API allocation failures release callback data" COMMAND holder_core_allocation_tests)
  endif()
endif()

# Keep dependency failure injection in its own process and out of production binaries.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_executable(holder_core_crypto_failure_tests CryptoDependencyFailure_test.cpp)
  target_link_libraries(holder_core_crypto_failure_tests PRIVATE Holder::Core)
  target_link_options(holder_core_crypto_failure_tests PRIVATE
    "LINKER:--wrap=sodium_init" "LINKER:--wrap=crypto_aead_xchacha20poly1305_ietf_encrypt")
  target_compile_definitions(holder_core_crypto_failure_tests PRIVATE
    SCHEMA_SQL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../schema/schema.sql")
  if(TARGET Catch2::Catch2)
    target_sources(holder_core_crypto_failure_tests PRIVATE catch_main.cpp)
    target_link_libraries(holder_core_crypto_failure_tests PRIVATE Catch2::Catch2)
  elseif(TARGET Catch2::Catch2WithMain)
    target_link_libraries(holder_core_crypto_failure_tests PRIVATE Catch2::Catch2WithMain)
  endif()
  if(HOLDER_CORE_TSAN_USE_SETARCH)
    find_program(SETARCH_EXECUTABLE setarch REQUIRED)
    add_test(NAME "Crypto dependency failures preserve recovery state"
      COMMAND ${SETARCH_EXECUTABLE} ${CMAKE_SYSTEM_PROCESSOR} -R $<TARGET_FILE:holder_core_crypto_failure_tests>)
  else()
    add_test(NAME "Crypto dependency failures preserve recovery state" COMMAND holder_core_crypto_failure_tests)
  endif()
endif()

# Linker wrapping sees std::mutex's pthread calls when libstdc++ defines lock inline.
# Keep this fault-injection runner out of other standard-library implementations.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("#include <mutex>
#ifndef __GLIBCXX__
#error This test requires libstdc++ inline mutex locking
#endif
int main() { return 0; }" HOLDER_CORE_TEST_HAS_LIBSTDCXX_MUTEX)
  if(HOLDER_CORE_TEST_HAS_LIBSTDCXX_MUTEX)
    add_executable(holder_core_host_registration_failure_tests HostRegistrationFailure_test.cpp)
    target_link_libraries(holder_core_host_registration_failure_tests PRIVATE Holder::Core)
    target_link_options(holder_core_host_registration_failure_tests PRIVATE "LINKER:--wrap=pthread_mutex_lock")
    if(TARGET Catch2::Catch2)
      target_sources(holder_core_host_registration_failure_tests PRIVATE catch_main.cpp)
      target_link_libraries(holder_core_host_registration_failure_tests PRIVATE Catch2::Catch2)
    elseif(TARGET Catch2::Catch2WithMain)
      target_link_libraries(holder_core_host_registration_failure_tests PRIVATE Catch2::Catch2WithMain)
    endif()
    if(HOLDER_CORE_TSAN_USE_SETARCH)
      find_program(SETARCH_EXECUTABLE setarch REQUIRED)
      add_test(NAME "Host registration lock failures preserve provider ownership"
        COMMAND ${SETARCH_EXECUTABLE} ${CMAKE_SYSTEM_PROCESSOR} -R $<TARGET_FILE:holder_core_host_registration_failure_tests>)
    else()
      add_test(NAME "Host registration lock failures preserve provider ownership" COMMAND holder_core_host_registration_failure_tests)
    endif()
  endif()
endif()

# Isolate allocation failures; YAML::Load wrapping needs the GNU C++11 string ABI.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8
   AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("#include <string>
#if !defined(__GLIBCXX__) || !_GLIBCXX_USE_CXX11_ABI
#error This test requires the GNU C++11 string ABI
#endif
int main() { return 0; }" HOLDER_CORE_TEST_HAS_GNU_STRING_ABI)
  if(HOLDER_CORE_TEST_HAS_GNU_STRING_ABI)
    add_executable(holder_core_read_allocation_tests ReadAllocationFailure_test.cpp)
    target_link_libraries(holder_core_read_allocation_tests PRIVATE Holder::Core)
    target_link_options(holder_core_read_allocation_tests PRIVATE "LINKER:--wrap=_Znwm"
      "LINKER:--wrap=_ZN4YAML4LoadERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE"
      "LINKER:--wrap=_ZN6holder5store9Rebuilder15rebuild_projectERKNS_5model7ProjectE")
    target_compile_definitions(holder_core_read_allocation_tests PRIVATE
      SCHEMA_SQL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../schema/schema.sql")
    if(TARGET Catch2::Catch2)
      target_sources(holder_core_read_allocation_tests PRIVATE catch_main.cpp)
      target_link_libraries(holder_core_read_allocation_tests PRIVATE Catch2::Catch2)
    elseif(TARGET Catch2::Catch2WithMain)
      target_link_libraries(holder_core_read_allocation_tests PRIVATE Catch2::Catch2WithMain)
    endif()
    if(HOLDER_CORE_TSAN_USE_SETARCH)
      find_program(SETARCH_EXECUTABLE setarch REQUIRED)
      add_test(NAME "Read allocation failures preserve complete results"
        COMMAND ${SETARCH_EXECUTABLE} ${CMAKE_SYSTEM_PROCESSOR} -R $<TARGET_FILE:holder_core_read_allocation_tests>)
    else()
      add_test(NAME "Read allocation failures preserve complete results" COMMAND holder_core_read_allocation_tests)
    endif()
  endif()
endif()

if(TARGET Catch2::Catch2)
  include(Catch)
endif()

add_executable(holder_core_tests
  AiMessageRepo_test.cpp
  AiMessageFrontMatter_test.cpp
  AiMessagePaths_test.cpp
  AiProviderCredentialRepo_test.cpp
  AiProviderSettingRepo_test.cpp
  AiRunRepo_test.cpp
  AiRunnerRepo_test.cpp
  AiThreadRepo_test.cpp
  AiThreadManifest_test.cpp
  AssetStorage_test.cpp
  CApiLegacyHostResource_test.cpp
  CApiCards_test.cpp
  CApiHistory_test.cpp
  CApiImport_test.cpp
  CApiLocations_test.cpp
  CApiMaintenance_test.cpp
  CApiProjects_test.cpp
  CApiRecovery_test.cpp
  CApiRuntime_test.cpp
  ContractContext_test.cpp
  ContractGit_test.cpp
  ContractHistory_test.cpp
  ContractTrash_test.cpp
  ContractSearch_test.cpp
  ContractBreadcrumbs_test.cpp
  ContractCardReference_test.cpp
  ContractProjectMaintenance_test.cpp
  ContractLinks_test.cpp
  ContractProjects_test.cpp
  ContractRecoverySnapshots_test.cpp
  ContractAssets_test.cpp
  TimeZones_test.cpp
  CardFrontMatter_test.cpp
  CardPaths_test.cpp
  CardPlacementResolver_test.cpp
  CardReferenceResolver_test.cpp
  CardRepo_test.cpp
  CardStore_test.cpp
  CardLifecycle_test.cpp
  CardStoreFsFailure_test.cpp
  Cascade_test.cpp
  DefaultProject_test.cpp
  Db_test.cpp
  FsAdapterFailure_test.cpp
  FtsIndexer_test.cpp
  GitAdapterFailure_test.cpp
  GitConcurrency_test.cpp
  GitCredentialProvider_test.cpp
  GitOps_test.cpp
  GitRepo_test.cpp
  IdScheme_test.cpp
  CardHistory_test.cpp
  ProjectHistory_test.cpp
  LinkKindCatalog_test.cpp
  LinkRepo_test.cpp
  MilestoneRepo_test.cpp
  MilestoneUpdate_test.cpp
  Migrations_test.cpp
  PrivacyCryptoService_test.cpp
  PrivacyError_test.cpp
  ProjectPaths_test.cpp
  ProjectPrivacy_test.cpp
  ProjectRegistry_test.cpp
  ProjectRepo_test.cpp
  ProjectStore_test.cpp
  ProjectManifest_test.cpp
  ProjectSyncOperation_test.cpp
  ProjectSyncPolicy_test.cpp
  ProjectSyncRepo_test.cpp
  PullConflictResolution_test.cpp
  PushResult_test.cpp
  RealFs_test.cpp
  Reindexer_test.cpp
  RemoteProbe_test.cpp
  GitHubHostTrust_test.cpp
  RepoSyncMetrics_test.cpp
  RevisionReferenceResolver_test.cpp
  ResourceRepo_test.cpp
  SecretStore_test.cpp
  StartupRecovery_test.cpp
  StorageProviderLifetime_test.cpp
  TagExtractor_test.cpp
  TagLineEditor_test.cpp
  TagRepo_test.cpp
  Uuid_test.cpp
)

target_link_libraries(holder_core_tests PRIVATE
  Holder::Core
)

target_compile_definitions(holder_core_tests PRIVATE
  EXPECTED_HOLDER_VERSION_STRING="${HOLDER_CORE_VERSION}"
  EXPECTED_HOLDER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  EXPECTED_HOLDER_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  EXPECTED_HOLDER_VERSION_PATCH=${PROJECT_VERSION_PATCH}
  SCHEMA_SQL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../schema/schema.sql"
)

if(TARGET Catch2::Catch2)
  target_sources(holder_core_tests PRIVATE catch_main.cpp)
  target_link_libraries(holder_core_tests PRIVATE Catch2::Catch2)
elseif(TARGET Catch2::Catch2WithMain)
  target_link_libraries(holder_core_tests PRIVATE Catch2::Catch2WithMain)
endif()

# HOLDER_TEST_KEYSTORE_DIR isolates the keystore; XDG_DATA_HOME/XDG_CONFIG_HOME/
# XDG_CACHE_HOME isolation is applied unconditionally in catch_main.cpp instead of
# here — catch_discover_tests()'s PROPERTIES argument can't carry a multi-entry
# ENVIRONMENT value (see the matching comment in holder-daemon/tests/CMakeLists.txt
# for why, and why a TEST_INCLUDE_FILES-based fixup doesn't work either).
set(HOLDER_CORE_TEST_ENVIRONMENT
  "HOLDER_TEST_KEYSTORE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-keystore"
)

if(HOLDER_CORE_CATCH_DISCOVER_TESTS AND COMMAND catch_discover_tests)
  catch_discover_tests(holder_core_tests
    PROPERTIES
      ENVIRONMENT "${HOLDER_CORE_TEST_ENVIRONMENT}"
  )
elseif(HOLDER_CORE_TSAN_USE_SETARCH AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_program(SETARCH_EXECUTABLE setarch REQUIRED)
  add_test(NAME holder_core_tests
    COMMAND ${SETARCH_EXECUTABLE} ${CMAKE_SYSTEM_PROCESSOR} -R $<TARGET_FILE:holder_core_tests>)
  set_tests_properties(holder_core_tests PROPERTIES ENVIRONMENT "${HOLDER_CORE_TEST_ENVIRONMENT}")
else()
  add_test(NAME holder_core_tests COMMAND holder_core_tests)
  set_tests_properties(holder_core_tests PROPERTIES ENVIRONMENT "${HOLDER_CORE_TEST_ENVIRONMENT}")
endif()
