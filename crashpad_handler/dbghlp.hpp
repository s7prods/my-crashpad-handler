#pragma once
#include "sdk.h"
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

static BOOL EnableDebugPrivilege() {
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp{};
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return FALSE;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue)) {
		__try { if (hToken) CloseHandle(hToken); }
		__except (EXCEPTION_EXECUTE_HANDLER) {};
		return FALSE;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
		__try { if (hToken) CloseHandle(hToken); }
		__except (EXCEPTION_EXECUTE_HANDLER) {};
		return FALSE;
	}
	__try { if (hToken) CloseHandle(hToken); }
	__except (EXCEPTION_EXECUTE_HANDLER) {};
	return TRUE;
}

struct EnterDebugLoopReturnValue {
	bool processDied;
	DWORD diedValue;
};

// https://learn.microsoft.com/zh-cn/windows/win32/debug/writing-the-debugger-s-main-loop
static EnterDebugLoopReturnValue EnterDebugLoop(const LPDEBUG_EVENT DebugEv, std::function<bool(const LPDEBUG_EVENT, LPDWORD lpdwContinueStatus)> OnException) {
	bool next = true;
	EnterDebugLoopReturnValue ret{};
	while (next)
	{
		DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 

		// Wait for a debugging event to occur. The second parameter indicates
		// that the function does not return until a debugging event occurs. 

		WaitForDebugEvent(DebugEv, INFINITE);

		// Process the debugging event code. 

		switch (DebugEv->dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:
			// Process the exception code. When handling 
			// exceptions, remember to set the continuation 
			// status parameter (dwContinueStatus). This value 
			// is used by the ContinueDebugEvent function. 
			if (OnException(DebugEv, &dwContinueStatus)) {
				next = false;
			}
			break;

		case CREATE_THREAD_DEBUG_EVENT:
			// As needed, examine or change the thread's registers 
			// with the GetThreadContext and SetThreadContext functions; 
			// and suspend and resume thread execution with the 
			// SuspendThread and ResumeThread functions. 

			//dwContinueStatus = OnCreateThreadDebugEvent(DebugEv);
			//if (DebugEv->u.CreateThread.hThread) CloseHandle(DebugEv->u.CreateThread.hThread);
			break;

		case CREATE_PROCESS_DEBUG_EVENT:
			// As needed, examine or change the registers of the
			// process's initial thread with the GetThreadContext and
			// SetThreadContext functions; read from and write to the
			// process's virtual memory with the ReadProcessMemory and
			// WriteProcessMemory functions; and suspend and resume
			// thread execution with the SuspendThread and ResumeThread
			// functions. Be sure to close the handle to the process image
			// file with CloseHandle.

			//dwContinueStatus = OnCreateProcessDebugEvent(DebugEv);
			if (DebugEv->u.CreateProcessInfo.hFile) CloseHandle(DebugEv->u.CreateProcessInfo.hFile);
			//if (DebugEv->u.CreateProcessInfo.hProcess) CloseHandle(DebugEv->u.CreateProcessInfo.hProcess);
			//if (DebugEv->u.CreateProcessInfo.hThread) CloseHandle(DebugEv->u.CreateProcessInfo.hThread);
			break;

		case EXIT_THREAD_DEBUG_EVENT:
			// Display the thread's exit code. 

			//dwContinueStatus = OnExitThreadDebugEvent(DebugEv);
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			// Display the process's exit code. 

			//dwContinueStatus = OnExitProcessDebugEvent(DebugEv);
			next = false;
			ret = EnterDebugLoopReturnValue{
				.processDied = true,
				.diedValue = DebugEv->u.ExitProcess.dwExitCode,
			};
			break;

		case LOAD_DLL_DEBUG_EVENT:
			// Read the debugging information included in the newly 
			// loaded DLL. Be sure to close the handle to the loaded DLL 
			// with CloseHandle.

			//dwContinueStatus = OnLoadDllDebugEvent(DebugEv);
			if (DebugEv->u.LoadDll.hFile) CloseHandle(DebugEv->u.LoadDll.hFile);
			break;

		case UNLOAD_DLL_DEBUG_EVENT:
			// Display a message that the DLL has been unloaded. 

			//dwContinueStatus = OnUnloadDllDebugEvent(DebugEv);
			break;

		case OUTPUT_DEBUG_STRING_EVENT:
			// Display the output debugging string. 

			//dwContinueStatus = OnOutputDebugStringEvent(DebugEv);
			break;

		case RIP_EVENT:
			//dwContinueStatus = OnRipEvent(DebugEv);
			break;
		}

		// Resume executing the thread that reported the debugging event. 

		ContinueDebugEvent(DebugEv->dwProcessId,
			DebugEv->dwThreadId,
			dwContinueStatus);
	}
	return ret;
}

