cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(Control VERSION 0.1 LANGUAGES CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOSTATICPLUGINS ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick Qml QuickControls2 QuickWidgets LinguistTools Widgets OpenGL EntryPointPrivate REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick Qml QuickControls2 QuickWidgets LinguistTools Widgets OpenGL EntryPointPrivate REQUIRED)

set(TS_FILES src/translations/zh_CN.ts)
set(PROJECT_SOURCES
		src/main.cpp
		src/mainwindow.cpp
		src/mainwindow.h
		src/mainwindow.ui
		src/control.manifest
		${TS_FILES}
)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	qt_add_executable(Control MANUAL_FINALIZATION ${PROJECT_SOURCES})
	qt_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
else()
	add_executable(Control ${PROJECT_SOURCES})
	qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR} ${TS_FILES})
endif()
if(NOT DEFINED TESTLINKSHARED)
	set_property(TARGET Control PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_link_options(Control PRIVATE /SUBSYSTEM:WINDOWS /MANIFEST:NO)
target_compile_definitions(Control
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(Control
  PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::EntryPointPrivate)
target_link_libraries(Control PRIVATE ${BOOST_LIBRARIES})
target_link_libraries(Control PRIVATE unqlite)
target_link_libraries(Control PRIVATE WinReg)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Control)
endif()
install(TARGETS Control DESTINATION ./bin)
