cmake_minimum_required(VERSION 3.27)

set(CMAKE_TOOLCHAIN_FILE ./vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(InvertTextFileExercise)

set(CMAKE_CXX_STANDARD 17)

find_package(utf8cpp CONFIG REQUIRED)

file(GLOB SOURCES
	src/*.h
	src/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
#target_link_libraries(${PROJECT_NAME} utf8cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE utf8::cpp utf8cpp::utf8cpp)