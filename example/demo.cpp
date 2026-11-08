#include <iostream>
#include <string>
#include <format>
#include "../user/crashpad_handler.h"
#include "argparse.hpp"
using namespace std;

const wchar_t* GlobalReleasePinnedList[] = {
	L"cf4981108cd1665801a0e2c2de889d2e88974a49119b3e9551bed855d21ee42b",
	L"a36e314a63bbce35d9a4aa64e4df6590d1894e21c8e3c78afb19cf56c244ac3b",
};

inline static wstring s2ws(const string& str) {
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

static LONG WINAPI myfilter(
	_In_ struct _EXCEPTION_POINTERS* ExceptionInfo
) {
	// Note: If the exception is handled by crashpad_handler, the top level exception handler might not be executed,
	// so please do not rely on it when using the product
	cout << format("\n\nException!!!\nCode : {}\nAddress : {}", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress) << endl;
	cerr << "execute fault now!\n";
	return EXCEPTION_EXECUTE_FAULT;
	return EXCEPTION_CONTINUE_SEARCH;
}

static int helphelp() {
	return helphelp();
}

int main(int argc, char** argv) {
	argparse::ArgumentParser program("demo");
	program.add_argument("--is-restart").help("is restart").default_value(false).implicit_value(true);
	program.add_argument("--restart-count")
		.help("restart count")
		.nargs(1);
	program.add_argument("--dump-file")
		.help("dump-file")
		.nargs(1);
	program.add_argument("--dump-ctx")
		.help("dump-ctx")
		.nargs(1);
	try { program.parse_args(argc, argv); }
	catch (const std::exception&) {}

	if (program.get<bool>("--is-restart")) {
		cout << "Is restart!!!" << endl;
		if (program.present("--restart-count")) try {
			std::string b_val = program.get<std::string>("--restart-count");
			cout << "Restart count: " << b_val << endl;
		}
		catch (...) {}
	}
	if (program.present("--dump-file")) try {
		std::string b_val = program.get<std::string>("--dump-file");
		cout << "Dump file is on: " << b_val << endl;
	} catch (...) {}
	if (program.present("--dump-ctx")) try {
		std::string b_val = program.get<std::string>("--dump-ctx");
		cout << "Dump context file is on: " << b_val << endl;
	} catch (...) {}

	char lpszPath[2048];
	cout << "Please input the path of MyCrashPadHandler: ";
	cout.flush();
	cin.getline(lpszPath, 2048);
	char buffer2[16]{};
	cout << "Do you want to customize options? ";
	cin.getline(buffer2, 16);
	auto options = make_unique<InitializeCrashpadHandlerOptions>(); // avoid creating the structure on stack
	options->cb = sizeof(InitializeCrashpadHandlerOptions);
	options->DumpOptions.EnableDump = true;
	options->DumpOptions.DumpType = MiniDumpWithFullMemory; // Please decide this according to your app, blindly use FullMemory may seriously impact performance and disk space
	options->DumpOptions.WithContext = true;
	if (program.present("--restart-count")) try {
		std::string b_val = program.get<std::string>("--restart-count");
		options->RestartOptions.RestartCount = stoul(b_val);
	} catch (...) {}
	options->RestartOptions.MaxRestartCount = 3;
	options->RestartOptions.AddRecoveryInformationOptions.AddRecoveryInformationInRestart = true;
	options->RestartOptions.AddRecoveryInformationOptions.AddRestartCount = true;
	options->RestartOptions.AddRecoveryInformationOptions.AddDumpFilePath = true;
	options->RestartOptions.AddRecoveryInformationOptions.AddDumpContextPath = true;
	wcscpy_s(options->RestartOptions.RestartCommandLine, L"App"); // change to your real command line or if your app dont need cmdline just use placeholder like this. Do not forget to remove already-existing --restart-count! Otherwise it will duplicate
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddCustomString, L"--is-restart");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddRestartCountPrefix, L"--restart-count=\"");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddRestartCountSuffix, L"\"");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddDumpFilePathPrefix, L"--dump-file=\"");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddDumpFilePathSuffix, L"\"");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddDumpContextPathPrefix, L"--dump-ctx=\"");
	wcscpy_s(options->RestartOptions.AddRecoveryInformationOptions.AddDumpContextPathSuffix, L"\"");
	
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
		cout << "Do you want to not to attach debugger? ";
		cin.getline(buffer2, 16);
		if (buffer2 == "y"s || buffer2 == "Y"s) options->AttachOptions.NoAttachDebugger = true;
		cout << "Do you want to suppress dialogs? ";
		cin.getline(buffer2, 16);
		if (buffer2 == "y"s || buffer2 == "Y"s) options->CrashOptions.SuppressErrorDialogs = true;
		cout << "Do you want to disallow restart? ";
		cin.getline(buffer2, 16);
		if (buffer2 == "y"s || buffer2 == "Y"s) options->RestartOptions.DisallowRestart = true;
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
		cout << "-1. Call main again so that you can initialize again" << endl;
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
		cout << "11. Use structured exception handling" << endl;
		cout << "Please choose: ";
		cout.flush();
		cin.getline(choice, 16);
		string chosen = choice;
		
		if (chosen == "-1") {
			auto r = main(argc, argv);
			cout << "Returned from main. Code: " << r << endl;
			continue;
		}
		if (chosen == "0") {
			return 0;
		}
		else if (chosen == "1" || chosen == "2") {
			// Important: For most applications you do NOT need to manually uninitialize the handler
			// because it will automatically end after your application exits. Only uninitialize 
			// manually if you want to initialize again or you have other special requirements
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
			cout << *((int*)((ULONG_PTR)(0xdeaddeaddeaddeadull)));
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
		else if (chosen == "11") {
			int r = ([] () -> int {
				__try { return *((int*)0xdead); }
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return 1;
				}
			})();
			cout << "Result: " << r << endl;
		}
		else {
			cout << "Invalid input!" << endl;
		}
	}
	
	return 0;
}