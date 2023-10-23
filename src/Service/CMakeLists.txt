cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(UACService VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
		src/ServiceMain.cpp
		src/FileUtils.hpp
		src/Privileges.hpp
		src/Utils.hpp
		src/UACService.manifest
)

add_executable(UACService ${PROJECT_SOURCES})

if(NOT DEFINED TESTLINKSHARED)
	set_property(TARGET UACService PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
SET_TARGET_PROPERTIES(UACService PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
target_compile_definitions(UACService
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(UACService PRIVATE ${BOOST_LIBRARIES})
target_link_libraries(UACService PRIVATE WinReg)
install(TARGETS UACService DESTINATION ./bin)