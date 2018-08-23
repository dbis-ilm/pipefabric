
# We download some 3rdparty modules from github.com via DownloadProject
include(DownloadProject)

set(THIRD_PARTY_DIR "${PROJECT_BINARY_DIR}/3rdparty")

#--------------------------------------------------------------------------------
# the Catch framework for testing
download_project(PROJ               Catch
                GIT_REPOSITORY      https://github.com/philsquared/Catch
                GIT_TAG             master
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
download_project(PROJ               json
                GIT_REPOSITORY      https://github.com/nlohmann/json.git
                GIT_TAG             develop
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E copy
                ${json_SOURCE_DIR}/single_include/nlohmann/json.hpp
                ${THIRD_PARTY_DIR}/json/)

include_directories("${THIRD_PARTY_DIR}/json")

#--------------------------------------------------------------------------------
# the format library
download_project(PROJ               Format
                GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
                GIT_TAG             4.1.0
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E copy
                ${Format_SOURCE_DIR}/fmt/format.*
                ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E copy
                ${Format_SOURCE_DIR}/fmt/ostream.*
                ${THIRD_PARTY_DIR}/fmt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_HEADER_ONLY=1")
include_directories("${THIRD_PARTY_DIR}/fmt")

#--------------------------------------------------------------------------------
# the SimpleWeb library
download_project(PROJ               SimpleWeb
                GIT_REPOSITORY      https://github.com/eidheim/Simple-Web-Server.git
                GIT_TAG             master
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
                GIT_TAG             v1.2.0
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
	    OUTPUT ${THIRD_PARTY_DIR}/benchmark
	    COMMAND ${CMAKE_COMMAND} -E chdir ${benchmark_SOURCE_DIR} cmake -DCMAKE_BUILD_TYPE=Release
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
	            GIT_TAG             v5.14.2
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
download_project(PROJ               data
	            GIT_REPOSITORY      https://github.com/dbis-ilm/data.git
	            GIT_TAG             master
	            UPDATE_DISCONNECTED 1
	            QUIET
)
file(COPY ${PROJECT_BINARY_DIR}/data-src/DEBS2017
     DESTINATION ${THIRD_PARTY_DIR}
)
endif()

#--------------------------------------------------------------------------------
if(USE_NVM_TABLE)
# Peristent Memory Development Kit (pmem.io)
download_project(PROJ               pmdk
                GIT_REPOSITORY      https://github.com/pmem/pmdk.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
    OUTPUT ${THIRD_PARTY_DIR}/pmdk
    COMMAND ${CMAKE_COMMAND} -E chdir ${pmdk_SOURCE_DIR} $(MAKE)
    COMMAND ${CMAKE_COMMAND} -E chdir ${pmdk_SOURCE_DIR} $(MAKE) install prefix=${THIRD_PARTY_DIR}/pmdk
)

# PTable (internal gitlab project) for NVM
download_project(PROJ               ptable
                GIT_REPOSITORY      https://dbgit.prakinf.tu-ilmenau.de/code/PTable.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
    OUTPUT ${THIRD_PARTY_DIR}/ptable
    COMMAND ${CMAKE_COMMAND} -E chdir ${ptable_SOURCE_DIR} cmake -DPTABLE_DIR=${THIRD_PARTY_DIR}/ptable src
    COMMAND ${CMAKE_COMMAND} -E chdir ${ptable_SOURCE_DIR} $(MAKE) install
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
