cmake_minimum_required(VERSION 3.11)
project (pipefabric)

set(CMAKE_MACOSX_RPATH 1)

set (PipeFabric_VERSION_MAJOR 0)
set (PipeFabric_VERSION_MINOR 3)

include(CTest)

############################################################################################
# Customization section                                                                    #
############################################################################################
#

# The following variables enable or disable additional functionalities,
# which can be switched off to reduce build time.

option(SUPPORT_MATRICES        "support matrix operations as tuple and state type"      OFF)
option(BUILD_USE_CASES         "build use cases to show functionality examples"         ON )
option(USE_RABBITMQ            "use RabbitMQ as network source"                         OFF)
option(USE_KAFKA               "use Apache Kafka as network source"                     OFF)
option(USE_MQTT                "use MQTT as network source"                             OFF)
option(USE_BOOST_SPIRIT_PARSER "use the boost::spirit::qi parsers (strings convertion)" ON )
option(USE_ROCKSDB_TABLE       "use RocksDB for implementing persistent tables"         OFF)
option(USE_NVM_TABLES          "use NVM for implementing persistent memory tables"      OFF)
option(BUILD_ONLY_LIBS         "build only the two pipefabric libraries"                ON )
option(BUILD_TEST_CASES        "build tests for pipefabric functionality"               ON )
option(BUILD_GOOGLE_BENCH      "build google benchmark"                                 OFF)
option(BUILD_BENCHMARKS        "build benchmark test for pipefabric"                    OFF)
option(BUILD_PYTHON            "build python interface for pipefabric"                  OFF)

# Set the mount path of your pmem device (only when using NVM tables)
set(PMEM_MNT_PATH "/mnt/pmem0/test")

############################################################################################
# End of customization section                                                             #
############################################################################################
#

# Benchmark test requires benchmark library
if (BUILD_BENCHMARKS)
  set(BUILD_GOOGLE_BENCH ON)
endif()

# Force using intel compiler
#include(CMakeForceCompiler)
#CMAKE_FORCE_C_COMPILER(icc "Intel C Compiler")
#CMAKE_FORCE_CXX_COMPILER(icpc "Intel C++ Compiler")

# C++ compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated -Wsign-compare")
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -Wno-#pragma-messages")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -Wno-unused")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd488 -wd597")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused -Wno-uninitialized")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG -g)
set(CMAKE_CXX_FLAGS_RELEASE -O3)
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()


# Add our CMake directory to CMake's module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# We download some 3rdparty modules from github.com before building the project.
include(Fetch3rdParty)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DYLIB_LIBRARY "-ldl")
else()
  set(DYLIB_LIBRARY "")
endif()

set(core_libs
  Threads::Threads
  ${DYLIB_LIBRARY}
  libcuckoo
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pfabric_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/pfabric_config.h
)

############################################################################################
# Memory allocator libraries                                                               #
############################################################################################
#
find_package(JeMalloc)
find_package(Tcmalloc)
if(JEMALLOC_FOUND)
  message(STATUS "using the jemalloc allocator")
  set(MALLOC_LIB ${JEMALLOC_LIBRARIES})
elseif (Tcmalloc_FOUND)

  message(STATUS "using the tcmalloc allocator")
  set(MALLOC_LIB ${Tcmalloc_LIBRARIES})
else()
  set(MALLOC_LIB "")
endif()

############################################################################################
# RocksDB database library                                                                 #
############################################################################################
#

if (USE_ROCKSDB_TABLE)
  message(STATUS "using RocksDB key-value store")
  add_definitions(-DUSE_ROCKSDB_TABLE)
  set (ROCKSDB_LIB rocksdb)
  include_directories("${rocksdb_SOURCE_DIR}/include")

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


############################################################################################
# Non-Volatile Memory Library                                                              #
############################################################################################
#

