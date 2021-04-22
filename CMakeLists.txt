cmake_minimum_required(VERSION 3.13)
project(map-viewer-thing VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
	message(WARNING "CMAKE_BUILD_TYPE not set, defaulting to Release")
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS         "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG   "-DGAME_BUILD_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "-DGAME_BUILD_RELEASE=1 -O2")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(PORTABLE_BUILD "Portable build, include all dependancies in the install" OFF)
include(GNUInstallDirs)

if (EXISTS ${PROJECT_SOURCE_DIR}/grend)
	message(STATUS "Found grend subdirectory, using that as library")
	set(GREND_PATH ./grend)
endif()

if (EXISTS ${PROJECT_SOURCE_DIR}/bullet)
	message(STATUS "Found bullet subdirectory, using that as library")
	set(BULLET_PHYSICS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/bullet")
endif()

if (GREND_PATH)
	message(STATUS "GREND_PATH: ${GREND_PATH}")
	add_subdirectory(${GREND_PATH})
endif()

if (BULLET_PHYSICS_SOURCE_DIR)
	message(STATUS "BULLET_PHYSICS_SOURCE_DIR: ${BULLET_PHYSICS_SOURCE_DIR}")
	add_subdirectory(${BULLET_PHYSICS_SOURCE_DIR})
endif()

set (DEMO_LINK_LIBS)
set (DEMO_LINK_OPTIONS)
if (ANDROID OR GREND_PATH)
	# everything needed should be pulled in here
	find_library(Grend Grend)
	list(APPEND DEMO_LINK_LIBS Grend)

elseif(EMSCRIPTEN)
	#link_directories("${CMAKE_INSTALL_PREFIX}/lib")
	#find_library(Grend Grend)
	#list(APPEND DEMO_LINK_LIBS Grend)

	set(CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
	set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_PREFIX_PATH}")
	message("PREFIX PATH: $ENV{PKG_CONFIG_PATH}")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(Grend REQUIRED IMPORTED_TARGET grend)
	list(APPEND DEMO_LINK_LIBS PkgConfig::Grend)

# TODO: assume unixen, presumably there'll be different library lookup
#       stuff for windows/etc
else()
	add_compile_options(-DDEMO_PREFIX="${CMAKE_INSTALL_PREFIX}/share/landscape-demo/")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(Grend REQUIRED IMPORTED_TARGET grend)
	list(APPEND DEMO_LINK_LIBS PkgConfig::Grend)
endif()

set(LANDSCAPE_DEMO_SRC
	src/components/healthbar.cpp
	src/components/area.cpp
	src/components/inputHandler.cpp
	src/components/boxSpawner.cpp
	src/components/team.cpp

	src/entities/enemy.cpp
	src/entities/enemySpawner.cpp
	src/entities/flag.cpp
	src/entities/player.cpp
	src/entities/targetArea.cpp
	src/entities/projectile.cpp
	src/entities/killedParticles.cpp

	src/logic/main.cpp
	src/logic/landscapeGenerator.cpp
	src/logic/UI.cpp
	src/logic/levelController.cpp
)

message(STATUS "CMake host: ${CMAKE_HOST_SYSTEM_NAME}")
if (ANDROID)
	message(STATUS "Setting library for android")
	set(TARGET_NAME main)
	add_library(main SHARED ${LANDSCAPE_DEMO_SRC})
	install(TARGETS main DESTINATION ${CMAKE_INSTALL_LIBDIR})
	list(APPEND DEMO_LINK_LIBS SDL2 SDL2main)

else()
	message(STATUS "Setting standalone executable")
	set(TARGET_NAME landscape-demo)
	add_executable(landscape-demo ${LANDSCAPE_DEMO_SRC})
	install(TARGETS landscape-demo DESTINATION ${CMAKE_INSTALL_BINDIR})

	if(MINGW OR WIN32 OR CYGWIN)
		message(STATUS "Building for/on windows")
		#add_compile_options(-lopengl32)
		# XXX: should be part of grend target compile options...
		add_compile_options(-D_WIN32)
		find_library(opengl32 opengl32)
		list(APPEND DEMO_LINK_LIBS opengl32)

	elseif (EMSCRIPTEN)
		message(STATUS "Building for emscripten")
		set(EMSCRIPTEN_MORE_STUFF
			--preload-file=/tmp/webbuild/share/grend@/
			--preload-file=${PROJECT_SOURCE_DIR}/assets/@/assets
			--shell-file=${PROJECT_SOURCE_DIR}/shell.html
		)

		set(EMSCRIPTEN_USE_STUFF
			-sUSE_SDL=2
			-sMIN_WEBGL_VERSION=2
			-sMAX_WEBGL_VERSION=2
			-sUSE_WEBGL2=1
			-sERROR_ON_UNDEFINED_SYMBOLS=0
			-sTOTAL_MEMORY=1048576000
			-sOFFSCREEN_FRAMEBUFFER=1
			-sFULL_ES3=1
		)

		set(CMAKE_EXECUTABLE_SUFFIX ".html")

		target_link_directories(landscape-demo PUBLIC "${CMAKE_INSTALL_PREFIX}/lib")

		#find_library(Grend Grend)
		#find_library(bullet bullet)
		#list(APPEND DEMO_LINK_LIBS Grend bullet)

		#target_link_libraries(landscape-demo PUBLIC ${DEMO_LINK_LIBS})
		target_compile_options(landscape-demo PUBLIC ${EMSCRIPTEN_USE_STUFF} ${EMSCRIPTEN_MORE_STUFF})
		target_link_options(landscape-demo  PUBLIC ${EMSCRIPTEN_USE_STUFF} ${EMSCRIPTEN_MORE_STUFF})

	else()
		# pthreads on unixen
		message(STATUS "Building for/on unixen, adding -pthread")
		list(APPEND DEMO_LINK_OPTIONS -pthread)
	endif()
endif()

### TODO: configurable data path
install(DIRECTORY assets DESTINATION ${CMAKE_INSTALL_DATADIR}/proj)

target_include_directories(${TARGET_NAME} PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(${TARGET_NAME} PUBLIC src)
target_include_directories(${TARGET_NAME} PUBLIC Grend)
target_link_libraries(${TARGET_NAME} ${DEMO_LINK_LIBS})
target_link_options(${TARGET_NAME} PUBLIC ${DEMO_LINK_OPTIONS})

if (PORTABLE_BUILD)
	# TODO: should also output a launcher script
	install(CODE "set(CMAKE_INSTALL_LIBDIR \"${CMAKE_INSTALL_LIBDIR}\")")
	install(CODE
	[[
		#include(BundleUtilities)
		#	fixup_bundle("landscape-test" "" "asdf")

		file(GET_RUNTIME_DEPENDENCIES
			RESOLVED_DEPENDENCIES_VAR   runtime_libs
			UNRESOLVED_DEPENDENCIES_VAR unresolved_libs
			EXECUTABLES landscape-demo
			# have to bring your own libGL
			POST_EXCLUDE_REGEXES ".*libGL.so" ".*opengl32.dll"
		)

		message("RESOLVED:   ${runtime_libs}")
		message("UNRESOLVED: ${unresolved_libs}")

		foreach(_lib ${runtime_libs})
			file(INSTALL
				DESTINATION "${CMAKE_INSTALL_LIBDIR}"
				TYPE SHARED_LIBRARY
				FOLLOW_SYMLINK_CHAIN
				FILES "${_lib}")
		endforeach()
	]])
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_VERSION_MAJOR "${Grend_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${Grend_VERSION_MINOR}")
include(CPack)
