#pragma once
#include <cstdint>
#include <Windows.h>

namespace Win32Internals {
	uint64_t GetThreadStartAddress(uint64_t thread_id);
}