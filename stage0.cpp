#include "stage0.h"
#include "stage1.h"
#include "mod.h"
#include "gui.h"

#include <cstdio>
#include <cstdint>

#include <windows.h>
#include <detours.h>
#include <psapi.h>
#include <winternl.h>
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <boost/container/set.hpp>

#include <fstream>

#include <filesystem>
namespace fs = std::filesystem;

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

std::vector<HMODULE> loadedDLL;

static HMODULE(WINAPI* TrueGetModuleHandleA)(LPCSTR lpModuleName) = GetModuleHandleA;
bool getModuleHooked = false;
uint8_t* targetSectionPtr = nullptr;
size_t targetSectionSize = 0;
HMODULE ModifiedGetModuleHandleA(
	LPCSTR lpModuleName
) {
	HMODULE returnValue = TrueGetModuleHandleA(lpModuleName);
	if (lpModuleName == NULL) {
		return returnValue;
	}
	// Check if module name is "ntdll.dll"
	// The first call to ntdll is after the target function is unpacked
	if (strncmp(lpModuleName, "ntdll.dll", 9) == 0) {
		getModuleHooked = false;
		//*(uint8_t*)(0x142D57EC0) = 0xC3;
		// This pattern is enough to find this function
		const static std::array<uint8_t, 0x18> targetFunctionPattern = {
			0x55,                                          //PUSH RBP
			0x48, 0x89, 0xE5,                              //MOV RBP,RSP
			0x48, 0x8D, 0xA4, 0x24, 0x20, 0xFD, 0xFF, 0xFF,//LEA RSP,QWORD PTR SS:[RSP-0x2E0]
			0x48, 0x89, 0x9D, 0x48, 0xFD, 0xFF, 0xFF,      //MOV QWORD PTR SS:[RBP-0x2B8],RBX
			0xB8, 0x01, 0x00, 0x00, 0x00                   //MOV EAX,1
		};
		uint8_t* targetSectionEndPtr = targetSectionPtr + targetSectionSize;

		uint8_t* targetFunctionPtr = std::search(
			targetSectionPtr, targetSectionEndPtr,
			targetFunctionPattern.data(), targetFunctionPattern.data() + targetFunctionPattern.size()
		);
		if (targetFunctionPtr != targetSectionEndPtr) {
			// This section is still writable
			// Replace the first instruction with RET to prevent ntdll patching
			*targetFunctionPtr = 0xC3;
		}
		// Detour isnt needed anymore
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)TrueGetModuleHandleA, (PVOID)ModifiedGetModuleHandleA);
		DetourTransactionCommit();
	}
	return returnValue;
}

static HWND(WINAPI* TrueCreateWindowExW)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) = CreateWindowExW;
bool initialized = FALSE;
HWND ModifiedCreateWindowExW(
	DWORD     dwExStyle,
	LPCWSTR   lpClassName,
	LPCWSTR   lpWindowName,
	DWORD     dwStyle,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight,
	HWND      hWndParent,
	HMENU     hMenu,
	HINSTANCE hInstance,
	LPVOID    lpParam
) {
	bool focusWindow = false;
	if ((!initialized) && (lpWindowName != NULL)) {
		int res = lstrcmpW(lpWindowName,
			L"Mega Man Zero/ZX Legacy Collection / ROCKMAN ZERO&ZX DOUBLE HERO COLLECTION"
		);
		if (res == 0) {
			initialized = TRUE;

			// Make all sections writable
			HMODULE exeBase = GetModuleHandleA("MZZXLC.exe");
			uint8_t* exeBasePtr = (uint8_t*)exeBase;
			PIMAGE_DOS_HEADER exeDosHeader = (PIMAGE_DOS_HEADER)exeBasePtr;
			PIMAGE_NT_HEADERS exeNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)exeBasePtr + exeDosHeader->e_lfanew);
			for (size_t i = 0; i < exeNtHeader->FileHeader.NumberOfSections; i++) {
				PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)((DWORD_PTR)IMAGE_FIRST_SECTION(exeNtHeader) + ((DWORD_PTR)IMAGE_SIZEOF_SECTION_HEADER * i));
				DWORD oldPerms;
				VirtualProtect(
					exeBasePtr + sectionHeader->VirtualAddress,
					sectionHeader->Misc.VirtualSize,
					PAGE_EXECUTE_READWRITE,
					&oldPerms
				);
			}

			bool fileCreateHookNeeded = false;
			toml::table tbl;
			{
				toml::parse_result loaderConfigResult = toml::parse_file("modloader.toml");
				if (loaderConfigResult) {
					tbl = std::move(loaderConfigResult).table();
					if (!tbl.contains("enabled_mods")) {
						tbl.insert("enabled_mods", toml::array());
					}
					if (!tbl.contains("show_console")) {
						tbl.insert("show_console", false);
					}
					if (!tbl.contains("skip_ui")) {
						tbl.insert("skip_ui", false);
					}
				}
				else {
					tbl = toml::table{
						{"enabled_mods", toml::array()},
						{"show_console", false},
						{"skip_ui", false},
					};
				}
			}

			boost::container::set<std::string> loaderMods;
			toml::array* loader_enabled_mods_array = tbl["enabled_mods"].as_array();
			if (loader_enabled_mods_array) {
				for (const auto& elem : *loader_enabled_mods_array) {
					auto elemStr = elem.as_string();
					loaderMods.emplace(*elemStr);
				}
			}
			else {
				// If for some reason this value is not an array reinsert it as an array
				tbl.erase("enabled_mods");
				tbl.insert("enabled_mods", toml::array());
				loader_enabled_mods_array = tbl["enabled_mods"].as_array();
			}

			//Create Mods directory if it does not exist
			const fs::path modsDir = "mods";
			if (!fs::is_directory(modsDir)) {
				fs::create_directory(modsDir);
			}

			std::vector<ModInfo> mods;
			mods.reserve(20);

			auto skip_ui = tbl["skip_ui"].value_or(false);
			if (skip_ui) {
				findMods(modsDir, mods);
			}
			else {
				bool windowClosed = gui_WinMain(hInstance, &mods, &loaderMods);
				if (!windowClosed) {
					exit(0);
				}
			}

