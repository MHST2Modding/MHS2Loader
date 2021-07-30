#include "PluginConfig.hpp"


PluginConfig::PluginConfig() {}


PluginConfig::PluginConfig(const PluginConfig& p1) {
	this->name = p1.name;
	this->author = p1.author;
	this->url = p1.url;
	this->version = p1.version;
	this->enabled = p1.enabled;
	this->executable = p1.executable;
}

PluginConfig::PluginConfig(std::string name, std::string author, std::string url, std::string version, std::string executable, bool enabled):
	name(name),
	author(author),
	url(url),
	version(version),
	executable(executable),
	enabled(enabled)
{}

void to_json(json& j, const PluginConfig& p) {
	j = json{
		{"name", p.name},
		{"author", p.author},
		{"url", p.url},
		{"version", p.version},
		{"enabled", p.enabled},
		{"executable", p.executable}
	};
}

void from_json(const json& j, PluginConfig& p) {
	j.at("name").get_to(p.name);
	j.at("author").get_to(p.author);
	j.at("url").get_to(p.url);
	j.at("version").get_to(p.version);
	j.at("enabled").get_to(p.enabled);
	j.at("executable").get_to(p.executable);

}