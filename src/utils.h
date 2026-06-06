#pragma once
#include <cstdint>
#include <Windows.h>
#include <string>
#include <vector>

uintptr_t PatternScan(uintptr_t base, size_t size, const char* pattern, const char* mask);
uintptr_t GetInterface(uintptr_t moduleBase, const char* interfaceName);
bool IsValidPtr(void* ptr); // implementation in utils.cpp (single definition)