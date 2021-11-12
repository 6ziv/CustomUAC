cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(UACmain VERSION 0.1 LANGUAGES CXX)
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

set(PROJECT_SOURCES
		src/main.cpp
		src/uacdata.h
		src/uacdata.cpp
		src/uacparser.cpp
		src/uacparser.h
		src/securedesktop.cpp
		src/securedesktop.h
		src/urlinterceptor.cpp
		src/urlinterceptor.h
		src/allowlist.cpp
		src/allowlist.h
		src/settings.cpp
		src/settings.h
		src/applicationengine.cpp
		src/applicationengine.h
        src/emergencyhandler.cpp
        src/emergencyhandler.h
		src/linkoptions.cpp
        src/UACmain.manifest
)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	qt_add_executable(UACmain MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
	add_executable(UACmain ${PROJECT_SOURCES})
endif()

add_subdirectory(default_qml)
qt_add_resources(UACmain default_theme_bundle PREFIX ${DEFAULT_THEME_PREFIX} BASE ${DEFAULT_THEME_BASE} FILES ${COMPILED_DEFAULT_THEME})

set(TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/translations/zh_CN.ts ${CMAKE_CURRENT_SOURCE_DIR}/src/translations/zh_TW.ts)
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
qt_add_translation(QM_TRANSLATIONS ${TS_FILES})
qt_add_resources(UACmain translation_files PREFIX "/application-translations" BASE "${CMAKE_CURRENT_BINARY_DIR}/translations" FILES ${QM_TRANSLATIONS})
if(NOT DEFINED TESTLINKSHARED)
	set_property(TARGET UACmain PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_compile_options(UACmain PRIVATE /W3 /nologo /EHsc)
target_link_options(UACmain PRIVATE /INTEGRITYCHECK /SUBSYSTEM:WINDOWS /MANIFEST:NO)
target_compile_definitions(UACmain
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(UACmain
  PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick Qt${QT_VERSION_MAJOR}::Qml Qt${QT_VERSION_MAJOR}::QuickControls2 Qt${QT_VERSION_MAJOR}::QuickWidgets Qt${QT_VERSION_MAJOR}::OpenGL Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::EntryPointPrivate)
target_link_libraries(UACmain PRIVATE ${BOOST_LIBRARIES})
target_link_libraries(UACmain PRIVATE unqlite)
target_link_libraries(UACmain PRIVATE WinReg)

qt_import_qml_plugins(UACmain)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(UACmain)
endif()

