#pragma once


static DWORD get_process_id(const wchar_t* process_name) {
    if (!process_name || !*process_name)
        return 0;

    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry))
    {
        do {
            if (_wcsicmp(entry.szExeFile, process_name) == 0)
            {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}


static std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_base) {
    if (module_base == nullptr || pid == 0) return 0;

    // Create a snapshot of the modules in the target process (works for 32/64 bit)
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32W me = {};
    me.dwSize = sizeof(me);

    std::uintptr_t result = 0;

    if (Module32FirstW(snap, &me)) {
        do {
            // Compare module name (e.g., "kernel32.dll") case-insensitively
            if (_wcsicmp(me.szModule, module_base) == 0) {
                result = (std::uintptr_t)me.modBaseAddr;
                break;
            }

            // Also allow matching by full path (module_base could be a filename or path tail)
            // Check if module_base is a suffix of me.szExePath
            size_t lenBase = wcslen(module_base);
            size_t lenPath = wcslen(me.szExePath);
            if (lenPath >= lenBase) {
                const wchar_t* tail = me.szExePath + (lenPath - lenBase);
                if (_wcsicmp(tail, module_base) == 0) {
                    result = (std::uintptr_t)me.modBaseAddr;
                    break;
                }
            }
        } while (Module32NextW(snap, &me));
    }

    CloseHandle(snap);
    return result;

}

