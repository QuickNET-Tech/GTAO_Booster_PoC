#pragma once
#include <stdint.h>

typedef uint8_t(__fastcall* VirtualFunction)(uint64_t* item);

typedef void(__fastcall* NetCatalogueInsert)(uint64_t catalog, uint64_t* key, uint64_t** item);

typedef size_t(__cdecl* Strlen)(char const* str);
