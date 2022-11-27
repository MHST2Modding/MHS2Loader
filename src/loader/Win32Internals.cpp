#include "Win32Internals.hpp"

#define ThreadQuerySetWin32StartAddress 9
#define STATUS_SUCCESS    ((NTSTATUS)0x00000000L)

typedef NTSTATUS(WINAPI* pNtQueryInformationThread)(
	HANDLE          ThreadHandle,
	LONG            ThreadInformationClass,
	PVOID           ThreadInformation,
	ULONG           ThreadInformationLength,
	PULONG          ReturnLength
);

namespace Win32Internals {
	uint64_t GetThreadStartAddress(uint64_t thread_id) {
		// Resolve the internal NtQueryInformationThread function (non-public).
		pNtQueryInformationThread NtQueryInformationThread = (pNtQueryInformationThread)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");
		if (NtQueryInformationThread == NULL) {
			return 0;
		}

		// Get a duplicate thread query handle.
		HANDLE hCurrentProcess = GetCurrentProcess();
		HANDLE hThreadQueryHandle = (HANDLE)0;
		if (!DuplicateHandle(hCurrentProcess, (HANDLE)thread_id, hCurrentProcess, &hThreadQueryHandle, THREAD_QUERY_INFORMATION, FALSE, 0)) {
			SetLastError(ERROR_ACCESS_DENIED);
			return 0;
		}
		
		// Query for the thread entry point address (ThreadQuerySetWin32StartAddress).
		DWORD_PTR dwStartAddress;
		NTSTATUS ntStatus = NtQueryInformationThread(hThreadQueryHandle, ThreadQuerySetWin32StartAddress, &dwStartAddress, sizeof(DWORD_PTR), NULL);
		CloseHandle(hThreadQueryHandle);

		if (ntStatus != STATUS_SUCCESS) {
			return 0;
		}

		return (uint64_t)dwStartAddress;
	}
}
