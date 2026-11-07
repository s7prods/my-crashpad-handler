#include <iostream>
#include <string>
#include <format>
#include "../user/crashpad_handler.h"
using namespace std;

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

int main() {
	char lpszPath[2048];
	cout << "Please input the path of MyCrashPadHandler!" << endl;
	cin.getline(lpszPath, 2048);
	
	HMODULE hModule{};
	LPVOID lpContext{};
#ifndef _DEBUG
	const wchar_t* pinned[] = {
		L"21935dc05248148761655de8375cc4eb0642ee3b6e531ce607e1b820c2286463",
		L"4ea2b278f0318dacef7e86730e0d870816175db6abae524a8fe033751765d5a3",
	};
	const SIZE_T pinnedCount = 2;
#else
	const wchar_t** pinned = NULL;
	const SIZE_T pinnedCount = 0;
#endif
	auto res = InitializeCrashpadHandler(NULL, s2ws(lpszPath).c_str(), pinned, pinnedCount, &hModule, &lpContext);
	
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
			if (!hModule || !lpContext) {
				cout << "Not initialized, cannot uninit" << endl;
				continue;
			} 
			auto res = UninitializeCrashpadHandler(hModule, lpContext);
			cout << "res: " << res << endl;
			if (res) {
				hModule = 0;
				lpContext = 0;
			}
			if (chosen == "2") {
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