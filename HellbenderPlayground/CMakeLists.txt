cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

project(HellbenderPlayground CXX)

set(PATHFINDER_FILES
        Pathfinder/Systems/AgentSystem.cpp
        Pathfinder/Systems/AgentSystem.h
        Pathfinder/Systems/SimTransformSystem.h
        Pathfinder/Systems/SimTransformSystem.cpp
        Pathfinder/Systems/PlayerInputSystem.cpp
        Pathfinder/Systems/PlayerInputSystem.h
        Pathfinder/Navmesh.cpp
        Pathfinder/Navmesh.h Pathfinder/Map.cpp
        Pathfinder/Map.h
        Pathfinder/Systems/SimCamera2DSystem.cpp
        Pathfinder/Systems/SimCamera2DSystem.h)

set(VOXEL_MODULE_FILES VoxelModule/VoxelRendererSystem.cpp)


set(EXEMPLES_FILES
        ../Exemples/Text/TextScene.h
        Examples/FPSCounter.h
        ../Exemples/Triangle/TriangleScene.h
        Examples/OrderedRenderingScene.h)

add_executable(${PROJECT_NAME}
        main.cpp
        ${EXEMPLES_FILES}
        ${VOXEL_MODULE_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC HBE)
target_include_directories(${PROJECT_NAME}
        PUBLIC "${${CMAKE_CURRENT_SOURCE_DIR}}")


set_target_properties(${PROJECT_NAME}
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${HBE_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${HBE_OUTPUT_DIR}
        )

#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${HBE_OUTPUT_DIR})

