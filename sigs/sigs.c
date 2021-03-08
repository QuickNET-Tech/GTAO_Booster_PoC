#include "../headers.h"

VirtualFunction unkItemTransactionFunc = NULL;
NetCatalogueInsert netCatalogueInsert = NULL;
Strlen originalStrlen = NULL;

HMODULE gtaHmod = NULL;
IMAGE_DOS_HEADER* gtaDosHeader = NULL;
IMAGE_NT_HEADERS* gtaNtHeader = NULL;
size_t gtaLen = 0;

uint8_t* netCatalogueInsertUniquePtr = NULL;
uint8_t* strlenPtr = NULL;
uint8_t* isSessionStartedPtr = NULL;

BOOL allPatternsFound = TRUE;

static void* gtaStart;
static uint8_t* gtaEnd;

char const* messageboxTitle = "Universal GTAO_Booster";

uint8_t aob[0xFF];
char mask[0xFF];

size_t sigByteCount(char const* sig) {
	size_t count = 0;

	for(size_t i = 0; sig[i]; ++i) {
		if(sig[i] == ' ') {
			++count;
		}
	}

	return ++count;
}

int32_t hexCharToInt(char const c) {
	if(c >= 'a' && c <= 'f') {
		return (int32_t)c - 87;
	}

	if(c >= 'A' && c <= 'F') {
		return (int32_t)c - 55;
	}

	if(c >= '0' && c <= '9') {
		return (int32_t)c - 48;
	}

	return 0;
}

/*
	takes two chars making up half of a byte each and turns them into a single byte
	e.g. makeHexByteIntoChar('E', '8') returns 0xE8
*/
char makeHexByteIntoChar(char first, char second) {
	return (char)(hexCharToInt(first) * 0x10 + hexCharToInt(second) & 0xFF);
}

void generateAob(char const* sig) {
	size_t aobCursor = 0;

	for(size_t sigCursor = 0; sigCursor <= strlen(sig);) {
		if(sig[sigCursor] == '?') {
			aob[aobCursor] = '?';

			++aobCursor;
			sigCursor += 2;
		}
		else if(sig[sigCursor] == ' ') {
			++sigCursor;
		}
		else {
			aob[aobCursor] = makeHexByteIntoChar(sig[sigCursor], sig[sigCursor + 1]);
			++aobCursor;
			sigCursor += 3;
		}
	}
}

void generateMask(char const* sig) {
	size_t maskCursor = 0;

	for(size_t sigCursor = 0; sigCursor < strlen(sig) - 1;) {
		if(sig[sigCursor] == '?') {
			mask[maskCursor] = '?';
			++maskCursor;
			sigCursor += 2;
		}
		else if(sig[sigCursor] == ' ') {
			++sigCursor;
		}
		else {
			mask[maskCursor] = 'x';
			++maskCursor;
			sigCursor += 3;
		}
	}
}

void zeroMemory(void* mem, size_t size) {
	for(size_t i = 0; i < size; ++i) {
		((char*)mem)[i] = 0;
	}
}

void printDebugSigInfo(char const* sig) {
	printf("sig : %s\naob : ", sig);	

	for(size_t i = 0; aob[i]; ++i) {
		if(mask[i] == '?') {
			printf("? ");
		}
		else {
			printf("%02X ", (uint32_t)aob[i] & 0xFF);
		}
	}

	printf("\nmask : ");

	for(size_t i = 0; i < sigByteCount(sig); ++i) {
		if(mask[i] == '?') {
			printf("?");
		}
		else {
			printf("x");
		}
	}

	printf("\n");
}

void zeroAobAndMaskBuffers(void) {
	zeroMemory(aob, 0xFF);
	zeroMemory(mask, 0xFF);
}

void fillAobAndMaskBuffers(char const* sig) {
	zeroAobAndMaskBuffers();

	generateAob(sig);
	generateMask(sig);
}

