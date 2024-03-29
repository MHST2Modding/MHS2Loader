#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <intrin.h>
#include <psapi.h>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <mutex>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "MinHook.h"

#ifdef  LOADER_NEXUS_CHECK
#include "cpp-httplib/httplib.h"
#endif // LOADER_NEXUS_CHECK

#include "Log.hpp"
#include "PluginManager.hpp"
#include "SigScan.hpp"
#include "Win32Internals.hpp"

#pragma intrinsic(_ReturnAddress)

#define VERSION_MESSAGE "MHS2Loader v4.0.0"

std::mutex g_initialized_mutex;
bool g_initialized;

typedef void(*GetSystemTimeAsFileTime_t)(LPFILETIME lpSystemTimeAsFileTime);
GetSystemTimeAsFileTime_t fpGetSystemTimeAsFileTime = NULL;
typedef int(*main_t)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);
main_t fpMain = NULL;
typedef int64_t(*mainCRTStartup_t)();
mainCRTStartup_t fpMainCRTStartup = NULL;
typedef bool(*anticheatDispatch_t)(int idx);
anticheatDispatch_t fpAnticheatDispatch = NULL;
typedef uint64_t(*anticheatGetPtr_t)();
anticheatGetPtr_t fpAnticheatGetPtr = NULL;
typedef bool(*anticheatCheckFunction_t)(uint64_t a1, uint64_t a2);
anticheatCheckFunction_t fpAnticheatCheckFunction = NULL;
typedef __int64(__fastcall* ObserverManager_NoteRand_t)(__int64 a1);
ObserverManager_NoteRand_t fpObserverManager_NoteRand = NULL;

int64_t hookedMainCRTStartup()
{	
	// Initialize the console and logger.
	Log::OpenConsole();
	Log::InitializeLogger();
	auto logger = spdlog::get("Loader");
	logger->info(VERSION_MESSAGE);
	logger->info("Loading mods from main thread (pre-main)");

	// Check if the nexus version has been (soft) disabled.
	// If there is no internet connection, the site no longer exists,
	// or if the file is unparsable, then the modloader will work
	// completely normally.
	// 
	// This check is only intended to future-proof against potential misuse
	// of Nexus's for-profit "Collection" feature, which would keep this mod
	// available to download even if I delete it from the mod listing.
	// 
	// (Nexus's collection feature is HEAVILY targeted towards paid users).
	#ifdef LOADER_NEXUS_CHECK
	try
	{
		httplib::Client cli("http://rawcdn.githack.com");
		cli.set_connection_timeout(0, 300000); // 300 ms
		cli.set_read_timeout(5, 0); // 5 seconds
		cli.set_write_timeout(5, 0); // 5 seconds
		auto res = cli.Get("/Andoryuuta/MHS2Loader/master/nexus_drm.json");
		if (res != nullptr) {
			json j = json::parse(res->body);

			bool disabled = j["nexus_version_disabled"].get<bool>();
			if (disabled) {
				auto message = j["reason"].get<std::string>();
				logger->info("The Nexus-based version of this mod loader has been disabled. Mods will not be loaded.");
				logger->info("Disabled for the following reason: {0}", message);

				// Nexus version has been disabled, start game without loading any mods.
				return fpMainCRTStartup();
			}
		}
	}
	catch (const std::exception& e)
	{
		// Silently ignore any unexpected failure to not hinder user experience.
		//logger->info(e.what());
	}

	#endif // LOADER_NEXUS_CHECK


	// Initialize the plugins and fire the starting events.
	auto&& pm = PluginManager::Instance();
	pm.InitPlugins();

	logger->info("Finished loading mods");

	pm.FireOnPreMainEvent();

	return fpMainCRTStartup();
}

int __stdcall hookedMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	PluginManager::Instance().FireOnMainEvent();
	return fpMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}


bool hookedAnticheatDispatch(int idx)
{
	return 0;
}

uint64_t hookedAnticheatGetPtr()
{
	return 0;
}

bool hookedAnticheatCheckFunction(uint64_t a1, uint64_t a2)
{
	return 0;
}

__int64 __fastcall HookedObserverManager_NoteRand(__int64 a1) {
	return 0;
}

