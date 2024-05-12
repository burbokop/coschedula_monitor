cmake_minimum_required(VERSION 3.27.2)

project(
  coschedula_monitor
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appcoschedula_monitor main.cpp)

qt_add_qml_module(
  appcoschedula_monitor
  URI
  coschedula_monitor
  VERSION
  1.0
  QML_FILES
  Main.qml
  SOURCES
  monitor.h
  monitor.cpp
  logitem.h
  logitem.cpp
  matrix.h)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1. If
# you are developing for iOS or macOS you should consider setting an explicit,
# fixed bundle identifier manually though.
set_target_properties(
  appcoschedula_monitor
  PROPERTIES # MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appcoschedula_monitor
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

target_link_libraries(appcoschedula_monitor PRIVATE Qt6::Quick)

include(ExternalProject)
set(DEPENDENCIES_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/dependencies_prefix)
ExternalProject_Add(
  CoSchedula
  GIT_REPOSITORY https://github.com/burbokop/coschedula.git
  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_PREFIX}
             -DENABLE_TESTS=OFF
             -DENABLE_BENCHMARKS=OFF
             -DENABLE_STATIC_BUILD=OFF)

add_dependencies(appcoschedula_monitor CoSchedula)

target_include_directories(appcoschedula_monitor
                           PRIVATE ${DEPENDENCIES_PREFIX}/include)
target_link_directories(appcoschedula_monitor PRIVATE
                        ${DEPENDENCIES_PREFIX}/lib)
target_link_libraries(appcoschedula_monitor PRIVATE coschedula)

include(GNUInstallDirs)
install(
  TARGETS appcoschedula_monitor
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
