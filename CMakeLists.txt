cmake_minimum_required(VERSION 3.10)

project(Grend VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
	message(WARNING "CMAKE_BUILD_TYPE not set, defaulting to Release")
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS         "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG   "-DGREND_BUILD_DEBUG=1 -Og -g")
set(CMAKE_CXX_FLAGS_RELEASE "-DGREND_BUILD_RELEASE=1")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#option(BUILD_SHARED_LIBS "build the shared stuff" ON)
option(PHYSICS_BULLET "Use the bullet physics library" ON)
option(GREND_ERROR_CHECK "Enable graphics library error checks" ON)
option(PORTABLE_BUILD    "Portable build, include all dependancies in the install" OFF)
message(STATUS "ASSETS:  ${CMAKE_ANDROID_ASSETS_DIRECTORIES}")
message(STATUS "ASSETS2: ${APK_DIR}")
message(STATUS "ASSETS2: ${APK_ANDROID_EXTRA_FILES}")

# GLSL version is also used to determine OpenGL API version
#set(GLSL_VERSION 100) # OpenGL ES 2.0
set(GLSL_VERSION 300) # OpenGL ES 3.0
#set(GLSL_VERSION 330)  # OpenGL core 3.30
#set(GLSL_VERSION 430) # OpenGL core 4.30

include(GNUInstallDirs)
add_subdirectory(libs/nanovg)

set(GREND_LINK_LIBS)
set(GR_PREFIX)
## used in the grend.pc file, so things can know which GL library to link
set(GREND_GL_PKG)

if (PORTABLE_BUILD)
	set(GR_PREFIX "./")
endif()

if (ANDROID)
	message(STATUS "Target: Android")
	# TODO: how's this going to work in production, just require assets
	#       be unpacked onto the SD card? I'm definitely not rewriting
	#       every library that touches C/C++ file i/o
	#add_compile_options(-DGR_PREFIX="/sdcard/grend/")
	if (NOT PORTABLE_BUILD)
		set(GR_PREFIX "/sdcard/grend/")
	endif()

	add_compile_options(-DMAX_LIGHTS=8)
	# TODO: toggleable for old adreno only...?
	#add_compile_options(-DUSE_SINGLE_UBO)
	add_compile_options(-DBROKEN_UINTS)

	# TODO: ideally this could be deduplicated
	if (GLSL_VERSION EQUAL 100)
		add_compile_options(-DIMGUI_IMPL_OPENGL_ES2)
		find_library(GLESv2 GLESv2)
		list(APPEND GREND_LINK_LIBS GLESv2)
	elseif(GLSL_VERSION EQUAL 300)
		add_compile_options(-DIMGUI_IMPL_OPENGL_ES3)
		find_library(GLESv3 GLESv3)
		list(APPEND GREND_LINK_LIBS GLESv3)
	else()
		message(FATAL_ERROR "Invalid GLSL version for android")
	endif()

	find_library(SDL2 SDL2)
	find_library(BulletCollision Bullet3Collision)
	find_library(BulletDynamics  Bullet3Dynamics)
	find_library(BulletSoftBody  BulletSoftBody)
	find_library(LinearMath      LinearMath)
	find_library(glm             glm)

	list(APPEND GREND_LINK_LIBS
	     SDL2 BulletCollision BulletDynamics BulletSoftBody LinearMath glm)

elseif (MINGW OR WIN32 OR CYGWIN)
	message(STATUS "Target: Windows")

	if (NOT PORTABLE_BUILD)
		set(GR_PREFIX "${CMAKE_INSTALL_PREFIX}/share/grend/")
	endif()

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL2   REQUIRED IMPORTED_TARGET sdl2)
	pkg_check_modules(GLM    REQUIRED IMPORTED_TARGET glm)

	# only have glew on windows
	pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)
	message(STATUS "GL: Using glew")
	set(GREND_GL_PKG "glew")
	add_compile_options(-D_WIN32) # mingw doesn't seem to define this?
	                              # might as well explicitly define it
	add_compile_options(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)

	# use in-tree library on windows, bullet packaged with msys2 seems
	# to be a bit borked (function signatures don't match, should submit
	# a bug report/fix...)
	find_library(BulletCollision Bullet3Collision)
	find_library(BulletDynamics  Bullet3Dynamics)
	find_library(BulletSoftBody  BulletSoftBody)
	find_library(LinearMath      LinearMath)
	find_library(opengl32        opengl32)

	list(APPEND GREND_LINK_LIBS
		PkgConfig::SDL2
		PkgConfig::Glew
		#PkgConfig::Bullet
		BulletCollision BulletDynamics BulletSoftBody LinearMath
		pthread
		opengl32
	)

