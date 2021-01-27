cmake_minimum_required(VERSION 3.10)
project(landscape-demo VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (GREND_PATH)
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
	src/enemy.cpp
	src/healthbar.cpp
	src/inputHandler.cpp
	src/landscapeGenerator.cpp
	src/main.cpp
	src/player.cpp
	src/projectile.cpp)

if (ANDROID)
	add_library(main SHARED ${LANDSCAPE_DEMO_SRC})

	target_include_directories(main PUBLIC "${PROJECT_BINARY_DIR}")
	target_link_libraries(main ${DEMO_LINK_LIBS})
	install(TARGETS main DESTINATION lib)

else()
	add_executable(landscape-demo ${LANDSCAPE_DEMO_SRC})
	target_include_directories(landscape-demo PUBLIC "${PROJECT_BINARY_DIR}")
	target_link_libraries(landscape-demo ${DEMO_LINK_LIBS})
	install(TARGETS landscape-demo DESTINATION bin)

	#include_directories(demos/map-viewer/)
	#add_executable(map-viewer demos/map-viewer/main.cpp)
	#target_link_libraries(map-viewer Grend)
endif()
