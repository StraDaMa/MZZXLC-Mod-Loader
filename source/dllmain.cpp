// Windows Header Files
#include <windows.h>

#include "MinHook.h"

#include "stage0.h"
#include "stage1.h"

extern "C" { int __afxForceUSRDLL; }

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
		bool mhInit = MH_Initialize() == MH_OK;
		return mhInit && stage0::install();
	}
	break;
	case DLL_PROCESS_DETACH:
	{
		FreeConsole();
		bool mhUninit = MH_Uninitialize() == MH_OK;
		bool result1 = stage0::uninstall();
		bool result2 = stage1::uninstall();
		return mhUninit && result1 && result2;
	}
	break;
	}
	return TRUE;
}