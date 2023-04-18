#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <string>
#include <mutex>
#include <map>

class Memory
{
public:
    Memory(const char* windowName) { connect(windowName); }

    bool connect(const char* windowName) {
        if (CreateHandle(windowName)) {
            if (getExecutableName()) {
                modules.clear();
                addModule(exeName);
                return true;
            }
        } return false;
    }

    const char* getExeName() { return exeName; }
    void addModule(const char* moduleName) { 
        modules[moduleName] = GetModuleBaseAddress(moduleName); 
    }
    bool isValid() { return IsWindow(Window); }


    template <typename... ptr>
    unsigned char* readMemory(uint32_t length, uintptr_t address, bool isPointer, const char* moduleBase, ptr&&... ptrs) {
        static CRITICAL_SECTION cs; static std::once_flag csFlag; // ensures protection over read data
        std::call_once(csFlag, []() { InitializeCriticalSection(&cs); }); // makes function thread safe
        
        address = itrPtrs(address, isPointer, moduleBase, ptrs...); // calculate the final address to read from
        unsigned char* output = new unsigned char[length]; // allocate memory to hold the read data

        if (TryEnterCriticalSection(&cs)) { // try and aquire mutex
            ReadProcessMemory(Handle, (LPVOID)address, output, length, 0) 
                ?void() : (void)(delete[] output, output = nullptr);
            LeaveCriticalSection(&cs);
        }

        return output;
    }

    template <typename... ptr>
    bool writeMemory(const unsigned char* data, uint32_t length, uintptr_t address,  bool isPointer, const char* moduleBase, ptr&&... ptrs) {
        static CRITICAL_SECTION cs; static std::once_flag csFlag; // ensures protection over write data
        std::call_once(csFlag, []() { InitializeCriticalSection(&cs); }); // makes function thread safe
            
        address = itrPtrs(address, isPointer, moduleBase, ptrs...); // calculate the final address to write to

        if (TryEnterCriticalSection(&cs)) { // try and acquire mutex
            bool success = WriteProcessMemory(Handle, (LPVOID)address, data, length, 0) != 0;
            LeaveCriticalSection(&cs);
            return success;
        }

        return false;
    }

    template <typename Type, typename... ptr>
    Type readValue(uintptr_t address, bool isPointer, const char* moduleBase, ptr&&... ptrs) { 
        Type output; address = itrPtrs(address, isPointer, moduleBase, ptrs...);
        ReadProcessMemory(Handle, (LPVOID)address, &output, sizeof(output), 0);
        return output;
    }

    template <typename Type, typename... ptr>
    void writeValue(Type input, uintptr_t address, bool isPointer, const char* moduleBase, ptr&&... ptrs) {
        address = itrPtrs(address, isPointer, moduleBase, ptrs...);
        WriteProcessMemory(Handle, (LPVOID)address, &input, sizeof(input), 0);
    }

private:
    HWND Window;    // Game Window
    DWORD ProcID;   // Game Thread ID
    HANDLE Handle;  // Game Process Handle
    const char* exeName;
    std::map<const char*,uintptr_t> modules; // Module Addresses

    bool CreateHandle(const char* windowName) {
        Window = FindWindowA(NULL, windowName);
        if(Window != NULL) {
            GetWindowThreadProcessId(Window, &ProcID);
            Handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcID);
            if(Handle != NULL) { // Connection Successful
                printf("Handle %p created successfully for process %lu with window '%s'.\n", Handle, ProcID, windowName);
                return true;  
            }
            else {
                printf("Failed to create handle for process %lu with window '%s'. Error code: %lu.\n", ProcID, windowName, GetLastError());
            }
        }
        printf("Failed to find window '%s'. Error code: %lu.\n", windowName, GetLastError());
        return false; // Could not Connect to the Game      
    }

    uintptr_t GetModuleBaseAddress(const char* moduleName) {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ProcID);
        if (hSnap != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W modEntry; modEntry.dwSize = sizeof(modEntry);
            if (Module32FirstW(hSnap, &modEntry)) {
                do { if (!_wcsicmp((const wchar_t*)modEntry.szModule, std::wstring(moduleName, moduleName + strlen(moduleName)).c_str())) {
                    printf("Module found: %s, base address: %p\n", moduleName, modEntry.modBaseAddr);
                    CloseHandle(hSnap); return (uintptr_t)modEntry.modBaseAddr;
                }} while (Module32NextW(hSnap, &modEntry));
            }
            CloseHandle(hSnap);
        }
        printf("Module not found: %s\n", moduleName);
        return 0;
    }

    bool getExecutableName() {
        static char exe[1024]= ".";
        if (GetModuleBaseNameA(Handle,NULL,exe,1024) == 0) {
            printf("Failed to get executable name\n");
            return false;
        }
        exeName = exe; printf("Executable name discovered: %s\n", exeName);
        return true;
    }

    template <typename... ptr>
    uintptr_t itrPtrs(uintptr_t address, bool isPointer, const char* moduleBase, ptr&&... ptrs) {
        address += modules[moduleBase]; // offset by module base address
        if (isPointer) ReadProcessMemory(Handle, (LPVOID)address, &address, sizeof(unsigned int), 0);

        uintptr_t offsets[] = { static_cast<uintptr_t>(ptrs)... };
        for (uintptr_t offset : offsets) {
            if (!isPointer) ReadProcessMemory(Handle, (LPVOID)address, &address, sizeof(unsigned int), 0);
            address += offset; // Add next pointer offset
            isPointer = false;
        }
        return address;
    }
};



    // uintptr_t GetModuleBaseAddress(const char* moduleName) {
    //     HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ProcID);
    //     if (hSnap != INVALID_HANDLE_VALUE) { MODULEENTRY32 modEntry; modEntry.dwSize = sizeof(modEntry);
    //         if (Module32First(hSnap, &modEntry)) {
    //             do { if (!_stricmp((const char*)modEntry.szModule, moduleName)) 
    //                 return (uintptr_t)modEntry.modBaseAddr;
    //             } while (Module32Next(hSnap, &modEntry));
    //     } } CloseHandle(hSnap);
    //     return 0;
    // }

    // template <typename... ptr> // traverse through pointer stack
    // uintptr_t itrPtrs(uintptr_t address, bool isPointer, const char* moduleBase, ptr&&... ptrs) {
    //     address += modules[moduleBase]; // offset by module base address
    //     if(isPointer) ReadProcessMemory(Handle, (LPVOID)address, &address, sizeof(unsigned int), 0);

    //     ([&]{ // Iterates variadic pointer stack
    //         if(!isPointer) ReadProcessMemory(Handle, (LPVOID)address, &address, sizeof(unsigned int), 0);
    //         address += ptrs; // Add next pointer offset
    //         isPointer = false;
    //     }(),...);

    //     return address;
    // }