#include "headers.h"

//#define ENABLE_DEBUG_PRINTS

static HANDLE uninjectThread = NULL;

static HMODULE gtaoBoosterHmod;

static char const* configFile = "Universal GTAO_Booster.ini";

// proper dll self unloading - not sure where I got this from
DWORD WINAPI unloadThread(LPVOID lpThreadParameter) {
	CloseHandle(uninjectThread);
	FreeLibraryAndExitThread(gtaoBoosterHmod, 0);
}

void unload(void) {
	MH_DisableHook((LPVOID)netCatalogueInsertUniquePtr);
	logMsg("Unhooked netCatalogueInsertUnique");

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

BOOL doesFileExist(char const* file) {
	uint32_t attr = GetFileAttributesA(file);
	return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

void createConfig(void) {
	FILE* file;
	fopen_s(&file, configFile, "w");
	if(file) {
		fputs("[settings]\nenableConsole=1", file);
		fclose(file);
		
		consoleEnabled = TRUE;
	}
	else {
		logMsgColor(consoleBrightRedOnBlack, "Unable to create %s", configFile);
	}
}

void readConfig(void) {
	ini_t* config = ini_load(configFile);
	
	if(config) {
		if(!ini_sget(config, "settings", "enableConsole", "%d", &consoleEnabled)) {
			MessageBoxA(NULL, "An error occurred while trying to read from Universal GTAO_Booster.ini", messageboxTitle, 0);
		}
	}
	else {
		MessageBoxA(NULL, "Failed to load ini file", messageboxTitle, 0);
	}

	ini_free(config);
}

void handleConfig(void) {
	if(doesFileExist(configFile)) {
		readConfig();
	}
	else {
		createConfig();
	}
}

DWORD WINAPI initialize(LPVOID lpParam) {
	waitForGameWindow();
	
	handleConfig();

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
