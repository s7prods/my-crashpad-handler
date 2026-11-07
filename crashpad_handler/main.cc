#include "./sdk.h"
#include <comdef.h>
#include <Wbemidl.h>
#include <vector>
#include "errhlp.hpp"
#pragma comment(lib, "wbemuuid.lib")

HMODULE hModule;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		::hModule = hModule;
		break;
	default:;
	}
	return TRUE;
}

using namespace std;


inline uint64_t MakeVersion(uint16_t major, uint16_t minor, uint16_t patch) {
	return static_cast<uint64_t>(major) * 1000000000000ull +
		static_cast<uint64_t>(minor) * 1000000ull +
		patch;
}

uint64_t __stdcall Version() {
	return MakeVersion(1, 2, 0);
}

constexpr uint64_t OpaqueContextNumber = 0x5a0408a8bfa9414eull;


static PVOID GetK32(const char* name) {
	HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
	if (!k32) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
	auto GetProcAddress = reinterpret_cast<decltype(&::GetProcAddress)>(::GetProcAddress(k32, "GetProcAddress"));
	if (!GetProcAddress) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
	return GetProcAddress(k32, name);
}


#pragma region Context helpers

inline static bool SetContext(LPBYTE MyContext, UINT_PTR nOffset, LPCVOID pData, SIZE_T nData) {
	SIZE_T written{};
	return (WriteProcessMemory(GetCurrentProcess(), MyContext + nOffset, pData, nData, &written)) && (nData == written);
}

template<typename _Ty>
inline bool SetContextValue(LPBYTE MyContext, UINT_PTR nOffset, _Ty Value) {
	return SetContext(MyContext, nOffset, &Value, sizeof(Value));
}

inline static bool GetContext(LPBYTE MyContext, UINT_PTR nOffset, LPVOID pData, SIZE_T nData) {
	SIZE_T read{};
	return (ReadProcessMemory(GetCurrentProcess(), MyContext + nOffset, pData, nData, &read)) && (nData == read);
}

inline static bool GetContext(LPVOID _InternalMyOpaqueContext, UINT_PTR nOffset, LPVOID pData, SIZE_T nData) {
	return GetContext((LPBYTE)_InternalMyOpaqueContext, nOffset, pData, nData);
}

#pragma endregion


