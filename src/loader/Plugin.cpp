#include "Plugin.hpp"



Plugin::Plugin(PluginConfig config, std::string path, bool structured) {
	this->config = config;
	this->path = path;
	this->isStructuredPlugin = structured;
}