# TODO: assume unixen, presumably there'll be different library lookup
#       stuff for windows/etc
else()
	message(STATUS "Target: Unix-ish")
	#add_compile_options(-DGR_PREFIX="${CMAKE_INSTALL_PREFIX}/share/grend/")
	if (NOT PORTABLE_BUILD)
		set(GR_PREFIX "${CMAKE_INSTALL_PREFIX}/share/grend/")
	endif()

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL2   REQUIRED IMPORTED_TARGET sdl2)
	pkg_check_modules(Bullet REQUIRED IMPORTED_TARGET bullet)
	pkg_check_modules(GLM    REQUIRED IMPORTED_TARGET glm)

	list(APPEND GREND_LINK_LIBS
		PkgConfig::SDL2
		PkgConfig::Bullet
		BulletCollision BulletDynamics BulletSoftBody LinearMath
		pthread
	)

	if (GLSL_VERSION EQUAL 100)
		message(STATUS "GL: Using GLESv2")
		set(GREND_GL_PKG "glesv2")

		add_compile_options(-DIMGUI_IMPL_OPENGL_ES2)
		pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
		list(APPEND GREND_LINK_LIBS GLESv2)

	elseif(GLSL_VERSION EQUAL 300)
		message(STATUS "GL: Using GLESv3")
		set(GREND_GL_PKG "glesv2")

		add_compile_options(-DIMGUI_IMPL_OPENGL_ES3)
		# seems gles3 is part of the gles2 library
		pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
		list(APPEND GREND_LINK_LIBS PkgConfig::GLESv2)

	else()
		message(STATUS "GL: Using glew")
		set(GREND_GL_PKG "glew")

		add_compile_options(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)
		pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)
		list(APPEND GREND_LINK_LIBS PkgConfig::Glew)
	endif()
endif()

set(INCLUDES include/grend)
set(LIBRARY_INCLUDES 
	libs
	libs/tinygltf
	libs/nanovg/src
	libs/imgui
	libs/json/single_include
	libs/stb)

#include_directories(include)
#include_directories(${LIBRARY_INCLUDES})
#include_directories(PkgConfig::GLM)

#[[
add_compile_definitions(
	GLSL_VERSION=${GLSL_VERSION}
	IMGUI_IMPL_OPENGL_LOADER_GLEW
	GREND_ERROR_CHECK=1 # TODO: use cmake debug/release build modes
	#GR_PREFIX="${PROJECT_SOURCE_DIR}/test-install/share/grend/"
	# TODO: this obviously breaks portable installs
	GR_PREFIX="${CMAKE_INSTALL_PREFIX}/share/grend/"
)
]]

if (PHYSICS_BULLET)
	#add_compile_definitions(PHYSICS_BULLET)
	#add_compile_options(-DPHYSICS_BULLET)
	set(PHYSICS_IMPLEMENTATION src/bulletPhysics.cpp)
else()
	#add_compile_definitions(PHYSICS_IMP)
	#add_compile_options(-DPHYSICS_IMP)
	set(PHYSICS_IMPLEMENTATION src/impPhysics.cpp)
endif()

