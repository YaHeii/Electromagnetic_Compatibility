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
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)
qt_standard_project_setup()


qt_add_executable(${PROJECT_NAME}
#        ${src}
        src/main.cpp
        src/mainwindow.cpp
        src/core/EMC_Engine.cpp
        src/core/fleet.cpp
        src/core/ship.cpp
        src/core/TransmitterSource.cpp
        src/models/path.cpp
        src/models/PathManager.cpp
        src/models/PropagationModle.cpp
        src/models/shortlist.cpp
        src/utils/data_get.cpp
#        ${header}
        include/mainwindow.h
        include/core/Antenna.h
        include/core/EMC_Engine.h
        include/core/Equipment.h
        include/core/filter.h
        include/core/fleet.h
        include/core/ship.h
        include/core/TransmitterSource.h
        include/models/move.h
        include/models/Path.h
        include/models/PathManager.h
        include/models/PropagationModle.h
        include/models/shortlist.h
        include/utils/conversions.h
        include/utils/data_get.h
        include/utils/farfield_get.hpp
        include/utils/point_2D.h
#        ${ui}
        ui/mainwindow.ui


)

set(DSPFilters_DIR ${CMAKE_SOURCE_DIR}/third_party/DSPFilters-master)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${DSPFilters_DIR}/shared/DSPFilters/include
)

# 为目标链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Widgets
)

include(GNUInstallDirs)

install(TARGETS Electromagnetic_Compatibility
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
        TARGET Electromagnetic_Compatibility
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})