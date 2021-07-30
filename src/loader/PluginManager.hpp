#pragma once
#include <memory>
#include <vector>
#include <string>

#include "Plugin.hpp"

class PluginManager {
public:
    static PluginManager& Instance();
    void InitPlugins();
    void FireOnPreMainEvent();
    void FireOnMainEvent();
private:
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    std::vector<std::unique_ptr<Plugin>> plugins;
};