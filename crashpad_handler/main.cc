#include "./sdk.h"
#include <comdef.h>
#include <Wbemidl.h>
#include <vector>
#include "errhlp.hpp"
#pragma comment(lib, "wbemuuid.lib")

HMODULE hModule;
BOOL QueryProcessCmdLineByWMI(DWORD targetPID, std::wstring& outCmdLine);

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


inline uint64_t MakeVersion(uint8_t major, uint8_t minor, uint8_t patch) {
	return major * 1000000ULL + minor * 1000ULL + patch;
}

uint64_t __stdcall Version() {
	return MakeVersion(1, 1, 3);
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
	std::wstring app = wstring(windows) + L"\\..\\System32\\rundll32.exe",
		cmd = L"RunDLL32 \""s + dll_path.get() + L"\",crashpad_handler_RunDLL " +
		to_wstring(dwProcessId) + L" \"" + exe_path.get() + L"\" " + to_wstring((ULONGLONG)hEvent);
	BOOL IsWow64 = FALSE; IsWow64Process(GetCurrentProcess(), &IsWow64);
	if (IsWow64) {
		app = wstring(windows) + L"\\..\\SysWOW64\\rundll32.exe";
	}
	if (!CreateProcessW(app.c_str(), cmd.data(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		return NULL;
	}
	HANDLE objects[] = { hEvent };
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
	if (WAIT_TIMEOUT == WaitForMultipleObjects(1, objects, FALSE, max_t)) {
		return die();
	}
	DWORD code = 1;
	if (!GetExitCodeProcess(pi.hProcess, &code)) {
		return die();
	}
	if (code != STILL_ACTIVE) {
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

	if (argc < 5) ExitProcess(ERROR_INVALID_PARAMETER);

	wstring exe = argv[3], pid_s = argv[2], sig_s = argv[4];
	ULONGLONG pid_n = std::stoull(pid_s), sig_n = std::stoull(sig_s);
	DWORD pid = (DWORD)pid_n;
	if (!pid || !sig_n) ExitProcess(ERROR_INVALID_PARAMETER);

	HANDLE hProcess = OpenProcess(GENERIC_READ | SYNCHRONIZE, FALSE, pid);
	if (!hProcess) ExitProcess(GetLastError());
	HANDLE hEvent = (HANDLE)(PVOID)sig_n;
	DWORD code = -1;

	wstring cmd;
	QueryProcessCmdLineByWMI(pid, cmd);

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





#pragma region 豆包+DeepSeek
BOOL QueryProcessCmdLineByWMI(DWORD targetPID, std::wstring& outCmdLine)
{
	outCmdLine.clear();

	// 1. 初始化 COM（宿主已初始化则复用；失败时不配对 CoUninitialize）
	HRESULT hres = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	const BOOL bUninit = SUCCEEDED(hres);
	if (FAILED(hres) && hres != RPC_E_CHANGED_MODE)
		return FALSE;

	// 2. 设置进程级 COM 安全（每进程只能设一次，重复设置返回 RPC_E_TOO_LATE，可忽略）
	hres = CoInitializeSecurity(
		NULL, -1, NULL, NULL,
		RPC_C_AUTHN_LEVEL_PKT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL, EOAC_NONE, NULL
	);
	if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
		if (bUninit) CoUninitialize();
		return FALSE;
	}

	// 3. 创建 WMI 定位器
	IWbemLocator* pLoc = nullptr;
	hres = CoCreateInstance(
		CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc
	);
	if (FAILED(hres) || !pLoc) {
		if (bUninit) CoUninitialize();
		return FALSE;
	}

	// 4. 连接 root\cimv2
	IWbemServices* pSvc = nullptr;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL, NULL, 0, NULL, 0, 0,
		&pSvc
	);
	pLoc->Release();
	if (FAILED(hres) || !pSvc) {
		if (pSvc) pSvc->Release();
		if (bUninit) CoUninitialize();
		return FALSE;
	}

	// 5. 设置代理安全
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
		RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL, EOAC_NONE
	);
	if (FAILED(hres)) {
		pSvc->Release();
		if (bUninit) CoUninitialize();
		return FALSE;
	}

	// 6. 执行 WQL 查询
	WCHAR wqlQuery[256]{};
	swprintf_s(wqlQuery, L"SELECT CommandLine FROM Win32_Process WHERE ProcessId = %lu", targetPID);

	IEnumWbemClassObject* pEnumerator = nullptr;
	hres = pSvc->ExecQuery(
		_bstr_t(L"WQL"),
		_bstr_t(wqlQuery),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator
	);
	pSvc->Release();
	if (FAILED(hres) || !pEnumerator) {
		if (pEnumerator) pEnumerator->Release();
		if (bUninit) CoUninitialize();
		return FALSE;
	}

	// 7. 取结果（一个 PID 最多一行）
	IWbemClassObject* pclsObj = nullptr;
	ULONG uReturn = 0;
	hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
	pEnumerator->Release();
	if (FAILED(hres) || uReturn == 0 || !pclsObj) {
		if (pclsObj) pclsObj->Release();
		if (bUninit) CoUninitialize();
		return FALSE;
	}

	// 8. 读取 CommandLine
	VARIANT vtProp;
	VariantInit(&vtProp);
	hres = pclsObj->Get(L"CommandLine", 0, &vtProp, NULL, NULL);
	pclsObj->Release();

	BOOL ok = FALSE;
	if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR && vtProp.bstrVal != nullptr) {
		outCmdLine.assign(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
		ok = TRUE;
	}
	VariantClear(&vtProp);

	if (bUninit) CoUninitialize();
	return ok;
}
#pragma endregion
