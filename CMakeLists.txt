cmake_minimum_required(VERSION 3.19)
project(Electromagnetic_Compatibility CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(Qt6_DIR "D:/QT/6.9.3/msvc2022_64/lib/cmake/Qt6")
find_package(Qt6 6.9.3 REQUIRED COMPONENTS Core Widgets Multimedia)
find_package(FFTW3 CONFIG REQUIRED)
qt_standard_project_setup()
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/resource/ui)
set(APP_TARGET_NAME Electromagnetic_Compatibility)

qt_add_executable(${APP_TARGET_NAME}
#        ${src}
        src/main.cpp
        src/core/EMC_Engine.cpp
        src/core/fleet.cpp
        src/core/ship.cpp
        src/core/TransmitterSource.cpp
        src/models/path.cpp
        src/models/PathManager.cpp
        src/models/PropagationModle.cpp
        src/models/shortlist.cpp
        src/utils/data_get.cpp
#        src/widget/devicewidget.cpp
        src/models/move.cpp
        src/mainwindow.cpp
        resource/ui/shipwidget.cpp
        src/models/DataModel.cpp
        include/utils/farfield_get.hpp
        include/log/LogQueue.hpp
        resource/ui/deviceonship.cpp
        resource/ui/devicewidget.cpp
        #        ${header}
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
        include/utils/point_2D.h
        include/mainwindow.h
        resource/ui/shipwidget.h
        include/models/DataModel.h
        resource/ui/devicewidget.h
        resource/ui/deviceonship.h

        #        ${ui}
        resource/ui/mainwindow.ui
        resource/ui/devicewidget.ui
        resource/ui/shipwidget.ui
        resource/ui/deviceonship.ui
        resource/ui/TreeViewManager.cpp
        resource/ui/TreeViewManager.h
        include/models/TransferToEngin.cpp
        include/models/TransferToEngin.h


)

set(DSPFilters_DIR ${CMAKE_SOURCE_DIR}/third_party/DSPFilters-master)
target_include_directories(${APP_TARGET_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${DSPFilters_DIR}/shared/DSPFilters/include
)

# 为目标链接 Qt 库
target_link_libraries(${APP_TARGET_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
)
target_link_libraries(${APP_TARGET_NAME} PRIVATE FFTW3::fftw3)

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
install(SCRIPT ${deploy_script})
