// Windows Header Files
#include <windows.h>
#include "detours.h"
#include "stage0.h"
#include "stage1.h"
#include <cstdio>

extern "C" { int __afxForceUSRDLL; }

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	if (DetourIsHelperProcess()) {
		return TRUE;
	}
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
		return stage0::install();
	}
	break;
	case DLL_PROCESS_DETACH:
	{
		FreeConsole();
		bool result1 = stage0::uninstall();
		bool result2 = stage1::uninstall();
		return result1 && result2;
	}
	break;
	}
	return TRUE;
}