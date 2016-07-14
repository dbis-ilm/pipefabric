# ZeroMQ.cmake from http://lists.gforge.inria.fr/pipermail/simgrid-devel/2012-July/001513.html

SET (ZEROMQ_FIND_QUIETLY TRUE)
SET (ZEROMQ_FIND_REQUIRED FALSE)

IF (NOT ZEROMQ_FOUND)
  # Search user environment for headers, then default paths
  FIND_PATH (ZEROMQ_INCLUDE_DIR zmq.hpp
    PATHS ${ZEROMQROOT}/include $ENV{ZEROMQROOT}/include /usr/local/include
    NO_DEFAULT_PATH)
  FIND_PATH (ZEROMQ_INCLUDE_DIR zmq.hpp)
  GET_FILENAME_COMPONENT (ZEROMQROOT ${ZEROMQ_INCLUDE_DIR} PATH)

  # Search user environment for libraries, then default paths
  FIND_LIBRARY (ZEROMQ_LIBRARIES zmq
    PATHS ${ZEROMQROOT}/lib $ENV{ZEROMQROOT}/lib /usr/local/lib
    NO_DEFAULT_PATH)
  FIND_LIBRARY (ZEROMQ_LIBRARIES zmq)

  # Set ZEROMQ_FOUND and error out if zmq is not found
  INCLUDE (FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS (ZEROMQ
    DEFAULT_MSG ZEROMQ_LIBRARIES ZEROMQ_INCLUDE_DIR)

  IF (ZEROMQ_FOUND)
    MESSAGE (STATUS "ZeroMQ found: zmq library and zmq.hpp ")
    MESSAGE (STATUS "  * includes: ${ZEROMQ_INCLUDE_DIR}")
    MESSAGE (STATUS "  * libs:     ${ZEROMQ_LIBRARIES}")
  ELSE (ZEROMQ_FOUND)
    IF(${ZEROMQ_LIBRARIES} STREQUAL "ZEROMQ_LIBRARIES-NOTFOUND")
       MESSAGE (STATUS "ZeroMQ library does not exist")
    ELSEIF(${ZEROMQ_INCLUDE_DIR} STREQUAL "ZEROMQ_INCLUDE_DIR-NOTFOUND")
	MESSAGE (STATUS "ZeroMQ has a problem: zmq.hpp does not exist, try to download it from https://github.com/zeromq/cppzmq/blob/master/zmq.hpp")
    ENDIF()
  ENDIF (ZEROMQ_FOUND)
ENDIF (NOT ZEROMQ_FOUND)
