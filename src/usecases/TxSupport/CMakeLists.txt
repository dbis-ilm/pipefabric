add_executable(txproc TxProcessing.cpp)
target_link_libraries(txproc
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

add_executable(txproc2 TxProcessing2.cpp)
target_link_libraries(txproc2
  pfabric_core
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_LOG_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  )