int EnableCoreHooks() {
	auto image_base = (uint64_t)GetModuleHandle(NULL);
	spdlog::get("PreLoader")->info("Image Base: {0:x}", image_base);

	void* mainCrtStartAddr = (void*)SigScan::Scan(image_base, "48 83 EC 28 E8 ?? ?? ?? ?? 48 83 C4 28 E9 ?? ?? ?? ??");
	if (mainCrtStartAddr == nullptr) {
		spdlog::get("PreLoader")->critical("Failed to scan for mainCrtStartAddr");
		return 1;
	}

	void* mainStartAddr = (void*)SigScan::Scan(image_base, "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B E9 41 8B F9 B9 ?? ?? ?? ??");
	if (mainStartAddr == nullptr) {
		spdlog::get("PreLoader")->critical("Failed to scan for mainStartAddr");
		return 1;
	}

	void* anticheatDispatchAddr = (void*)SigScan::Scan(image_base, "40 53 48 83 EC 20 4C 8B 05");
	if (anticheatDispatchAddr == nullptr) {
		spdlog::get("PreLoader")->critical("Failed to scan for anticheatDispatchAddr");
		return 1;
	}

	void* anticheatGetPtrAddr = (void*)SigScan::Scan(image_base, "48 8B 15 ?? ?? ?? ?? 33 C0 4C 8B 05 ?? ?? ?? ?? 48");
	if (anticheatGetPtrAddr == nullptr) {
		spdlog::get("PreLoader")->critical("Failed to scan for anticheatGetPtrAddr");
		return 1;
	}

	void* anticheatCheckFunctionAddr = (void*)SigScan::Scan(image_base, "48 89 5C 24 10 56 48 83 EC 20 4C 8B 0D");
	if (anticheatCheckFunctionAddr == nullptr) {
		spdlog::get("PreLoader")->critical("Failed to scan for anticheatCheckFunctionAddr");
		return 1;
	}

	// AOB (skip first): 40 53 48 83 EC 20 48 8B D9 48 8B 0D ?? ?? ?? ?? 48 85 C9 74 4E 80 79 38 00
	const char* sObserverManagerNoteRandAddrAOB = "40 53 48 83 EC 20 48 8B D9 48 8B 0D ?? ?? ?? ?? 48 85 C9 74 4E 80 79 38 00";
	auto matchsObserverManagerNoteRandAddr = SigScan::ScanAll(image_base, sObserverManagerNoteRandAddrAOB);
	if (matchsObserverManagerNoteRandAddr.size() < 2) {
		spdlog::get("PreLoader")->critical("Failed to scan for sObserverManagerNoteRandAddr");
		return 1;
	}

	void* sObserverManagerNoteRandAddr = (void*)matchsObserverManagerNoteRandAddr[1];
	
	//void* sObserverManagerNoteRandAddr = (void*)0x140994280; // v1.3.0
	//void* sObserverManagerNoteRandAddr = (void*)0x140998100; // v1.2.0
	//void* sObserverManagerNoteRandAddr = (void*)0x140995B50; // v1.1.0
	//void* sObserverManagerNoteRandAddr = (void*)0x140995990; // v0

	/*
	void* mainCrtStartAddr = (void*)0x141455174; // 48 83 EC 28 E8 ?? ?? ?? ?? 48 83 C4 28 E9 ?? ?? ?? ??
	void* anticheatDispatchAddr = (void*)0x1408C10F0; // 40 53 48 83 EC 20 4C 8B 05
	void* anticheatGetPtrAddr = (void*)0x1408C1630; // 48 8B 15 ?? ?? ?? ?? 33 C0 4C 8B 05 ?? ?? ?? ?? 48
	void* anticheatCheckFunctionAddr = (void*)0x1408C1670; // 48 89 5C 24 10 56 48 83 EC 20 4C 8B 0D
	void* sObserverManagerNoteRandAddr = (void*)0x140995990;
	*/

	if (MH_CreateHook(anticheatDispatchAddr, &hookedAnticheatDispatch, reinterpret_cast<LPVOID*>(&fpAnticheatDispatch)) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to create anticheatDispatch hook");
		return 1;
	}
	if (MH_EnableHook(anticheatDispatchAddr) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to enable anticheatDispatch hook");
		return 1;
	}

	if (MH_CreateHook(anticheatGetPtrAddr, &hookedAnticheatGetPtr, reinterpret_cast<LPVOID*>(&fpAnticheatGetPtr)) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to create anticheatGetPtr hook");
		return 1;
	}
	if (MH_EnableHook(anticheatGetPtrAddr) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to enable anticheatGetPtr hook");
		return 1;
	}

	if (MH_CreateHook(anticheatCheckFunctionAddr, &hookedAnticheatCheckFunction, reinterpret_cast<LPVOID*>(&fpAnticheatCheckFunction)) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to create anticheatCheckFunction hook");
		return 1;
	}
	if (MH_EnableHook(anticheatCheckFunctionAddr) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to enable anticheatCheckFunction hook");
		return 1;
	}

	if (MH_CreateHook(sObserverManagerNoteRandAddr, &HookedObserverManager_NoteRand, reinterpret_cast<LPVOID*>(&fpObserverManager_NoteRand)) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to create observerManager_NoteRand hook");
		return 1;
	}
	if (MH_EnableHook(sObserverManagerNoteRandAddr) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to enable observerManager_NoteRand hook");
		return 1;
	}

	if (MH_CreateHook(mainCrtStartAddr, &hookedMainCRTStartup, reinterpret_cast<LPVOID*>(&fpMainCRTStartup)) != MH_OK)
	{
		spdlog::get("PreLoader")->error("Failed to create mainCRTStartup hook");
		return 1;
	}
	if (MH_EnableHook(mainCrtStartAddr) != MH_OK)
	{
		spdlog::get("PreLoader")->error("Failed to enable mainCRTStartup hook");
		return 1;
	}

	if (MH_CreateHook(mainStartAddr, &hookedMain, reinterpret_cast<LPVOID*>(&fpMain)) != MH_OK)
	{
		spdlog::get("PreLoader")->error("Failed to create main hook");
		return 1;
	}

	if (MH_EnableHook(mainStartAddr) != MH_OK)
	{
		spdlog::get("PreLoader")->error("Failed to enable main hook");
		return 1;
	}

	return 0;
}

