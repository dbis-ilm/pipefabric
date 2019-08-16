
# We download some 3rdparty modules from github.com via DownloadProject
include(DownloadProject)

set(THIRD_PARTY_DIR "${PROJECT_BINARY_DIR}/3rdparty")

#--------------------------------------------------------------------------------
# the Catch framework for testing
download_project(PROJ               Catch
                GIT_REPOSITORY      https://github.com/philsquared/Catch
                GIT_TAG             master
                GIT_SHALLOW         1
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/catch
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${Catch_SOURCE_DIR}/single_include/catch2/catch.hpp
                ${PROJECT_SOURCE_DIR}/test)

#--------------------------------------------------------------------------------
# the JSON library
# this repository is too big (~500MB)
#download_project(PROJ               json
#                GIT_REPOSITORY      https://github.com/nlohmann/json.git
#                GIT_TAG             develop
#                GIT_SHALLOW         1
#                UPDATE_DISCONNECTED 1
#                QUIET
#)
download_project(PROJ json
                 URL https://raw.githubusercontent.com/nlohmann/json/v3.7.0/single_include/nlohmann/json.hpp
                 DOWNLOAD_NO_EXTRACT 1
                 CONFIGURE_COMMAND ""
                 BUILD_COMMAND ""
                 INSTALL_COMMAND ""
                 QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E copy
                ${json_SOURCE_DIR}/../json-download/json-download-prefix/src/json.hpp
                ${THIRD_PARTY_DIR}/json/)

include_directories("${THIRD_PARTY_DIR}/json")

