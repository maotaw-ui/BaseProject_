#pragma once

#include <Windows.h>
#include <cstdint>
#include <string_view>
#include <type_traits>

class Memory final {
public:
    ~Memory();

    Memory() = default;
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    [[nodiscard]] bool Attach(std::wstring_view processName);
    void Detach() noexcept;

    [[nodiscard]] bool IsAttached() const noexcept;
    [[nodiscard]] std::uintptr_t Module(std::wstring_view moduleName) const;

    template <typename T>
    [[nodiscard]] bool Read(std::uintptr_t address, T& value) const {
        static_assert(std::is_trivially_copyable_v<T>);

        value = {};
        SIZE_T bytesRead{};

        return process_ && address &&
               ReadProcessMemory(
                   process_,
                   reinterpret_cast<const void*>(address),
                   &value,
                   sizeof(T),
                   &bytesRead) &&
               bytesRead == sizeof(T);
    }

    template <typename T>
    [[nodiscard]] T Read(std::uintptr_t address) const {
        T value{};
        (void)Read(address, value);
        return value;
    }

private:
    [[nodiscard]] static DWORD FindProcess(std::wstring_view processName);
    [[nodiscard]] static std::uintptr_t FindModule(
        DWORD processId,
        std::wstring_view moduleName);

    HANDLE process_{};
    DWORD processId_{};
};
