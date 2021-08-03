#include "PluginManager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <Windows.h>
#include "Log.hpp"
#include "Plugin.hpp"
#include "spdlog/spdlog.h"
namespace fs = std::filesystem;

PluginManager& PluginManager::Instance() {
    static PluginManager instance;
    return instance;
}

void PluginManager::InitPlugins() {
	
	auto logger = spdlog::get("Loader");
	auto& manager = PluginManager::Instance();

	try {
		// Iterate over mods folder and locate
		//     * Structured mods (folders with a plugin.json)
		//     * Unstructured mods (plain .dll files in the root of the mods dir).
		if (fs::exists("mods/")) {
			for (auto& p : fs::directory_iterator("mods")) {


				if (fs::is_directory(p.path()) && fs::exists(p.path() / "plugin.json")) {

					// Structured mod folder
					auto pluginJsonPath = (fs::current_path() / p.path() / "plugin.json").string();

					//logger->info("Got structured mod at {0}", pluginJsonPath);
					//logger->info("Name: {0}", p.path().string());

					try
					{
						// Load config
						std::ifstream ifs(pluginJsonPath);
						json j;
						ifs >> j;
						ifs.close();
						auto config = j.get<PluginConfig>();

						auto mod = std::make_unique<Plugin>(config, (fs::current_path() / p.path()).string(), true);
						this->plugins.push_back(std::move(mod));
					}
					catch (const std::exception& e)
					{
						logger->error("Error loading structure mod config at {0}", (p.path() / "plugin.json").string());
						logger->error("Exception: {0}", e.what());
					}

				}
				else if (p.path().extension() == ".dll") {
					// Unstructured mod
					PluginConfig config;
					config.name = p.path().stem().string();
					config.author = "Unknown";
					config.version = "Unknown";
					config.executable = p.path().filename().string();
					config.enabled = true;

					auto mod = std::make_unique<Plugin>(config, (fs::current_path() / p.path().parent_path()).string(), false);
					this->plugins.push_back(std::move(mod));
				}
			}
		}
		else {
			logger->warn("Directory \"mods\" does not exist. No mods will be loaded.");
		}

	}
	catch (const std::filesystem::filesystem_error& e) {
		logger->error("Got error: {0}", e.what());
	}


	for (auto&& plugin : this->plugins) {
		logger->info("Loading \"{0}\" [Version: {1}, Author: {2}]", plugin->config.name, plugin->config.version, plugin->config.author);
		auto pluginDllPath = (fs::path(plugin->path) / plugin->config.executable).string();
		//logger->info("\t{0}", pluginDllPath);

		plugin->moduleHandle = LoadLibraryA(pluginDllPath.c_str());
	}

	FireOnInitializeEvent();
}


void PluginManager::FireOnInitializeEvent() {
	typedef void (*OnInitializeEvent_t)(int loggerHandle, const char* path);
	for (auto&& plugin : this->plugins) {
		auto loggerHandle = Log::NewPluginLogger(plugin->config.name);
		auto cb = (OnInitializeEvent_t)GetProcAddress(plugin->moduleHandle, "OnInitialize");
		if (cb != nullptr)
			cb(loggerHandle, plugin->path.c_str());
	}

}

void PluginManager::FireOnPreMainEvent() {
	typedef void (*OnPreMainEvent_t)();
	for (auto&& plugin : this->plugins) {
		auto cb = (OnPreMainEvent_t)GetProcAddress(plugin->moduleHandle, "OnPreMain");
		if (cb != nullptr)
			cb();
	}
}

void PluginManager::FireOnMainEvent() {
	typedef void (*OnMainEvent_t)();
	for (auto&& plugin : this->plugins) {
		auto cb = (OnMainEvent_t)GetProcAddress(plugin->moduleHandle, "OnMain");
		if (cb != nullptr)
			cb();
	}
}