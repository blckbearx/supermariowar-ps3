#-----------------------------------------------------------------------------
#
# Super Mario War project configuration file
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)

option(PS3_BUILD "Build for PlayStation 3 using SDL2 cross-toolchain" OFF)
set(PS3_SDL2_CONFIG "sdl2-config" CACHE FILEPATH "SDL2 config script for PlayStation 3")
set(PS3_SDL2_EXTRA_LIBS "-lSDL2_image -lSDL2_mixer -lSDL2_ttf" CACHE STRING "Additional PS3 SDL2-related libraries")

if(PS3_BUILD)
	set(CMAKE_SYSTEM_NAME "PS3" CACHE STRING "Target system name for PlayStation 3" FORCE)
	set(CMAKE_SYSTEM_PROCESSOR "powerpc" CACHE STRING "Target processor for PlayStation 3" FORCE)
	set(CMAKE_C_COMPILER "ppu-gcc" CACHE FILEPATH "PS3 C compiler" FORCE)
	set(CMAKE_CXX_COMPILER "ppu-g++" CACHE FILEPATH "PS3 C++ compiler" FORCE)
	set(CMAKE_C_STANDARD 99 CACHE STRING "C standard for PS3 build" FORCE)
	set(CMAKE_C_STANDARD_REQUIRED ON CACHE BOOL "Require C standard for PS3 build" FORCE)
	set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard for PS3 build" FORCE)
	set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard for PS3 build" FORCE)
	set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "Disable compiler-specific C++ extensions for PS3 build" FORCE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
	add_compile_definitions(PS3_BUILD)
endif()

project(smw
	VERSION 2.0.0
	DESCRIPTION "A fan-made multiplayer Super Mario Bros. style deathmatch game"
	HOMEPAGE_URL "https://github.com/mmatyas/supermariowar"
	LANGUAGES CXX
)

#
# Options for customizing builds
#

option(USE_PNG_SAVE "Use PNG for screenshots and thumbnails" ON)
option(NO_NETWORK "Disable all network communication" OFF)
if(PS3_BUILD)
	set(NO_NETWORK ON CACHE BOOL "Disable all network communication" FORCE)
	message(STATUS "PS3 build: disabling all network communication")
endif()
option(SDL2_FORCE_GLES "(SDL2) Use OpenGL ES rendering if possible" OFF)
option(BUILD_TESTS "Build the unit tests" ON)

set(SMW_INSTALL_PORTABLE_DEFAULT OFF)
if(WIN32)
	set(SMW_INSTALL_PORTABLE_DEFAULT ON)
endif()
option(SMW_INSTALL_PORTABLE "Install to a single directory" ${SMW_INSTALL_PORTABLE_DEFAULT})

# Additional CMake search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING
		"Build type (Release/Debug/RelWithDebInfo/MinSizeRel)" FORCE)
endif()

# Set where the binary files will be built.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(SmwGitInfo)

