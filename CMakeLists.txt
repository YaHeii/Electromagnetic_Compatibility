cmake_minimum_required(VERSION 3.19)
list(APPEND CMAKE_PREFIX_PATH "D:/QT/6.9.3/msvc2022_64")

project(Electromagnetic_Compatibility CXX C)

set(APP_TARGET_NAME Electromagnetic_Compatibility)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(Qt6_DIR "D:/QT/6.9.3/msvc2022_64/lib/cmake/Qt6")
find_package(Qt6 6.9.3 REQUIRED COMPONENTS Core Widgets Multimedia PrintSupport Network Gui)
find_package(FFTW3 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Make sure spdlog include is before target_include_directories.
include_directories(${spdlog_INCLUDE_DIRS})

if(MSVC)
    add_compile_options("/utf-8")
endif()

qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/Resource/ui)

file(GLOB_RECURSE MODELS CONFIGURE_DEPENDS "Models/*.cpp" "Models/*.h")
file(GLOB_RECURSE SIMULATION CONFIGURE_DEPENDS "Simulation/*.cpp" "Simulation/*.h")
file(GLOB_RECURSE INTERFACEE CONFIGURE_DEPENDS "Interface/*.cpp" "Interface/*.h")
file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS "Resource/ui/*.ui")
file(GLOB_RECURSE SRC_UI_RES CONFIGURE_DEPENDS "Resource/ui/*.cpp")
file(GLOB_RECURSE HDR_UI_RES CONFIGURE_DEPENDS "Resource/ui/*.h")
file(GLOB_RECURSE UTILS CONFIGURE_DEPENDS "Utils/*.cpp" "Utils/*.h")
file(GLOB_RECURSE EXAMPLE_PAGE CONFIGURE_DEPENDS "ExamplePage/*.cpp" "ExamplePage/*.h")
file(GLOB_RECURSE MODEL_VIEW CONFIGURE_DEPENDS "ModelView/*.cpp" "ModelView/*.h")
file(GLOB_RECURSE RESOURCE_IMAGES CONFIGURE_DEPENDS "Resource/Image/*.*")

option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_TESTING "Build tests" ON)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/ElaWidgetTools)

add_executable(${APP_TARGET_NAME}
    main.cpp
    ${MODELS}
    ${SIMULATION}
    ${SRC_UI_RES}
    ${HDR_UI_RES}
    ${UI_FILES}
    ${INTERFACEE}
    ${UTILS}
    ${EXAMPLE_PAGE}
    ${MODEL_VIEW}
    ${RESOURCE_IMAGES}
)

qt_add_resources(${APP_TARGET_NAME} "app_images"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Image"
    FILES ${RESOURCE_IMAGES}
)

target_include_directories(${APP_TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

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

if(BUILD_TESTING)
    enable_testing()
    find_package(Catch2 CONFIG REQUIRED)

    add_executable(SchemaDtoValidationTests
        Tests/SchemaDtoValidationTests.cpp
        Tests/ReportflowTests.cpp
        Tests/SimulationResultTests.cpp
        Tests/SimulationResultGalleryTests.cpp
        Tests/SimulationSchedulerTests.cpp
        Tests/UiMainlineTests.cpp
        Tests/TreeViewModelTests.cpp
        Interface/ReportFlowContract.h
        Interface/DataModel.h
        Interface/SimulationResult.h
        Interface/SchemaConstants.h
        Interface/TransferToEngin.cpp
        Interface/TransferToEngin.h
        Utils/JsonLoader.hpp
        Simulation/EMC_Engine.cpp
        Simulation/EMCComputationResult.h
        Simulation/EMC_Engine.h
        Simulation/EMCMetricsCalculator.cpp
        Simulation/EMCMetricsCalculator.h
        Simulation/PEModel.cpp
        Simulation/PEModel.h
        Simulation/PEPropagationSolver.cpp
        Simulation/PEPropagationSolver.h
        Simulation/simSchedulerCtx.cpp
        Simulation/simSchedulerCtx.h
        ModelView/T_TreeItem.cpp
        ModelView/T_TreeItem.h
        ModelView/T_TreeViewModel.cpp
        ModelView/T_TreeViewModel.h
        Resource/ui/BasePage.cpp
        Resource/ui/BasePage.h
        Resource/ui/DeviceWidget.cpp
        Resource/ui/DeviceWidget.h
        Resource/ui/EnvironmentWidget.cpp
        Resource/ui/EnvironmentWidget.h
        Resource/ui/qcustomplot.cpp
        Resource/ui/qcustomplot.h
        Resource/ui/SimulationResultCatalog.cpp
        Resource/ui/SimulationResultCatalog.h
        Resource/ui/TreeView.cpp
        Resource/ui/TreeView.h
        Resource/ui/deviceonship.cpp
        Resource/ui/deviceonship.h
        Resource/ui/shipwidget.cpp
        Resource/ui/shipwidget.h
        Utils/Reportflow/ReportContextBuilder.cpp
        Utils/Reportflow/ReportContextBuilder.h
        Utils/Reportflow/ReportJobExporter.cpp
        Utils/Reportflow/ReportJobExporter.h
        Utils/SimulationChartRenderer.cpp
        Utils/SimulationChartRenderer.h
    )

    target_include_directories(SchemaDtoValidationTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_compile_definitions(SchemaDtoValidationTests PRIVATE
        EMC_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(SchemaDtoValidationTests PRIVATE
        Catch2::Catch2WithMain
        Qt6::Core
        Qt6::Gui
        Qt6::PrintSupport
        Qt6::Widgets
        spdlog::spdlog
        ElaWidgetTools
        Eigen3::Eigen
        FFTW3::fftw3
        OpenMP::OpenMP_CXX
    )

    include(Catch)
    catch_discover_tests(SchemaDtoValidationTests)
endif()

include(GNUInstallDirs)

install(TARGETS ${APP_TARGET_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ${APP_TARGET_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
