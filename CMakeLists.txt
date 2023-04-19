cmake_minimum_required(VERSION 3.0.0)
project(sigrok2vega VERSION 0.1.0)

include(CTest)
enable_testing()

#-------------------------------------------------------------------------------
# Find dependencies
#-------------------------------------------------------------------------------
add_subdirectory(SDL)
set (SDL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include)

#-------------------------------------------------------------------------------
# Set target
#-------------------------------------------------------------------------------
add_executable(sigrok2vega main.cpp)


include_directories(${SDL_INCLUDE_DIRS})
target_link_libraries(sigrok2vega ${SDL_LIBRARIES})


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
