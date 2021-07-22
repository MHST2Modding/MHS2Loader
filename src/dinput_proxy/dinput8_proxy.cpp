#include <Windows.h>
typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
DirectInput8Create_t originalDirectInput8Create = nullptr;

void InitializeDinputProxy() {
	// Resolve the original DirectInput8Create function.
	char path[MAX_PATH];
	GetSystemDirectoryA(path, MAX_PATH);
	strcat_s(path, "\\dinput8.dll");
	auto handle = LoadLibraryA(path);
	if (handle != NULL) {
		originalDirectInput8Create = (DirectInput8Create_t)GetProcAddress(handle, "DirectInput8Create");
	}
}

extern "C" {
	__declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
		if (originalDirectInput8Create == nullptr) {
			InitializeDinputProxy();
		}

		return originalDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		LoadLibraryA("LoaderCore.dll");
	}

	return TRUE;
}