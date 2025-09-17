# 1. 基本项目配置
cmake_minimum_required(VERSION 3.19)
project(Electromagnetic_Compatibility CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

set(DSPFilters_DIR ${CMAKE_SOURCE_DIR}/third_party/DSPFilters-master)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.c"
)
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/include/*.h"
        “${DSPFilters_DIR}/shared/DSPFilters/include/*.h"
)


add_executable(${PROJECT_NAME}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)


target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${DSPFilters_DIR}/shared/DSPFilters/include
)

# 为目标链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Widgets
)

# 6. (可选) 设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)