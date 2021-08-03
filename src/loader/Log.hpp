#pragma once
#include <string>
#include <vector>

namespace Log {
	void OpenConsole();
	int InitializePreLogger();
	int InitializeLogger();


	extern std::vector<std::string> modLoggers;
	int NewPluginLogger(std::string name);
}