if(PS3_BUILD)
	find_program(PS3_SDL2_CONFIG_EXECUTABLE NAMES ${PS3_SDL2_CONFIG})
	if(NOT PS3_SDL2_CONFIG_EXECUTABLE)
		message(FATAL_ERROR "PS3 build requested but ${PS3_SDL2_CONFIG} was not found in PATH")
	endif()

	execute_process(COMMAND "${PS3_SDL2_CONFIG_EXECUTABLE}" --cflags
		OUTPUT_VARIABLE PS3_SDL2_CFLAGS
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND "${PS3_SDL2_CONFIG_EXECUTABLE}" --libs
		OUTPUT_VARIABLE PS3_SDL2_LDFLAGS
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	string(REPLACE " " ";" PS3_SDL2_CFLAGS_LIST "${PS3_SDL2_CFLAGS}")
	string(REPLACE " " ";" PS3_SDL2_LDFLAGS_LIST "${PS3_SDL2_EXTRA_LIBS} ${PS3_SDL2_LDFLAGS}")

	message(STATUS "PS3 SDL2 cflags: ${PS3_SDL2_CFLAGS_LIST}")
	message(STATUS "PS3 SDL2 ldflags: ${PS3_SDL2_LDFLAGS_LIST}")
endif()

#-----------------------------------------------------------------------------
#
# Platform-specific settings
#
#-----------------------------------------------------------------------------

if (EMSCRIPTEN)
	include(PlatformEmscripten)
endif()

#-----------------------------------------------------------------------------
#
# Dependencies
#
#-----------------------------------------------------------------------------

include(BundledDeps)

#-----------------------------------------------------------------------------
#
# Compiler flags
#
#-----------------------------------------------------------------------------

# Debugging CFLAGS. Turn optimizations off; turn debugging symbols on.
if (CMAKE_BUILD_TYPE STREQUAL Debug)
	add_definitions(-DDEBUG -D_DEBUG -DPNG_DEBUG=1)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g -O0")
else()
	add_definitions(-DNDEBUG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O3")

	# Strip binaries
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
	endif()
endif()

if (EMSCRIPTEN)
	if (CMAKE_BUILD_TYPE STREQUAL Debug)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g4")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g4")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 -g4")
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3")
	endif()
endif()

include(CheckCXXCompilerFlag)
macro(check_and_add_flag var flag)
	CHECK_CXX_COMPILER_FLAG(${flag} FLAG_${var})
	if(FLAG_${var})
		set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
	endif()
endmacro()

# Enabling all warnings in MSVC spams too much
if(NOT MSVC)
	# TODO: would like these but they produce overwhelming amounts of warnings
	check_and_add_flag(ALL -Wall)
	check_and_add_flag(EXTRA -Wextra)
	check_and_add_flag(CXX11_NARROWING_OFF -Wno-c++11-narrowing)
	check_and_add_flag(OVERLOADED_VIRTUAL_OFF -Wno-overloaded-virtual)
	check_and_add_flag(UNUSED_PARAMETER_OFF -Wno-unused-parameter)
	check_and_add_flag(ALWAYS_RETURN -Werror=return-type)

	# gcc uses some optimizations which might break stuff without this flag
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -fexceptions -Wno-shadow")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -fexceptions -Wno-shadow")

endif(NOT MSVC)

if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")
endif()

## MAIN CONFIG

check_and_add_flag(VISIBILITY_INLINES_HIDDEN -fvisibility-inlines-hidden)

if(UNIX AND NOT APPLE)
	check_and_add_flag(VISIBILITY_HIDDEN -fvisibility=hidden)
endif()

#-----------------------------------------------------------------------------
#
# Installation locations
#
#-----------------------------------------------------------------------------

# Common install locations, also passed to the game components

if(WIN32 OR SMW_INSTALL_PORTABLE)
	# CMAKE_INSTALL_PREFIX defaults to `C:/Program Files/smw`
	set(SMW_BINDIR  .    CACHE PATH "Installation location of the game binaries")
	set(SMW_DATADIR data CACHE PATH "Installation location of the game data")
	set(SMW_DOCDIR  .    CACHE PATH "Installation location of the game documentation")
