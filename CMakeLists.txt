cmake_minimum_required (VERSION 3.8)

project(MHS2Loader VERSION "0.0.1")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(extern)

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
)
target_include_directories (LoaderCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories (LoaderCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern)
target_link_libraries(LoaderCore PUBLIC spdlog::spdlog)
target_link_libraries(LoaderCore PUBLIC minhook)