include(../../cmake/Testing.cmake.in)

if (BUILD_BENCHMARKS)
	if (BUILD_GOOGLE_BENCH)
      do_test(TopologyBenchmarks)
      do_test(MemoryBenchmarks)
    endif()
endif()
