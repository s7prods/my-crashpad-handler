#pragma once
#include "sdk.h"
#include <DbgHelp.h>


#if 0
//incompatible with cfg, trashed
static PVOID GetK32(const char* name) {
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    if (!k32) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
    auto GetProcAddress = reinterpret_cast<decltype(&::GetProcAddress)>(::GetProcAddress(k32, "GetProcAddress"));
    if (!GetProcAddress) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
    return GetProcAddress(k32, name);
}
#endif


BOOL
WINAPI
HackedMiniDumpWriteDump(
    _In_ HANDLE hProcess,
    _In_ DWORD ProcessId,
    _In_ HANDLE hFile,
    _In_ MINIDUMP_TYPE DumpType,
    _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
) {
    HMODULE hDbgHelp = LoadLibraryExW(L"dbghelp.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hDbgHelp) return FALSE;
    auto mm = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (!mm) {
        DWORD err = GetLastError();
        FreeLibrary(hDbgHelp);
        SetLastError(err);
        return FALSE;
    }

    auto ret = mm(hProcess, ProcessId, hFile, DumpType, ExceptionParam, UserStreamParam, CallbackParam);

    DWORD err = GetLastError();
    FreeLibrary(hDbgHelp);
    SetLastError(err);
    return ret;
}

