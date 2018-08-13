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

add_executable(mvcc MVCCExample.cpp)
target_link_libraries(mvcc
  pfabric_core
  ${ROCKSDB_LIB}
  ${Boost_SYSTEM_LIBRARY}
  #${BENCHMARK_LIB}
  ${MALLOC_LIB}
  )

add_executable(s2pl S2PLExample.cpp)
target_link_libraries(s2pl
  pfabric_core
  ${ROCKSDB_LIB}
  ${Boost_SYSTEM_LIBRARY}
  ${MALLOC_LIB}
  )

add_executable(generateWorkloads generateWorkloads.cpp)
target_link_libraries(generateWorkloads
  pfabric_core
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_LOG_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  )
