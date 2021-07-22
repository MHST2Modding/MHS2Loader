#pragma once

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

};