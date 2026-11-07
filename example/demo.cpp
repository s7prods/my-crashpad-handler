#include <iostream>
#include <string>
#include <format>
#include "../user/crashpad_handler.h"
#include "argparse.hpp"
using namespace std;

const wchar_t* GlobalReleasePinnedList[] = {
	L"add47056b6a3b205e74c16a5e1c14f70bb9f2be3c3fa48716530260378c5a1c7",
	L"354b1082ce47df0e16a1e25d2176dc89bd3df0090adf2a3c3161c5cdb76773f9",
};

inline wstring s2ws(const string& str) {
	wstring result;
	size_t len = MultiByteToWideChar(CP_ACP, 0, str.c_str(),
		(int)(str.size()), NULL, 0);
	if (len < 0) return result;
	wchar_t* buffer = new wchar_t[len + 1];
	if (buffer == NULL) return result;
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)(str.size()),
		buffer, (int)len);
	buffer[len] = '\0';
	result.append(buffer);
	delete[] buffer;
	return result;
}

LONG WINAPI myfilter(
	_In_ struct _EXCEPTION_POINTERS* ExceptionInfo
) {
	cout << format("\n\nException!!!\nCode : {}\nAddress : {}", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress) << endl;
	cerr << "execute fault now!\n";
	return EXCEPTION_EXECUTE_FAULT;
	return EXCEPTION_CONTINUE_SEARCH;
}

int helphelp() {
	return helphelp();
}

int main(int argc, char** argv) {
	argparse::ArgumentParser program("demo");
	program.add_argument("--is-restart").help("is restart").default_value(false).implicit_value(true);
	program.add_argument("--restart-count")
		.help("restart count")
		.nargs(1);
	try { program.parse_args(argc, argv); }
	catch (const std::exception& err) {}

	if (program.get<bool>("--is-restart")) {
		cout << "Is restart!!!" << endl;
		if (program.present("--restart-count")) try {
			std::string b_val = program.get<std::string>("--restart-count");
			cout << "Restart count: " << b_val << endl;
		}
		catch (...) {}
	}

	char lpszPath[2048];
	cout << "Please input the path of MyCrashPadHandler: ";
	cout.flush();
	cin.getline(lpszPath, 2048);
	char buffer2[16]{};
	cout << "Do you want to customize options? ";
	cin.getline(buffer2, 16);
	auto options = make_unique<InitializeCrashpadHandlerOptions>();
	options->cb = sizeof(InitializeCrashpadHandlerOptions);
	options->DumpOptions.bEnableDump = true;
	options->DumpOptions.DumpType = MiniDumpWithFullMemory;
	options->DumpOptions.WithContext = true;
	if (program.present("--restart-count")) try {
		std::string b_val = program.get<std::string>("--restart-count");
		options->RestartOptions.RestartCount = stoul(b_val);
	} catch (...) {}
	options->RestartOptions.MaxRestartCount = 3;
	options->RestartOptions.AddRecoveryInformationOptions.bAddRecoveryInformationInRestart = true;
	options->RestartOptions.AddRecoveryInformationOptions.AddRestartCount = true;
	wcscpy_s(options->RestartOptions.RestartCommandLine, L"App"); // change to your real command line or if your app dont need cmdline just use placeholder like this. Do not forget to remove already-existing --restart-count! Otherwise it will duplicate
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddRestartCountPrefix, L"--restart-count=\"");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddRestartCountSuffix, L"\"");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddCustomString, L"--is-restart");
	
	HMODULE hModule{};
	LPVOID lpContext{};
#ifdef NDEBUG
	const wchar_t** pinned = GlobalReleasePinnedList;
	const SIZE_T pinnedCount = 2;
#else
	const wchar_t** pinned = NULL;
	const SIZE_T pinnedCount = 0;
