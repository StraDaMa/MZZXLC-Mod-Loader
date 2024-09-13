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
		return stage0_install();
	}
	break;
	case DLL_PROCESS_DETACH:
	{
		FreeConsole();
		return stage0_uninstall() && stage1_install();
	}
	break;
	}
	return TRUE;
}