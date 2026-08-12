#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <algorithm>
#include <cstdint>
#include <cwchar>

#pragma comment(lib, "Psapi.lib")

namespace MEMORY {
    inline DWORD processId = 0;
    inline HANDLE hProcess = nullptr;
    inline uintptr_t baseAddress = 0;

    inline void Detach() {
        if (hProcess) CloseHandle(hProcess);
        hProcess = nullptr;
        processId = 0;
        baseAddress = 0;
    }

    [[nodiscard]] inline DWORD FindProcess(const wchar_t* processName) {
        const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        DWORD result = 0;

        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, processName) == 0) {
                    result = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return result;
    }

    [[nodiscard]] inline uintptr_t FindModule(DWORD pid, const wchar_t* moduleName) {
        const HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                           FALSE, pid);
        if (!process) return 0;

        HMODULE modules[1024]{};
        DWORD bytesNeeded = 0;
        uintptr_t result = 0;

        if (EnumProcessModulesEx(process, modules, sizeof(modules),
                                 &bytesNeeded, LIST_MODULES_ALL)) {
            const DWORD moduleCount = (std::min)(
                bytesNeeded / static_cast<DWORD>(sizeof(HMODULE)),
                static_cast<DWORD>(sizeof(modules) / sizeof(modules[0])));
            for (DWORD i = 0; i < moduleCount; ++i) {
                wchar_t name[MAX_PATH]{};
                if (GetModuleBaseNameW(process, modules[i], name, MAX_PATH) &&
                    _wcsicmp(name, moduleName) == 0) {
                    result = reinterpret_cast<uintptr_t>(modules[i]);
                    break;
                }
            }
        }

        CloseHandle(process);
        return result;
    }

    inline bool Attach(const wchar_t* processName, const wchar_t* moduleName) {
        Detach();
        processId = FindProcess(processName);
        if (!processId) return false;

        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
                               FALSE, processId);
        if (!hProcess) return false;

        baseAddress = FindModule(processId, moduleName);
        if (!baseAddress) {
            Detach();
            return false;
        }
        return true;
    }

    template <typename T>
    [[nodiscard]] inline bool TryRead(uintptr_t address, T& value) {
        value = {};
        SIZE_T bytesRead = 0;
        return hProcess && address &&
               ReadProcessMemory(hProcess, reinterpret_cast<const void*>(address),
                                 &value, sizeof(T), &bytesRead) &&
               bytesRead == sizeof(T);
    }

    template <typename T>
    [[nodiscard]] inline T Read(uintptr_t address) {
        T value{};
        TryRead(address, value);
        return value;
    }
}