#endif
	if (buffer2 == "y"s || buffer2 == "Y"s) {
		cout << "Do you want to suppress dialogs? ";
		cin.getline(buffer2, 16);
		if (buffer2 == "y"s || buffer2 == "Y"s) options->CrashOptions.SuppressErrorDialogs = true;
		cout << "Do you want to disallow user restart? ";
		cin.getline(buffer2, 16);
		if (buffer2 == "y"s || buffer2 == "Y"s) options->RestartOptions.bDisallowUserRestart = true;
		cout << "Do you want to auto restart? ";
		cin.getline(buffer2, 16);
		if (buffer2 == "y"s || buffer2 == "Y"s) options->RestartOptions.AutoRestart = true;
	}
	else {
		options = nullptr;
	}
	auto res = InitializeCrashpadHandler(NULL, s2ws(lpszPath).c_str(), pinned, pinnedCount, options.get(), &hModule, &lpContext);
	
	cout << "Init result: " << res << " indicates " << (!!res ? "failure" : "success") << endl;
	
	while (true) {
		char choice[16]{};
		cout << string(40, '=') << endl;
		Sleep(500);
		cout << "0. Exit normally" << endl;
		cout << "1. Uninitialize immediately" << endl;
		cout << "2. Uninitialize and exit normally" << endl;
		cout << "3. Exit using specific exit code" << endl;
		cout << "4. Trigger Access Violation" << endl;
		cout << "5. Trigger Stack Overflow" << endl;
		cout << "6. Trigger heap corruption" << endl;
		cout << "7. Trigger heap corruption method 2" << endl;
		cout << "8. Trigger fast fail method native" << endl;
		cout << "9. Trigger fast fail method syscall" << endl;
		cout << "10. Trigger fast fail method custom syscall" << endl;
		cout << "Please choose: ";
		cout.flush();
		cin.getline(choice, 16);
		string chosen = choice;
		
		if (chosen == "0") {
			return 0;
		}
		else if (chosen == "1" || chosen == "2") {
			DWORD res = 0;
			if (!hModule || !lpContext) {
				if (chosen == "2") goto b_2_2;
				cout << "Not initialized, cannot uninit" << endl;
				continue;
			} 
			res = UninitializeCrashpadHandler(hModule, lpContext);
			cout << "res: " << res << endl;
			if (0 == res) {
				hModule = 0;
				lpContext = 0;
			}
			if (chosen == "2") {
				b_2_2:
				cout << "Going to exit..." << endl;
				Sleep(2000);
				break;
			}
		}
		else if (chosen == "3") {
			uint32_t code = 1;
			cout << "Your code is: ";
			cin >> code;
			return code;
		}
		else if (chosen == "4") {
			SetUnhandledExceptionFilter(myfilter);
			((void(*)())((ULONG_PTR)(0xdeaddeaddeaddeadull)))();
		}
		else if (chosen == "5") {
			helphelp();
		}
		else if (chosen == "6") {
			uint64_t* lp = new uint64_t;
			delete lp;
			delete lp;
			delete lp;
			delete lp;
			delete lp;
			delete lp;
			delete lp;
			delete lp;
			delete lp;
			delete lp;
			// still no die?
			cerr << "Unbeliveable!!\n";
			CONTEXT ctx{}; ctx.ContextFlags = CONTEXT_FULL; RtlCaptureContext(&ctx);
			EXCEPTION_RECORD exc{}; exc.ExceptionCode = 0xC0000001; exc.ExceptionAddress = _ReturnAddress();
			RaiseFailFastException(&exc, &ctx, 0);
			// still no die???
			TerminateProcess(GetCurrentProcess(), 0xc0000001);
			// still no die?????
			return -1;
		}
		else if (chosen == "7") {
			HANDLE heap = HeapCreate(0,0,0);
			if (heap == NULL) {
				cout << "no create heap error=" << GetLastError() << endl;
				continue;
			}
			void* p = HeapAlloc(heap, 0, 1024);
			if (p != NULL) {
				HeapFree(heap, 0, p);
				HeapFree(heap, 0, p);
				HeapFree(heap, 0, p);
				HeapFree(heap, 0, p);
				HeapFree(heap, 0, p);
			}
			HeapDestroy(heap);
		}
		else if (chosen == "8") {
			SetUnhandledExceptionFilter(myfilter);
			__fastfail(2);
		}
		else if (chosen == "9") {
			SetUnhandledExceptionFilter(myfilter);
			RaiseFailFastException(NULL, NULL, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
		}
		else if (chosen == "10") {
			SetUnhandledExceptionFilter(myfilter);
			CONTEXT ctx{}; ctx.ContextFlags = CONTEXT_FULL; RtlCaptureContext(&ctx);
			EXCEPTION_RECORD exc{}; exc.ExceptionCode = 0xC000000C; exc.ExceptionAddress = _ReturnAddress();
			RaiseFailFastException(&exc, &ctx, 0);
		}
		else {
			cout << "Invalid input!" << endl;
		}
	}
	
	return 0;
}