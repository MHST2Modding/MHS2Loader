#include <Windows.h>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "MinHook.h"

#include "Log.hpp"
#include "PluginManager.hpp"
#include "SigScan.hpp"

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
	
	Log::OpenConsole();
	Log::InitializeLogger();
	auto logger = spdlog::get("Loader");

	logger->info("Loading plugins from main thread (pre-main)");

	PluginManager::Instance().InitPlugins();


	return fpMainCRTStartup();
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
	spdlog::get("PreLoader")->debug("Image Base: {0:x}", image_base);

	void* mainCrtStartAddr = (void*)SigScan::Scan(image_base, "48 83 EC 28 E8 ?? ?? ?? ?? 48 83 C4 28 E9 ?? ?? ?? ??");
	if (mainCrtStartAddr == nullptr) {
		spdlog::get("PreLoader")->critical("Failed to scan for mainCrtStartAddr");
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

	// TODO(Andoryuuta): Sigscan me!
	/*
	void* sObserverManagerNoteRandAddr = (void*)SigScan::Scan(image_base, "");
	if (sObserverManagerNoteRandAddr == nullptr) {
		spdlog::get("PreLoader")->critical("Failed to scan for sObserverManagerNoteRandAddr");
		return 1;
	}*/
	void* sObserverManagerNoteRandAddr = (void*)0x140995B50; // v1.1.0
	//void* sObserverManagerNoteRandAddr = (void*)0x140995990; // v0

	/*
	void* mainCrtStartAddr = (void*)0x141455174; // 48 83 EC 28 E8 ?? ?? ?? ?? 48 83 C4 28 E9 ?? ?? ?? ??
	void* anticheatDispatchAddr = (void*)0x1408C10F0; // 40 53 48 83 EC 20 4C 8B 05
	void* anticheatGetPtrAddr = (void*)0x1408C1630; // 48 8B 15 ?? ?? ?? ?? 33 C0 4C 8B 05 ?? ?? ?? ?? 48
	void* anticheatCheckFunctionAddr = (void*)0x1408C1670; // 48 89 5C 24 10 56 48 83 EC 20 4C 8B 0D
	void* sObserverManagerNoteRandAddr = (void*)0x140995990;
	*/

	if (MH_Initialize() != MH_OK)
	{
		spdlog::get("PreLoader")->error("Failed to enable core hooks!");
		return 1;
	}

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

	return 0;
}

// This function is called from the loader-locked DllMain,
// does the bare-minimum to get control flow in the main thread
// by hooking the CRT startup and bypassing capcom's anti-tamper code.
void LoaderLockedInitialize() {
	Log::InitializePreLogger();
	if (!EnableCoreHooks()) {
		spdlog::get("PreLoader")->error("Failed to enable core hooks!");
		return;
	}
	spdlog::get("PreLoader")->debug("Enabled core hooks.");
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		LoaderLockedInitialize();
	}

	return TRUE;
}