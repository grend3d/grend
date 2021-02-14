cmake_minimum_required(VERSION 3.10)
project(map-viewer-thing VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
	message(WARNING "CMAKE_BUILD_TYPE not set, defaulting to Release")
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS         "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG   "-DGAME_BUILD_DEBUG=1 -Og -g")
set(CMAKE_CXX_FLAGS_RELEASE "-DGAME_BUILD_RELEASE=1")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (EXISTS ${PROJECT_SOURCE_DIR}/grend)
	message(STATUS "Found grend subdirectory, using that as library")
	set(GREND_PATH ./grend)
endif()

if (EXISTS ${PROJECT_SOURCE_DIR}/bullet)
	message(STATUS "Found bullet subdirectory, using that as library")
	add_subdirectory(bullet)
	set(BULLET_PHYSICS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/bullet")
endif()

if (GREND_PATH)
	message(STATUS "GREND_PATH: ${GREND_PATH}")
	add_subdirectory(${GREND_PATH})
endif()

set (DEMO_LINK_LIBS)
if (ANDROID OR GREND_PATH)
	# everything needed should be pulled in here
	find_library(Grend Grend)
	list(APPEND DEMO_LINK_LIBS Grend)

# TODO: assume unixen, presumably there'll be different library lookup
#       stuff for windows/etc
else()
	add_compile_options(-DDEMO_PREFIX="${CMAKE_INSTALL_PREFIX}/share/landscape-demo/")
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(Grend REQUIRED IMPORTED_TARGET grend)
	list(APPEND DEMO_LINK_LIBS PkgConfig::Grend)
endif()

set(LANDSCAPE_DEMO_SRC
	src/boxSpawner.cpp
	src/boxSpawner.hpp
	src/enemy.cpp
	src/enemySpawner.cpp
	src/healthbar.cpp
	src/inputHandler.cpp
	src/landscapeGenerator.cpp
	src/main.cpp
	src/player.cpp
	src/projectile.cpp
	src/UI.cpp
	src/flag.cpp
	src/team.cpp
	src/area.cpp
	src/levelController.cpp
	src/killedParticles.cpp
)

if (ANDROID)
	message(STATUS "Setting library for android")
	set(TARGET_NAME main)
	add_library(main SHARED ${LANDSCAPE_DEMO_SRC})
	install(TARGETS main DESTINATION lib)
	list(APPEND DEMO_LINK_LIBS SDL2 SDL2main)

else()
	if(MINGW OR WIN32 OR CYGWIN)
		message(STATUS "Got here")
		#add_compile_options(-lopengl32)
		find_library(opengl32 opengl32)
		list(APPEND DEMO_LINK_LIBS opengl32)
	endif()

	message(STATUS "Setting standalone executable")
	set(TARGET_NAME landscape-demo)
	add_executable(landscape-demo ${LANDSCAPE_DEMO_SRC})
	install(TARGETS landscape-demo DESTINATION bin)
endif()

target_include_directories(${TARGET_NAME} PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(${TARGET_NAME} PUBLIC Grend)
target_link_libraries(${TARGET_NAME} ${DEMO_LINK_LIBS})
