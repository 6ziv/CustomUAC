cmake_minimum_required(VERSION 3.20)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick Qml QuickControls2 QuickWidgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick Qml QuickControls2 QuickWidgets REQUIRED)

set(RESOURCE_FILENAMES background.qml main.qml config.ini)
set(TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/translations/zh_CN.ts ${CMAKE_CURRENT_SOURCE_DIR}/src/translations/zh_TW.ts)

set(EXPANDED_QRC_RESOURCE_FILES "")
foreach(FILENAME ${RESOURCE_FILENAMES})
	string(APPEND EXPANDED_QRC_RESOURCE_FILES "\t" <file> ${FILENAME} </file> "\n")
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/${FILENAME}" "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}" COPYONLY)
endforeach()

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
qt_add_translation(QM_TRANSLATIONS ${TS_FILES})
foreach(FILE ${QM_TRANSLATIONS})
	file(RELATIVE_PATH FILENAME "${CMAKE_CURRENT_BINARY_DIR}" "${FILE}")
	string(APPEND EXPANDED_QRC_RESOURCE_FILES "\t" <file> ${FILENAME} </file> "\n")
endforeach()


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/default.qrc.in" "${CMAKE_CURRENT_BINARY_DIR}/default.qrc")
qt_add_binary_resources(default_theme "${CMAKE_CURRENT_BINARY_DIR}/default.qrc" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/default.rcc")

#set(COMPILED_THEME "")

#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/default_embedded.qrc.in" "${CMAKE_CURRENT_BINARY_DIR}/default_embedded.qrc" COPYONLY)
#qt_add_resources(COMPILED_THEME "${CMAKE_CURRENT_BINARY_DIR}/default_embedded.qrc")

set(COMPILED_DEFAULT_THEME "${CMAKE_CURRENT_BINARY_DIR}/default.rcc" PARENT_SCOPE)
set(DEFAULT_THEME_PREFIX "/uac-internal" PARENT_SCOPE)
set(DEFAULT_THEME_BASE "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
install(TARGETS UACmain DESTINATION ./bin)