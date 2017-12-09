if (NOT BUILD_ONLY_LIBS)
	 add_executable(TrackerServer TrackerServer.cpp
      TrajectoryDB.cpp
      GeoUtils.cpp
      QueryLoop.cpp
      Pattern.cpp
      PrefixSpan.cpp
      WebServer.cpp)

	 target_link_libraries(TrackerServer
     pfabric_qcomp
 		 pfabric_core
      ${ROCKSDB_LIB}
 		 ${Boost_PROGRAM_OPTIONS_LIBRARY}
 		 ${Boost_DATE_TIME_LIBRARY}
 		 ${Boost_FILESYSTEM_LIBRARY}
 		 ${Boost_SYSTEM_LIBRARY}
 		 ${Boost_LOG_LIBRARY}
 		 ${Boost_FILESYSTEM_LIBRARY}
 		 ${Boost_THREAD_LIBRARY}
 	 )

endif()
