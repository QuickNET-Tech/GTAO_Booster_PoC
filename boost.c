
// ReSharper disable CppInconsistentNaming
#include <stdint.h>
#include <stdio.h>

#include "minhook/include/MinHook.h"
#include "Psapi.h"

//#define ENABLE_DEBUG_PRINTS

LPCWSTR messagebox_title = L"Universal GTAO_Booster";

typedef uint8_t(__fastcall* virtual_function)(uint64_t*);
virtual_function vfunc = NULL;

typedef void(__fastcall* netcat_insert_direct_t)(uint64_t catalog, uint64_t* key, uint64_t** item);
netcat_insert_direct_t netcat_insert_direct = NULL;

typedef size_t(__cdecl* strlen_t)(const char *str);
strlen_t builtin_strlen = NULL;

HANDLE uninject_thread = NULL;
HANDLE wait_for_uninject_thread = NULL;

uint8_t* netcat_insert_dedupe_addr = NULL;
uint8_t* strlen_addr = NULL;
uint8_t* is_session_started = NULL;
uint32_t* game_state = NULL;

uint8_t aob[0xFF];
char mask[0xFF];

static HMODULE gtao_booster_hmod;
static HMODULE gta_hmod;
static IMAGE_DOS_HEADER* gta_dos_header;
static IMAGE_NT_HEADERS* gta_nt_header;
static void* gta_start;
static size_t gta_len;
static uint8_t* gta_end;

BOOL should_exit_due_to_failure = FALSE;

void create_console_and_redirect_io(void)
{
	AllocConsole();

	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	SetConsoleTitle(L"Grand Theft Auto V : Universal GTAO_Booster v1.0.1 by QuickNET ");

	FILE* file = NULL;

	freopen_s(&file, "CONIN$", "r", stdin);
	freopen_s(&file, "CONOUT$", "w", stdout);
	freopen_s(&file, "CONOUT$", "w", stderr);
}

void remove_console_and_io_redirect(void) {
	HWND console_window = GetConsoleWindow();
	FILE* file = NULL;

	freopen_s(&file, "NUL:", "r", stdin);
	freopen_s(&file, "NUL:", "w", stdout);
	freopen_s(&file, "NUL:", "w", stderr);

	FreeConsole();
	DestroyWindow(console_window);
}

// proper dll self unloading - not sure where I got this from
DWORD WINAPI unload_thread(LPVOID lpThreadParameter)
{
	CloseHandle(uninject_thread);
	FreeLibraryAndExitThread(gtao_booster_hmod, 0);
}

void unload(void)
{
	printf("Unhooked netcat_insert_dedupe\n");
	MH_DisableHook((LPVOID)netcat_insert_dedupe_addr);

	printf("Unloading...\n");

	Sleep(1000);

	remove_console_and_io_redirect();

	uninject_thread = CreateThread(NULL, 0, &unload_thread, NULL, 0, NULL);
}

// not-really-safe strlen
// comes with a built in "cache" for exactly one item
size_t strlen_cacher(char* str)
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
			MH_DisableHook((LPVOID)strlen_addr);
		}

		// super-fast return!
		return len;
	}

	// count the actual length
	// we need at least one measurement of the large JSON
	// or normal strlen for other strings
	len = builtin_strlen(str);

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
char __fastcall netcat_insert_dedupe_hooked(uint64_t catalog, uint64_t* key, uint64_t* item)
{
	// didn't bother reversing the structure
	uint64_t not_a_hashmap = catalog + 88;

	// no idea what this does, but repeat what the original did
	vfunc = *(virtual_function*)(*item + 48);
	if(!vfunc(item)) {
	    return 0;
	}

	// insert directly
	netcat_insert_direct(not_a_hashmap, key, &item);

	return 1;
}

size_t sig_byte_count(char const* sig)
{
	size_t count = 0;

	for(size_t i = 0; sig[i]; ++i)
	{
		if(sig[i] == ' ')
		{
			++count;
		}
	}

	return ++count;
}

int32_t hex_char_to_int(char const c)
{
	if(c >= 'a' && c <= 'f')
	{
		return (int32_t)c - 87;
	}

	if(c >= 'A' && c <= 'F')
	{
		return (int32_t)c - 55;
	}

	if(c >= '0' && c <= '9')
	{
		return (int32_t)c - 48;
	}

	return 0;
}

