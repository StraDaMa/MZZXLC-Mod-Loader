#include "stage1.h"
#include <windows.h>
#include <detours.h>
namespace fs = std::filesystem;

namespace stage1 {
	static HANDLE(WINAPI* TrueCreateFileA)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) = CreateFileA;
	boost::unordered::unordered_flat_map<fs::path, std::string> _assetReplacements;

	HANDLE ModifiedCreateFileA(
		LPCSTR lpFileName,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes,
		HANDLE hTemplateFile
	) {
		LPCSTR finalFileName = lpFileName;
		auto findResult = _assetReplacements.find(lpFileName);
		if (findResult != _assetReplacements.end()) {
			finalFileName = findResult->second.c_str();
		}
		return TrueCreateFileA(finalFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}

	bool install() {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)TrueCreateFileA, (PVOID)ModifiedCreateFileA);
		DetourTransactionCommit();
		return TRUE;
	}

	bool uninstall() {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)TrueCreateFileA, (PVOID)ModifiedCreateFileA);
		DetourTransactionCommit();
		return TRUE;
	}
}