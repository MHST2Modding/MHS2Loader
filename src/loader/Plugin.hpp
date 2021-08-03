#pragma once
#include <string>
#include <memory>
#include <vector>
#include <Windows.h>
#include "PluginConfig.hpp"

class Plugin {
public:
	Plugin(PluginConfig config, std::string path, bool structured);

	std::string path;
	bool isStructuredPlugin;
	PluginConfig config;
	HMODULE moduleHandle;
};