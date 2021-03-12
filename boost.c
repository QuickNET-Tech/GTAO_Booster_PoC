#include "headers.h"
#include <initguid.h>
#include <KnownFolders.h>
#include <ShlObj.h>
//#define ENABLE_DEBUG_PRINTS

static HANDLE uninjectThread = NULL;

static HMODULE gtaoBoosterHmod;

static char const* configFile = "Universal GTAO_Booster.ini";

BOOL shouldApplyLegalAndLogoPatches = FALSE;

void getDocumentsPath(wchar_t* path) {
	SHGetKnownFolderPath(&FOLDERID_Documents, 0, NULL, &path);
}

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
	if(FindWindowA("grcWindow", NULL)) {
		return;
	}

	shouldApplyLegalAndLogoPatches = TRUE;
	
	while(!FindWindowA("grcWindow", NULL)) {
		Sleep(50);
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
		/*
			27 is just big enough to hold
			
			[settings]
			enableConsole=%d

			where `%d` is a single digit number
		*/
		char iniText[27];
		
		int32_t enableConsoleValue = 1;

		int32_t result = MessageBoxA(NULL, "A config file was either not found or was malformed and was unable to be read.\n\nDo you want the console to be enabled?", messageboxTitle, MB_TOPMOST | MB_ICONQUESTION | MB_YESNO);
		// any response other than clicking "No" will be interpreted as yes.
		if(result == IDNO) {
			enableConsoleValue = 0;
		}

		// format ini text before writing
		sprintf_s(iniText, sizeof(iniText), "[settings]\nenableConsole=%d", enableConsoleValue);

		// write formatted ini text
		fputs(iniText, file);

		fclose(file);

		consoleEnabled = enableConsoleValue;
	}
	else {
		logMsgColor(consoleBrightRedOnBlack, "Unable to create %s, console will remain enabled", configFile);
	}
}

void readConfig(void) {
	ini_t* config = ini_load(configFile);
	
	if(config) {
		if(!ini_sget(config, "settings", "enableConsole", "%d", &consoleEnabled)) {
			// there was an error attempting to read the file, recreate the file
			createConfig();
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
	// if injected too early patterns will fail so we must wait to prevent failures
	waitForGameWindow();
	
	handleConfig();

	// create console, if not enabled, does nothing
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

		applyLegalAndLogoPatches();
		
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
