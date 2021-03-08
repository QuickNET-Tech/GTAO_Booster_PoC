#include "../headers.h"

// not-really-safe strlen
// comes with a built in "cache" for exactly one item
size_t strlenHook(char* str)
{
	static char* start = NULL;
	static char* end = NULL;
	size_t len = 0;
	const size_t cap = 20000;

	// if we have a "cached" string and current pointer is within it
	if(start && str >= start && str <= end)
	{
		// calculate the new strlen
		len = end - str;

		// if we're near the end, unload self
		// we don't want to mess something else up
		if(len < cap / 2)
		{
			printf("Unhooked strlen\n");
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
	if(len > cap)
	{
		start = str;
		end = str + len;
	}

	// slow, boring return
	return len;
}

// normally this checks for duplicates before inserting
// but to speed things up we just skip that and insert directly
char __fastcall netCatalogueInsertUniqueHook(uint64_t catalog, uint64_t* key, uint64_t* item)
{
	// didn't bother reversing the structure
	uint64_t hashArray = catalog + 88;

	// no idea what this does, but repeat what the original did
	vfunc = *(VirtualFunction*)(*item + 48);
	if(!vfunc(item)) {
		return 0;
	}

	// insert directly
	netCatalogueInsert(hashArray, key, &item);

	return 1;
}

void createHooks(void)
{
	MH_CreateHook((LPVOID)strlenPtr, (LPVOID)&strlenHook, (LPVOID*)&originalStrlen);
	MH_CreateHook((LPVOID)netCatalogueInsertUniquePtr, (LPVOID)&netCatalogueInsertUniqueHook, NULL);	
}

void enableHooks(void)
{
	MH_EnableHook((LPVOID)strlenPtr);
	MH_EnableHook((LPVOID)netCatalogueInsertUniquePtr);
}

void initHooks(void)
{
	MH_Initialize();
	printf("MinHook initialized\n");

	createHooks();
	printf("Hooks created\n");

	enableHooks();
	printf("Hooks enabled\n");
}
