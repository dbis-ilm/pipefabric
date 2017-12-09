if(NOT BUILD_ONLY_LIBS)
	#-----------------------------------------------------------------------------------------
	#
	# Building PipeFabric query compiler library
	#
	#add_library(pfabric_qcomp SHARED
	#  Plan.cpp
	#  PlanCache.cpp
	#  QueryCompiler.cpp
	#  TypeManager.cpp
	#  UniqueNameGenerator.cpp
#  SQLParser.cpp)

	#target_link_libraries(pfabric_qcomp
	 # pfabric_core
	 # ${Boost_PROGRAM_OPTIONS_LIBRARY}
	 # ${Boost_FILESYSTEM_LIBRARY}
	 # ${BOOST_LIBRARIES}
 #)

	add_executable(CompilerMain
	  CompilerMain.cpp
	)

	target_link_libraries(CompilerMain
	  pfabric_qcomp
	  pfabric_core
	  ${Boost_PROGRAM_OPTIONS_LIBRARY}
	  ${Boost_DATE_TIME_LIBRARY}
	  ${Boost_FILESYSTEM_LIBRARY}
	  ${Boost_SYSTEM_LIBRARY}
	  ${Boost_LOG_LIBRARY}
	  ${Boost_THREAD_LIBRARY}
	  ${ROCKSDB_LIB}
	)
endif()
