#pragma once
#include "sdk.h"


static DWORD GetParentIdentifier(DWORD Identifier) {
    if (Identifier == 0) {
        return 0;
    }

    DWORD dwParentId = 0;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == Identifier) {
                dwParentId = pe.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return dwParentId;
}

