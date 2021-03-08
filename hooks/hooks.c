#include "../headers.h"

// not-really-safe strlen
// comes with a built in "cache" for exactly one item
size_t strlenHook(char* str) {
	static char* start = NULL;
	static char* end = NULL;
	size_t len = 0;
	const size_t cap = 20000;

	// if we have a "cached" string and current pointer is within it
	if(start && str >= start && str <= end) {
		// calculate the new strlen
		len = end - str;

		// if we're near the end, unload self
		// we don't want to mess something else up
		if(len < cap / 2) {
			logMsg("Unhooked strlen");
			MH_DisableHook((LPVOID)strlenPtr);
		}

		// super-fast return!
		return len;
	}

	// count the actual length
	// we need at least one measurement of the large JSON
	// or normal strlen for other strings
	len = originalStrlen(str);

	// if it was the really long string
	// save it's start and end addresses
	if(len > cap) {
		start = str;
		end = str + len;
	}

	// slow, boring return
	return len;
}

// hook to skip duplicate check and insert directly
char __fastcall netCatalogueInsertUniqueHook(uint64_t catalog, uint64_t* key, uint64_t* item) {
	// didn't bother reversing the structure
	uint64_t hashArray = catalog + 88;

	// some virtual method that's part of the netCatalogInventoryItem vtable
	// the function appears to be related in some way to transactions?
	unkItemTransactionFunc = *(VirtualFunction*)(*item + 48);
	if(!unkItemTransactionFunc(item)) {
		return 0;
	}

	// insert directly
	netCatalogueInsert(hashArray, key, &item);

	return 1;
}

void createHooks(void) {
	MH_CreateHook((LPVOID)strlenPtr, (LPVOID)&strlenHook, (LPVOID*)&originalStrlen);
	MH_CreateHook((LPVOID)netCatalogueInsertUniquePtr, (LPVOID)&netCatalogueInsertUniqueHook, NULL);	
}

void enableHooks(void) {
	MH_EnableHook((LPVOID)strlenPtr);
	MH_EnableHook((LPVOID)netCatalogueInsertUniquePtr);
}

void initHooks(void) {
	MH_Initialize();
	logMsg("MinHook initialized");

	createHooks();
	logMsg("Hooks created");

	enableHooks();
	logMsg("Hooks enabled");
}
