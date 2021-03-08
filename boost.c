#include "headers.h"

//#define ENABLE_DEBUG_PRINTS

static HANDLE uninjectThread = NULL;

static HMODULE gtaoBoosterHmod;

void createConsoleAndRedirectIo(void)
{
	AllocConsole();

	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	SetConsoleTitle(L"Grand Theft Auto V : Universal GTAO_Booster v1.0.1 by QuickNET ");

	FILE* file = NULL;

	freopen_s(&file, "CONIN$", "r", stdin);
	freopen_s(&file, "CONOUT$", "w", stdout);
	freopen_s(&file, "CONOUT$", "w", stderr);
}

void removeConsoleAndIoRedirect(void) {
	HWND consoleWindow = GetConsoleWindow();
	FILE* file = NULL;

	freopen_s(&file, "NUL:", "r", stdin);
	freopen_s(&file, "NUL:", "w", stdout);
	freopen_s(&file, "NUL:", "w", stderr);

	FreeConsole();
	DestroyWindow(consoleWindow); // shouldn't be necessary at all, but can't hurt to be a little forceful
}

// proper dll self unloading - not sure where I got this from
DWORD WINAPI unloadThread(LPVOID lpThreadParameter)
{
	CloseHandle(uninjectThread);
	FreeLibraryAndExitThread(gtaoBoosterHmod, 0);
}

void unload(void)
{
	MH_DisableHook((LPVOID)netCatalogueInsertUniquePtr);
	printf("Unhooked netcat_insert_dedupe\n");

	printf("Unloading...\n");

	Sleep(1000);

	removeConsoleAndIoRedirect();

	uninjectThread = CreateThread(NULL, 0, &unloadThread, NULL, 0, NULL);
}

void printUserInstruction(void)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0A);
	printf("Load online when you're ready\nUniversal GTAO_Booster will unload and this window will disappear automatically, this is normal!\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
}

void waitToUnload(void)
{
	while(!*isSessionStartedPtr)
	{
		Sleep(1);
	}
}

void waitForGameWindow(void)
{
	while(!FindWindowA("grcWindow", NULL))
	{
		Sleep(1000);
	}
}

DWORD WINAPI initialize(LPVOID lpParam)
{
	createConsoleAndRedirectIo();
	printf(
		"____________________________________________________________\n"
		"                                                            \n"
		"             Welcome to Universal GTAO_Booster!             \n"
		"  Massive thanks to tostercx for the original GTAO_Booster  \n"
		"        Universal GTAO_Booster created by QuickNET          \n"
		"____________________________________________________________\n"
		"                                                            \n"
		"Allocated console\n"
	);
	
	uint64_t startTime = GetTickCount64();
	
	if(findSigs())
	{
		printf("Finished finding pointers in %llums\n", GetTickCount64() - startTime);

		initHooks();

		printUserInstruction();

		waitToUnload();
	}
	else {
		printf("One or more errors occurred while finding pointers\n");
	}
	
	unload();

	return 0;
}

BOOL WINAPI dllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReversed)
{
	switch (fdwReason)
	{
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
