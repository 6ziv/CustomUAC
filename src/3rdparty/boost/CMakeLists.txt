cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
set(BOOST_SUBDIRS
	array
	assert
	bind
	config
	concept_check
	container_hash
	conversion
	core
	detail
	describe
	format
	function
	functional
	function_types
	fusion
	integer
	io
	iterator
	move
	mpl
	optional
	predef
	preprocessor
	range
	regex
	scope_exit
	smart_ptr
	static_assert
	throw_exception
	tuple
	type_index
	type_traits
	typeof
	utility

	filesystem
	atomic
	align
	winapi
	system
	variant2
	mp11
)
set(COMPILED_BOOST_LIBRARIES "")
foreach(BOOST_SUBDIR ${BOOST_SUBDIRS})
	add_subdirectory(${BOOST_SUBDIR})
	list(APPEND COMPILED_BOOST_LIBRARIES boost_${BOOST_SUBDIR})
endforeach()
if(NOT DEFINED TESTLINKSHARED)
	set_property(TARGET ${COMPILED_BOOST_LIBRARIES} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
set(BOOST_LIBRARIES ${COMPILED_BOOST_LIBRARIES} PARENT_SCOPE)