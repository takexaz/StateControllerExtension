﻿cmake_minimum_required (VERSION 3.12)

project (StateControllerExtension VERSION 0.1.0 LANGUAGES CXX)

file (GLOB src src/*.cpp src/*.c)
file (GLOB include include/*.hpp include/*.h)

add_library (
	${PROJECT_NAME} SHARED
	${src}
	${include}
)
add_library (${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

find_package(Mebius REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Mebius::Mebius)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".mx")

target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_definitions (
	${PROJECT_NAME}
	PRIVATE
	STCEX_EXPORT
	WIN32
	_WINDOWS
	UNICODE
	_UNICODE
	$<$<CONFIG:Debug>: 
		_DEBUG
		DEBUG
	>
	$<$<CONFIG:Release>:
		NDEBUG
	>
)

target_compile_options (
	${PROJECT_NAME}
	PRIVATE
	/W4
	/arch:AVX
	/EHsc
	/fp:fast
	/GS
	/GT
	/MP
	/nologo
	/Oi
	/Qpar
	/utf-8
	$<$<CONFIG:Debug>:
		/GR
		/guard:cf
		/MDd
		/Ob0
		/Od
		/Zi
	>
	$<$<CONFIG:Release>:
		/GL
		/GR-
		/guard:cf-
		/MD
		/O2
		/Os
		/Zc:inline
	>
)
target_link_options (
	${PROJECT_NAME}
	PRIVATE
	/LTCG
	/nologo
	$<$<CONFIG:Debug>:
		/debug
		/opt:noref
	>
	$<$<CONFIG:Release>:
		/opt:ref
	>
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories (
	${PROJECT_NAME}
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/share>
	$<INSTALL_INTERFACE:include>
)

install (
	TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-config
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

install (
	EXPORT ${PROJECT_NAME}-config
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION cmake
)

install (
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/
	DESTINATION include
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file (
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
	COMPATIBILITY SameMajorVersion
)

install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
	DESTINATION cmake
)