#ifndef _DEBUG
			auto show_console = tbl["show_console"].value_or(false);
			if (show_console) {
				AllocConsole();
				FILE* fDummy;
				freopen_s(&fDummy, "CONIN$", "r", stdin);
				freopen_s(&fDummy, "CONOUT$", "w", stderr);
				freopen_s(&fDummy, "CONOUT$", "w", stdout);
			}
#endif
			loader_enabled_mods_array->clear();
			for (size_t i = 0; i < mods.size(); i++) {
				if (mods[i].enabled) {
					const ModInfo& mod = mods[i];
					if (!checkCompatibility(mod.version_requirement)) {
						printf("\"%s\" is not compatible. Skipping.\n", mod.title.c_str());
						continue;
					}
					loader_enabled_mods_array->push_back(mod.name);
					fs::path modPath = modsDir;
					modPath /= mod.name;
					fs::path dllPath = modPath;
					dllPath /= mod.name;
					dllPath += ".dll";
					if (fs::exists(dllPath)) {
						wprintf(L"Loading Mod DLL: %s\n", dllPath.c_str());
						HMODULE modLib = LoadLibrary(dllPath.c_str());
						if (modLib != NULL) {
							loadedDLL.push_back(modLib);
							auto lpModOpen = (void(*)())GetProcAddress(modLib, "mod_open");
							if (lpModOpen != NULL) {
								lpModOpen();
							}
						}
					}
					fs::path assetsFolder = modPath;
					assetsFolder /= "nativePCx64";
					if (fs::is_directory(assetsFolder)) {
						wprintf(L"Setting up asset redirects from: %s\n", assetsFolder.c_str());
						fileCreateHookNeeded = true;
						for (const auto& assetEntry : fs::recursive_directory_iterator(assetsFolder)) {
							// Get the original asset this is replacing by getting the path after nativePCx64
							// and adding it to the exe base path
							_assetReplacements.emplace(
								fs::absolute(fs::relative(assetEntry.path(), modPath)),
								fs::absolute(assetEntry.path()).u8string()
							);
						}
					}
				}
			}
			{
				std::ofstream ofs("modloader.toml");
				if (ofs) {
					ofs << tbl;
					ofs.close();
				}
			}
			if (fileCreateHookNeeded) {
				stage1_install();
			}
			focusWindow = true;
		}
	}
	HWND returnVal = TrueCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	if (focusWindow)
		SetForegroundWindow(returnVal);
	return returnVal;
};

bool stage0_install() {
#ifdef _DEBUG
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	char injectorCmd[64] = {0};
	sprintf_s(injectorCmd, "InjectorCLIx64.exe pid:%d HookLibraryx64.dll nowait", GetCurrentProcessId());
	system(injectorCmd);
#endif
	HMODULE exeBase = GetModuleHandleA("MZZXLC.exe");
	if (exeBase == NULL) {
		return FALSE;
	}
	// Check if exe is protected
	uint8_t* exeBasePtr = (uint8_t*)exeBase;
	PIMAGE_DOS_HEADER exeDosHeader = (PIMAGE_DOS_HEADER)exeBasePtr;
	PIMAGE_NT_HEADERS exeNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)exeBasePtr + exeDosHeader->e_lfanew);
	PIMAGE_SECTION_HEADER firstSection = (PIMAGE_SECTION_HEADER)((DWORD_PTR)IMAGE_FIRST_SECTION(exeNtHeader));
	// If the first section has its name removed, it is probably protected
	if (*(uint64_t*)firstSection->Name == 0x0000000000000000) {
		// Section with target code is in the one right after .rsrc
		for (size_t i = 0; i < exeNtHeader->FileHeader.NumberOfSections; i++) {
			PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)((DWORD_PTR)IMAGE_FIRST_SECTION(exeNtHeader) + ((DWORD_PTR)IMAGE_SIZEOF_SECTION_HEADER * i));
			if (strncmp((char*)sectionHeader->Name, ".rsrc", 8) == 0) {
				PIMAGE_SECTION_HEADER targetSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD_PTR)IMAGE_FIRST_SECTION(exeNtHeader) + ((DWORD_PTR)IMAGE_SIZEOF_SECTION_HEADER * (i + 1)));
				targetSectionPtr = exeBasePtr + targetSectionHeader->VirtualAddress;
				targetSectionSize = targetSectionHeader->Misc.VirtualSize;
				break;
			}
		}
		getModuleHooked = true;
	}

	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (getModuleHooked) {
		DetourAttach(&(PVOID&)TrueGetModuleHandleA, (PVOID)ModifiedGetModuleHandleA);
	}
	DetourAttach(&(PVOID&)TrueCreateWindowExW, (PVOID)ModifiedCreateWindowExW);
	DetourTransactionCommit();
	loadedDLL.reserve(20);
	return TRUE;
}

bool stage0_uninstall() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (getModuleHooked) {
		DetourDetach(&(PVOID&)TrueGetModuleHandleA, (PVOID)ModifiedGetModuleHandleA);
	}
	DetourDetach(&(PVOID&)TrueCreateWindowExW, (PVOID)ModifiedCreateWindowExW);
	DetourTransactionCommit();
	for (size_t i = 0; i < loadedDLL.size(); i++)
	{
		FreeLibrary(loadedDLL[i]);
	}
	return TRUE;
}