// GetSystemTimeAsFileTime gets called pre-WinMain in the MSVCRT setup.
void hookedGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime) {
	// Get return address of this hooked call.
	uint64_t retAddress = (uint64_t)_ReturnAddress();

	// Get the start and end of the base process module (the main .exe).
	MODULEINFO moduleInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &moduleInfo, sizeof(MODULEINFO));
	uint64_t baseModStart = (uint64_t)moduleInfo.lpBaseOfDll;
	uint64_t baseModEnd = (uint64_t)moduleInfo.lpBaseOfDll + (uint64_t)moduleInfo.lpBaseOfDll;
	
	// If we are being called from within the module memory (e.g. not a random library),
	// then we can assume this is likely the real code post-unpack.
	if (retAddress >= baseModStart && retAddress <= baseModEnd) {
		spdlog::get("PreLoader")->info("Got GetSystemTimeAsFileTime call with valid return address. Base module start: {0:x}, Base module end: {1:x}, GetSystemTimeAsFileTime Return address: {2:x}",
			baseModStart,
			baseModEnd,
			retAddress);

		// Enable our hooks and, if successful, disable the GetSystemTimeAsFileTime as we will not need it anymore.
		if (EnableCoreHooks()) {
			spdlog::get("PreLoader")->error("Failed to enable core hooks!");
			return;
		}

		else {
			spdlog::get("PreLoader")->info("Enabled core hooks");

			if (MH_DisableHook(GetSystemTimeAsFileTime) != MH_OK) {
				spdlog::get("PreLoader")->error("Failed to disable GetSystemTimeAsFileTime hook");
				return;
			}
		}
	}

	fpGetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}

// This function is called from the loader-locked DllMain,
// does the bare-minimum to get control flow in the main thread
// by hooking a function called in the CRT startup (GetSystemTimeAsFileTime) to
// bypass capcom's anti-tamper code early before any C++ static initalizers are called.
int LoaderLockedInitialize() {
	Log::InitializePreLogger();

	spdlog::get("PreLoader")->info("Begin enabling post-unpack hooks on thread: {0}", GetCurrentThreadId());

	if (MH_Initialize() != MH_OK)
	{
		spdlog::get("PreLoader")->error("Failed to initialize minhook!");
		return 1;
	}

	// Hook GetSystemTimeAsFileTime in order to get control of execution after the .exe is unpacked in memory.
	if (MH_CreateHook(GetSystemTimeAsFileTime, &hookedGetSystemTimeAsFileTime, reinterpret_cast<LPVOID*>(&fpGetSystemTimeAsFileTime)) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to create GetSystemTimeAsFileTime hook");
		return 1;
	}
	if (MH_EnableHook(GetSystemTimeAsFileTime) != MH_OK) {
		spdlog::get("PreLoader")->error("Failed to enable GetSystemTimeAsFileTime hook");
		return 1;
	}

	return 0;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_THREAD_ATTACH || fdwReason == DLL_PROCESS_ATTACH) {
		std::lock_guard<std::mutex> guard(g_initialized_mutex);
		if (g_initialized == false) {
			if (!LoaderLockedInitialize()) {
				g_initialized = true;
			}
		}
	}

	return TRUE;
}