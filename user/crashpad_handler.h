#pragma once
// Version: 1.3.1
#include <Windows.h>
#include <bcrypt.h>
#include <DbgHelp.h>
#include <stdint.h>
#pragma comment(lib, "bcrypt.lib")
/*
								 Apache License
						   Version 2.0, January 2004
						http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

	  "License" shall mean the terms and conditions for use, reproduction,
	  and distribution as defined by Sections 1 through 9 of this document.

	  "Licensor" shall mean the copyright owner or entity authorized by
	  the copyright owner that is granting the License.

	  "Legal Entity" shall mean the union of the acting entity and all
	  other entities that control, are controlled by, or are under common
	  control with that entity. For the purposes of this definition,
	  "control" means (i) the power, direct or indirect, to cause the
	  direction or management of such entity, whether by contract or
	  otherwise, or (ii) ownership of fifty percent (50%) or more of the
	  outstanding shares, or (iii) beneficial ownership of such entity.

	  "You" (or "Your") shall mean an individual or Legal Entity
	  exercising permissions granted by this License.

	  "Source" form shall mean the preferred form for making modifications,
	  including but not limited to software source code, documentation
	  source, and configuration files.

	  "Object" form shall mean any form resulting from mechanical
	  transformation or translation of a Source form, including but
	  not limited to compiled object code, generated documentation,
	  and conversions to other media types.

	  "Work" shall mean the work of authorship, whether in Source or
	  Object form, made available under the License, as indicated by a
	  copyright notice that is included in or attached to the work
	  (an example is provided in the Appendix below).

	  "Derivative Works" shall mean any work, whether in Source or Object
	  form, that is based on (or derived from) the Work and for which the
	  editorial revisions, annotations, elaborations, or other modifications
	  represent, as a whole, an original work of authorship. For the purposes
	  of this License, Derivative Works shall not include works that remain
	  separable from, or merely link (or bind by name) to the interfaces of,
	  the Work and Derivative Works thereof.

	  "Contribution" shall mean any work of authorship, including
	  the original version of the Work and any modifications or additions
	  to that Work or Derivative Works thereof, that is intentionally
	  submitted to Licensor for inclusion in the Work by the copyright owner
	  or by an individual or Legal Entity authorized to submit on behalf of
	  the copyright owner. For the purposes of this definition, "submitted"
	  means any form of electronic, verbal, or written communication sent
	  to the Licensor or its representatives, including but not limited to
	  communication on electronic mailing lists, source code control systems,
	  and issue tracking systems that are managed by, or on behalf of, the
	  Licensor for the purpose of discussing and improving the Work, but
	  excluding communication that is conspicuously marked or otherwise
	  designated in writing by the copyright owner as "Not a Contribution."

	  "Contributor" shall mean Licensor and any individual or Legal Entity
	  on behalf of whom a Contribution has been received by Licensor and
	  subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
	  this License, each Contributor hereby grants to You a perpetual,
	  worldwide, non-exclusive, no-charge, royalty-free, irrevocable
	  copyright license to reproduce, prepare Derivative Works of,
	  publicly display, publicly perform, sublicense, and distribute the
	  Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
	  this License, each Contributor hereby grants to You a perpetual,
	  worldwide, non-exclusive, no-charge, royalty-free, irrevocable
	  (except as stated in this section) patent license to make, have made,
	  use, offer to sell, sell, import, and otherwise transfer the Work,
	  where such license applies only to those patent claims licensable
	  by such Contributor that are necessarily infringed by their
	  Contribution(s) alone or by combination of their Contribution(s)
	  with the Work to which such Contribution(s) was submitted. If You
	  institute patent litigation against any entity (including a
	  cross-claim or counterclaim in a lawsuit) alleging that the Work
	  or a Contribution incorporated within the Work constitutes direct
	  or contributory patent infringement, then any patent licenses
	  granted to You under this License for that Work shall terminate
	  as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
	  Work or Derivative Works thereof in any medium, with or without
	  modifications, and in Source or Object form, provided that You
	  meet the following conditions:

	  (a) You must give any other recipients of the Work or
		  Derivative Works a copy of this License; and

	  (b) You must cause any modified files to carry prominent notices
		  stating that You changed the files; and

	  (c) You must retain, in the Source form of any Derivative Works
		  that You distribute, all copyright, patent, trademark, and
		  attribution notices from the Source form of the Work,
		  excluding those notices that do not pertain to any part of
		  the Derivative Works; and

	  (d) If the Work includes a "NOTICE" text file as part of its
		  distribution, then any Derivative Works that You distribute must
		  include a readable copy of the attribution notices contained
		  within such NOTICE file, excluding those notices that do not
		  pertain to any part of the Derivative Works, in at least one
		  of the following places: within a NOTICE text file distributed
		  as part of the Derivative Works; within the Source form or
		  documentation, if provided along with the Derivative Works; or,
		  within a display generated by the Derivative Works, if and
		  wherever such third-party notices normally appear. The contents
		  of the NOTICE file are for informational purposes only and
		  do not modify the License. You may add Your own attribution
		  notices within Derivative Works that You distribute, alongside
		  or as an addendum to the NOTICE text from the Work, provided
		  that such additional attribution notices cannot be construed
		  as modifying the License.

	  You may add Your own copyright statement to Your modifications and
	  may provide additional or different license terms and conditions
	  for use, reproduction, or distribution of Your modifications, or
	  for any such Derivative Works as a whole, provided Your use,
	  reproduction, and distribution of the Work otherwise complies with
	  the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
	  any Contribution intentionally submitted for inclusion in the Work
	  by You to the Licensor shall be under the terms and conditions of
	  this License, without any additional terms or conditions.
	  Notwithstanding the above, nothing herein shall supersede or modify
	  the terms of any separate license agreement you may have executed
	  with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
	  names, trademarks, service marks, or product names of the Licensor,
	  except as required for reasonable and customary use in describing the
	  origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
	  agreed to in writing, Licensor provides the Work (and each
	  Contributor provides its Contributions) on an "AS IS" BASIS,
	  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
	  implied, including, without limitation, any warranties or conditions
	  of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
	  PARTICULAR PURPOSE. You are solely responsible for determining the
	  appropriateness of using or redistributing the Work and assume any
	  risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
	  whether in tort (including negligence), contract, or otherwise,
	  unless required by applicable law (such as deliberate and grossly
	  negligent acts) or agreed to in writing, shall any Contributor be
	  liable to You for damages, including any direct, indirect, special,
	  incidental, or consequential damages of any character arising as a
	  result of this License or out of the use or inability to use the
	  Work (including but not limited to damages for loss of goodwill,
	  work stoppage, computer failure or malfunction, or any and all
	  other commercial damages or losses), even if such Contributor
	  has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
	  the Work or Derivative Works thereof, You may choose to offer,
	  and charge a fee for, acceptance of support, warranty, indemnity,
	  or other liability obligations and/or rights consistent with this
	  License. However, in accepting such obligations, You may act only
	  on Your own behalf and on Your sole responsibility, not on behalf
	  of any other Contributor, and only if You agree to indemnify,
	  defend, and hold each Contributor harmless for any liability
	  incurred by, or claims asserted against, such Contributor by reason
	  of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

	  To apply the Apache License to your work, attach the following
	  boilerplate notice, with the fields enclosed by brackets "[]"
	  replaced with your own identifying information. (Don't include
	  the brackets!)  The text should be enclosed in the appropriate
	  comment syntax for the file format. We also recommend that a
	  file or class name and description of purpose be included on the
	  same "printed page" as the copyright notice for easier
	  identification within third-party archives.

   Copyright 2026 [shc0743](https://github.com/shc0743)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/
#ifdef __cplusplus
extern "C" {
#endif


#define Internal_InitializeCrashpadHandler_Version 1000003000001ull




/// <summary>
/// The options for InitializeCrashpadHandler
/// </summary>
#pragma pack(push, 1)
struct InitializeCrashpadHandlerOptions {
	DWORD cb;
	struct {
		BOOL LoadFullQualified; // if used, please give full qualified dll path. Can be used to enhance security
	} LoadOptions;
	struct {
		// Some error dialogs appear even if you turned AutoRestart on. If you want to suppress all dialogs please set this
		BOOL SuppressErrorDialogs;
	} CrashOptions;
	struct {
		BOOL bEnableDump;
		MINIDUMP_TYPE DumpType;
		WCHAR DumpDirectory[1536];
		WCHAR CustomizedDumpNameFormat[512]; // {exe} = executable name; {exe-} = exe without suffix; {pid} = PID; {ts} = timestamp
		BOOL WithContext;
		WCHAR CustomizedContextNameFormat[512]; // see also CustomizedDumpNameFormat
	} DumpOptions;
	struct {
		BOOL bDisallowUserRestart;
		WCHAR RestartCommandLine[32768];
		// Important: You should try to maintain this field in a real product, otherwise the application may restart endlessly
		UINT RestartCount;
		// If the increased RestartCount is greater than MaxRestartCount (e.g. you give RestartCount=0,Max=1, the actual compared 
		// expression is `1>1` which is false; then you give RestartCount=1,Max=1, `2>1` is compared and it blocks further restart)
		// special rule: If MaxRestartCount=0 and AutoRestart=0, actually restart is allowed (but manually prompted to the user); if
		// MaxRestartCount=0 and AutoRestart=1 then the entire restart functionality is blocked. (the user might be prompted according
		// to SuppressErrorDialogs=0 but the restart option will be disabled)
		UINT MaxRestartCount;
		BOOL AutoRestart;
		struct {
			BOOL bAddRecoveryInformationInRestart;
			WCHAR AddCustomString[256];
			BOOL AddRestartCount;
			WCHAR AddRestartCountPrefix[32];
			WCHAR AddRestartCountSuffix[32];
		} AddRecoveryInformationOptions;
	} RestartOptions;
};
#pragma pack(pop)

/// <summary>
/// Initialize the crashpad handler
/// </summary>
/// <param name="hApplication">The handle of your application main module, Use NULL to automatically use the handle of the main process</param>
/// <param name="lpszExternalHandler">The path of the handler library</param>
/// <param name="lpszPinnedHandlerHashList">Optionally provide a list of lowercase hex format SHA-256 values to pin them to prevent tampering</param>
/// <param name="nCountPinnedHandlerHashList">The count of the pinned handler hash list</param>
/// <param name="lpOptions">Optionally provide a crashpad handler option</param>
/// <param name="lpModule">Optionally provide a output pointer to receive the loaded module so that you can uninitialize later, NEVER free it manually</param>
/// <param name="lpContext">Optionally provide a output pointer to receive the opaque context handle so that you can uninitialize later</param>
/// <returns>The error code. ERROR_SUCCESS means succeed</returns>
_Success_(return == ERROR_SUCCESS) static DWORD InitializeCrashpadHandler(
	_In_opt_ HMODULE hApplication,
	_In_z_ LPCWSTR lpszExternalHandler,
	_In_opt_count_(nCountPinnedHandlerHashList) const LPCWSTR* lpszPinnedHandlerHashList,
	_In_ SIZE_T nCountPinnedHandlerHashList,
	_In_opt_ const InitializeCrashpadHandlerOptions* lpOptions,
	_Out_opt_ HMODULE* lpModule,
	_Out_opt_ LPVOID* lpContext
) {
	if (lpszExternalHandler == NULL || *lpszExternalHandler == L'\0') {
		return ERROR_INVALID_PARAMETER;
	}

	HANDLE hInput = CreateFileW(lpszExternalHandler, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hInput == INVALID_HANDLE_VALUE || !hInput) {
		DWORD err = GetLastError();
		if (!err) err = 1;
		return err;
	}
	if (lpszPinnedHandlerHashList != NULL) {
		if (nCountPinnedHandlerHashList == 0) {
			CloseHandle(hInput);
			return ERROR_INVALID_PARAMETER;
		}

		BCRYPT_ALG_HANDLE  hAlg = NULL;
		BCRYPT_HASH_HANDLE hHash = NULL;
		PUCHAR pObj = NULL;
		BYTE digest[32];
		BYTE* lpBuffer;
		const size_t cbBuffer = 16 * 1024 * sizeof(BYTE);
		DWORD cbObj = 0, cbResult = 0, cbRead = 0;
		NTSTATUS st = 0;
		WCHAR actualHash[65] = { 0 };
		static const WCHAR kHex[] = L"0123456789abcdef";
		SIZE_T i = 0;
		int k = 0;

		lpBuffer = (BYTE*)VirtualAlloc(0, cbBuffer, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!lpBuffer) { CloseHandle(hInput); return ERROR_NOT_ENOUGH_MEMORY; }

		st = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
		if (!(st >= 0)) { VirtualFree(lpBuffer, 0, MEM_RELEASE); CloseHandle(hInput); return ERROR_NOT_ENOUGH_MEMORY; }

		st = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH,
			(PUCHAR)&cbObj, sizeof(cbObj), &cbResult, 0);
		if (!(st >= 0)) { BCryptCloseAlgorithmProvider(hAlg, 0); VirtualFree(lpBuffer, 0, MEM_RELEASE); CloseHandle(hInput); return ERROR_NOT_ENOUGH_MEMORY; }

		pObj = (PUCHAR)HeapAlloc(GetProcessHeap(), 0, cbObj);
		if (!pObj) { BCryptCloseAlgorithmProvider(hAlg, 0); VirtualFree(lpBuffer, 0, MEM_RELEASE); CloseHandle(hInput); return ERROR_NOT_ENOUGH_MEMORY; }

		st = BCryptCreateHash(hAlg, &hHash, pObj, cbObj, NULL, 0, 0);
		if (!(st >= 0)) { HeapFree(GetProcessHeap(), 0, pObj); BCryptCloseAlgorithmProvider(hAlg, 0); VirtualFree(lpBuffer, 0, MEM_RELEASE); CloseHandle(hInput); return ERROR_NOT_ENOUGH_MEMORY; }

		for (;;) {
			if (!ReadFile(hInput, lpBuffer, cbBuffer, &cbRead, NULL)) {
				DWORD err = GetLastError();
				if (!err) err = 1;
				BCryptDestroyHash(hHash); HeapFree(GetProcessHeap(), 0, pObj);
				BCryptCloseAlgorithmProvider(hAlg, 0); VirtualFree(lpBuffer, 0, MEM_RELEASE); CloseHandle(hInput);
				return err;
			}
			if (cbRead == 0) break;
			st = BCryptHashData(hHash, lpBuffer, cbRead, 0);
			if (!(st >= 0)) {
				BCryptDestroyHash(hHash); HeapFree(GetProcessHeap(), 0, pObj);
				BCryptCloseAlgorithmProvider(hAlg, 0); VirtualFree(lpBuffer, 0, MEM_RELEASE); CloseHandle(hInput);
				return ERROR_NOT_ENOUGH_MEMORY;
			}
		}

		st = BCryptFinishHash(hHash, digest, sizeof(digest), 0);
		BCryptDestroyHash(hHash);
		HeapFree(GetProcessHeap(), 0, pObj);
		BCryptCloseAlgorithmProvider(hAlg, 0);
		VirtualFree(lpBuffer, 0, MEM_RELEASE);
		lpBuffer = NULL;
		if (!(st >= 0)) { CloseHandle(hInput); return ERROR_NOT_ENOUGH_MEMORY; }

		for (k = 0; k < 32; ++k) {
			actualHash[k * 2] = kHex[digest[k] >> 4];
			actualHash[k * 2 + 1] = kHex[digest[k] & 0x0F];
		}
		actualHash[64] = L'\0';

		BOOL success = FALSE;
		for (SIZE_T i = 0; i < nCountPinnedHandlerHashList; ++i) {
			LPCWSTR hash = lpszPinnedHandlerHashList[i];
			if (hash == NULL || *hash == L'\0') {
				CloseHandle(hInput);
				return ERROR_INVALID_PARAMETER;
			}
			if (wcscmp(hash, actualHash) == 0) {
				success = TRUE;
				break;
			}
		}
		if (!success) {
			CloseHandle(hInput);
			return ERROR_FILE_CORRUPT;
		}
	}
	else {
		if (nCountPinnedHandlerHashList != 0) {
			CloseHandle(hInput);
			return ERROR_INVALID_PARAMETER;
		}
	}

	HMODULE hModule = NULL;
	if (lpOptions && lpOptions->LoadOptions.LoadFullQualified) hModule = LoadLibraryExW(lpszExternalHandler, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
	else hModule = LoadLibraryW(lpszExternalHandler);
	if (!hModule) {
		CloseHandle(hInput);
		DWORD err = GetLastError();
		if (!err) err = 1;
		return err;
	}

	typedef uint64_t(__stdcall* version_t)();
	typedef LPVOID(WINAPI* main_t)(HMODULE, DWORD, const InitializeCrashpadHandlerOptions*);

	version_t Version = (version_t)GetProcAddress(hModule, "Version");
	main_t handle_crash = (main_t)GetProcAddress(hModule, "crashpad_handler_Builtin");
	if (!Version || !handle_crash) {
		FreeLibrary(hModule);
		CloseHandle(hInput);
		return ERROR_FILE_NOT_SUPPORTED;
	}
	uint64_t version = Version();
	if (version != Internal_InitializeCrashpadHandler_Version) {
		FreeLibrary(hModule);
		CloseHandle(hInput);
		return ERROR_FILE_NOT_SUPPORTED;
	}

	LPVOID ret = handle_crash(hApplication, GetCurrentProcessId(), lpOptions); // This is implementation-specific and do not try to hack it
	if (!ret) {
		DWORD err = GetLastError();
		if (!err) err = 1;
		FreeLibrary(hModule);
		CloseHandle(hInput);
		return err;
	}
	if (lpModule) *lpModule = hModule;
	if (lpContext) *lpContext = ret;
	// DO NOT release the module directly!
	// such as the bad code:
	//FreeLibrary(hModule);
	// instead, always use the Uninitialize function
	// DO NOT operate the context directly!
	// its implementation is implementation-specific and we do not guarantee its content despite the product code is visible
	// instead, always use the Uninitialize function

	CloseHandle(hInput);
	return ERROR_SUCCESS;
}

/// <summary>
/// Uninitialize the crashpad handler
/// </summary>
/// <param name="hModule">The module handle returned by the initialization function</param>
/// <param name="lpContext">The context handle returned by the initialization function</param>
/// <returns>The error code. ERROR_SUCCESS means succeed</returns>
_Success_(return == ERROR_SUCCESS) static DWORD UninitializeCrashpadHandler(_In_ HMODULE hModule, _In_ LPVOID lpContext) {
	if (!hModule || !lpContext) {
		return ERROR_INVALID_PARAMETER;
	}

	typedef uint64_t(__stdcall* version_t)();
	typedef BOOL(WINAPI* main_t)(LPVOID);

	version_t Version = (version_t)GetProcAddress(hModule, "Version");
	main_t main_func = (main_t)GetProcAddress(hModule, "crashpad_handler_Builtin_Free");
	if (!Version || !main_func) {
		return ERROR_FILE_NOT_SUPPORTED;
	}
	uint64_t version = Version();
	if (version != Internal_InitializeCrashpadHandler_Version) {
		return ERROR_FILE_NOT_SUPPORTED;
	}

	BOOL success = main_func(lpContext);
	if (!success) {
		DWORD err = GetLastError();
		if (!err) err = 1;
		return err;
	}

	// finally release the module
	success = FreeLibrary(hModule);
	if (!success) {
		DWORD err = GetLastError();
		if (!err) err = 1;
		return err;
	}
	return ERROR_SUCCESS;
}



#undef Internal_InitializeCrashpadHandler_Version

#ifdef __cplusplus
}
#endif