/*	takes two chars making up half of a byte each and turns them into a single byte
e.g. make_hex_byte_into_char('E', '8') returns 0xE8	*/
char make_hex_byte_into_char(char first, char second)
{
	return (char)(hex_char_to_int(first) * 0x10 + hex_char_to_int(second) & 0xFF);
}

void generate_aob(char const* sig)
{
	size_t aob_cursor = 0;

	for(size_t sig_cursor = 0; sig_cursor <= strlen(sig);)
	{
		if(sig[sig_cursor] == '?')
		{
			aob[aob_cursor] = '?';

			++aob_cursor;
			sig_cursor += 2;
		}
		else if(sig[sig_cursor] == ' ')
		{
			++sig_cursor;
		}
		else
		{
			aob[aob_cursor] = make_hex_byte_into_char(sig[sig_cursor], sig[sig_cursor + 1]);
			++aob_cursor;
			sig_cursor += 3;
		}
	}
}

void generate_mask(char const* sig)
{
	size_t mask_cursor = 0;

	for(size_t sig_cursor = 0; sig_cursor < strlen(sig) - 1;)
	{
		if(sig[sig_cursor] == '?')
		{
			mask[mask_cursor] = '?';
			++mask_cursor;
			sig_cursor += 2;
		}
		else if(sig[sig_cursor] == ' ')
		{
			++sig_cursor;
		}
		else
		{
			mask[mask_cursor] = 'x';
			++mask_cursor;
			sig_cursor += 3;
		}
	}
}

void zero_memory(void* mem, size_t size)
{
	for(size_t i = 0; i < size; ++i)
	{
		((char*)mem)[i] = 0;
	}
}

void print_debug_sig_info(char const* sig)
{
	printf("sig : %s\naob : ", sig);	

	for(size_t i = 0; aob[i]; ++i)
	{
		if(mask[i] == '?')
		{
			printf("? ");
		}
		else
		{
			printf("%02X ", (uint32_t)aob[i] & 0xFF);
		}
	}

	printf("\nmask : ");

	for(size_t i = 0; i < sig_byte_count(sig); ++i)
	{
		if(mask[i] == '?')
		{
			printf("?");
		}
		else
		{
			printf("x");
		}
	}

	printf("\n");
}

void zero_aob_and_mask_buffers(void)
{
	zero_memory(aob, 0xFF);
	zero_memory(mask, 0xFF);
}

void fill_aob_and_mask_buffers(char const* sig)
{
	zero_aob_and_mask_buffers();

	generate_aob(sig);
	generate_mask(sig);
}

void notify_on_scan_failure(char const* name)
{
	wchar_t* wname = malloc(sizeof(wchar_t) * strlen(name) + 1);
	mbstowcs_s(NULL, wname, strlen(name) + 1, name, strlen(name));

	wchar_t buf[0x7F];
	int32_t result = swprintf_s(buf, _countof(buf), L"Pattern '%s' failed!", wname);

	if(result >= 0 && result <= (int32_t)_countof(buf))
	{
		MessageBox(NULL, buf, messagebox_title, 0);
	}
	else
	{
		MessageBox(NULL, L"Unknown pattern failed.\nPattern unknown because 'swprintf_s' also failed.", messagebox_title, 0);
	}

	free(wname);
}

BOOL does_sig_match(uint8_t const* scan_cursor)
{
	for(size_t cursor = 0; cursor < strlen(mask); ++cursor)
	{
		if(mask[cursor] != '?' && aob[cursor] != scan_cursor[cursor])
		{
			return FALSE;
		}
	}

	return TRUE;
}

uint8_t* scan(char const* name, char const* sig, int64_t offset)
{
	fill_aob_and_mask_buffers(sig);

#ifdef ENABLE_DEBUG_PRINTS
	print_debug_sig_info(sig);
#endif
	
	uint8_t* scan_end = gta_end - sig_byte_count(sig);
	for(uint8_t* scan_cursor = gta_start; scan_cursor < scan_end; ++scan_cursor)
	{
		if(does_sig_match(scan_cursor))
		{
			printf("Found %s\n", name);
			return scan_cursor + offset;
		}
	}

	notify_on_scan_failure(name);

	should_exit_due_to_failure = TRUE;
	
	return NULL;
}

