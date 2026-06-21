cmake_minimum_required(VERSION 3.28)
project(ClothSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(dependencies/glfw)

option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_OBJ_EXPORTER TRUE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
add_subdirectory(dependencies/assimp)


if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()


include_directories(headers/
        dependencies/assimp/include/
        dependencies/glad/include/
        dependencies/glfw/include/
        dependencies/glm/
        dependencies/stb/)

file(GLOB DEPENDENCY_SOURCES dependencies/glad/src/glad.c)
file(GLOB PROJECT_HEADERS headers/*.hpp
        headers/*.h)
file(GLOB PROJECT_SOURCES sources/*.cpp)
file(GLOB PROJECT_SHADERS shaders/*.comp
        shaders/*.frag
        shaders/*.geom
        shaders/*.vert)
file(GLOB_RECURSE PROJECT_RESOURCES resources/*)


source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Shaders" FILES ${PROJECT_SHADERS})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Dependencies" FILES ${DEPENDENCY_SOURCES})
source_group("Resources" FILES ${PROJECT_RESOURCES})
set_source_files_properties(${PROJECT_RESOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_definitions(-DGLFW_INCLUDE_NONE
        -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS}
        ${PROJECT_SHADERS}
        ${DEPENDENCY_SOURCES} ${PROJECT_RESOURCES}
        headers/SoftBody.h
        sources/SoftBody.cpp
        headers/Particle.h
        headers/Spring.h
        sources/Spring.cpp
        sources/Particle.cpp
        headers/Fluid.h
        sources/Fluid.cpp
        headers/Utils.h
        sources/Utils.cpp)
target_link_libraries(${PROJECT_NAME} assimp glfw
        ${GLFW_LIBRARIES} ${GLAD_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        DEPENDS ${PROJECT_SHADERS})


add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
        DEPENDS ${PROJECT_RESOURCES})


#add_executable(ClothSim main.cpp)
