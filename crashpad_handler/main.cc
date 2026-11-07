#include "./sdk.h"
#include <filesystem>
#include "errhlp.hpp"
#include "dbghlp.hpp"
#include "../user/crashpad_handler.h"

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
	return MakeVersion(1, 3, 0);
}

constexpr uint64_t OpaqueContextNumber = 0x5a0408a8bfa9414eull;


static PVOID GetK32(const char* name) {
	HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
	if (!k32) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
	auto GetProcAddress = reinterpret_cast<decltype(&::GetProcAddress)>(::GetProcAddress(k32, "GetProcAddress"));
	if (!GetProcAddress) { __fastfail(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE); }
	return GetProcAddress(k32, name);
}

static wstring& str_replace(std::wstring& strBase, const std::wstring strSrc, const std::wstring strDes) {
	wstring::size_type pos = 0;
	wstring::size_type srcLen = strSrc.size();
	wstring::size_type desLen = strDes.size();
	pos = strBase.find(strSrc, pos);
	while ((pos != wstring::npos))
	{
		strBase.replace(pos, srcLen, strDes);
		pos = strBase.find(strSrc, (pos + desLen));
	}
	return strBase;
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


LPVOID WINAPI crashpad_handler_Builtin(_In_opt_ HMODULE hModule, _In_ DWORD dwProcessId, _In_opt_ const InitializeCrashpadHandlerOptions* lpOptions) {
	STARTUPINFOW si{}; PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	auto lpOptions2 = make_unique<InitializeCrashpadHandlerOptions>();
	if (lpOptions) {
		memcpy(lpOptions2.get(), lpOptions, sizeof(InitializeCrashpadHandlerOptions));
	}
	else {
		memset(lpOptions2.get(), 0, sizeof(InitializeCrashpadHandlerOptions));
	}
	if (lpOptions2->cb && lpOptions2->cb != sizeof(InitializeCrashpadHandlerOptions)) {
		SetLastError(ERROR_FILE_NOT_SUPPORTED);
		return NULL;
	}
	auto& options = *lpOptions2;
	LPVOID lpOpaqueContext = VirtualAlloc(0, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!lpOpaqueContext) return NULL;
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
	HANDLE hEventShouldStop = CreateEventW(&CanInherit, TRUE, FALSE, NULL);
	HANDLE hEventShouldStopResponse = CreateEventW(&CanInherit, TRUE, FALSE, NULL);
	if (!hEvent || !hEventShouldStop || !hEventShouldStopResponse) {
		if (hEvent) CloseHandle(hEvent);
		if (hEventShouldStop) CloseHandle(hEventShouldStop);
		if (hEventShouldStopResponse) CloseHandle(hEventShouldStopResponse);
		return NULL;
	}
	std::wstring app = wstring(windows) + L"\\..\\System32\\rundll32.exe";
	BOOL IsWow64 = FALSE; IsWow64Process(GetCurrentProcess(), &IsWow64);
	if (IsWow64) {
		app = wstring(windows) + L"\\..\\SysWOW64\\rundll32.exe";
	}
	std::wstring AppCmdLine = GetCommandLineW();
	std::wstring cmd = L"RunDLL32 \""s + dll_path.get() + L"\",crashpad_handler_RunDLL " +
		to_wstring(dwProcessId) + L" \"" + exe_path.get() + L"\" " + to_wstring((ULONGLONG)hEvent) + L" " +
		to_wstring((ULONGLONG)(ULONG_PTR)AppCmdLine.data()) + L" " +
		to_wstring(ULONGLONG(AppCmdLine.size()) * sizeof(decltype(AppCmdLine)::value_type)) + L" " +
		to_wstring((ULONGLONG)(ULONG_PTR)lpOptions2.get()) + L" " +
		to_wstring((ULONGLONG)sizeof(InitializeCrashpadHandlerOptions)) + L" " +
		to_wstring((ULONGLONG)hEventShouldStop) + L" " +
		to_wstring((ULONGLONG)hEventShouldStopResponse) + L" " ;
	if (!CreateProcessW(app.c_str(), cmd.data(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		return NULL;
	}
	HANDLE objects[] = { pi.hProcess, hEvent };
	ResumeThread(pi.hThread);
	const DWORD max_t = 30000;
	auto die = [&lpOpaqueContext, &hEvent, &hEventShouldStop, &pi] () -> LPVOID {
		DWORD err = GetLastError();
		TerminateProcess(pi.hProcess, err);
		CloseHandle(hEvent); hEvent = NULL;
		CloseHandle(hEventShouldStop); hEventShouldStop = NULL;
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		VirtualFree(lpOpaqueContext, 0, MEM_RELEASE);
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
	// NO close hEventStop and response
	// NO close pi.hProcess
	// NO close lpOpaqueContext

	LPBYTE MyContext = (LPBYTE)lpOpaqueContext;
	SetContextValue(MyContext, 0, OpaqueContextNumber);
	SetContextValue(MyContext, 8, Version());
	SetContextValue(MyContext, 16, hEventShouldStop);
	SetContextValue(MyContext, 24, hEventShouldStopResponse);
	SetContextValue(MyContext, 32, pi.dwProcessId);
	SetContextValue(MyContext, 40, pi.hProcess);

	return lpOpaqueContext;
}


BOOL WINAPI crashpad_handler_Builtin_Free(LPVOID lpContext) {
	if (!lpContext) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}

	uint64_t number = 0, version = 0;
	HANDLE hEvent = 0, hEventResp = 0;
	DWORD dwProcessId = 0;
	HANDLE hProcess = 0;

	if (!GetContext(lpContext, 0, &number, sizeof(number))) return false;
	if (!GetContext(lpContext, 8, &version, sizeof(version))) return false;
	if (!GetContext(lpContext, 16, &hEvent, sizeof(hEvent))) return false;
	if (!GetContext(lpContext, 24, &hEventResp, sizeof(hEventResp))) return false;
	if (!GetContext(lpContext, 32, &dwProcessId, sizeof(dwProcessId))) return false;
	if (!GetContext(lpContext, 40, &hProcess, sizeof(hProcess))) return false;

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
	ok = SetEvent(hEvent) && ok; // NO directly kill process otherwise main app will also be killed
	ok = CloseHandle(hEvent) && ok;
	ok = (WaitForSingleObject(hEventResp, 10000) == WAIT_OBJECT_0) && ok;
	ok = CloseHandle(hEventResp) && ok;
	if (IsDebuggerPresent()) DebugBreak();
	//ok = (WaitForSingleObject(hProcess, 10000) == WAIT_OBJECT_0) && ok; // change: let the child itself go, no wait anymore
	ok = CloseHandle(hProcess) && ok;
	ok = VirtualFree(lpContext, 0, MEM_RELEASE) && ok;
	return ok;
}


VOID WINAPI crashpad_handler_RunDLL(HWND, HINSTANCE, PSTR, int) {
	int argc{};
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc < 11) {
		LocalFree(argv);
		ExitProcess(ERROR_INVALID_PARAMETER);
	}

	wstring exe = argv[3], pid_s = argv[2], sig_s = argv[4], mem_s = argv[5], cnt_s = argv[6], opt_s = argv[7], oct_s = argv[8], ext_s = argv[9], per_s = argv[10];
	ULONGLONG pid_n = stoull(pid_s), sig_n = stoull(sig_s), mem_n = stoull(mem_s), cnt_n = stoull(cnt_s), opt_n = stoull(opt_s), oct_n = stoull(oct_s), ext_n = stoull(ext_s), per_n = stoull(per_s);
	DWORD pid = (DWORD)pid_n;
	LPCVOID mem = (LPCVOID)(UINT_PTR)mem_n; SIZE_T cnt = (SIZE_T)cnt_n;
	LPCVOID opt = (LPCVOID)(UINT_PTR)opt_n; SIZE_T oct = (SIZE_T)oct_n;
	if (!pid || !sig_n || !mem || !cnt || !opt || !oct || !ext_n || !per_n) {
		LocalFree(argv);
		ExitProcess(ERROR_INVALID_PARAMETER);
	}
	if (cnt > 65536) {
		LocalFree(argv);
		ExitProcess(ERROR_INVALID_PARAMETER);
	}
	if (oct != sizeof(InitializeCrashpadHandlerOptions)) {
		LocalFree(argv);
		ExitProcess(ERROR_INVALID_DATA);
	}

	wstring exe_path = filesystem::path(exe).parent_path().wstring();
	wstring exe_pure = filesystem::path(exe).filename().wstring();
	wstring exe_stem = filesystem::path(exe).stem().wstring();

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | SYNCHRONIZE |
		PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE | STANDARD_RIGHTS_REQUIRED, FALSE, pid);
	if (!hProcess) {
		DWORD err = GetLastError();
		LocalFree(argv);
		ExitProcess(err);
	}
	HANDLE hEvent = (HANDLE)(PVOID)sig_n;
	HANDLE hExitEvent = (HANDLE)(PVOID)ext_n;
	HANDLE hExitEventPeer = (HANDLE)(PVOID)per_n;
	DWORD code = -1;

	wstring cmd;
	{
		const size_t bytes = cnt;
		std::wstring tmp(bytes / sizeof(wstring::value_type) + 2, L'\0');
		SIZE_T read = 0;
		if (!ReadProcessMemory(hProcess, mem, tmp.data(), bytes, &read) || read != bytes) {
			DWORD err = GetLastError();
			CloseHandle(hProcess);
			LocalFree(argv);
			ExitProcess(err);
		}
		cmd = wstring(tmp.c_str());
	}

	auto lpOptions2 = make_unique<InitializeCrashpadHandlerOptions>();
	{
		SIZE_T read = 0;
		if (!ReadProcessMemory(hProcess, opt, lpOptions2.get(), oct, &read) || read != oct) {
			DWORD err = GetLastError();
			CloseHandle(hProcess);
			LocalFree(argv);
			ExitProcess(err);
		}
		if (lpOptions2->cb && lpOptions2->cb != sizeof(InitializeCrashpadHandlerOptions)) {
			CloseHandle(hProcess);
			LocalFree(argv);
			ExitProcess(ERROR_INVALID_DATA);
		}
	}
	auto& options = *lpOptions2;
#define SafeStr(Path) Path [std::size( Path ) - 1] = 0;
	SafeStr(options.DumpOptions.DumpDirectory);
	SafeStr(options.DumpOptions.CustomizedDumpNameFormat);
	SafeStr(options.DumpOptions.CustomizedContextNameFormat);
	SafeStr(options.RestartOptions.RestartCommandLine);
	SafeStr(options.RestartOptions.AddRecoveryInformationOptions.AddCustomString);
	SafeStr(options.RestartOptions.AddRecoveryInformationOptions.AddRestartCountPrefix);
	SafeStr(options.RestartOptions.AddRecoveryInformationOptions.AddRestartCountSuffix);
#undef SafeStr

	if (options.RestartOptions.RestartCommandLine[0]) cmd = options.RestartOptions.RestartCommandLine;

	SetEvent(hEvent); CloseHandle(hEvent);

	EnableDebugPrivilege();
	DebugSetProcessKillOnExit(FALSE);
	if (!DebugActiveProcess(pid)) {
		DWORD err = GetLastError();
		CloseHandle(hProcess);
		LocalFree(argv);
		ExitProcess(err);
	}
	volatile bool shouldStop = false;
	std::thread waiter([hProcess, pid, hExitEvent, &hExitEventPeer, &shouldStop] {
		HANDLE handles[]{ hExitEvent, hProcess };
		auto res = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (res == WAIT_OBJECT_0) {
			shouldStop = true;
			MemoryBarrier();
			SleepEx(5, TRUE);
			SetEvent(hExitEventPeer);
			CloseHandle(hExitEventPeer);
			hExitEventPeer = NULL;
			return;
		}
		else return;
	});

	DEBUG_EVENT ev{};
	auto res = EnterDebugLoop(&ev, [&](const LPDEBUG_EVENT ev, LPDWORD lpdwContinueStatus) -> bool {
		switch (ev->u.Exception.ExceptionRecord.ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			// First chance: Pass this on to the system. 
			// Last chance: Display an appropriate error. 
			break;

		case EXCEPTION_BREAKPOINT:
			// First chance: Display the current 
			// instruction and register values. 
			if (shouldStop) {
				*lpdwContinueStatus = DBG_CONTINUE;
				return true;
			}
			break;

		case EXCEPTION_DATATYPE_MISALIGNMENT:
			// First chance: Pass this on to the system. 
			// Last chance: Display an appropriate error. 
			break;

		case EXCEPTION_SINGLE_STEP:
			// First chance: Update the display of the 
			// current instruction and register values. 
			break;

		case DBG_CONTROL_C:
			// First chance: Pass this on to the system. 
			// Last chance: Display an appropriate error. 
			break;

		default:
			// Handle other exceptions. 
			break;
		}
		if (!(ev->u.Exception.dwFirstChance)) {
			// last chance
			if (options.DumpOptions.bEnableDump) do {
				// Create a dump.
				wstring dump_dir = options.DumpOptions.DumpDirectory[0] ? options.DumpOptions.DumpDirectory : exe_path;
				wstring custom = options.DumpOptions.CustomizedDumpNameFormat;
				wstring custom_ctx = options.DumpOptions.CustomizedContextNameFormat;
				auto ts = time(0);
				auto work = [&exe_pure, &exe_stem, &pid_s, &ts](wstring& in) {
					str_replace(in, L"{exe}", exe_pure);
					str_replace(in, L"{exe-}", exe_stem);
					str_replace(in, L"{pid}", pid_s);
					str_replace(in, L"{ts}", to_wstring(ts));
				};
				if (custom.empty()) custom = format(L"{}.{}-{}.dmp", exe_pure, pid, ts);
				else work(custom);
				if (custom_ctx.empty()) custom_ctx = format(L"{}.{}-{}.crash_report", exe_pure, pid, ts);
				else work(custom_ctx);

				CONTEXT ctx{};
				ctx.ContextFlags = CONTEXT_ALL;
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, ev->dwThreadId);
				if (hThread) {
					GetThreadContext(hThread, &ctx);
					CloseHandle(hThread);
				}
				EXCEPTION_POINTERS ep = {};
				ep.ExceptionRecord = &ev->u.Exception.ExceptionRecord;
				ep.ContextRecord = &ctx;

				// Dump file.
				wstring finalDumpFile = filesystem::path(dump_dir) / custom;
				HANDLE hFile = CreateFileW(finalDumpFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile && hFile != INVALID_HANDLE_VALUE) {
					MINIDUMP_EXCEPTION_INFORMATION info{};
					info.ThreadId = ev->dwThreadId;
					info.ExceptionPointers = &ep;
					info.ClientPointers = NULL;
					MiniDumpWriteDump(hProcess, pid, hFile, options.DumpOptions.DumpType, &info, NULL, NULL);
					CloseHandle(hFile);
				}
				
				if (!options.DumpOptions.WithContext) break;

				wstring finalDumpFile2 = filesystem::path(dump_dir) / custom_ctx;
				hFile = CreateFileW(finalDumpFile2.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile && hFile != INVALID_HANDLE_VALUE) {
					DWORD tmp{};
					string Char1 = "Application has crashed!!\nProcess Identifier: " + to_string(pid) + "\nException record: [Length=" +
						to_string(sizeof(EXCEPTION_RECORD)) + "]";
					WriteFile(hFile, Char1.data(), (DWORD)Char1.size(), &tmp, NULL);
					WriteFile(hFile, &ev->u.Exception.ExceptionRecord, sizeof(EXCEPTION_RECORD), &tmp, NULL);
					Char1 = "\nContext: [Length=" + to_string(sizeof(CONTEXT)) + "]";
					WriteFile(hFile, Char1.data(), (DWORD)Char1.size(), &tmp, NULL);
					WriteFile(hFile, &ctx, sizeof(CONTEXT), &tmp, NULL);
					CloseHandle(hFile);
				}
			} while (0);
		}
		*lpdwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
		return false;
	});

	if (res.processDied) {
		code = res.diedValue;
	}
	else {
		DebugActiveProcessStop(pid);
		if (shouldStop) ExitProcess(0);
		WaitForSingleObject(hProcess, INFINITE);
		GetExitCodeProcess(hProcess, &code);
	}
	if (waiter.joinable()) waiter.join();

	if (code != ERROR_SUCCESS && code != 1) {
		// crashed maybe
		wstring text = L"Application has crashed!\nProcess Identifier: " + to_wstring(pid) + L"\nExit code: " + to_wstring(code) +
			L"\n\n" + ErrorChecker(code).raw_message();
		int user;
		bool AutoRestart = options.RestartOptions.AutoRestart;
		bool NoUserRestart = options.RestartOptions.bDisallowUserRestart;
		auto RestartCount = options.RestartOptions.RestartCount;
		if ((++RestartCount) == 0) {
			// overflow happened!!!
			NoUserRestart = true;
			AutoRestart = false;
		}

		do {
			user = 0;
			if (!options.RestartOptions.MaxRestartCount) {
				if (AutoRestart) {
					NoUserRestart = true;
				}
			}
			else if (RestartCount > options.RestartOptions.MaxRestartCount) {
				// if the count exceeded limit then the restart should be disabled regardless automatically or manually
				NoUserRestart = true;
			}
			else if (options.RestartOptions.AutoRestart) {
				// now restart is configured properly and count does not exceeded the limit.
				// do auto restart now
				user = IDYES;
				break;
			}

			if (!NoUserRestart) text += L"\n\nDo you want to relaunch the application?";
			if (!options.CrashOptions.SuppressErrorDialogs) user = MessageBoxW(NULL, text.c_str(), (exe + L" - System Error").c_str(),
				MB_ICONERROR | (NoUserRestart ? MB_OK : MB_YESNO) | MB_SERVICE_NOTIFICATION);
		} while (0);
		// append restart command line according to configurations
		if (options.RestartOptions.AddRecoveryInformationOptions.bAddRecoveryInformationInRestart) {
			if (options.RestartOptions.AddRecoveryInformationOptions.AddCustomString[0]) {
				cmd += (L" ");
				cmd += (options.RestartOptions.AddRecoveryInformationOptions.AddCustomString);
			}
			if (options.RestartOptions.AddRecoveryInformationOptions.AddRestartCount) {
				cmd += (format(L" {}{}{}",
					options.RestartOptions.AddRecoveryInformationOptions.AddRestartCountPrefix,
					RestartCount,
					options.RestartOptions.AddRecoveryInformationOptions.AddRestartCountSuffix));
			}
		}
		if (user == IDYES) {
			STARTUPINFO si{}; PROCESS_INFORMATION pi{};
			if (!CreateProcessW(exe.c_str(), cmd.data(), 0, 0, 0, 0, 0, 0, &si, &pi)) {
				MessageBoxW(0, ErrorChecker().message().c_str(), NULL, MB_ICONERROR | MB_SERVICE_NOTIFICATION);
				goto clean;
			}
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			CloseHandle(hProcess);
			LocalFree(argv);
			ExitProcess(0);
		}
		CloseHandle(hProcess);
		LocalFree(argv);
		ExitProcess(code);
	}

clean:
	if (hExitEventPeer) CloseHandle(hExitEventPeer);
	if (hExitEvent) CloseHandle(hExitEvent);
	CloseHandle(hProcess);
	LocalFree(argv);
}




