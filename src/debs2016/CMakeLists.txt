include(../../cmake/Testing.cmake.in)

add_executable(query1 query1.cpp)

target_link_libraries(query1
     pfabric_core
     ${Boost_PROGRAM_OPTIONS_LIBRARY}
     ${Boost_DATE_TIME_LIBRARY}
     ${Boost_FILESYSTEM_LIBRARY}
     ${Boost_SYSTEM_LIBRARY}
     ${Boost_LOG_LIBRARY}
     ${Boost_FILESYSTEM_LIBRARY}
     ${Boost_THREAD_LIBRARY}
     ${LEVELDB_LIB}
 )

 do_test(DEBSTest)
