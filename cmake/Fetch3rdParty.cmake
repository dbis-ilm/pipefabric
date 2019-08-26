message("===== Fetching 3rd Party Dependencies ==========================================")
include(FetchContent REQUIRED)

set(THIRD_PARTY_DIR "${PROJECT_BINARY_DIR}/_deps")
# including here enables reuse of dependencies
include_directories(${THIRD_PARTY_DIR})

# ============================================================================ #
# the JSON library ///< this repository is too big (~500MB)                    #
# ============================================================================ #
FetchContent_Declare(
  json
  URL                 https://raw.githubusercontent.com/nlohmann/json/v3.7.0/single_include/nlohmann/json.hpp
  DOWNLOAD_NO_EXTRACT TRUE
  SOURCE_DIR          ${THIRD_PARTY_DIR}/json
  DOWNLOAD_DIR        ${THIRD_PARTY_DIR}/json
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  message(STATUS "Populating json")
  FetchContent_Populate(json)
endif()



# ============================================================================ #
# the format library                                                           #
# ============================================================================ #
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        5.3.0
  GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  message(STATUS "Populating fmt (Format)")
  FetchContent_Populate(fmt)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_HEADER_ONLY=1")
  file(COPY ${fmt_SOURCE_DIR}/include/fmt/core.h
            ${fmt_SOURCE_DIR}/include/fmt/format-inl.h
            ${fmt_SOURCE_DIR}/include/fmt/format.h
            ${fmt_SOURCE_DIR}/include/fmt/ostream.h
       DESTINATION ${THIRD_PARTY_DIR}/fmt
  )
endif()



# ============================================================================ #
# the SimpleWeb library                                                        #
# ============================================================================ #
FetchContent_Declare(
  SimpleWeb
  GIT_REPOSITORY      https://gitlab.com/eidheim/Simple-Web-Server.git
  GIT_TAG             master
  GIT_SHALLOW         TRUE
  SOURCE_DIR          ${THIRD_PARTY_DIR}/SimpleWeb
)
FetchContent_GetProperties(SimpleWeb)
if(NOT simpleweb_POPULATED)
  message(STATUS "Populating SimpleWeb")
  FetchContent_Populate(SimpleWeb)
endif()



# ============================================================================ #
# Concurrent hash table                                                        #
# ============================================================================ #
FetchContent_Declare(
  libcuckoo
  GIT_REPOSITORY https://github.com/efficient/libcuckoo
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(libcuckoo)
if(NOT libcuckoo_POPULATED)
  message(STATUS "Populating libcuckoo")
  set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
  set(BUILD_TESTS OFF CACHE INTERNAL "")
  FetchContent_Populate(libcuckoo)
  add_subdirectory(${libcuckoo_SOURCE_DIR} ${libcuckoo_BINARY_DIR} EXCLUDE_FROM_ALL)
  include_directories(${libcuckoo_SOURCE_DIR})
endif()



if(BUILD_TEST_CASES)
  # ========================================================================== #
  # the Catch framework for testing                                            #
  # ========================================================================== #
  FetchContent_Declare(
    catch
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.9.2
    GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(catch)
  if(NOT catch_POPULATED)
    message(STATUS "Populating catch")
    FetchContent_Populate(catch)
    set(CATCH_INCLUDE_DIR ${catch_SOURCE_DIR}/single_include/catch2)
    add_library(Catch2::Catch IMPORTED INTERFACE)
    set_property(TARGET Catch2::Catch PROPERTY INTERFACE_INCLUDE_DIRECTORIES
      "${CATCH_INCLUDE_DIR}")
  endif()
endif()



if (BUILD_GOOGLE_BENCH)
  # ========================================================================== #
  # Google Benchmark framework                                                 #
  # ========================================================================== #
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.5.0
    GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(benchmark)
  if(NOT benchmark_POPULATED)
    message(STATUS "Populating benchmark (google)")
    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE INTERNAL "")
    FetchContent_Populate(benchmark)
    add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()



if(USE_ROCKSDB_TABLE)
  # ========================================================================== #
  # RocksDB key-value store                                                    #
  # ========================================================================== #
  FetchContent_Declare(
    rocksdb
    GIT_REPOSITORY https://github.com/facebook/rocksdb
    GIT_TAG        v6.2.2
    GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(rocksdb)
  if(NOT rocksdb_POPULATED)
    message(STATUS "Populating rocksdb (facebook)")
    set(USE_RTTI 1)
    set(WITH_TESTS OFF CACHE INTERNAL "")
    set(WITH_TOOLS OFF CACHE INTERNAL "")
    set(CMAKE_ENABLE_SHARED OFF CACHE INTERNAL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-copy -Wno-deprecated-copy -Wno-error=pessimizing-move -Wno-pessimizing-move")
    FetchContent_Populate(rocksdb)
    add_subdirectory(${rocksdb_SOURCE_DIR} ${rocksdb_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()



if(BUILD_USE_CASES)
  # ========================================================================== #
  # data for use cases                                                         #
  # ========================================================================== #
  FetchContent_Declare(
    data
    GIT_REPOSITORY https://github.com/dbis-ilm/data.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    SOURCE_DIR     ${THIRD_PARTY_DIR}/data
  )
  FetchContent_GetProperties(data)
  if(NOT data_POPULATED)
    message(STATUS "Populating data (for use cases)")
    FetchContent_Populate(data)
  endif()

  # ========================================================================== #
  # Linear Road Data Driver                                                    #
  # ========================================================================== #
  FetchContent_Declare(
    linroad
    GIT_REPOSITORY https://github.com/yxian29/Linear-Road-Benchmark-Data-Driver.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(linroad)
  if(NOT linroad_POPULATED)
    message(STATUS "Populating linear Road (benchmark)")
    FetchContent_Populate(linroad)
    file(COPY ${PROJECT_SOURCE_DIR}/usecases/LinearRoad/DataProvider/CMakeLists.txt
         DESTINATION ${linroad_SOURCE_DIR}/src)
    file(COPY ${linroad_SOURCE_DIR}/src/Data/datafile20seconds.dat
         DESTINATION ${data_SOURCE_DIR}/linroad)
    add_subdirectory(${linroad_SOURCE_DIR}/src ${linroad_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()



if(USE_NVM_TABLES)
  # ========================================================================== #
  # Peristent Memory Development Kit (pmem.io)                                 #
  # ========================================================================== #
  find_package(PkgConfig REQUIRED)
  message(STATUS "Searching for PMDK")
  find_path(PMDK_INCLUDE_DIR libpmem.h)
  pkg_check_modules(PMDK REQUIRED libpmemobj++>=1.5)
  set(PMDK_INCLUDE_DIRS ${PMDK_INCLUDE_DIRS} ${PMDK_INCLUDE_DIR})
  if(NOT PMDK_INCLUDE_DIRS OR "${PMDK_INCLUDE_DIRS}" STREQUAL "")
    message(FATAL_ERROR "ERROR: libpmem include directory not found.")
  endif()
  message(STATUS "  libpmem.h found in ${PMDK_INCLUDE_DIRS}")
  mark_as_advanced(PMDK_LIBRARIES PMDK_INCLUDE_DIRS)

  # ========================================================================== #
  # PTable and PBPTree (part of nvm-based data structures) for NVM             #
  # ========================================================================== #
  FetchContent_Declare(
    nvmDS
    GIT_REPOSITORY https://dbgit.prakinf.tu-ilmenau.de/code/nvm-based_data_structures.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(nvmDS)
  if(NOT nvmds_POPULATED)
    message(STATUS "Populating NVM-based data structures (nvmDS)")
    set(BUILD_TEST_CASES_PARENT ${BUILD_TEST_CASES})
    set(BUILD_BENCHMARKS_PARENT ${BUILD_TEST_CASES})
    set(BUILD_TEST_CASES OFF CACHE INTERNAL "")
    set(BUILD_BENCHMARKS OFF CACHE INTERNAL "")
    FetchContent_Populate(nvmDS)
    add_subdirectory(${nvmds_SOURCE_DIR}/src ${nvmds_BINARY_DIR})
    set(BUILD_TEST_CASES ${BUILD_TEST_CASES_PARENT} CACHE INTERNAL "")
    set(BUILD_BENCHMARKS ${BUILD_TEST_CASES_PARENT} CACHE INTERNAL "")
  endif()
endif()

message("===== Finished fetching 3rd Party Dependencies =================================")
