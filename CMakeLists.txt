cmake_minimum_required(VERSION 3.10)

project(Grend VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "build the shared stuff" ON)
option(PHYSICS_BULLET "Use the bullet physics library" ON)
option(GREND_ERROR_CHECK "Enable graphics library error checks" ON)
option(PORTABLE_BUILD    "Portable build, include all dependancies in the install" OFF)

# GLSL version is also used to determine OpenGL API version
#set(GLSL_VERSION 100) # OpenGL ES 2.0
set(GLSL_VERSION 300) # OpenGL ES 3.0
#set(GLSL_VERSION 330)  # OpenGL core 3.30
#set(GLSL_VERSION 430) # OpenGL core 4.30

add_subdirectory(libs/nanovg)
#find_package(SDL2 REQUIRED COMPONENTS SDL2::SDL2)
find_library(GLESv3 GLESv3)
find_library(SDL2 SDL2)
find_library(BulletCollision Bullet3Collision)
find_library(BulletDynamics  Bullet3Dynamics)
find_library(BulletSoftBody  BulletSoftBody)
find_library(LinearMath      LinearMath)

#find_package(PkgConfig REQUIRED)
#pkg_check_modules(SDL2     REQUIRED IMPORTED_TARGET sdl2)
#pkg_check_modules(SDL2_ttf REQUIRED IMPORTED_TARGET SDL2_ttf)
#pkg_check_modules(Bullet   REQUIRED IMPORTED_TARGET bullet)
#pkg_check_modules(Glew     REQUIRED IMPORTED_TARGET glew)
#pkg_check_modules(GLM      REQUIRED IMPORTED_TARGET glm)

set(INCLUDES include/grend)
set(LIBRARY_INCLUDES 
	libs
	libs/tinygltf
	libs/nanovg/src
	libs/imgui
	libs/json/single_include
	libs/stb)

include_directories(include)
include_directories(${LIBRARY_INCLUDES})
include_directories(PkgConfig::GLM)

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

add_compile_options(
	-DGLSL_VERSION=${GLSL_VERSION}
	-DIMGUI_IMPL_OPENGL_LOADER_GLEW
	-DGREND_ERROR_CHECK=1 # TODO: use cmake debug/release build modes
	#GR_PREFIX="${PROJECT_SOURCE_DIR}/test-install/share/grend/"
	# TODO: this obviously breaks portable installs
	-DGR_PREFIX="${CMAKE_INSTALL_PREFIX}/share/grend/"
)

if (PHYSICS_BULLET)
	#add_compile_definitions(PHYSICS_BULLET)
	add_compile_options(-DPHYSICS_BULLET)
	set(PHYSICS_IMPLEMENTATION src/bulletPhysics.cpp)
else()
	#add_compile_definitions(PHYSICS_IMP)
	add_compile_options(-DPHYSICS_IMP)
	set(PHYSICS_IMPLEMENTATION src/impPhysics.cpp)
endif()

configure_file(config.h.in config.h)

add_library(Grend SHARED
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
	libs/imgui/imgui_widgets.cpp
	libs/imgui/examples/imgui_impl_opengl3.cpp
	libs/imgui/examples/imgui_impl_sdl.cpp

	libs/stb/stbi.cpp
	libs/stb/stb_vorbis.c
)

target_include_directories(Grend PUBLIC
	"${PROJECT_BINARY_DIR}"
	${BULLET_PHYSICS_SOURCE_DIR}/src)
target_link_libraries(Grend
	SDL2
	#PkgConfig::SDL2_ttf
	BulletCollision
	BulletDynamics
	BulletSoftBody
	LinearMath
	nanovg
	#pthread
	glm
	GLESv3
)
target_compile_options(Grend PUBLIC "-O2")

include_directories(demos/landscape-test/)
add_library(main SHARED
	demos/landscape-test/boxSpawner.cpp
	demos/landscape-test/enemy.cpp
	demos/landscape-test/healthbar.cpp
	demos/landscape-test/inputHandler.cpp
	demos/landscape-test/landscapeGenerator.cpp
	demos/landscape-test/main.cpp
	demos/landscape-test/player.cpp
	demos/landscape-test/projectile.cpp
)

target_include_directories(main PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(main Grend SDL2 SDL2main)

#include_directories(demos/map-viewer/)
#add_library(map-viewer demos/map-viewer/main.cpp)
#target_link_libraries(map-viewer Grend)

# TODO: need to have library includes under grend folder
install(TARGETS Grend                 DESTINATION lib)
#install(TARGETS nanovg                DESTINATION lib)
install(TARGETS main                  DESTINATION lib)
#install(TARGETS map-viewer            DESTINATION bin)
install(DIRECTORY ${INCLUDES}         DESTINATION include)
install(DIRECTORY ${LIBRARY_INCLUDES} DESTINATION include/grend)
install(DIRECTORY shaders/lib         DESTINATION share/grend/shaders)
install(DIRECTORY shaders/lighting    DESTINATION share/grend/shaders)
install(DIRECTORY shaders/src         DESTINATION share/grend/shaders)
install(DIRECTORY assets              DESTINATION share/grend)
install(CODE "message(\"Testing this! \${CMAKE_INSTALL_PREFIX}\")")

if (PORTABLE_BUILD)
	# TODO: should also output a launcher script
	install(CODE
	[[
		#include(BundleUtilities)
		#	fixup_bundle("landscape-test" "" "asdf")

		file(GET_RUNTIME_DEPENDENCIES
			RESOLVED_DEPENDENCIES_VAR   runtime_libs
			UNRESOLVED_DEPENDENCIES_VAR unresolved_libs
			EXECUTABLES landscape-test map-viewer
			# have to bring your own libGL
			POST_EXCLUDE_REGEXES ".*libGL.so"
		)

		message("RESOLVED:   ${runtime_libs}")
		message("UNRESOLVED: ${unresolved_libs}")

		foreach(_lib ${runtime_libs})
			file(INSTALL
				DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
				TYPE SHARED_LIBRARY
				#FOLLOW_SYMLINK_CHAIN
				FILES "${_lib}")
		endforeach()
	]])
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_VERSION_MAJOR "${Grend_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${Grend_VERSION_MINOR}")
include(CPack)