if (USE_NVM_TABLES)
  message(STATUS "using NVM based persistent table")
  add_definitions(-DUSE_NVM_TABLES)
  include_directories(
    ${nvmds_SOURCE_DIR}/src
    ${nvmds_BINARY_DIR}/generated
  )
  set(core_libs
    ${core_libs}
    ptable
    #    ${nvmds_BINARY_DIR}/ptable/libptable.so
    ${PMDK_LIBRARIES} ndctl daxctl
  )
else ()
  message(STATUS "don't use NVM based persistent table")
endif()


############################################################################################
# Boost C++ library                                                                        #
############################################################################################
#

SET(BOOST_MIN_VERSION "1.60.0")
SET(BOOST_COMPONENTS
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
if (BUILD_PYTHON)
  SET(BOOST_COMPONENTS
    ${BOOST_COMPONENTS}
    python3
  )
endif()
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS ${BOOST_COMPONENTS})

if (NOT Boost_FOUND)
  message(FATAL_ERROR "Fatal error: Boost (version >= ${BOOST_MIN_VERSION}) required.\n")
else ()
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
)
set(core_libs
  ${core_libs}
  ${BOOST_LIBRARIES}
)


############################################################################################
# Python                                                                                   #
############################################################################################
#

if (BUILD_PYTHON)
  find_package(PythonLibs)
  FIND_PACKAGE(PythonInterp)
  if (PYTHONLIBS_FOUND)
    message(STATUS "Python Version ${PYTHON_VERSION_MAJOR} found - building the Python API.")
    set (PYTHON_INCLUDE_DIRS "/Users/kai/miniconda3/include/python3.5m")
    message("PythonInclude ${PYTHON_INCLUDE_DIRS}")
    message("PythonLibs ${PYTHON_LIBDIR}")

    include_directories(${PYTHON_INCLUDE_DIRS})
    link_directories(${PYTHON_LIBDIR})

    set(PYTHON_SOURCES
      python/PyTopology.cpp
      python/PyAggregateState.cpp
    )
  else()
    set (PYTHON_SOURCES "")
  endif(PYTHONLIBS_FOUND)
else ()
	message(STATUS "Don't build python interface")
	set (PYTHON_SOURCES "")
endif()


############################################################################################
# ZeroMQ library                                                                           #
############################################################################################
#

SET(ZMQ_MIN_VERSION "4.3.1")
find_package(ZeroMQ ${ZMQ_MIN_VERSION} REQUIRED)
if (ZEROMQ_FOUND)
  add_definitions( "-DHAS_ZMQ" )
  include_directories(${ZEROMQ_INCLUDE_DIR})
  link_directories(${ZEROMQ_LIBRARY_DIR})
  set (ZEROMQ_SOURCES
    src/net/ZMQSocket.cpp
    src/qop/ZMQSource.cpp
  )
  set(core_libs
    ${core_libs}
    ${ZEROMQ_LIBRARIES}
  )
  set(core_sources
    ${core_sources}
    ${ZEROMQ_SOURCES}
  )
else()
  message(FATAL_ERROR "Fatal error: ZeroMQ (version >= ${ZMQ_MIN_VERSION}) required.\n")
endif()


############################################################################################
# Matrix Support                                                                           #
############################################################################################
#

if(SUPPORT_MATRICES)
  find_package(Eigen3)
  if(EIGEN3_FOUND)
    message(STATUS "using Eigen3 library for linear algebra")
    include_directories(${EIGEN3_INCLUDE_DIR})
  else()
    message(STATUS "Eigen3 not found")
  endif()
  find_package(OpenCV REQUIRED)
  if(OpenCV_FOUND)
    message(STATUS "using OpenCV library for image processing")
    include_directories(${OpenCV_INCLUDE_DIR})
  else()
    message(STATUS "OpenCV not found")
  endif()
  add_definitions(-DSUPPORT_MATRICES)
endif()


############################################################################################
# RabbitMQ                                                                                 #
############################################################################################
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


############################################################################################
# Apache Kafka                                                                             #
############################################################################################
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


