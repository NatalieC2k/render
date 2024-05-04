cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 17)
project(engine)

add_library(engine)
target_sources(engine PUBLIC 
${CMAKE_SOURCE_DIR}/include/window.h ${CMAKE_SOURCE_DIR}/source/window.cpp 
${CMAKE_SOURCE_DIR}/include/threadpool.h ${CMAKE_SOURCE_DIR}/source/threadpool.cpp

${CMAKE_SOURCE_DIR}/include/render.h ${CMAKE_SOURCE_DIR}/source/render.cpp

${CMAKE_SOURCE_DIR}/include/asset.h ${CMAKE_SOURCE_DIR}/source/asset.cpp)

include_directories(${CMAKE_SOURCE_DIR}/include)


find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(engine PUBLIC SDL2::SDL2main SDL2::SDL2)

find_package(spdlog REQUIRED)
include_directories(${SPDLOG_INCLUDE_DIRS})
target_link_libraries(engine PUBLIC spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)


find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/VulkanMemoryAllocator)

target_link_libraries(engine PUBLIC Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)


include_directories(vendor/stb)

set(BUILD_STATIC_LIB ON)
set(BUILD_ASSIMP_TOOLS  ON)
set(ASSIMP_BUILD_STATIC_LIB ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/assimp)
include_directories(${CMAKE_SOURCE_DIR}/vendor/assimp/include)
target_link_libraries(engine PUBLIC assimp)

add_executable(runtime)
target_sources(runtime PUBLIC ${CMAKE_SOURCE_DIR}/main.cpp)
target_link_libraries(runtime PUBLIC engine)


