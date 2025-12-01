#include "stage1.h"
#include <windows.h>
#include "MinHook.h"

namespace fs = std::filesystem;

namespace stage1 {
	typedef HANDLE(WINAPI* CreateFileAFunc)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	static CreateFileAFunc oCreateFileA = CreateFileA;
	static CreateFileAFunc opCreateFileA = nullptr;
	//boost::unordered::unordered_flat_map<fs::path, std::string> g_assetReplacements;

	HANDLE mCreateFileA(
		LPCSTR lpFileName,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes,
		HANDLE hTemplateFile
	) {
		LPCSTR finalFileName = lpFileName;
		auto findResult = g_assetReplacements.find(lpFileName);
		if (findResult != g_assetReplacements.end()) {
			finalFileName = findResult->second.c_str();
		}
		return opCreateFileA(finalFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}

	bool install() {
		if (MH_CreateHook(oCreateFileA, &mCreateFileA, (LPVOID*)&opCreateFileA) != MH_OK) [[unlikely]] {
			return FALSE;
		}
		if (MH_EnableHook(oCreateFileA) != MH_OK) [[unlikely]] {
			return FALSE;
		}
		return TRUE;
	}

	bool uninstall() {
		if (MH_DisableHook(oCreateFileA) != MH_OK) [[unliekly]] {
			return FALSE;
		}
		return TRUE;
	}
}