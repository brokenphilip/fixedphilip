cmake_minimum_required (VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(BOT_NAME "fixedphilip")

project(${BOT_NAME})
aux_source_directory("src" coresrc)
add_executable(${BOT_NAME} ${coresrc})

string(ASCII 27 Esc)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_target_properties(${BOT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_package(DPP)
if(APPLE)
	if(CMAKE_APPLE_SILICON_PROCESSOR)
		set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl")
	else()
		set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
	endif()
	find_package(OpenSSL REQUIRED)
else()
	find_package(OpenSSL REQUIRED)
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
	set(DL_LIBRARY dl)
endif()

target_include_directories(${BOT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(${BOT_NAME}
    ${DL_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_CRYPTO_LIBRARY} 
    ${OPENSSL_SSL_LIBRARY}
)

if (DPP_FOUND)
    target_link_libraries(${BOT_NAME} ${DPP_LIBRARIES})
    target_include_directories(${BOT_NAME} PUBLIC ${DPP_INCLUDE_DIR})
else()
	find_package(Git)
	message(WARNING "Could not find DPP install. Building from source instead.")
	if (GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
				RESULT_VARIABLE RESULT)
		if (NOT RESULT EQUAL 0)
			message(FATAL_ERROR "Failed to update the D++ submodule. Code: ${RESULT}.")
		else()
			#<slopgpt>

			# ====================================================================
			# FIX FOR DPP AARCH64 CROSS-COMPILATION (Place right BEFORE add_subdirectory(DPP))
			# ====================================================================
			if(NOT TARGET std::filesystem)
				# 1. Force create the target DPP is looking for as a Global Interface
				add_library(std::filesystem INTERFACE IMPORTED GLOBAL)
    
				# 2. Force C++17 properties to bind to this target
				target_compile_features(std::filesystem INTERFACE cxx_std_17)

				# 3. Handle older GCC cross-compilers in WSL that need explicit linkage
				if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
					target_link_libraries(std::filesystem INTERFACE "stdc++fs")
				endif()
			endif()

			# Tell DPP's custom Finder that we already satisfied this dependency
			set(Filesystem_FOUND TRUE CACHE BOOL "" FORCE)
			set(STD_FILESYSTEM_FOUND TRUE CACHE BOOL "" FORCE)

			# NOW call the DPP subdirectory safely
			add_subdirectory(DPP) 

			#</slopgpt>
			#add_subdirectory(DPP)
			target_link_libraries(${BOT_NAME} dpp)
		endif()
	else()
		message(FATAL_ERROR "Failed to find the git executable. Can't update the D++ submodule.")
	endif()
endif()
