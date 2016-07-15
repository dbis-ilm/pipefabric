
#=============================================
#
# Define a macro to simplify building and linking test executables
#
#=============================================
macro( build_executable arg )
    add_executable( ${arg} "${arg}.cpp" )
    target_link_libraries( ${arg}
      pfabric_core
      ${Boost_SYSTEM_LIBRARY}
       ${MALLOC_LIB}
    )
endmacro( build_executable )

#=============================================
#
# Define a macro to simplify adding tests
#
#=============================================
macro (do_test arg)
    add_test(
        NAME ${arg}
        COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${arg}"
    )
    build_executable( ${arg} )
endmacro(do_test)

set (CTEST_ENVIRONMENT
  "DYLD_FALLBACK_LIBRARY_PATH=${DYLD_LIBRARY_PATH}"
)

#=============================================

do_test(FlowTest)
do_test(TupleTest)
do_test(StreamElementTraitsTest)
do_test(SourceTest)
do_test(SinkTest)
do_test(SignalTest)
do_test(SelectChannelParametersTest)
do_test(ChannelGroupTest)
do_test(TextFileSourceTest)
do_test(RESTSourceTest)
do_test(MapTest)
do_test(WhereTest)
do_test(QueueTest)
do_test(TupleExtractorTest)
do_test(WriterTest)
do_test(WindowTest)
do_test(SHJoinTest)
do_test(TopologyTest)
do_test(AggregateFuncTest)
do_test(AggregationTest)
do_test(GroupedAggregationTest)
do_test(ZMQSourceTest)
# do_test(SeqCEPTest)
do_test(TableTest)
do_test(ToTableTest)
do_test(FromTableTest)
do_test(ContextTest)