add_library(Grend
	${PHYSICS_IMPLEMENTATION}
	src/animation.cpp
	src/controllers.cpp
	src/gameObject.cpp
	src/octree.cpp
	src/quadtree.cpp
	src/utility.cpp
	src/audioMixer.cpp
	src/camera.cpp
	src/editorInput.cpp
	src/editorMap.cpp
	src/editorMapWindow.cpp
	src/editorMenubar.cpp
	src/editorMetricsWindow.cpp
	src/editorObjectEditorWindow.cpp
	src/editorObjectSelectWindow.cpp
	src/fileDialog.cpp
	src/geometryGeneration.cpp
	src/glManager.cpp
	src/glManagerBuffer.cpp
	src/glManagerShader.cpp
	src/glManagerTexture.cpp
	src/shaderPreprocess.cpp
	src/mainLogic.cpp
	src/modalSDLInput.cpp
	src/model.cpp
	src/renderer.cpp
	src/rendererProbes.cpp
	src/sdlContext.cpp
	src/spatialAudioChannel.cpp
	src/text.cpp
	src/textureAtlas.cpp
	src/timers.cpp
	src/vecGUI.cpp
	src/gameMainDevWindow.cpp
	src/jobQueue.cpp
	src/bufferAllocator.cpp
	src/gameMain.cpp
	src/renderFramebuffer.cpp
	src/plane.cpp
	src/gltfModel.cpp
	src/objModel.cpp
	src/editor.cpp
	src/skybox.cpp
	src/ecsEntityManager.cpp
	src/ecsCollision.cpp
	src/ecsRigidBody.cpp
	src/gameView.cpp
	src/ecsSerializer.cpp
	src/editorEditorSelectWindow.cpp
	src/renderQueue.cpp
	src/playerView.cpp
	src/tinygltf.cpp

	libs/imgui/imgui.cpp
	libs/imgui/imgui_demo.cpp
	libs/imgui/imgui_draw.cpp
	libs/imgui/imgui_tables.cpp
	libs/imgui/imgui_widgets.cpp

	libs/imgui/backends/imgui_impl_opengl3.cpp
	libs/imgui/backends/imgui_impl_sdl.cpp

	libs/stb/stbi.cpp
	libs/stb/stb_vorbis.c
)

target_include_directories(Grend PUBLIC
	"${PROJECT_BINARY_DIR}"
	${BULLET_PHYSICS_SOURCE_DIR}/src
	include
	${LIBRARY_INCLUDES}
)
target_link_libraries(Grend
	${GREND_LINK_LIBS}
	nanovg
)
#target_compile_options(Grend PUBLIC "-O2")

get_target_property(GREND_COMPILE_OPTIONS_LIST Grend COMPILE_OPTIONS)
set(GREND_COMPILE_OPTIONS)
foreach (item ${GREND_COMPILE_OPTIONS_LIST})
	string(APPEND GREND_COMPILE_OPTIONS "${item} ")
endforeach()

message(STATUS "Compile options: ${GREND_COMPILE_OPTIONS}")
configure_file(grend-config.h.in grend-config.h)
configure_file(grend.pc.in grend.pc @ONLY)

# TODO: need to have library includes under grend folder
install(TARGETS Grend DESTINATION ${CMAKE_INSTALL_LIBDIR})
if (NOT ANDROID)
	# XXX: figure out how to do this properly
	install(TARGETS nanovg DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(FILES     ${PROJECT_BINARY_DIR}/grend.pc       DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(FILES     ${PROJECT_BINARY_DIR}/grend-config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${INCLUDES}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${LIBRARY_INCLUDES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/grend)
install(DIRECTORY shaders/lib         DESTINATION ${CMAKE_INSTALL_DATADIR}/grend/shaders)
install(DIRECTORY shaders/lighting    DESTINATION ${CMAKE_INSTALL_DATADIR}/grend/shaders)
install(DIRECTORY shaders/src         DESTINATION ${CMAKE_INSTALL_DATADIR}/grend/shaders)
install(DIRECTORY assets              DESTINATION ${CMAKE_INSTALL_DATADIR}/grend)
install(CODE "message(\"Testing this! \${CMAKE_INSTALL_PREFIX}\")")
