if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

### Zlib ###
if (UseZlib)
	set(SKIP_INSTALL_ALL FALSE)
	add_subdirectory(zlib)
endif(UseZlib)

### GLFW ###

add_subdirectory (glfw-3.1.2)

include_directories(
	glfw-3.1.2/include/
	glew-1.13.0/include/
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	-lGL -lGLU -lXrandr -lXext -lX11 -lrt
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

### GLEW ###

set(GLEW_SOURCE
	glew-1.13.0/src/glew.c
)

set(GLEW_HEADERS
)


add_library( GLEW_1130 STATIC
	${GLEW_SOURCE}
	${GLEW_INCLUDE}
)

target_link_libraries(GLEW_1130
	${OPENGL_LIBRARY}
	${EXTRA_LIBS}
)


### ANTTWEAKBAR ###

if(UseAntTweakBar)
	set(ANTTWEAKBAR_HEADERS
		AntTweakBar-1.16/src/AntPerfTimer.h
		AntTweakBar-1.16/src/LoadOGL.h
		AntTweakBar-1.16/src/LoadOGLCore.h
		AntTweakBar-1.16/src/MiniGLFW.h
		AntTweakBar-1.16/src/resource.h
		AntTweakBar-1.16/src/TwBar.h
		AntTweakBar-1.16/src/TwColors.h
		AntTweakBar-1.16/src/TwFonts.h
		AntTweakBar-1.16/src/TwGraph.h
		AntTweakBar-1.16/src/TwMgr.h
		AntTweakBar-1.16/src/TwOpenGL.h
		AntTweakBar-1.16/src/TwOpenGLCore.h
		AntTweakBar-1.16/src/TwPrecomp.h
	)

	include_directories(
		AntTweakBar-1.16/include/
	)

	add_library( ANTTWEAKBAR_116_OGLCORE_GLFW STATIC
		${ANTTWEAKBAR_SOURCE}
		${ANTTWEAKBAR_HEADERS}
	)

	target_link_libraries(ANTTWEAKBAR_116_OGLCORE_GLFW
		${OPENGL_LIBRARY}
		${EXTRA_LIBS}
	)
endif(UseAntTweakBar)

### ASSIMP ###
# AssImp already has a CMakeLists.txt so let's use these

if(UseAssImp)
	add_subdirectory(assimp)
endif(UseAssImp)


### BULLET ###
# Bullet already has a CMakeLists.txt so let's use these

if(UseBullet)
	set(BULLET_VERSION 2.81)
	include_directories(
		bullet-2.81-rev2613/src
	)
	add_subdirectory( bullet-2.81-rev2613/src/BulletSoftBody )
	add_subdirectory( bullet-2.81-rev2613/src/BulletCollision )
	add_subdirectory( bullet-2.81-rev2613/src/BulletDynamics )
	add_subdirectory( bullet-2.81-rev2613/src/LinearMath )
endif(UseBullet)

### FreeImage ###
if (UseFreeImage)
	add_subdirectory(FreeImage)
endif(UseFreeImage)

### ImGUI ###
add_subdirectory(ImGUI)

### NV_DDS ###
if (UseNVDDS)
	# supoort : DXTC1, DXTC3, DXTC5, RGB (888, 888X), and RGBA (8888)
	include_directories( nv_dds )
	add_library( nv_dds STATIC nv_dds/nv_dds.cpp)
endif(UseNVDDS)

### GLI ###
if (UseGLI)
	add_subdirectory(gli)
endif(UseGLI)
