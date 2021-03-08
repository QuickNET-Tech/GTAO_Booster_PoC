#pragma once

extern VirtualFunction vfunc;
extern NetCatalogueInsert netCatalogueInsert;
extern Strlen originalStrlen;

extern uint8_t* netCatalogueInsertUniquePtr;
extern uint8_t* strlenPtr;
extern uint8_t* isSessionStartedPtr;

extern HMODULE gtaHmod;
extern IMAGE_DOS_HEADER* gtaDosHeader;
extern IMAGE_NT_HEADERS* gtaNtHeader;
extern size_t gtaLen;

// returns FALSE on a sig fail, TRUE otherwise 
BOOL findSigs(void);