#--------------------------------------------------------------------------------
# the format library
download_project(PROJ               Format
                GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
                GIT_TAG             5.3.0
                GIT_SHALLOW         1
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E copy
                ${Format_SOURCE_DIR}/include/fmt/core.h
                ${Format_SOURCE_DIR}/include/fmt/format-inl.h
                ${Format_SOURCE_DIR}/include/fmt/format.h
                ${THIRD_PARTY_DIR}/fmt/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_HEADER_ONLY=1")
include_directories("${THIRD_PARTY_DIR}/fmt")

#--------------------------------------------------------------------------------
# the SimpleWeb library
download_project(PROJ               SimpleWeb
                GIT_REPOSITORY      https://github.com/eidheim/Simple-Web-Server.git
                GIT_TAG             master
                GIT_SHALLOW         1
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/SimpleWeb
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/SimpleWeb
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${SimpleWeb_SOURCE_DIR}
                ${THIRD_PARTY_DIR}/SimpleWeb)

#--------------------------------------------------------------------------------
# Google Benchmark framework
if (BUILD_GOOGLE_BENCH)
download_project(PROJ               benchmark
                GIT_REPOSITORY      https://github.com/google/benchmark.git
                GIT_TAG             v1.5.0
                GIT_SHALLOW         1
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
      OUTPUT ${THIRD_PARTY_DIR}/benchmark
      COMMAND ${CMAKE_COMMAND} -E chdir ${benchmark_SOURCE_DIR} cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_TESTING=OFF, -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
      COMMAND ${CMAKE_COMMAND} -E chdir ${benchmark_SOURCE_DIR} $(MAKE)
      COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/benchmark/include
      COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/benchmark/lib
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              ${benchmark_SOURCE_DIR}/include
              ${THIRD_PARTY_DIR}/benchmark/include
      COMMAND ${CMAKE_COMMAND} -E copy
              ${benchmark_SOURCE_DIR}/src/libbenchmark.a
              ${THIRD_PARTY_DIR}/benchmark/lib
)
endif()

#--------------------------------------------------------------------------------
if(USE_ROCKSDB_TABLE)
# RocksDB key-value store
download_project(PROJ             rocksdb
              GIT_REPOSITORY      https://github.com/facebook/rocksdb
              GIT_TAG             v6.2.2
              GIT_SHALLOW         1
              UPDATE_DISCONNECTED 1
              QUIET
)
add_custom_command(
      OUTPUT ${THIRD_PARTY_DIR}/rocksdb
      COMMAND ${CMAKE_COMMAND} -E chdir ${rocksdb_SOURCE_DIR} $(MAKE) static_lib
      COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/rocksdb/include
      COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/rocksdb/lib
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              ${rocksdb_SOURCE_DIR}/include
              ${THIRD_PARTY_DIR}/rocksdb/include
      COMMAND ${CMAKE_COMMAND} -E copy
              ${rocksdb_SOURCE_DIR}/librocksdb.a
              ${THIRD_PARTY_DIR}/rocksdb/lib
)
endif()

#--------------------------------------------------------------------------------
if(BUILD_USE_CASES)
# data for use cases
download_project(PROJ             data
              GIT_REPOSITORY      https://github.com/dbis-ilm/data.git
              GIT_TAG             master
              GIT_SHALLOW         1
              UPDATE_DISCONNECTED 1
              QUIET
)
file(COPY ${PROJECT_BINARY_DIR}/data-src/DEBS2017
     DESTINATION ${THIRD_PARTY_DIR}
)

# Linear Road Data Driver
download_project(PROJ               linroad
              GIT_REPOSITORY      https://github.com/samsonxian/Linear-Road-Benchmark-Data-Driver.git
              GIT_TAG             master
              GIT_SHALLOW         1
              UPDATE_DISCONNECTED 1
              QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/linroad
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/usecases/LinearRoad/CMakeLists.txt
                ${linroad_SOURCE_DIR}/src
        COMMAND ${CMAKE_COMMAND} -E chdir ${linroad_SOURCE_DIR}/src cmake .
        COMMAND ${CMAKE_COMMAND} -E chdir ${linroad_SOURCE_DIR}/src $(MAKE)
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/linroad/data
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/linroad/include
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/linroad/lib
        COMMAND ${CMAKE_COMMAND} -E copy
              ${linroad_SOURCE_DIR}/src/Data/datafile20seconds.dat
              ${THIRD_PARTY_DIR}/linroad/data
        COMMAND ${CMAKE_COMMAND} -E copy
              ${linroad_SOURCE_DIR}/src/libLRDataProvider.a
              ${THIRD_PARTY_DIR}/linroad/lib
        COMMAND ${CMAKE_COMMAND} -E copy
              ${linroad_SOURCE_DIR}/src/LRDataProvider.h
              ${THIRD_PARTY_DIR}/linroad/include
)
endif()

#--------------------------------------------------------------------------------
if(USE_NVM_TABLE)
# Peristent Memory Development Kit (pmem.io)
download_project(PROJ               pmdk
                GIT_REPOSITORY      https://github.com/pmem/pmdk.git
                GIT_TAG             master
                GIT_SHALLOW         1
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
    OUTPUT ${THIRD_PARTY_DIR}/pmdk
    COMMAND ${CMAKE_COMMAND} -E chdir ${pmdk_SOURCE_DIR} $(MAKE)
    COMMAND ${CMAKE_COMMAND} -E chdir ${pmdk_SOURCE_DIR} $(MAKE) install prefix=${THIRD_PARTY_DIR}/pmdk
)

download_project(PROJ               pmdk-cpp
                GIT_REPOSITORY      https://github.com/pmem/libpmemobj-cpp
                GIT_TAG             master
                GIT_SHALLOW         1
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/pmdk-cpp
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${pmdk-cpp_SOURCE_DIR}/include
                ${THIRD_PARTY_DIR}/pmdk/include
)

# PTable (part of nvm-based data structures) for NVM
download_project(PROJ               nvmDS
                GIT_REPOSITORY      https://dbgit.prakinf.tu-ilmenau.de/code/nvm-based_data_structures.git
                GIT_TAG             master
                GIT_SHALLOW         1
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
  OUTPUT ${THIRD_PARTY_DIR}/nvmDS
  COMMAND ${CMAKE_COMMAND} -E chdir ${nvmDS_SOURCE_DIR} cmake -DBUILD_TEST_CASES=OFF -DPROJECT_INSTALL_DIR=${THIRD_PARTY_DIR}/nvmDS src
  COMMAND ${CMAKE_COMMAND} -E chdir ${nvmDS_SOURCE_DIR} $(MAKE) install
)
endif()

#--------------------------------------------------------------------------------
# Concurrent hash table
download_project(PROJ               libcuckoo
                GIT_REPOSITORY      https://github.com/efficient/libcuckoo
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/libcuckoo
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/libcuckoo
        COMMAND ${CMAKE_COMMAND} -E chdir ${libcuckoo_SOURCE_DIR} cmake -DBUILD_EXAMPLES=0 -DBUILD_TESTS=0
        COMMAND ${CMAKE_COMMAND} -E chdir ${libcuckoo_SOURCE_DIR} $(MAKE) all
        COMMAND ${CMAKE_COMMAND} -E copy
                ${libcuckoo_SOURCE_DIR}/libcuckoo/*.hh
                ${THIRD_PARTY_DIR}/libcuckoo/)