LPVOID WINAPI crashpad_handler_Builtin(HMODULE hModule, DWORD dwProcessId) {
	STARTUPINFOW si{}; PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	WCHAR windows[MAX_PATH]{};
	GetSystemDirectoryW(windows, MAX_PATH);
	auto dll_path = make_unique<WCHAR[]>(32768);
	GetModuleFileNameW(::hModule, dll_path.get(), 32768);
	auto exe_path = make_unique<WCHAR[]>(32768);
	GetModuleFileNameW(hModule, exe_path.get(), 32768);
	SECURITY_ATTRIBUTES CanInherit{
		.nLength = sizeof(SECURITY_ATTRIBUTES),
		.bInheritHandle = TRUE,
	};
	HANDLE hEvent = CreateEventW(&CanInherit, TRUE, FALSE, NULL);
	if (!hEvent) return NULL;
	std::wstring app = wstring(windows) + L"\\..\\System32\\rundll32.exe";
	BOOL IsWow64 = FALSE; IsWow64Process(GetCurrentProcess(), &IsWow64);
	if (IsWow64) {
		app = wstring(windows) + L"\\..\\SysWOW64\\rundll32.exe";
	}
	std::wstring AppCmdLine = GetCommandLineW();
	std::wstring cmd = L"RunDLL32 \""s + dll_path.get() + L"\",crashpad_handler_RunDLL " +
		to_wstring(dwProcessId) + L" \"" + exe_path.get() + L"\" " + to_wstring((ULONGLONG)hEvent) + L" " +
		to_wstring((ULONGLONG)(ULONG_PTR)AppCmdLine.data()) + L" " +
		to_wstring(ULONGLONG(AppCmdLine.size()) * sizeof(decltype(AppCmdLine)::value_type)) + L" ";
	if (!CreateProcessW(app.c_str(), cmd.data(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		return NULL;
	}
	HANDLE objects[] = { pi.hProcess, hEvent };
	ResumeThread(pi.hThread);
	const DWORD max_t = 30000;
	auto die = [&hEvent, &pi] () -> LPVOID {
		DWORD err = GetLastError();
		TerminateProcess(pi.hProcess, err);
		CloseHandle(hEvent); hEvent = NULL;
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		SetLastError(err);
		return NULL;
	};
	auto WaitResult = WaitForMultipleObjects(2, objects, FALSE, max_t);
	if (WAIT_TIMEOUT == WaitResult) {
		SetLastError(ERROR_TIMEOUT);
		return die();
	}
	else if (WAIT_OBJECT_0 == WaitResult) {
		DWORD code = 1;
		if (GetExitCodeProcess(pi.hProcess, &code) && code != STILL_ACTIVE) {
			SetLastError(code);
		}
		else SetLastError(ERROR_TIMEOUT);
		return die();
	}
	DWORD code = 1;
	if (!GetExitCodeProcess(pi.hProcess, &code)) {
		return die();
	}
	if (code != STILL_ACTIVE) {
		SetLastError(code);
		return die();
	}
	CloseHandle(hEvent);
	CloseHandle(pi.hThread);

	LPVOID lpOpaqueContext = VirtualAlloc(0, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!lpOpaqueContext) {
		DWORD err = GetLastError();
		TerminateProcess(pi.hProcess, err);
		CloseHandle(pi.hProcess);
		SetLastError(err);
		return NULL;
	}
	LPBYTE MyContext = (LPBYTE)lpOpaqueContext;
	SetContextValue(MyContext, 0, OpaqueContextNumber);
	SetContextValue(MyContext, 8, Version());
	SetContextValue(MyContext, 16, pi.dwProcessId);
	SetContextValue(MyContext, 24, pi.hProcess);

	return lpOpaqueContext;
}


BOOL WINAPI crashpad_handler_Builtin_Free(LPVOID lpContext) {
	if (!lpContext) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}

	uint64_t number = 0, version = 0;
	DWORD dwProcessId = 0;
	HANDLE hProcess = 0;

	if (!GetContext(lpContext, 0, &number, sizeof(number))) return false;
	if (!GetContext(lpContext, 8, &version, sizeof(version))) return false;
	if (!GetContext(lpContext, 16, &dwProcessId, sizeof(dwProcessId))) return false;
	if (!GetContext(lpContext, 24, &hProcess, sizeof(hProcess))) return false;

	if (number != OpaqueContextNumber) {
		SetLastError(ERROR_INVALID_DATA);
		return false;
	}
	if (version != Version()) {
		SetLastError(ERROR_UNSUPPORTED_TYPE);
		return false;
	}
	if (!dwProcessId) {
		SetLastError(ERROR_INVALID_DATA);
		return false;
	}

	// main logic is here
	bool ok = true;
	ok = TerminateProcess(hProcess, 0) && ok;
	ok = CloseHandle(hProcess) && ok;
	ok = VirtualFree(lpContext, 0, MEM_RELEASE) && ok;
	return ok;
}


VOID WINAPI crashpad_handler_RunDLL(HWND, HINSTANCE, PSTR, int) {
	int argc{};
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc < 7) ExitProcess(ERROR_INVALID_PARAMETER);

	wstring exe = argv[3], pid_s = argv[2], sig_s = argv[4], mem_s = argv[5], cnt_s = argv[6];
	ULONGLONG pid_n = std::stoull(pid_s), sig_n = std::stoull(sig_s), mem_n = std::stoull(mem_s), cnt_n = std::stoull(cnt_s);
	DWORD pid = (DWORD)pid_n;
	LPCVOID mem = (LPCVOID)(UINT_PTR)mem_n; SIZE_T cnt = (SIZE_T)cnt_n;
	if (!pid || !sig_n || !mem || !cnt) ExitProcess(ERROR_INVALID_PARAMETER);
	if (cnt > 65536) ExitProcess(ERROR_INVALID_PARAMETER);

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | SYNCHRONIZE |
		PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, pid);
	if (!hProcess) ExitProcess(GetLastError());
	HANDLE hEvent = (HANDLE)(PVOID)sig_n;
	DWORD code = -1;

	wstring cmd;
	{
		const size_t bytes = cnt;
		std::wstring tmp(bytes / sizeof(wstring::value_type) + 2, L'\0');
		SIZE_T read = 0;
		if (!ReadProcessMemory(hProcess, mem, tmp.data(), bytes, &read) || read != bytes) {
			DWORD err = GetLastError();
			CloseHandle(hProcess);
			ExitProcess(err);
		}
		cmd = std::move(tmp);
	}

	SetEvent(hEvent); CloseHandle(hEvent);

	WaitForSingleObject(hProcess, INFINITE);
	GetExitCodeProcess(hProcess, &code);

	if (code != ERROR_SUCCESS && code != 1) {
		// crashed maybe
		wstring text = L"Application has crashed!\nProcess Identifier: " + to_wstring(pid) + L"\nExit code: " + to_wstring(code) +
			L"\n\n" + ErrorChecker(code).raw_message() + L"\n\nDo you want to relaunch the application?";
		int user = MessageBoxW(NULL, text.c_str(), (exe + L" - System Error").c_str(), MB_ICONERROR | MB_YESNO | MB_SERVICE_NOTIFICATION);
		if (user == IDYES) {
			STARTUPINFO si{}; PROCESS_INFORMATION pi{};
			if (!CreateProcessW(exe.c_str(), cmd.data(), 0, 0, 0, 0, 0, 0, &si, &pi)) {
				MessageBoxW(0, ErrorChecker().message().c_str(), NULL, MB_ICONERROR | MB_SERVICE_NOTIFICATION);
				goto clean;
			}
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			ExitProcess(0);
		}
		ExitProcess(code);
	}

clean:
	CloseHandle(hProcess);
	LocalFree(argv);
}




