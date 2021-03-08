#include "headers.h"

//#define ENABLE_DEBUG_PRINTS

static HANDLE uninjectThread = NULL;

static HMODULE gtaoBoosterHmod;

// proper dll self unloading - not sure where I got this from
DWORD WINAPI unloadThread(LPVOID lpThreadParameter) {
	CloseHandle(uninjectThread);
	FreeLibraryAndExitThread(gtaoBoosterHmod, 0);
}

void unload(void) {
	MH_DisableHook((LPVOID)netCatalogueInsertUniquePtr);
	logMsg("Unhooked netcat_insert_dedupe");

	logMsg("Unloading...");

	Sleep(1000);

	removeConsoleAndIoRedirect();

	uninjectThread = CreateThread(NULL, 0, &unloadThread, NULL, 0, NULL);
}

void waitToUnload(void) {
	while(!*isSessionStartedPtr) {
		Sleep(1);
	}
}

void waitForGameWindow(void) {
	while(!FindWindowA("grcWindow", NULL)) {
		Sleep(1000);
	}
}

DWORD WINAPI initialize(LPVOID lpParam) {
	createConsoleAndRedirectIo();
	logMsgColor(consoleBrightWhiteOnBlack,
		"____________________________________________________________\n"
		"                                                            \n"
		"             Welcome to Universal GTAO_Booster!             \n"
		"  Massive thanks to tostercx for the original GTAO_Booster  \n"
		"        Universal GTAO_Booster created by QuickNET          \n"
		"____________________________________________________________\n"
	);
	logMsg("Allocated console");
	
	uint64_t startTime = GetTickCount64();
	
	if(findSigs()) {
		logMsg("Finished finding pointers in %llums", GetTickCount64() - startTime);

		initHooks();

		logMsgColor(consoleBrightGreenOnBlack, "Load online when you're ready\nUniversal GTAO_Booster will unload and this window will disappear automatically, this is normal!");

		waitToUnload();
	}
	else {
		logMsg("One or more errors occurred while finding pointers");
	}
	
	unload();

	return 0;
}

BOOL WINAPI dllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReversed) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		gtaoBoosterHmod = hinstDll;
		CreateThread(NULL, 0, initialize, hinstDll, 0, NULL);
		break;

	case DLL_PROCESS_DETACH:
		MH_Uninitialize();
		break;

	default:
		break;
	}

	return TRUE;
}
