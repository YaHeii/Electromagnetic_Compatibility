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

# make sure spdlog include is before target_include_directories 
include_directories(${spdlog_INCLUDE_DIRS})

if(MSVC)
    add_compile_options("/utf-8")
endif()

qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/resource/ui)

# scan models
file(GLOB_RECURSE MODELS CONFIGURE_DEPENDS "Models/*.cpp" "Models/*.h")
# scan Simulation
file(GLOB_RECURSE SIMULATION CONFIGURE_DEPENDS "Simulation/*.cpp" "Simulation/*.h")
# scan Interface
file(GLOB_RECURSE INTERFACEE CONFIGURE_DEPENDS "Interface/*.cpp" "Interface/*.h")
# scan UI
file(GLOB_RECURSE UI_FILES   CONFIGURE_DEPENDS "Resource/ui/*.ui")
file(GLOB_RECURSE SRC_UI_RES CONFIGURE_DEPENDS "Resource/ui/*.cpp")
file(GLOB_RECURSE HDR_UI_RES CONFIGURE_DEPENDS "Resource/ui/*.h")
# scan Utils 模块
file(GLOB_RECURSE UTILS CONFIGURE_DEPENDS "Utils/*.cpp" "Utils/*.h")
#  scan Page
file(GLOB EXAMPLE_PAGE CONFIGURE_DEPENDS "ExamplePage/*.cpp" "ExamplePage/*.h") 
file(GLOB MODEL_VIEW CONFIGURE_DEPENDS "ModelView/*.cpp" "ModelView/*.h") 
#  scan pic
file(GLOB_RECURSE RESOURCE_IMAGES CONFIGURE_DEPENDS "resource/Image/*.*")

# add UI
add_subdirectory(third_party/ElaWidgetTools)
message(STATUS "DEBUG: MODELS = ${MODELS}")
message(STATUS "DEBUG: SIMULATION = ${SIMULATION}")
qt_add_executable(${APP_TARGET_NAME}
    ${MODELS}    ${SIMULATION}
    ${SRC_UI_RES}    ${HDR_UI_RES}  ${UI_FILES}
    ${INTERFACEE}     ${UTILS}
    ${EXAMPLE_PAGE}  ${MODEL_VIEW}
    ${RESOURCE_IMAGES}
)

# Add Qt resource files to the target
# Use consistent indentation and argument naming for clarity
qt_add_resources(${APP_TARGET_NAME} "app_images"
    PREFIX "/Image"    
    BASE "resource/Image"     
    FILES ${RESOURCE_IMAGES}  
)


# set(DSPFilters_DIR ${CMAKE_SOURCE_DIR}/third_party/DSPFilters-master)
# target_include_directories(${APP_TARGET_NAME} PUBLIC
#         ${DSPFilters_DIR}/shared/DSPFilters/include
# )
set(ELA_QT_VERSION 6 CACHE STRING "Qt Version" FORCE)
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