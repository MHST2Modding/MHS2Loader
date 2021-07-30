#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct PluginConfig {
	PluginConfig();
	PluginConfig(const PluginConfig& p1);
	PluginConfig(std::string name, std::string author, std::string url, std::string version, std::string executable, bool enabled);
public:
	std::string name;
	std::string author;
	std::string url;
	std::string version;
	std::string executable;
	bool enabled;
};

void to_json(json& j, const PluginConfig& p);
void from_json(const json& j, PluginConfig& p);