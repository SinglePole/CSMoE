# This module defines
# SDL2_LIBRARY, the name of the library to link against
# SDL2_FOUND, if false, do not try to link to SDL2
# SDL2_INCLUDE_DIR, where to find SDL.h
#
# This module responds to the the flag:
# SDL2_BUILDING_LIBRARY
# If this is defined, then no SDL2main will be linked in because
# only applications need main().
# Otherwise, it is assumed you are building an application and this
# module will attempt to locate and set the the proper link flags
# as part of the returned SDL2_LIBRARY variable.
#
# Don't forget to include SDLmain.h and SDLmain.m your project for the
# OS X framework based version. (Other versions link to -lSDL2main which
# this module will try to find on your behalf.) Also for OS X, this
# module will automatically add the -framework Cocoa on your behalf.
#
#
# Additional Note: If you see an empty SDL2_LIBRARY_TEMP in your configuration
# and no SDL2_LIBRARY, it means CMake did not find your SDL2 library
# (SDL2.dll, libsdl2.so, SDL2.framework, etc).
# Set SDL2_LIBRARY_TEMP to point to your SDL2 library, and configure again.
# Similarly, if you see an empty SDL2MAIN_LIBRARY, you should set this value
# as appropriate. These values are used to generate the final SDL2_LIBRARY
# variable, but when these values are unset, SDL2_LIBRARY does not get created.
#
#
# $SDL2DIR is an environment variable that would
# correspond to the ./configure --prefix=$SDL2DIR
# used in building SDL2.
# l.e.galup  9-20-02
#
# Modified by Eric Wing.
# Added code to assist with automated building by using environmental variables
# and providing a more controlled/consistent search behavior.
# Added new modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).
# Also corrected the header search path to follow "proper" SDL guidelines.
# Added a search for SDL2main which is needed by some platforms.
# Added a search for threads which is needed by some platforms.
# Added needed compile switches for MinGW.
#
# On OSX, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of
# SDL2_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.
#
# Note that the header path has changed from SDL2/SDL.h to just SDL.h
# This needed to change because "proper" SDL convention
# is #include "SDL.h", not <SDL2/SDL.h>. This is done for portability
# reasons because not all systems place things in SDL2/ (see FreeBSD).

#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

message(STATUS "<FindSDL2.cmake> ${SDL2_PATH}")

if(WIN32 AND (NOT SDL2_PATH))
	message(FATAL_ERROR "To find SDL2 correctly, you need to pass SDL2_PATH variable to CMake")
endif()

set(SDL2_SEARCH_PATHS
	${SDL2_PATH}
	${CMAKE_LIBRARY_PATH}

	# OSX paths
	~/Library/Frameworks
	/Library/Frameworks

	# *nix
	/usr/local
	/usr
)

find_path(SDL2_INCLUDE_DIR SDL.h
	PATH_SUFFIXES include/SDL2 include
	PATHS ${SDL2_SEARCH_PATHS}
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
	set(SDL2_LIBRARY_PATH_SUFFIXES
		lib
		lib/x86_64-linux-gnu
		lib/x64
	)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
	set(SDL2_LIBRARY_PATH_SUFFIXES
		lib
		lib/i386-linux-gnu
		lib/x86
		)
endif()

find_library(SDL2_LIBRARY_TEMP
	NAMES SDL2 SDL2.dll
	PATH_SUFFIXES ${SDL2_LIBRARY_PATH_SUFFIXES}
	PATHS ${SDL2_SEARCH_PATHS}
)

# Call sdl2-config if SDL2_LIBRARY still not found
if(NOT SDL2_LIBRARY_TEMP AND NOT WIN32)
	execute_process(COMMAND sdl2-config --libs
		RESULT_VARIABLE SDL2_CONFIG_RETVAL
		OUTPUT_VARIABLE SDL2_LIBRARY_TEMP
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(SDL2_CONFIG_RETVAL)
		message(SEND_ERROR "sdl2-config --libs returned code ${SDL2_CONFIG_RETVAL}. Check that sdl2-config is working.")
	endif()
endif()

if(SDL2_BUILDING_EXECUTABLE)
	if(NOT ${SDL2_INCLUDE_DIR} MATCHES ".framework")
		# Non-OS X framework versions expect you to also dynamically link to
		# SDL2main. This is mainly for Windows and OS X. Other (Unix) platforms
		# seem to provide SDL2main for compatibility even though they don't
		# necessarily need it.
		find_library(SDL2MAIN_LIBRARY
			NAMES SDL2main libSDL2main.a
			PATH_SUFFIXES ${SDL2_LIBRARY_PATH_SUFFIXES}
			PATHS ${SDL2_SEARCH_PATHS}
		)
    endif()
endif()

# SDL2 may require threads on your system.
# The Apple build may not need an explicit flag because one of the
# frameworks may already provide it.
# But for non-OSX systems, I will use the CMake Threads package.
if(NOT APPLE)
	find_package(Threads)
endif()

# MinGW needs an additional library, mwindows
# It's total link flags should look like -lmingw32 -lSDL2main -lSDL2 -lmwindows
# (Actually on second look, I think it only needs one of the m* libraries.)
if(MINGW)
	set(MINGW32_LIBRARY mingw32 CACHE STRING "mwindows for MinGW")
endif()

if(SDL2_LIBRARY_TEMP)
	# For SDL2main
	if(SDL2_BUILDING_EXECUTABLE AND SDL2MAIN_LIBRARY)
		set(SDL2_LIBRARY_TEMP ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY_TEMP})
	endif()

	# For OS X, SDL2 uses Cocoa as a backend so it must link to Cocoa.
	# CMake doesn't display the -framework Cocoa string in the UI even
	# though it actually is there if I modify a pre-used variable.
	# I think it has something to do with the CACHE STRING.
	# So I use a temporary variable until the end so I can set the
	# "real" variable in one-shot.
	if(APPLE)
		set(SDL2_LIBRARY_TEMP ${SDL2_LIBRARY_TEMP} "-framework Cocoa")
	endif()

	# For threads, as mentioned Apple doesn't need this.
	# In fact, there seems to be a problem if I used the Threads package
	# and try using this line, so I'm just skipping it entirely for OS X.
	if(NOT APPLE)
		set(SDL2_LIBRARY_TEMP ${SDL2_LIBRARY_TEMP} ${CMAKE_THREAD_LIBS_INIT})
	endif()

	# For MinGW library
	if(MINGW)
		set(SDL2_LIBRARY_TEMP ${MINGW32_LIBRARY} ${SDL2_LIBRARY_TEMP})
	endif()

	# Set the final string here so the GUI reflects the final state.
	set(SDL2_LIBRARY ${SDL2_LIBRARY_TEMP} CACHE STRING "Where the SDL2 Library can be found")
	# Set the temp variable to INTERNAL so it is not seen in the CMake GUI
	set(SDL2_LIBRARY_TEMP "${SDL2_LIBRARY_TEMP}" CACHE INTERNAL "")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SDL2 REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR)

message(STATUS "</FindSDL2.cmake> ${SDL2_LIBRARY}")
