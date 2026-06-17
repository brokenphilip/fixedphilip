cmake_minimum_required (VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(BOT_NAME "fixedphilip")

project(${BOT_NAME})

aux_source_directory("src" core_src)
aux_source_directory("src/commands" core_src_commands)
add_executable(${BOT_NAME} ${core_src} ${core_src_commands})

add_custom_target(BEFORE_BUILD
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMAND git rev-list --count HEAD && ${CMAKE_COMMAND} -E echo "#pragma once" > include/git.h && ${CMAKE_COMMAND} -E echo_append "#define FIXEDPHILIP_GIT_REVISION_COUNT " >> include/git.h && git rev-list --count HEAD >> include/git.h || ${CMAKE_COMMAND} -E true
	COMMAND ${CMAKE_COMMAND} -E touch "src/version.cpp"
	COMMENT "Updating git.h with latest revision count for HEAD (if possible), and touching version.cpp..."
	VERBATIM
)

add_dependencies(${BOT_NAME} BEFORE_BUILD)

string(ASCII 27 Esc)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_target_properties(${BOT_NAME} PROPERTIES
    CXX_STANDARD 20
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
			add_subdirectory(DPP)
			target_link_libraries(${BOT_NAME} dpp)
		endif()
	else()
		message(FATAL_ERROR "Failed to find the git executable. Can't update the D++ submodule.")
	endif()
endif()
