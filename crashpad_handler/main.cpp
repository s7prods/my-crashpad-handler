#include "./sdk.h"
#include <comdef.h>
#include <Wbemidl.h>
#include <iostream>
#include "errhlp.hpp"
#pragma comment(lib, "wbemuuid.lib")

HMODULE hModule;
BOOL QueryProcessCmdLineByWMI(DWORD targetPID, std::wstring& outCmdLine);

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
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


uint64_t __stdcall Version() {
	return 1000000;
}


static PVOID GetK32(const char* name) {
	HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
	if (!k32) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
	auto GetProcAddress = reinterpret_cast<decltype(&::GetProcAddress)>(::GetProcAddress(k32, "GetProcAddress"));
	if (!GetProcAddress) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
	return GetProcAddress(k32, name);
}


VOID WINAPI crashpad_handler_Builtin(HMODULE hModule, DWORD dwProcessId) {
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
	if (!hEvent) __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE);
	std::wstring app = wstring(windows) + L"\\..\\System32"
#if !defined(_WIN64) && defined(_WIN32)
		L"\\..\\SysWOW64"
#endif
		L"\\rundll32.exe", cmd = L"RunDLL32 \""s + dll_path.get() + L"\",crashpad_handler_RunDLL " +
		to_wstring(dwProcessId) + L" \"" + exe_path.get() + L"\" " + to_wstring((ULONGLONG)hEvent);
	if (!CreateProcessW(app.c_str(), cmd.data(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		EXCEPTION_RECORD er{
			.ExceptionCode = GetLastError(),
			.ExceptionFlags = EXCEPTION_NONCONTINUABLE,
		};
		CONTEXT ctx{}; RtlCaptureContext(&ctx);
		RaiseFailFastException(&er, &ctx, 0);
		__fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE);
	}
	HANDLE objects[] = { hEvent };
	ResumeThread(pi.hThread);
	const DWORD max_t = 30000;
	if (WAIT_TIMEOUT == WaitForMultipleObjects(1, objects, FALSE, max_t)) {
		TerminateProcess(pi.hProcess, GetLastError());
		__fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE);
	}
	DWORD code = 1;
	if (!GetExitCodeProcess(pi.hProcess, &code)) {
		TerminateProcess(pi.hProcess, GetLastError());
		__fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE);
	}
	if (code != STILL_ACTIVE) {
		TerminateProcess(pi.hProcess, GetLastError());
		__fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE);
	}
	CloseHandle(hEvent);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
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





#pragma region 豆包
// 根据PID查询进程命令行
BOOL QueryProcessCmdLineByWMI(DWORD targetPID, std::wstring& outCmdLine)
{
	HRESULT hres;

	// 1. 初始化COM库
	hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (FAILED(hres))
	{
		return FALSE;
	}

	// 2. 设置COM安全级别
	hres = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);

	// 3. 创建WMI定位器
	IWbemLocator* pLoc = nullptr;
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID*)&pLoc
	);
	if (FAILED(hres))
	{
		CoUninitialize();
		return FALSE;
	}

	// 4. 连接到root\cimv2命名空间
	IWbemServices* pSvc = nullptr;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL, NULL,
		0,
		NULL,
		0, 0,
		&pSvc
	);
	pLoc->Release();
	pLoc = nullptr;
	if (FAILED(hres))
	{
		CoUninitialize();
		return FALSE;
	}

	// 5. 设置WMI调用安全
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

	// 6. WQL查询语句：根据PID筛选进程
	WCHAR wqlQuery[256]{};
	swprintf_s(wqlQuery, L"SELECT CommandLine FROM Win32_Process WHERE ProcessId = %u", targetPID);

	IEnumWbemClassObject* pEnumerator = nullptr;
	hres = pSvc->ExecQuery(
		_bstr_t(L"WQL"),
		_bstr_t(wqlQuery),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator
	);

	// 7. 遍历结果，读取CommandLine属性
	IWbemClassObject* pclsObj = nullptr;
	ULONG uReturn = 0;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (uReturn == 0) break;

		VARIANT vtProp;
		// 获取CommandLine字段
		hr = pclsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hr) && vtProp.vt != VT_NULL)
		{
			outCmdLine = vtProp.bstrVal;
		}
		VariantClear(&vtProp);
		pclsObj->Release();
	}

	// 资源释放
	pEnumerator->Release();
	pSvc->Release();
	CoUninitialize();
	return TRUE;
}
#pragma endregion
