#include "Log.hpp"
#include <Windows.h>
#include <cstdio>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"


// Open new console for c(in/out/err)
void Log::OpenConsole() {
	AllocConsole();
	FILE* cinStream;
	FILE* coutStream;
	FILE* cerrStream;
	freopen_s(&cinStream, "CONIN$", "r", stdin);
	freopen_s(&coutStream, "CONOUT$", "w", stdout);
	freopen_s(&cerrStream, "CONOUT$", "w", stderr);

	// From: https://stackoverflow.com/a/45622802 to deal with UTF8 CP:
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IOFBF, 1000);
}

int Log::InitializePreLogger() {
	// Setup async logger.
	spdlog::init_thread_pool(8192, 1);
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	auto rotating_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("PreLoaderLog.txt");
	std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
	auto logger = std::make_shared<spdlog::async_logger>("PreLoader", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	logger->flush_on(spdlog::level::debug);
	spdlog::register_logger(logger);

	return 0;
}

int Log::InitializeLogger() {
	// Setup async logger.
	spdlog::init_thread_pool(8192, 1);
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	auto rotating_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("LoaderLog.txt");
	std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };
	auto logger = std::make_shared<spdlog::async_logger>("Loader", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	logger->flush_on(spdlog::level::debug);
	spdlog::register_logger(logger);

	return 0;
}


extern "C" {
	__declspec(dllexport) void LogDebug(char* s){
		spdlog::get("Loader")->debug(s);
	}
	__declspec(dllexport) void LogWarn(char* s) {
		spdlog::get("Loader")->warn(s);
	}
	__declspec(dllexport) void LogInfo(char* s) {
		spdlog::get("Loader")->info(s);
	}
	__declspec(dllexport) void LogError(char* s) {
		spdlog::get("Loader")->error(s);
	}
	__declspec(dllexport) void LogCritical(char* s) {
		spdlog::get("Loader")->critical(s);
	}
}