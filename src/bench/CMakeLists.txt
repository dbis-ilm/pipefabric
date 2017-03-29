include(../../cmake/Testing.cmake.in)

if (BUILD_BENCHMARKS)
  do_test(TopologyBenchmarks)
endif()
