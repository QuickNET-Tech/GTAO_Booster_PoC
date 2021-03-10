#pragma once

extern VirtualFunction unkItemTransactionFunc;
extern NetCatalogueInsert netCatalogueInsertPtr;
extern Strlen originalStrlen;

extern uint8_t* netCatalogueInsertUniquePtr;
extern uint8_t* strlenPtr;
extern uint8_t* isSessionStartedPtr;

extern HMODULE gtaHmod;
extern IMAGE_DOS_HEADER* gtaDosHeader;
extern IMAGE_NT_HEADERS* gtaNtHeader;
extern size_t gtaLen;

extern char const* messageboxTitle;

extern BOOL shouldApplyLegalAndLogoPatches;

void applyLegalAndLogoPatches(void);

// returns FALSE on a sig fail, TRUE otherwise 
BOOL findSigs(void);