############################################################################################
# MQTT                                                                                     #
############################################################################################
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

############################################################################################
# Linear Road Data Provider                                                                #
############################################################################################
#

if(BUILD_USE_CASES)
  include_directories(${linroad_SOURCE_DIR}/src)
  set(core_libs
    ${core_libs}
    LRDataProvider
  )
endif()

############################################################################################
# Building PipeFabric core library                                                         #
############################################################################################

include_directories(${PROJECT_SOURCE_DIR}/src
                    ${PROJECT_BINARY_DIR}/generated
                    ${PROJECT_SOURCE_DIR}/src/pubsub)

set(core_sources
  ${core_sources}
  src/core/TimestampHelper.cpp
  src/core/Punctuation.cpp
  src/qop/TextFileSource.cpp
  src/qop/RESTSource.cpp
  src/qop/Window.cpp
  src/qop/TriggerNotifier.cpp
  src/dsl/Topology.cpp
  src/dsl/Dataflow.cpp
  src/dsl/PFabricContext.cpp
  src/table/TableInfo.cpp
  src/table/StateContext.cpp
)

add_library(pfabric_core SHARED ${core_sources})
target_link_libraries(pfabric_core ${core_libs})

############################################################################################
# Building PipeFabric CEP library                                                          #
############################################################################################
#

if(NOT BUILD_ONLY_LIB)
  add_library(pfabric_cep SHARED src/cep/Matcher.cpp)
  target_link_libraries(pfabric_cep
    ${BOOST_LIBRARIES}
  )
endif()

############################################################################################
# Building PipeFabric Python library                                                       #
############################################################################################
#

if (BUILD_PYTHON)
  add_library(pyfabric SHARED ${PYTHON_SOURCES})
  target_link_libraries(pyfabric
    ${Boost_PYTHON3_LIBRARY}
    ${PYTHON_LIBRARIES}
    pfabric_core
    ${BOOST_LIBRARIES}
  )
  set_target_properties(pyfabric PROPERTIES PREFIX "" SUFFIX ".so")
endif()

############################################################################################
# Experimental SQL query compiler                                                          #
############################################################################################
#

if(BUILD_USE_CASES OR NOT BUILD_ONLY_LIB)
  add_library(pfabric_qcomp SHARED
    src/qcomp/Plan.cpp
    src/qcomp/PlanCache.cpp
    src/qcomp/QueryCompiler.cpp
    src/qcomp/TypeManager.cpp
    src/qcomp/UniqueNameGenerator.cpp
    src/qcomp/SQLParser.cpp
  )

  target_link_libraries(pfabric_qcomp
    pfabric_core
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${BOOST_LIBRARIES}
  )

  add_subdirectory(src/qcomp qcomp)
endif()


############################################################################################
# Demo project                                                                             #
############################################################################################
#

if(NOT BUILD_ONLY_LIB)
  add_subdirectory(src/demo demo)
endif()


############################################################################################
# Build use cases                                                                          #
############################################################################################
#

if(BUILD_USE_CASES)
  add_definitions(-DBUILD_USE_CASES)
  add_subdirectory(src/usecases usecases)
endif()

############################################################################################
# Unit tests using Catch                                                                   #
############################################################################################
#

enable_testing()
add_subdirectory(test)


############################################################################################
# Installation                                                                             #
############################################################################################
#

# Installation path
set(PIPEFABRIC_DIR "/usr/local/pfabric")

if(BUILD_ONLY_LIB)
  set(PIPEFABRIC_LIBS pfabric_core)
else()
  set(PIPEFABRIC_LIBS pfabric_core pfabric_cep)
endif()
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

############################################################################################
# Misc                                                                                     #
############################################################################################
#

#add_executable(ExecutableName ExecutableFile.cpp)
#target_link_libraries(ExecutableName pfabric_core)

# show used compiler
message("Using Compiler: ${CMAKE_CXX_COMPILER_ID} (Config: ${CMAKE_BUILD_TYPE}).")
