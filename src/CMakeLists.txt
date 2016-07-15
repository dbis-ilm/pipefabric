cmake_minimum_required(VERSION 3.0)
project (pipefabric)

set(CMAKE_MACOSX_RPATH 1)

set (PipeFabric_VERSION_MAJOR 0)
set (PipeFabric_VERSION_MINOR 1)

include(CTest)

#############################
# customization section
#############################
#
# Add our CMake directory to CMake's module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/")

option( USE_BOOST_SPIRIT_PARSER
    "use the boost::spirit::qi parsers for converting strings to tuple attributes"
    ON
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wno-deprecated -Wno-unused-local-typedefs -g -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused -Wno-uninitialized")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
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
include_directories("${PROJECT_SOURCE_DIR}/3rdparty")

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
  topology/PFabricContext.cpp
  3rdparty/format/format.cpp
  ${ZEROMQ_SOURCES}
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

# add_subdirectory(debs2016)
