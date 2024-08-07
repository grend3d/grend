cmake_minimum_required(VERSION 3.10)

project(Grend VERSION 0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
	message(WARNING "CMAKE_BUILD_TYPE not set, defaulting to Release")
	set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(libs/parserlib)

# for building grend as a library, including bullet as part of it
if (NOT BULLET_PHYSICS_SOURCE_DIR AND EXISTS ${PROJECT_SOURCE_DIR}/bullet)
	message(STATUS "Found bullet subdirectory, using that as library")
	set(BULLET_PHYSICS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/bullet")
	add_subdirectory(${BULLET_PHYSICS_SOURCE_DIR})
endif()

set(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_C_FLAGS "-Wall -Wl,--export-dynamic -Wl,--export-dynamic")
if (NOT EMSCRIPTEN)
	set(CMAKE_C_FLAGS_DEBUG   "-DGREND_BUILD_DEBUG=1 -Og -g")
	set(CMAKE_C_FLAGS_RELEASE "-DGREND_BUILD_RELEASE=1 -O2")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wnon-virtual-dtor -Werror=non-virtual-dtor -fconcepts-diagnostics-depth=2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#option(BUILD_SHARED_LIBS "build the shared stuff" ON)
option(GREND_ERROR_CHECK  "Enable graphics library error checks (Development only)" ON)
option(GREND_USE_G_BUFFER "Enable G-Buffer output (uses more memory, but more advanced rendering techniques depend on it)" ON)
option(PHYSICS_BULLET "Use the bullet physics library" ON)
option(PORTABLE_BUILD    "Portable build, include all dependancies in the install" OFF)
message(STATUS "ASSETS:  ${CMAKE_ANDROID_ASSETS_DIRECTORIES}")
message(STATUS "ASSETS2: ${APK_DIR}")
message(STATUS "ASSETS2: ${APK_ANDROID_EXTRA_FILES}")

# GLSL version is also used to determine OpenGL API version
# 100 -> OpenGL ES 2.0
# 300 -> OpenGL ES 3.0
# 330 -> OpenGL Core 3.30
# 430 -> OpenGL Core 4.30
set(GLSL_VERSION 300 CACHE STRING "GL shading language version (determines target opengl version)")

include(GNUInstallDirs)

set(GREND_LINK_LIBS)
set(GR_PREFIX)
## used in the grend.pc file, so things can know which GL library to link
set(GREND_PKG)
set(GREND_GL_PKG)

# add parser combinator lib as a dependency
list(APPEND GREND_LINK_LIBS ParserLib)

if (PORTABLE_BUILD)
	set(GR_PREFIX "./share/grend/")
endif()

if (PHYSICS_BULLET)
	#add_compile_definitions(PHYSICS_BULLET)
	#add_compile_options(-DPHYSICS_BULLET)
	set(PHYSICS_IMPLEMENTATION src/bulletPhysics.cpp src/bulletDebugDrawer.cpp)
else()
	#add_compile_definitions(PHYSICS_IMP)
	#add_compile_options(-DPHYSICS_IMP)
	set(PHYSICS_IMPLEMENTATION src/impPhysics.cpp)
endif()

add_library(Grend
	${PHYSICS_IMPLEMENTATION}
	src/animation.cpp
	src/animationController.cpp
	src/controllers.cpp
	src/gameObject.cpp
	src/octree.cpp
	src/quadtree.cpp
	src/utility.cpp
	src/audioMixer.cpp
	src/camera.cpp
	src/compiledModel.cpp

	src/editor/editor.cpp
	src/editor/input.cpp
	src/editor/map.cpp
	src/editor/mapWindow.cpp
	src/editor/menubar.cpp
	src/editor/metricsWindow.cpp
	src/editor/objectEditorWindow.cpp
	src/editor/objectSelectWindow.cpp
	src/editor/profiler.cpp
	src/editor/settings.cpp
	src/editor/entitySelectWindow.cpp
	src/editor/logWindow.cpp
	src/editor/filePane.cpp
	src/editor/fileDialog.cpp
	src/editor/fileBookmarks.cpp
	src/editor/ui.cpp
	src/editor/ecsTypes.cpp
	src/editor/nodeTypes.cpp

	src/editor/thumbnails.cpp

	# TODO: maybe just part of the editor
	src/gridDraw.cpp

	src/logger.cpp
	src/geometryGeneration.cpp
	src/glManager.cpp
	src/glManagerBuffer.cpp
	src/glManagerShader.cpp
	src/glManagerTexture.cpp
	src/IoC.cpp
	src/shaderPreprocess.cpp
	src/mainLogic.cpp
	src/modalSDLInput.cpp
	src/model.cpp
	src/renderer.cpp
	src/renderPostStage.cpp
	src/renderPostChain.cpp
	src/renderPostJoin.cpp
	src/rendererProbes.cpp
	src/renderFramebuffer.cpp
	src/renderQueue.cpp
	src/renderUtils.cpp
	src/multiRenderQueue.cpp
	src/sdlContext.cpp
	src/spatialAudioChannel.cpp
	src/text.cpp
	src/textureAtlas.cpp
	src/timers.cpp
	src/gameMainDevWindow.cpp
	src/jobQueue.cpp
	src/bufferAllocator.cpp
	src/gameMain.cpp
	src/plane.cpp
	src/gltfModel.cpp
	src/objModel.cpp
	src/skybox.cpp
	src/ecsEntityManager.cpp
	src/ecsCollision.cpp
	src/ecsRigidBody.cpp
	src/ecsShader.cpp
	src/gameView.cpp
	src/ecsSerializer.cpp
	src/playerView.cpp
	src/tinygltf.cpp
	src/scancodes.cpp
	src/glslParser.cpp
	src/glslObject.cpp
	src/textureData.cpp
	src/transform.cpp
	src/serializeDefs.cpp

	src/base64.c

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

add_executable(shaderComp shaderComp.cpp)
target_link_libraries(shaderComp Grend)
install(TARGETS shaderComp DESTINATION ${CMAKE_INSTALL_BINDIR})

if (ANDROID)
	message(STATUS "Target: Android")
	# TODO: how's this going to work in production, just require assets
	#       be unpacked onto the SD card? I'm definitely not rewriting
	#       every library that touches C/C++ file i/o
	#add_compile_options(-DGR_PREFIX="/sdcard/grend/")
	if (NOT PORTABLE_BUILD)
		set(GR_PREFIX "${CMAKE_INSTALL_PREFIX}/share/grend/")
	endif()

	target_compile_options(Grend PUBLIC -DMAX_LIGHTS=8)
	# TODO: toggleable for old adreno only...?
	#add_compile_options(-DUSE_SINGLE_UBO)
	target_compile_options(Grend PUBLIC -DBROKEN_UINTS)

	# TODO: ideally this could be deduplicated
	if (GLSL_VERSION EQUAL 100)
		target_compile_options(Grend PUBLIC -DIMGUI_IMPL_OPENGL_ES2)
		find_library(GLESv2 GLESv2)
		list(APPEND GREND_LINK_LIBS GLESv2)
	elseif(GLSL_VERSION EQUAL 300)
		target_compile_options(Grend PUBLIC -DIMGUI_IMPL_OPENGL_ES3)
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
	set(GREND_PKG) # empty, shouldn't be using pkg-config on android

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
	target_compile_options(Grend PUBLIC -D_WIN32) # mingw doesn't seem to define this?
	                              # might as well explicitly define it
	target_compile_options(Grend -DIMGUI_IMPL_OPENGL_LOADER_GLEW)

	# use in-tree library on windows, bullet packaged with msys2 seems
	# to be a bit borked (function signatures don't match, should submit
	# a bug report/fix...)
	find_library(BulletCollision Bullet3Collision)
	find_library(BulletDynamics  Bullet3Dynamics)
	find_library(BulletSoftBody  BulletSoftBody)
	find_library(LinearMath      LinearMath)
	find_library(opengl32        opengl32)
	set(GREND_PKG "sdl2 bullet glm")

	list(APPEND GREND_LINK_LIBS
		PkgConfig::SDL2
		PkgConfig::Glew
		#PkgConfig::Bullet
		BulletCollision BulletDynamics BulletSoftBody LinearMath
		pthread
		opengl32
	)

elseif (EMSCRIPTEN)
	message(STATUS "Target: Emscripten")
	#add_compile_options(-DGR_PREFIX="${CMAKE_INSTALL_PREFIX}/share/grend/")
	if (NOT PORTABLE_BUILD)
		set(GR_PREFIX "./share/grend/")
	endif()

	find_library(SDL2 SDL2)
	find_library(BulletCollision Bullet3Collision)
	find_library(BulletDynamics  Bullet3Dynamics)
	find_library(BulletSoftBody  BulletSoftBody)
	find_library(LinearMath      LinearMath)
	#find_library(glm             glm)
	set(GREND_PKG "bullet") # only bullet, which should be built as part of grend...

	set(EMSCRIPTEN_USE_STUFF
		-sUSE_SDL=2
		-sUSE_WEBGL2=1
		-sMIN_WEBGL_VERSION=2
		-sMAX_WEBGL_VERSION=2
		-sERROR_ON_UNDEFINED_SYMBOLS=0
		-sTOTAL_MEMORY=1048576000
		-sOFFSCREEN_FRAMEBUFFER=1
		-sFULL_ES3=1
	)

	target_compile_options(Grend PUBLIC ${EMSCRIPTEN_USE_STUFF})
	target_link_options(Grend PUBLIC ${EMSCRIPTEN_USE_STUFF})

	#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_USE_STUFF}")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_USE_STUFF}")
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	# TODO: ideally this could be deduplicated
	if (GLSL_VERSION EQUAL 100)
		target_compile_options(Grend PUBLIC -DIMGUI_IMPL_OPENGL_ES2)
		find_library(GLESv2 GLESv2)
		list(APPEND GREND_LINK_LIBS GLESv2)
	elseif(GLSL_VERSION EQUAL 300)
		target_compile_options(Grend PUBLIC -DIMGUI_IMPL_OPENGL_ES3)
		find_library(GLESv3 GLESv3)
		#list(APPEND GREND_LINK_LIBS GLESv3)
	else()
		message(FATAL_ERROR "Invalid GLSL version for android")
	endif()

	list(APPEND GREND_LINK_LIBS
		 SDL2 BulletCollision BulletDynamics BulletSoftBody LinearMath)

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
	#pkg_check_modules(GLM    REQUIRED IMPORTED_TARGET glm)

	list(APPEND GREND_LINK_LIBS
		PkgConfig::SDL2
		PkgConfig::Bullet
		BulletCollision BulletDynamics BulletSoftBody LinearMath
		pthread
	)
	#set(GREND_PKG "sdl2 bullet glm")
	set(GREND_PKG "sdl2 bullet")

	if (GLSL_VERSION EQUAL 100)
		message(STATUS "GL: Using GLESv2")
		set(GREND_GL_PKG "glesv2")

		target_compile_options(Grend PUBLIC -DIMGUI_IMPL_OPENGL_ES2)
		pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
		list(APPEND GREND_LINK_LIBS GLESv2)

	elseif(GLSL_VERSION EQUAL 300)
		message(STATUS "GL: Using GLESv3")
		set(GREND_GL_PKG "glesv2")

		target_compile_options(Grend PUBLIC -DIMGUI_IMPL_OPENGL_ES3)
		# seems gles3 is part of the gles2 library
		pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
		list(APPEND GREND_LINK_LIBS PkgConfig::GLESv2)

	else()
		message(STATUS "GL: Using glew")
		set(GREND_GL_PKG "glew")

		target_compile_options(Grend PUBLIC -DIMGUI_IMPL_OPENGL_LOADER_GLEW)
		pkg_check_modules(Glew REQUIRED IMPORTED_TARGET glew)
		list(APPEND GREND_LINK_LIBS PkgConfig::Glew)
	endif()
endif()

set(INCLUDES include/grend)
set(LIBRARY_INCLUDES 
	libs
	libs/tinygltf
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

target_include_directories(Grend PUBLIC
	"${PROJECT_BINARY_DIR}"
	${BULLET_PHYSICS_SOURCE_DIR}/src
	include
	${LIBRARY_INCLUDES}
)
target_link_libraries(Grend ${GREND_LINK_LIBS})
#target_compile_options(Grend PUBLIC "-O2")

get_target_property(GREND_COMPILE_OPTIONS_LIST Grend COMPILE_OPTIONS)
set(GREND_COMPILE_OPTIONS)
foreach (item ${GREND_COMPILE_OPTIONS_LIST})
	string(APPEND GREND_COMPILE_OPTIONS "${item} ")
endforeach()

message(STATUS "Compile options: ${GREND_COMPILE_OPTIONS}")
configure_file(grend-config.h.in grend-config.h)
configure_file(grend.pc.in grend.pc @ONLY)

# XXX: added a "shader preprocessor" but that's not quite good enough,
#      lots of glsl implementations don't handle all of the preprocessor macros
#      left in, and I can't justify finishing that preprocessor at the moment,
#      so just bake the base engine shaders for now
file(GLOB SHADER_SRC
     LIST_DIRECTORIES false
	 ${PROJECT_SOURCE_DIR}/shaders/src/*.vert
	 ${PROJECT_SOURCE_DIR}/shaders/src/*.frag)

set(SHADER_OUT)
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/shader_out/")

foreach(_shaderpath ${SHADER_SRC})
	get_filename_component(_shadername ${_shaderpath} NAME)
	set(_shaderout ${PROJECT_BINARY_DIR}/shader_out/${_shadername})
	list(APPEND SHADER_OUT ${_shaderout})

	add_custom_command(OUTPUT ${_shaderout}
		COMMAND cpp
			-DGREND_USE_G_BUFFER=${GREND_USE_G_BUFFER}
			-DGLSL_VERSION=${GLSL_VERSION}
			-I${PROJECT_SOURCE_DIR}/shaders
			-I${PROJECT_BINARY_DIR}
			-P ${_shaderpath} ${_shaderout}
		DEPENDS ${_shaderpath}

		COMMAND sed -i
			-e "s/#pragma.*glsl\\[ \\t\\]*//"
			${_shaderout}
	)
endforeach()

add_custom_target(shaderPreprocessEngine SOURCES ${SHADER_OUT})
add_dependencies(Grend shaderPreprocessEngine)

# TODO: need to have library includes under grend folder
install(TARGETS Grend DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES     ${PROJECT_BINARY_DIR}/grend.pc       DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(FILES     ${PROJECT_BINARY_DIR}/grend-config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${INCLUDES}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${LIBRARY_INCLUDES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/grend)
install(DIRECTORY shaders/lib         DESTINATION ${CMAKE_INSTALL_DATADIR}/grend/shaders)
install(DIRECTORY shaders/lighting    DESTINATION ${CMAKE_INSTALL_DATADIR}/grend/shaders)
install(DIRECTORY shaders/src         DESTINATION ${CMAKE_INSTALL_DATADIR}/grend/shaders)
install(FILES     ${SHADER_OUT}       DESTINATION ${CMAKE_INSTALL_DATADIR}/grend/shaders/baked)
install(DIRECTORY assets              DESTINATION ${CMAKE_INSTALL_DATADIR}/grend)
install(CODE "message(\"Testing this! \${CMAKE_INSTALL_PREFIX}\")")
