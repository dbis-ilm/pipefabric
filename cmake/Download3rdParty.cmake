
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
                ${Catch_SOURCE_DIR}/single_include/catch.hpp
                ${PROJECT_SOURCE_DIR}/test)

#--------------------------------------------------------------------------------
# the JSON library
download_project(PROJ               json
                GIT_REPOSITORY      https://github.com/nlohmann/json.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E copy
                ${json_SOURCE_DIR}/src/json.hpp
                ${THIRD_PARTY_DIR}/json)

#--------------------------------------------------------------------------------
# the format library
download_project(PROJ               Format
                GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E copy
                ${Format_SOURCE_DIR}/fmt/format.h
                ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E copy
                ${Format_SOURCE_DIR}/fmt/format.cc
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
if (BUILD_GOOGLE_BENCH)
# Google Benchmark framework
download_project(PROJ               benchmark
                GIT_REPOSITORY      https://github.com/google/benchmark.git
                GIT_TAG             master
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
download_project(PROJ               rocksdb
	            GIT_REPOSITORY      https://github.com/facebook/rocksdb
	            GIT_TAG             master
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