void notifyOnScanFailure(char const* name) {
	logMsgColor(consoleBrightRedOnBlack, "Pattern '%s' failed.", name);
	
	char buf[0xFF];
	int result = sprintf_s(buf, sizeof(buf), "Pattern '%s' failed.", name);

	if(result >= 0 && result <= (int32_t)sizeof(buf)) {
		MessageBoxA(NULL, buf, messageboxTitle, 0);
	}
	else {
		MessageBoxA(NULL, "Unknown pattern failed.\nPattern unknown because 'sprintf_s' also failed.", messageboxTitle, 0);
	}
}

BOOL doesSigMatch(uint8_t const* scanCursor) {
	for(size_t cursor = 0; cursor < strlen(mask); ++cursor) {
		if(mask[cursor] != '?' && aob[cursor] != scanCursor[cursor]) {
			return FALSE;
		}
	}
	
	return TRUE;
}

uint8_t* scan(char const* name, char const* sig, int64_t offset) {
	fillAobAndMaskBuffers(sig);

#ifdef ENABLE_DEBUG_PRINTS
	printDebugSigInfo(sig);
#endif
	
	uint8_t* scanEnd = gtaEnd - sigByteCount(sig);
	for(uint8_t* scanCursor = gtaStart; scanCursor < scanEnd; ++scanCursor) {
		if(doesSigMatch(scanCursor)) {
			logMsgColor(consoleGrayOnBlack, "Found %s", name);
			return scanCursor + offset;
		}
	}

	notifyOnScanFailure(name);

	allPatternsFound = FALSE;

	return NULL;
}

uint8_t* rip(uint8_t* address) {
	return address
		? address + *(int32_t*)address + 4  // NOLINT(clang-diagnostic-cast-align) // intended behavior
		: NULL;
}

void initGlobalVars(void) {
	gtaHmod = GetModuleHandleA(NULL);
	gtaDosHeader = (IMAGE_DOS_HEADER*)gtaHmod;
	gtaNtHeader = (IMAGE_NT_HEADERS*)((char*)gtaHmod + gtaDosHeader->e_lfanew);  // NOLINT(clang-diagnostic-cast-align)
	gtaStart = (void*)gtaHmod;
	gtaLen = gtaNtHeader->OptionalHeader.SizeOfImage;
	gtaEnd = (uint8_t*)gtaStart + gtaLen;

	logMsg("Variables initialized");
}

BOOL findSigs(void) {
	initGlobalVars();
	
	netCatalogueInsertUniquePtr = scan("netCatalogueInsertUnique", "4C 89 44 24 18 57 48 83 EC ? 48 8B FA", -0x5);

	strlenPtr = rip(scan("strlen", "48 3B C1 4C 8B C6", -0x11));

	netCatalogueInsert = (NetCatalogueInsert)rip(scan("netCatalogueInsert", "3B D1 B0 01 0F 4E D1", -0x11));  // NOLINT(clang-diagnostic-cast-align)

	isSessionStartedPtr = rip(scan("isSessionStarted", "40 38 35 ? ? ? ? 74 ? 48 8B CF E8", 0x3));

#ifdef ENABLE_DEBUG_PRINTS
	printf("GTA5.exe == 0x%llX\n", (uint64_t)gtaStart);
	printf("netcat_insert_dedupe == 0x%llX | GTA5.exe + 0x%llX\n", (uint64_t)netCatalogueInsertUniquePtr, (uint64_t)netCatalogueInsertUniquePtr - (uint64_t)gtaStart);
	printf("strlen == 0x%llX | GTA5.exe + 0x%llX\n", (uint64_t)strlenPtr, (uint64_t)strlenPtr - (uint64_t)gtaStart);
	printf("netCatalogueInsert == 0x%llX | GTA5.exe + 0x%llX\n", (uint64_t)netCatalogueInsert, (uint64_t)netCatalogueInsert - (uint64_t)gtaStart);
#endif

	return allPatternsFound;
}
