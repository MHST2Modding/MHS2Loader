#include "PluginManager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <Windows.h>
#include "spdlog/spdlog.h"
namespace fs = std::filesystem;

PluginManager& PluginManager::Instance() {
    static PluginManager instance;
    return instance;
}

void PluginManager::InitPlugins() {
	
	auto logger = spdlog::get("Loader");

	for (auto& p : fs::directory_iterator("mods"))
	{
		if (p.path().extension() == ".dll") {
			logger->info("Loading {0}", p.path().string());
			auto absDllPath = (fs::current_path() / p.path()).string();

			// Actually load it.
			LoadLibraryA(absDllPath.c_str());
		}
	}
}

void PluginManager::FireOnPreMainEvent() {

}

void PluginManager::FireOnMainEvent() {

}