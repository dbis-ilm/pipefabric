file(COPY ${PROJECT_SOURCE_DIR}/usecases/LinearRoad/LinearRoadTest.cpp
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/test
)

add_executable(LinRoad LinRoad.cpp)
target_link_libraries(LinRoad pfabric_core)
