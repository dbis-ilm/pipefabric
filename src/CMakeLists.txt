cmake_minimum_required(VERSION 3.2)
project (pipefabric)

set(CMAKE_MACOSX_RPATH 1)

set (PipeFabric_VERSION_MAJOR 0)
set (PipeFabric_VERSION_MINOR 3)

include(CTest)

################################################################################
# customization section                                                        #
################################################################################

# Installation path
set(PIPEFABRIC_DIR "/usr/local/pfabric")

# The following variables enable or disable additional functionalities,
# which can be switched off to reduce build time.

# Support Matrix Operations (needs Eigen library to be installed)
option(SUPPORT_MATRICES
  "support matrix operations as tuple and state type"
  OFF
)

# Build use cases
option(BUILD_USE_CASES
  "build use cases to show functionality examples"
  OFF
)

# Use RabbitMQ as network source
option(USE_RABBITMQ
  "use RabbitMQ as network source"
  OFF
)
# Use Apache Kafka as network source
option(USE_KAFKA
  "use Apache Kafka as network source"
  OFF
)
# Use MQTT as network source
option(USE_MQTT
  "use MQTT as network source"
  OFF
)

# Use the boost::spirit parser for converting strings to numbers
option(USE_BOOST_SPIRIT_PARSER
  "use the boost::spirit::qi parsers for converting strings to tuple attributes"
  ON
)

# Use RocksDB key-value store for implementing tables
# If switched to off, it will not be downloaded, saving initial building time.
option(USE_ROCKSDB_TABLE
  "use RocksDB for implementing persistent tables"
  OFF
)

# Use Non-Volatile Memory Library for implementing tables
option(USE_NVML_TABLE
	"use nvml for implementing persistent memory tables"
  OFF
)

# Build only pipefabric libraries (cep and core)
# If switched to on, no other components (like RestDemo, QueryCompiler,...) will be built.
# Building test cases is independent from this.
option(BUILD_ONLY_LIBS
  "build only the two pipefabric libraries"
  ON
)

# Build test cases for pipefabric functionality
# If switched to off, no tests will be build
option(BUILD_TEST_CASES
  "build tests for pipefabric functionality"
  ON
)

#Build google benchmark library
option(BUILD_GOOGLE_BENCH
  "build google benchmark"
  ON
)

# Build benchmark test
option(BUILD_BENCHMARKS
  "build benchmarks for pipefabric"
  ON
)

################################
# End of customization section #
################################

# Use cases require matrix support (image and graph processing)
if(BUILD_USE_CASES)
	set(SUPPORT_MATRICES ON)
endif()

# Benchmark test requires benchmark library
if (BUILD_BENCHMARKS)
  set(BUILD_GOOGLE_BENCH ON)
endif()

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

# Add our CMake directory to CMake's module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/")

# We download some 3rdparty modules from github.com before building the project.
include(Download3rdParty)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(DYLIB_LIBRARY "-ldl")
else()
set(DYLIB_LIBRARY "")
endif()

##############################
# memory allocator libraries #
##############################
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
# Google benchmark library
########################
#
if (BUILD_GOOGLE_BENCH)
  include_directories("${THIRD_PARTY_DIR}/benchmark/include")
  set (BENCHMARK_LIB "${THIRD_PARTY_DIR}/benchmark/lib/libbenchmark.a")
else()
  set (BENCHMARK_LIB "")
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

########################
# Non-Volatile Memory Library
########################
#
if (USE_NVML_TABLE)
	message(STATUS "using NVML based persistent table")
	add_definitions(-DUSE_NVML_TABLE)
	set (NVML_LIBRARIES
            "${THIRD_PARTY_DIR}/nvml/lib/libpmemblk.a"
            "${THIRD_PARTY_DIR}/nvml/lib/libpmemlog.a"
            "${THIRD_PARTY_DIR}/nvml/lib/libpmemobj.a"
            "${THIRD_PARTY_DIR}/nvml/lib/libpmempool.a"
            "${THIRD_PARTY_DIR}/nvml/lib/libpmem.a"
            ${DYLIB_LIBRARY}
            "${THIRD_PARTY_DIR}/ptable/lib/libptable.so"
            )
    include_directories(
            "${THIRD_PARTY_DIR}/nvml/include"
            "${THIRD_PARTY_DIR}/ptable/include"
            )
else ()
	message(STATUS "don't use NVML based persistent table")
	set (NVML_LIBRARIES "")
endif()

######################
# Boost C++ library
######################
#
SET(BOOST_MIN_VERSION "1.60.0")
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
    python
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
# Python
######################
#
find_package(PythonLibs)
FIND_PACKAGE(PythonInterp)
if (PYTHONLIBS_FOUND)
message(STATUS "Python Version ${PYTHON_VERSION_MAJOR} found - building the Python API.")
#set (PYTHON_INCLUDE_DIRS "/Users/kai/miniconda3/include/python3.5m")
message("PythonInclude ${PYTHON_INCLUDE_DIRS}")
message("PythonLibs ${PYTHON_LIBRARIES}")

include_directories(${PYTHON_INCLUDE_DIRS})
link_directories(${PYTHON_LIBRARIES})

set(PYTHON_SOURCES
  python/PyTopology.cpp
)
else()
  set (PYTHON_SOURCES "")

