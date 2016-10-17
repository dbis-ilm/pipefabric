add_executable(RestDemo RestDemo.cpp)
target_link_libraries(RestDemo
     pfabric_core
     ${Boost_PROGRAM_OPTIONS_LIBRARY}
     ${Boost_DATE_TIME_LIBRARY}
     ${Boost_FILESYSTEM_LIBRARY}
     ${Boost_SYSTEM_LIBRARY}
     ${Boost_LOG_LIBRARY}
     ${Boost_FILESYSTEM_LIBRARY}
     ${Boost_THREAD_LIBRARY}
 )
