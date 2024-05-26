cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(NoMercyGameModule CXX)

message(STATUS "CMake version: ${CMAKE_VERSION} target system: ${CMAKE_SYSTEM_NAME}")

if (MSVC)
	# set (CMAKE_GENERATOR "Visual Studio 17 2022" CACHE INTERNAL "" FORCE)

	if (X86_BUILD EQUAL 0)
		return() # FIXME: error MSB4126: The specified solution configuration "Release|Win64" is invalid. Please specify a valid solution configuration using the Configuration and Platform properties
	else()
		set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "CMAKE_GENERATOR_PLATFORM" FORCE)
	endif()
endif()

# Output dir
set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_SYSTEM_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR})

if (MSVC)
	message(STATUS "MSVC: ${MSVC_VERSION}")
else()
	message(STATUS "Compiler: ${CMAKE_SYSTEM_NAME}")
endif()

# C++ options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Static build
set(BUILD_STATIC ON)
# set(BUILD_SHARED ON)
set(BUILD_SHARED_LIBS OFF)

# Custom definitions for external dependencies
add_definitions(-DFMT_HEADER_ONLY) # fmt
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT) # httplib

# Linked libraries
if(MSVC)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") 
		set(BIN_EXTRA_LIBS
			libcryptod libssld # OpenSSL
			zlibd libcurld cprd # cURL
		)
	else()
		set(BIN_EXTRA_LIBS
			libcrypto libssl
			zlib libcurl cpr # cURL
		)
	endif()

	set(BIN_EXTRA_LIBS ${BIN_EXTRA_LIBS} crypt32 Wldap32 ws2_32)
else()
	set(BIN_EXTRA_LIBS -lc -lssl -lcrypto)
endif()
if (CMAKE_SYSTEM_NAME MATCHES ".*Linux")
	set(BIN_EXTRA_LIBS ${BIN_EXTRA_LIBS} -ldl)
endif()

# Platform defination
if (X86_BUILD EQUAL 0)
	add_definitions(-DTARGET_ARCH=64)
	set(TARGET_ARCH_STR "64")
	message(STATUS "Arch: x64")
else() 
	add_definitions(-DTARGET_ARCH=86)
	set(TARGET_ARCH_STR "86")
	message(STATUS "Arch: x86")
endif()

# Compile flags
if (MSVC)
	set(LIB_RT_SUFFIX "mt")
	set(LIB_RT_OPTION "/MT")

	foreach(flag_var  CMAKE_C_FLAGS  CMAKE_CXX_FLAGS)
		foreach(config_name  ""  DEBUG  RELEASE  MINSIZEREL  RELWITHDEBINFO)
			set(var_name "${flag_var}")

			if(NOT "${config_name}" STREQUAL "")
				set(var_name "${var_name}_${config_name}")
			endif()
			
			string(REPLACE "/MD" "${LIB_RT_OPTION}" ${var_name} "${${var_name}}")
			set(${var_name} "${${var_name}}" CACHE STRING "" FORCE)
		endforeach()
	endforeach()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libci.lib")
	
	add_definitions(-DNOMINMAX)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
#	# x86 build for x64 os
	if (X86_BUILD EQUAL 1)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -mtune=i686 -msse2 -mssse3")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -mtune=i686 -msse2 -mssse3")
# 		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
# 		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	endif()
	
	# Debug
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")

	# Pipe
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe")

	# Threading
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -D_THREAD_SAFE")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -D_THREAD_SAFE")

	# Build mode specific flags
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -fexceptions")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -fexceptions")

	# Stack protector
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_NAME MATCHES ".*Linux")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
	endif()

	# if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	# 	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
	# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
	# 	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
	# endif()
endif()

include_directories(${PROJECT_SOURCE_DIR}/extern)
link_directories(${PROJECT_SOURCE_DIR}/extern/x${TARGET_ARCH_STR})
if (NOT MSVC)
	if (CMAKE_SYSTEM_NAME MATCHES ".*Linux")
		if (X86_BUILD EQUAL 1)
			link_directories("/usr/lib/i386-linux-gnu")
			include_directories(/usr/include/i386-linux-gnu)
		endif()

		link_directories(/usr/local/lib)
	else()
		if (X86_BUILD EQUAL 1)
			link_directories(/usr/lib32)
			link_directories(/usr/local/lib32)
			# link_directories(/usr/local/lib/gcc13)
			link_directories(/usr/local/lib32/gcc13)
		endif()
	endif()
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/module)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)
