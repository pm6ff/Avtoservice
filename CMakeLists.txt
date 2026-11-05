cmake_minimum_required(VERSION 3.16)
project(AutoService VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Qt 6 (рекомендуется) или Qt 5.15+
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network Svg)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
list(APPEND SOURCES resources/resources.qrc)
if(WIN32)
    enable_language(RC)
    list(APPEND SOURCES resources/app.rc)   # иконка самого .exe
endif()

add_executable(AutoService ${SOURCES})
target_include_directories(AutoService PRIVATE src)
target_link_libraries(AutoService PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg)

if(WIN32)
    set_target_properties(AutoService PROPERTIES WIN32_EXECUTABLE ON)
endif()
