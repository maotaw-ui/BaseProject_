#include "Memory.hpp"

#include <TlHelp32.h>
#include <Psapi.h>
#include <algorithm>
#include <string>

#pragma comment(lib, "Psapi.lib")

namespace {
class Snapshot final {
public:
    explicit Snapshot(HANDLE handle) noexcept
        : handle_(handle) {
    }

    ~Snapshot() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
    }

    [[nodiscard]] HANDLE Get() const noexcept {
        return handle_;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return handle_ != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE handle_{};
};
}

Memory::~Memory() {
    Detach();
}

bool Memory::Attach(std::wstring_view processName) {
    Detach();

    processId_ = FindProcess(processName);
    if (!processId_) {
        return false;
    }

    process_ = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        processId_);

    if (!process_) {
        processId_ = 0;
        return false;
    }

    return true;
}

void Memory::Detach() noexcept {
    if (process_) {
        CloseHandle(process_);
    }

    process_ = nullptr;
    processId_ = 0;
}

bool Memory::IsAttached() const noexcept {
    return process_ != nullptr;
}

std::uintptr_t Memory::Module(std::wstring_view moduleName) const {
    return FindModule(processId_, moduleName);
}

DWORD Memory::FindProcess(std::wstring_view processName) {
    const Snapshot snapshot{
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    };

    if (!snapshot) {
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    const std::wstring wanted{ processName };

    if (!Process32FirstW(snapshot.Get(), &entry)) {
        return 0;
    }

    do {
        if (_wcsicmp(entry.szExeFile, wanted.c_str()) == 0) {
            return entry.th32ProcessID;
        }
    } while (Process32NextW(snapshot.Get(), &entry));

    return 0;
}

std::uintptr_t Memory::FindModule(
    DWORD processId,
    std::wstring_view moduleName) {

    const HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        processId);

    if (!process) {
        return 0;
    }

    HMODULE modules[1024]{};
    DWORD bytesNeeded{};
    std::uintptr_t result{};
    const std::wstring wanted{ moduleName };

    if (EnumProcessModulesEx(
            process,
            modules,
            sizeof(modules),
            &bytesNeeded,
            LIST_MODULES_ALL)) {

        const DWORD count = (std::min)(
            bytesNeeded / static_cast<DWORD>(sizeof(HMODULE)),
            static_cast<DWORD>(std::size(modules)));

        for (DWORD index = 0; index < count; ++index) {
            wchar_t name[MAX_PATH]{};

            if (GetModuleBaseNameW(
                    process,
                    modules[index],
                    name,
                    MAX_PATH) &&
                _wcsicmp(name, wanted.c_str()) == 0) {

                result = reinterpret_cast<std::uintptr_t>(modules[index]);
                break;
            }
        }
    }

    CloseHandle(process);
    return result;
}
