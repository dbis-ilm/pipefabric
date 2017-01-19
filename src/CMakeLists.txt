cmake_minimum_required(VERSION 3.0)
project (pipefabric)

set(CMAKE_MACOSX_RPATH 1)

set (PipeFabric_VERSION_MAJOR 0)
set (PipeFabric_VERSION_MINOR 2)

include(CTest)

# Add our CMake directory to CMake's module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/")

# We download some 3rdparty modules from github.com before building
# the project.
include(Download3rdParty)

#############################
# customization section
#############################
#

# Installation path
set(PIPEFABRIC_DIR "/usr/local/pfabric")

# Use the boost::spirit parser for converting strings to numbers
option(USE_BOOST_SPIRIT_PARSER
    "use the boost::spirit::qi parsers for converting strings to tuple attributes"
    ON
)

# Use RocksDB key-value store for implementing tables
option(USE_ROCKSDB_TABLE
  "use RocksDB for implementing persistent tables"
  ON
)

# Force using intel compiler
#include(CMakeForceCompiler)
#CMAKE_FORCE_C_COMPILER(icc "Intel C Compiler")
#CMAKE_FORCE_CXX_COMPILER(icpc "Intel C++ Compiler")

# C++ compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wno-deprecated -g -O3 -Wsign-compare")
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -Wno-#pragma-messages")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -Wno-unused")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd488 -wd597")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused -Wno-uninitialized")

# End of customization section
#---------------------------------------------------------------------------
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(DYLIB_LIBRARY "-ldl")
else()
set(DYLIB_LIBRARY "")
endif()

#############################
# memory allocator libraries
#############################
#
find_package(JeMalloc)
find_package(Tcmalloc)
if (Tcmalloc_FOUND)
     message(STATUS "using the tcmalloc allocator")
	 set(MALLOC_LIB ${Tcmalloc_LIBRARIES})
elseif(JEMALLOC_FOUND)
	  message(STATUS "using the jemalloc allocator")
	 set(MALLOC_LIB ${JEMALLOC_LIBRARIES})
else()
     set(MALLOC_LIB "")
endif()

########################
# RocksDB database library 
########################
#
if (USE_ROCKSDB_TABLE)
  message(STATUS "using RocksDB key-value store")
  add_definitions(-DUSE_ROCKSDB_TABLE)
  set (ROCKSDB_LIB "${THIRD_PARTY_DIR}/rocksdb/lib/librocksdb.a")
  include_directories("${THIRD_PARTY_DIR}/rocksdb/include")

  find_package( BZip2 )
  if ( BZIP2_FOUND )
    #  include_directories( ${BZIP2_INCLUDE_DIRS} )
    set (ROCKSDB_LIB ${ROCKSDB_LIB} ${BZIP2_LIBRARIES})
  endif( BZIP2_FOUND )

  find_package( ZLIB )
  if ( ZLIB_FOUND )
    #  include_directories( ${BZIP2_INCLUDE_DIRS} )
    set (ROCKSDB_LIB ${ROCKSDB_LIB} ${ZLIB_LIBRARIES})
  endif( ZLIB_FOUND )
else()
  message(STATUS "don't use RocksDB key-value store")
  set (ROCKSDB_LIB "")
endif()

######################
# Boost C++ library
######################
#
SET(BOOST_MIN_VERSION "1.61.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS
    program_options
    system
    coroutine
    iostreams
    log
    filesystem
    timer
    serialization
    thread
    regex
    chrono
    date_time
)
if (NOT Boost_FOUND)
     message(FATAL_ERROR "Fatal error: Boost (version >= ${BOOST_MIN_VERSION}) required.\n"
 )
endif (NOT Boost_FOUND)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    add_definitions( "-DHAS_BOOST" )
    add_definitions( "-DBOOST_LOG_DYN_LINK" )
    if(USE_BOOST_SPIRIT_PARSER)
        add_definitions( "-DUSE_BOOST_SPIRIT_PARSER" )
    endif()
endif()

set(BOOST_LIBRARIES
		${Boost_DATE_TIME_LIBRARY}
		${Boost_LOG_LIBRARY}
		${Boost_COROUTINE_LIBRARY}
		${Boost_IOSTREAMS_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
		${Boost_SYSTEM_LIBRARY}
		${Boost_CHRONO_LIBRARY}
		${Boost_TIMER_LIBRARY}
		${Boost_THREAD_LIBRARY}
		${Boost_REGEX_LIBRARY}
	${DYLIB_LIBRARY}
)

######################
# ZeroMQ library
######################
#
#
find_package(ZeroMQ)
if (ZEROMQ_FOUND)
    add_definitions( "-DHAS_ZMQ" )
    include_directories(${ZEROMQ_INCLUDE_DIR})
    link_directories(${ZEROMQ_LIBRARY_DIR})
endif(ZEROMQ_FOUND)

#-----------------------------------------------------------------------------------------
#
# Building PipeFabric core library
#

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/pubsub")

# because we have downloaded external projects into build we have to add them here
include_directories("${THIRD_PARTY_DIR}")

if (ZEROMQ_FOUND)
set (ZEROMQ_SOURCES
  net/ZMQSocket.cpp
  qop/ZMQSource.cpp)
else()
set (ZEROMQ_SOURCES "")
endif (ZEROMQ_FOUND)

add_library(pfabric_core SHARED
  core/TimestampHelper.cpp
  core/Punctuation.cpp
  qop/TextFileSource.cpp
  qop/RESTSource.cpp
  qop/Window.cpp
  qop/TriggerNotifier.cpp
  topology/Topology.cpp
  topology/Dataflow.cpp
  topology/PFabricContext.cpp
  table/TableInfo.cpp
  ${ZEROMQ_SOURCES}
  # we need this as dependency to download the sources from github
  ${THIRD_PARTY_DIR}/json
  ${THIRD_PARTY_DIR}/fmt
  ${THIRD_PARTY_DIR}/SimpleWeb
  ${THIRD_PARTY_DIR}/rocksdb
  ${THIRD_PARTY_DIR}/catch
)


target_link_libraries(pfabric_core
		${BOOST_LIBRARIES}
		 ${ZEROMQ_LIBRARIES}
)

#-----------------------------------------------------------------------------------------
#
# Building PipeFabric CEP library
#
add_library(pfabric_cep SHARED
                  cep/Matcher.cpp
                  # we need this as dependency to download the sources from github
                  ${THIRD_PARTY_DIR}/fmt
)

target_link_libraries(pfabric_cep
                ${BOOST_LIBRARIES}
)

#-----------------------------------------------------------------------------------------
#
# Unit tests using Catch
#
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE test)

enable_testing()
add_subdirectory(test)

#-----------------------------------------------------------------------------------------
#
# Installation
#
set(PIPEFABRIC_LIBS pfabric_core pfabric_cep)
set(PIPEFABRIC_LIB_DIR "${PIPEFABRIC_DIR}/lib")
set(PIPEFABRIC_INCLUDE_DIR "${PIPEFABRIC_DIR}/include")

foreach(LIB ${PIPEFABRIC_LIBS})
install(TARGETS ${LIB} DESTINATION ${PIPEFABRIC_LIB_DIR})
endforeach(LIB)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
   DESTINATION ${PIPEFABRIC_INCLUDE_DIR}
   FILES_MATCHING
   PATTERN "*.hpp"
   PATTERN "*.h"
)

add_subdirectory(demo)
add_subdirectory(debs2016)
add_subdirectory(qcomp)

# show used compiler
message("Using Compiler: ${CMAKE_CXX_COMPILER_ID}.")
