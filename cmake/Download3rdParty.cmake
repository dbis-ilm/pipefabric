
# We download some 3rdparty modules from github.com via DownloadProject
include(DownloadProject)

set(THIRD_PARTY_DIR "${PROJECT_BINARY_DIR}/3rdparty")

# the JSON library
download_project(PROJ               json
                GIT_REPOSITORY      https://github.com/nlohmann/json.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/json
        COMMAND ${CMAKE_COMMAND} -E copy
                ${json_SOURCE_DIR}/src/json.hpp
                ${THIRD_PARTY_DIR}/json)

# the format library
download_project(PROJ               Format
                GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E copy
                ${Format_SOURCE_DIR}/fmt/format.h
                ${THIRD_PARTY_DIR}/fmt
        COMMAND ${CMAKE_COMMAND} -E copy
                ${Format_SOURCE_DIR}/fmt/format.cc
                ${THIRD_PARTY_DIR}/fmt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_HEADER_ONLY=1")
include_directories("${THIRD_PARTY_DIR}/fmt")

# the SimpleWeb library
download_project(PROJ               SimpleWeb
                GIT_REPOSITORY      https://github.com/eidheim/Simple-Web-Server.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
                QUIET
)
add_custom_command(
        OUTPUT ${THIRD_PARTY_DIR}/SimpleWeb
        COMMAND ${CMAKE_COMMAND} -E make_directory ${THIRD_PARTY_DIR}/SimpleWeb
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${SimpleWeb_SOURCE_DIR}
                ${THIRD_PARTY_DIR}/SimpleWeb)

download_project(PROJ               benchmark
                GIT_REPOSITORY      https://github.com/google/benchmark.git
                GIT_TAG             master
                UPDATE_DISCONNECTED 1
#                QUIET
)