else()
	include(GNUInstallDirs)
	# CMAKE_INSTALL_PREFIX defaults to `/usr/local`
	set(SMW_BINDIR "${CMAKE_INSTALL_PREFIX}/games" CACHE PATH "Installation location of the game binaries")
	set(SMW_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/games/smw" CACHE PATH "Installation location of the game data")
	set(SMW_DOCDIR "${CMAKE_INSTALL_FULL_DOCDIR}" CACHE PATH "Installation location of the game documentation")
	set(SMW_DESKTOPDIR "${CMAKE_INSTALL_FULL_DATADIR}/applications" CACHE PATH "Installation location of the desktop launchers")
	set(SMW_PIXMAPDIR "${CMAKE_INSTALL_FULL_DATADIR}/pixmaps" CACHE PATH "Installation location of the desktop pixmaps")
	set(SMW_ICONDIR "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor" CACHE PATH "Installation location of the desktop icons")
endif()

#
# Data directory install location
#

install(DIRECTORY data/
	DESTINATION "${SMW_DATADIR}"
	PATTERN .git EXCLUDE
	PATTERN ZZleveleditor.map EXCLUDE
)
install(FILES docs/readme-v1.8.html DESTINATION "${SMW_DOCDIR}")

#-----------------------------------------------------------------------------
#
# Ready to build!
#
#-----------------------------------------------------------------------------

# Dump some information
message("    SYSTEM:      ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message("    BUILD:       ${CMAKE_BUILD_TYPE}")
message("    CC:          ${CMAKE_C_COMPILER}")
message("    CXX:         ${CMAKE_CXX_COMPILER}")
message("    C FLAGS:     ${CMAKE_C_FLAGS}")
message("    CXX FLAGS:   ${CMAKE_CXX_FLAGS}")
message("    LD FLAGS:    ${CMAKE_EXE_LINKER_FLAGS}")

get_directory_property(MSG_DEFINITIONS COMPILE_DEFINITIONS)
message("    DEFINITIONS: ${MSG_DEFINITIONS}")

message("  [install]")
message("    Binary dir:  ${SMW_BINDIR}")
message("    Data dir:    ${SMW_DATADIR}")
message("    Docs dir:    ${SMW_DOCDIR}")

#
# Go through every sub-project
#

include_directories(src/common)
include_directories(src/common_netplay)
include_directories(src/smw)

add_subdirectory(src/common)
add_subdirectory(src/common_netplay)
add_subdirectory(src/smw)
add_subdirectory(src/leveleditor)
add_subdirectory(src/worldeditor)

if(NOT NO_NETWORK)
	add_subdirectory(src/server)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTS)
	include(CTest)
	include(dependencies/doctest/doctest.cmake)
	add_subdirectory(dependencies/doctest)
	add_subdirectory(tests)
endif()

if(PS3_BUILD)
	set(PS3_EBOOT_CONTENT_ID "UP0001-SMW00PS33_00-0000000000000000" CACHE STRING "PS3 NPDRM content ID for EBOOT.BIN packaging")
	set(PS3_EBOOT_APPID "SMW00PS33" CACHE STRING "PS3 package application ID")
	set(PS3_EBOOT_TITLE "Super Mario War" CACHE STRING "PS3 package title")
	set(PS3_SFO_XML "${CMAKE_SOURCE_DIR}/pkgfiles/sfo.xml" CACHE FILEPATH "PS3 SFO XML template file")

	find_program(PS3_EBOOT_TOOL NAMES make_self_npdrm
		PATHS /usr/local/ps3dev/bin /usr/local/ps3dev/ppu/bin
		ENV PS3DEV
	)
	set(PS3_EBOOT_OUTPUT "${CMAKE_BINARY_DIR}/EBOOT.BIN")

	if(PS3_EBOOT_TOOL)
		add_custom_command(
			OUTPUT "${PS3_EBOOT_OUTPUT}"
			COMMAND "${PS3_EBOOT_TOOL}" "$<TARGET_FILE:smw>" "${PS3_EBOOT_OUTPUT}" "${PS3_EBOOT_CONTENT_ID}"
			DEPENDS smw
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
			COMMENT "Generating PS3 EBOOT.BIN from smw"
		)
		add_custom_target(ps3_eboot DEPENDS "${PS3_EBOOT_OUTPUT}")
	else()
		add_custom_target(ps3_eboot
			COMMAND "${CMAKE_COMMAND}" -E echo "PS3 EBOOT tool not found. Install make_self_npdrm to build EBOOT.BIN."
			COMMAND "${CMAKE_COMMAND}" -E false
			COMMENT "PS3 EBOOT.BIN target requires make_self_npdrm"
		)
	endif()

	find_program(PS3_PKG_TOOL NAMES pkg.py
		PATHS /usr/local/ps3dev/bin /usr/local/ps3dev/ppu/bin
		ENV PS3DEV
	)
	find_program(PS3_SFO_TOOL NAMES sfo.py
		PATHS /usr/local/ps3dev/bin /usr/local/ps3dev/ppu/bin
		ENV PS3DEV
	)
	find_program(PS3_PACKAGE_FINALIZE NAMES package_finalize
		PATHS /usr/local/ps3dev/bin /usr/local/ps3dev/ppu/bin
		ENV PS3DEV
	)

	if(NOT PS3_PKG_TOOL)
		message(FATAL_ERROR "PS3 package generation requires pkg.py on PATH")
	endif()
	if(NOT PS3_SFO_TOOL)
		message(FATAL_ERROR "PS3 package generation requires sfo.py on PATH")
	endif()

	set(PS3_PKG_STAGING_DIR "${CMAKE_BINARY_DIR}/ps3_pkgfiles")

	if(PS3_PACKAGE_FINALIZE)
		set(PS3_PKG_OUTPUT "${CMAKE_BINARY_DIR}/${PS3_EBOOT_APPID}.gnpdrm.pkg" CACHE FILEPATH "PS3 package output file")
		set(PS3_PKG_RAW_OUTPUT "${CMAKE_BINARY_DIR}/${PS3_EBOOT_APPID}.pkg")
	else()
		set(PS3_PKG_OUTPUT "${CMAKE_BINARY_DIR}/${PS3_EBOOT_APPID}.pkg" CACHE FILEPATH "PS3 package output file")
		set(PS3_PKG_RAW_OUTPUT "${PS3_PKG_OUTPUT}")
	endif()

	add_custom_command(
		OUTPUT "${PS3_PKG_OUTPUT}"
		COMMAND "${CMAKE_COMMAND}" -E remove_directory "${PS3_PKG_STAGING_DIR}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${PS3_PKG_STAGING_DIR}"
		COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/pkgfiles" "${PS3_PKG_STAGING_DIR}"
		COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/data" "${PS3_PKG_STAGING_DIR}/USRDIR/data"
		COMMAND "${CMAKE_COMMAND}" -E copy "${PS3_EBOOT_OUTPUT}" "${PS3_PKG_STAGING_DIR}/USRDIR/EBOOT.BIN"
		COMMAND "${PS3_SFO_TOOL}" --fromxml "${PS3_SFO_XML}" "${PS3_PKG_STAGING_DIR}/PARAM.SFO"
		COMMAND "${PS3_PKG_TOOL}" -c "${PS3_EBOOT_CONTENT_ID}" "${PS3_PKG_STAGING_DIR}/" "${PS3_PKG_RAW_OUTPUT}"
		COMMAND "${CMAKE_COMMAND}" -E copy "${PS3_PKG_RAW_OUTPUT}" "${PS3_PKG_OUTPUT}"
		DEPENDS ps3_eboot "${PS3_SFO_XML}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Building PS3 package ${PS3_PKG_OUTPUT}"
	)

	add_custom_target(ps3_pkg DEPENDS "${PS3_PKG_OUTPUT}")

	if(PS3_PACKAGE_FINALIZE)
		add_custom_command(TARGET ps3_pkg POST_BUILD
			COMMAND "${PS3_PACKAGE_FINALIZE}" "${PS3_PKG_OUTPUT}"
		)
	endif()
endif()

#-----------------------------------------------------------------------------
#
# Packaging
#
#-----------------------------------------------------------------------------

#
# Common settings
#

set(CPACK_PACKAGE_NAME "smw")
set(CPACK_PACKAGE_VENDOR "72dpiarmy")
set(CPACK_PACKAGE_VERSION_MAJOR ${SMW_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${SMW_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${SMW_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_FILE ${PROJECT_SOURCE_DIR}/resources/package_description.txt)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Super Mario War, arcade platform multiplayer game")

set(CPACK_SET_DESTDIR ON)
set(CPACK_SOURCE_IGNORE_FILES  "\\\\.#;/#;.*~;\\\\.swp;/\\\\.git")
list(APPEND CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR}")

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")

#
# Debian
#
set(CPACK_DEBIAN_PACKAGE_SECTION Games)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "nobody")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libsdl2-mixer-2.0-0, libsdl2-image-2.0-0")
if (NOT BUILD_STATIC_LIBS)
	if(NOT NO_NETWORK)
		set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libenet2a")
	endif()
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libyaml-cpp0.5")
endif()

#
# RPM
#
set(CPACK_RPM_PACKAGE_GROUP Amusements/Games)
set(CPACK_RPM_PACKAGE_LICENSE GPL-2.0)
set(CPACK_RPM_PACKAGE_REQUIRES "SDL2, SDL2_image, SDL2_mixer")
if (NOT BUILD_STATIC_LIBS)
	if(NOT NO_NETWORK)
		set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, enet")
	endif()
	set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, yaml-cpp")
endif()

#
# OSX, Windows: TODO
#
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
# TODO: CPACK_RESOURCE_FILE_WELCOME
set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/src/resources/smw.ico)
# TODO: CPACK_NSIS_*

# CPack must be included after the CPACK_* variables are set in order for those
# variables to take effect.
include(CPack)
