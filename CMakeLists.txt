cmake_minimum_required(VERSION 3.19)
list(APPEND CMAKE_PREFIX_PATH "D:/QT/6.9.3/msvc2022_64")
project(Electromagnetic_Compatibility CXX C)
set(APP_TARGET_NAME Electromagnetic_Compatibility)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(Qt6_DIR "D:/QT/6.9.3/msvc2022_64/lib/cmake/Qt6")
find_package(Qt6 6.9.3 REQUIRED COMPONENTS Core Widgets Multimedia PrintSupport Network Gui)
find_package(FFTW3 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED) 
find_package(OpenMP REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# 确保 spdlog 的 include 目录在 target_include_directories 之前
include_directories(${spdlog_INCLUDE_DIRS})

if(MSVC)
    add_compile_options("/utf-8")
endif()

qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/resource/ui)

# 扫描core
file(GLOB_RECURSE SRC_CORE CONFIGURE_DEPENDS "src/core/*.cpp" "src/core/*.c")
file(GLOB_RECURSE HDR_CORE CONFIGURE_DEPENDS "include/core/*.h")
# 扫描models
file(GLOB_RECURSE SRC_MODELS CONFIGURE_DEPENDS "src/models/*.cpp" "include/models/*.cpp")
file(GLOB_RECURSE HDR_MODELS CONFIGURE_DEPENDS "include/models/*.h" "include/models/*.hpp")
# 扫描UI
file(GLOB_RECURSE UI_FILES   CONFIGURE_DEPENDS "resource/ui/*.ui")
file(GLOB_RECURSE SRC_UI_RES CONFIGURE_DEPENDS "resource/ui/*.cpp")
file(GLOB_RECURSE HDR_UI_RES CONFIGURE_DEPENDS "resource/ui/*.h")
# 扫描 Utils 模块
file(GLOB_RECURSE SRC_UTILS CONFIGURE_DEPENDS "src/utils/*.cpp")
file(GLOB_RECURSE HDR_UTILS CONFIGURE_DEPENDS "include/utils/*.h" "include/utils/*.hpp" "include/utils/*.cpp")
# 扫描SRC
file(GLOB SRC_MAIN CONFIGURE_DEPENDS "src/*.cpp") 
file(GLOB HDR_MAIN CONFIGURE_DEPENDS "include/*.h")
# 扫描图片
file(GLOB_RECURSE RESOURCE_IMAGES CONFIGURE_DEPENDS "resource/Image/*.*")

# 添加UI
add_subdirectory(third_party/ElaWidgetTools)

qt_add_executable(${APP_TARGET_NAME}
    ${SRC_MAIN}      ${HDR_MAIN}
    ${SRC_CORE}      ${HDR_CORE}
    ${SRC_MODELS}    ${HDR_MODELS}
    ${SRC_UI_RES}    ${HDR_UI_RES}  ${UI_FILES}
    ${SRC_UTILS}     ${HDR_UTILS}
)
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
)
# Add Qt resource files to the target
# Use consistent indentation and argument naming for clarity
qt_add_resources(${APP_TARGET_NAME} "app_images"
    PREFIX "/Image"    # 对应 C++ 中的 :/include/Image
    BASE "resource/Image"      # 剪掉物理路径中的 resource/image
    FILES ${RESOURCE_IMAGES}   # 填入图片文件列表
)


# 包含 DSPFilters 库头文件路径
set(DSPFilters_DIR ${CMAKE_SOURCE_DIR}/third_party/DSPFilters-master)
target_include_directories(${APP_TARGET_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/resource/ui
        ${DSPFilters_DIR}/shared/DSPFilters/include
)
set(ELA_QT_VERSION 6 CACHE STRING "Qt Version" FORCE)
# 为目标链接 Qt 库
target_link_libraries(${APP_TARGET_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::Gui
        FFTW3::fftw3
        Eigen3::Eigen
        Qt6::Network
        Qt6::PrintSupport
        spdlog::spdlog
        ElaWidgetTools
)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${APP_TARGET_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

include(GNUInstallDirs)

install(TARGETS ${APP_TARGET_NAME}
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
        TARGET ${APP_TARGET_NAME}
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
)