uint8_t* rip(uint8_t* address)
{
	return address
		? address + *(int32_t*)address + 4  // NOLINT(clang-diagnostic-cast-align)
		: NULL;
}

void find_pointers(void)
{
	netcat_insert_dedupe_addr = scan("netcat_insert_dedupe", "4C 89 44 24 18 57 48 83 EC ? 48 8B FA", -0x5);

	strlen_addr = rip(scan("strlen", "48 3B C1 4C 8B C6", -0x11));

	netcat_insert_direct = (netcat_insert_direct_t)rip(scan("netcat_insert_direct", "3B D1 B0 01 0F 4E D1", -0x11));  // NOLINT(clang-diagnostic-cast-align)

	is_session_started = rip(scan("is_session_started", "40 38 35 ? ? ? ? 74 ? 48 8B CF E8", 0x3));

#ifdef ENABLE_DEBUG_PRINTS
	printf("GTA5.exe == 0x%llX\n", (uint64_t)gta_start);
	printf("netcat_insert_dedupe == 0x%llX | GTA5.exe + 0x%llX\n", (uint64_t)netcat_insert_dedupe_addr, (uint64_t)netcat_insert_dedupe_addr - (uint64_t)gta_start);
	printf("strlen == 0x%llX | GTA5.exe + 0x%llX\n", (uint64_t)strlen_addr, (uint64_t)strlen_addr - (uint64_t)gta_start);
	printf("netcat_insert_direct == 0x%llX | GTA5.exe + 0x%llX\n", (uint64_t)netcat_insert_direct, (uint64_t)netcat_insert_direct - (uint64_t)gta_start);
#endif
}

void init_global_vars(void) {
	gta_hmod = GetModuleHandleA(NULL);
	gta_dos_header = (IMAGE_DOS_HEADER*)gta_hmod;
	gta_nt_header = (IMAGE_NT_HEADERS*)((char*)gta_hmod + gta_dos_header->e_lfanew);  // NOLINT(clang-diagnostic-cast-align)
	gta_start = (void*)gta_hmod;
	gta_len = gta_nt_header->OptionalHeader.SizeOfImage;
	gta_end = (uint8_t*)gta_start + gta_len;
}

DWORD WINAPI initialize(LPVOID lpParam)
{
	while(!FindWindowW(L"grcWindow", NULL))
	{
		Sleep(1000);
	}

	create_console_and_redirect_io();
	printf(
		"____________________________________________________________\n"
		"                                                            \n"
		"             Welcome to Universal GTAO_Booster!             \n"
		"  Massive thanks to tostercx for the original GTAO_Booster  \n"
		"        Universal GTAO_Booster created by QuickNET          \n"
		"____________________________________________________________\n"
		"                                                            \n"
	);

	printf("Allocated console\n");

	init_global_vars();
	printf("Variables initialized\n");

	uint64_t start_time = GetTickCount64();

	find_pointers();

	if(should_exit_due_to_failure)
	{
		printf("One or more errors occurred while finding pointers\n");

		unload();

		return 0;
	}
	
	printf("Finished finding pointers in %llums\n", GetTickCount64() - start_time);
		
	MH_Initialize();
	printf("MinHook initialized\n");

	MH_CreateHook((LPVOID)strlen_addr, (LPVOID)&strlen_cacher, (LPVOID*)&builtin_strlen);
	MH_CreateHook((LPVOID)netcat_insert_dedupe_addr, (LPVOID)&netcat_insert_dedupe_hooked, NULL);
	printf("Hooks created\n");

	MH_EnableHook((LPVOID)strlen_addr);
	MH_EnableHook((LPVOID)netcat_insert_dedupe_addr);
	printf("Hooks enabled\n");

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0A);
	printf("Load online when you're ready\nUniversal GTAO_Booster will unload and this window will disappear automatically, this is normal!\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07); // might be 0x0F

	while(!*is_session_started)
	{
		Sleep(1);
	}

	unload();

	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReversed)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		gtao_booster_hmod = hinstDLL;
		CreateThread(NULL, 0, initialize, hinstDLL, 0, NULL);
		break;

	case DLL_PROCESS_DETACH:
		MH_Uninitialize();
		break;

	default:
		break;
	}

	return TRUE;
}