endif(PYTHONLIBS_FOUND)

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
# Matrix Support
#

if(SUPPORT_MATRICES)
	####################################
	# Eigen library for linear algebra #
	####################################
	find_package(Eigen3)
 	if(EIGEN3_FOUND)
	  message(STATUS "using Eigen3 library for linear algebra")
		include_directories(${EIGEN3_INCLUDE_DIR})
	else()
		message(STATUS "Eigen3 not found")
	endif()
	add_definitions(-DSUPPORT_MATRICES)
endif()

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

set(core_sources
  core/TimestampHelper.cpp
  core/Punctuation.cpp
  qop/TextFileSource.cpp
  qop/RESTSource.cpp
  qop/Window.cpp
  qop/TriggerNotifier.cpp
  dsl/Topology.cpp
  dsl/Dataflow.cpp
  dsl/PFabricContext.cpp
  table/TableInfo.cpp
  ${ZEROMQ_SOURCES}
  # we need this as dependency to download the sources from github
  ${THIRD_PARTY_DIR}/json
  ${THIRD_PARTY_DIR}/fmt
  ${THIRD_PARTY_DIR}/SimpleWeb
  ${THIRD_PARTY_DIR}/catch
)

if(USE_ROCKSDB_TABLE)
  set(core_sources
    ${core_sources}
    ${THIRD_PARTY_DIR}/rocksdb
  )
endif()

if(BUILD_GOOGLE_BENCH)
  set(core_sources
    ${core_sources}
    ${THIRD_PARTY_DIR}/benchmark
  )
endif()

set(core_libs
  ${BOOST_LIBRARIES}
  ${ZEROMQ_LIBRARIES}
)

#-----------------------------------------------------------------------------------------
#
##########
# RabbitMQ
##########
#
#
if(USE_RABBITMQ)
  add_definitions(-DUSE_RABBITMQ)
  set(core_libs
    ${core_libs}
    amqpcpp
    rabbitmq
  )
  set(core_sources
    ${core_sources}
    net/RabbitMQSource.cpp
  )
endif()
#-----------------------------------------------------------------------------------------
#
##############
# Apache Kafka
##############
#
#
if(USE_KAFKA)
  add_definitions(-DUSE_KAFKA)
  set(core_libs
    ${core_libs}
    cppkafka
    rdkafka
  )
  set(core_sources
    ${core_sources}
    net/KafkaSource.cpp
  )
endif()

#-----------------------------------------------------------------------------------------
#
######
# MQTT
######
#
#
if(USE_MQTT)
  add_definitions(-DUSE_MQTT)
  set(core_libs
    ${core_libs}
    paho-mqtt3c
    paho-mqtt3cs
    paho-mqtt3a
    paho-mqtt3as
    paho-mqttpp3
  )
  set(core_sources
    ${core_sources}
    net/MQTTSource.cpp
  )
endif()

if(USE_NVML_TABLE)
  add_definitions(-DDO_LOG=0)
  set(core_sources
    ${core_sources}
    ${THIRD_PARTY_DIR}/nvml
    ${THIRD_PARTY_DIR}/ptable
  )
  set(core_libs
    ${core_libs}
    ${NVML_LIBRARIES}
  )
endif()
#-----------------------------------------------------------------------------------------

add_library(pfabric_core SHARED
  ${core_sources}
)

target_link_libraries(pfabric_core
  ${core_libs}
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
# Building PipeFabric Python library
#
add_library(pfabric SHARED
                  ${PYTHON_SOURCES}
                  # we need this as dependency to download the sources from github
                  ${THIRD_PARTY_DIR}/fmt
)

target_link_libraries(pfabric
              ${Boost_PYTHON_LIBRARY}
              ${PYTHON_LIBRARIES}
              pfabric_core
              ${BOOST_LIBRARIES}
)

#-----------------------------------------------------------------------------------------
#
# Experimental SQL query compiler
#
# if(!USE_NVML_TABLE)
  #-----------------------------------------------------------------------------------------
  #
  # Building PipeFabric query compiler library
  #
  add_library(pfabric_qcomp SHARED
    qcomp/Plan.cpp
    qcomp/PlanCache.cpp
    qcomp/QueryCompiler.cpp
    qcomp/TypeManager.cpp
    qcomp/UniqueNameGenerator.cpp
    qcomp/SQLParser.cpp)

  target_link_libraries(pfabric_qcomp
    pfabric_core
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${BOOST_LIBRARIES}
  )

  add_subdirectory(qcomp)
# endif()

#-----------------------------------------------------------------------------------------
#
# Demo project
#
add_subdirectory(demo)

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
# Micro-benchmarking using Google Benchmark
#
add_subdirectory(bench)

#-----------------------------------------------------------------------------------------
#
# Build use cases
#
if(BUILD_USE_CASES)
	#######################################
	# OpenCV library for image processing #
	#######################################
	find_package(OpenCV REQUIRED)
	if(OpenCV_FOUND)
	 message(STATUS "using OpenCV library for image processing")
	 include_directories(${OpenCV_INCLUDE_DIR})
	else()
		message(STATUS "OpenCV not found")
	endif()
endif()
  add_subdirectory(usecases)
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

# show used compiler
message("Using Compiler: ${CMAKE_CXX_COMPILER_ID}.")
