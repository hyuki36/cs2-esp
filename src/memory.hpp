#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <cstdint>

class Memory {
public:
    HANDLE handle = nullptr;
    DWORD pid = 0;
    uintptr_t base = 0;
    std::wstring procName;

    ~Memory() { Detach(); }

    void Detach() {
        if (handle && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            handle = nullptr;
        }
        pid = 0;
        base = 0;
    }

    DWORD FindPid(const std::wstring& name) {
        DWORD found = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                if (name == pe.szExeFile) {
                    found = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return found;
    }

    uintptr_t GetModuleBase(DWORD _pid, const std::wstring& modName) {
        uintptr_t out = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, _pid);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        if (Module32FirstW(snap, &me)) {
            do {
                if (modName == me.szModule) {
                    out = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
        return out;
    }

    // Returns false if Hyperion stripped the handle (ACCESS_DENIED).
    // In that case you need a handle bypass / vulnerable driver to elevate.
    bool Attach(const std::wstring& processName, const std::wstring& moduleName) {
        Detach();
        procName = processName;
        pid = FindPid(processName);
        if (!pid) return false;

        handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!handle || handle == INVALID_HANDLE_VALUE) {
            handle = nullptr;
            return false;
        }

        base = GetModuleBase(pid, moduleName);
        if (!base) return false;
        return true;
    }

    bool IsValid() const { return handle && handle != INVALID_HANDLE_VALUE && pid != 0 && base != 0; }

    template <typename T>
    bool Read(uintptr_t addr, T& out) const {
        if (!addr) return false;
        SIZE_T read = 0;
        return ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(addr), &out, sizeof(T), &read) && read == sizeof(T);
    }

    template <typename T>
    T ReadValue(uintptr_t addr, T def = T{}) const {
        T out = def;
        Read<T>(addr, out);
        return out;
    }

    bool ReadRaw(uintptr_t addr, void* buf, SIZE_T size) const {
        if (!addr || !buf || !size) return false;
        SIZE_T read = 0;
        return ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(addr), buf, size, &read) && read == size;
    }

    std::string ReadString(uintptr_t addr, size_t maxLen = 64) const {
        std::string s;
        s.resize(maxLen);
        SIZE_T read = 0;
        if (!ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(addr), s.data(), maxLen, &read) || read == 0) {
            return {};
        }
        // Roblox names are usually inline SSO or pointer; caller resolves pointer first.
        // Truncate at first null.
        size_t len = 0;
        while (len < read && s[len] != '\0') len++;
        s.resize(len);
        return s;
    }
};
