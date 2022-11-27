cmake_minimum_required (VERSION 3.8)

project(MHS2Loader VERSION "4.0.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(extern)

option(LOADER_NEXUS_CHECK "A check only for the nexus version to future-proof against collection misuse" OFF)

add_library(dinput8 SHARED
	"src/dinput_proxy/dinput8_proxy.cpp"
)

add_library(LoaderCore SHARED
	"src/loader/entry.cpp"
	"src/loader/SigScan.cpp"
	"src/loader/SigScan.hpp"
	"src/loader/Log.hpp"
	"src/loader/Log.cpp"
	"src/loader/PluginManager.cpp"
	"src/loader/PluginManager.hpp"
	"src/loader/Plugin.cpp"
	"src/loader/Plugin.hpp"
	"src/loader/PluginConfig.hpp"
	"src/loader/PluginConfig.cpp"
	#"src/loader/Win32Internals.hpp"
	#"src/loader/Win32Internals.cpp"
)
target_include_directories (LoaderCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories (LoaderCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern)
target_link_libraries(LoaderCore PUBLIC spdlog::spdlog)
target_link_libraries(LoaderCore PUBLIC minhook)

if(LOADER_NEXUS_CHECK)
	message("Nexus check enabled")
	target_compile_definitions(LoaderCore PRIVATE LOADER_NEXUS_CHECK=LOADER_NEXUS_CHECK)
endif()