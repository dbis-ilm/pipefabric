add_definitions(-DTEST_DATA_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/data/")
include_directories(${PROJECT_SOURCE_DIR}/test) #</ catch.hpp
add_executable(graph_test GraphTest.cpp)
target_link_libraries(graph_test
        pfabric_core
        ${Boost_SYSTEM_LIBRARY}
        )

add_executable(image_test ImageProcessingTest.cpp)
target_link_libraries(image_test
        pfabric_core
        ${Boost_SYSTEM_LIBRARY}
        ${OpenCV_LIBS}
        )