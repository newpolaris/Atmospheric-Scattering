# CMake entry point
cmake_minimum_required (VERSION 3.0)
project (LightScattering)

set(APP_TARGET LightScattering.app)

#if (APPLE)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
#endif()
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC) 

find_package(OpenGL REQUIRED)

# On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)
include(MSVCMultipleProcessCompile) # /MP

if(INCLUDE_DISTRIB)
	add_subdirectory(distrib)
endif(INCLUDE_DISTRIB)

include_directories(external/zlib)

set(UseGLI TRUE)
set(UseZlib TRUE)
set(UseAssImp TRUE)

# Compile external dependencies 
add_subdirectory (external)

set(ASSIMP_INCLUDE_DIRS 
	external/assimp/include 
	${CMAKE_BINARY_DIR}/external/assimp/include)

include_directories(
	src/
	external/
	external/AntTweakBar-1.16/include/
	external/glfw-3.1.2/include/GLFW/
	external/glm/
	external/glew-1.13.0/include/
	${ASSIMP_INCLUDE_DIRS}
	external/bullet-2.81-rev2613/src/
	external/gli/
	external/zlib/
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	GLEW_1130
	imgui
	gli
	assimp
	zlibstatic
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

file( GLOB GLSW external/glsw/*.c external/glsw/*.h )
add_library( glsw ${GLSW} )

file( GLOB_RECURSE SRC src/* )
file( GLOB_RECURSE SHADER_FILE shaders/*.glsl shaders/*.glsli )

set(RESOURCE_FILES
	${SHADER_FILE})

source_group("Resource Files" FILES ${RESOURCE_FILES})

add_executable(${APP_TARGET} ${SRC} ${RESOURCE_FILES})
target_link_libraries(${APP_TARGET} glsw ${ALL_LIBS})

# Xcode and Visual working directories
set_target_properties(${APP_TARGET} PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")
create_target_launcher(${APP_TARGET} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")
