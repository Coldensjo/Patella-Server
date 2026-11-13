#include <cstdint>
#include <cstdlib>

#include "objects.hh"
#include "script.hh"
#include "threads.hh"
#include "map.hh"
#include "dbfuncs.hh"
#include "config.hh"
#include "utils.hh"

// 0x0806c040  dbfuncs.c:31
void TOutfit::load(TReadScriptFile & /*Script*/) { abort(); /* STUB */ }

// 0x0806c0d0  dbfuncs.c:44
void TOutfit::save(TWriteScriptFile & /*Script*/) { abort(); /* STUB */ }

// 0x0806c180  dbfuncs.c:59
void PlayerDataPath(uint32_t /*CharacterID*/, char * /*Extension*/, char * /*Path*/) { abort(); /* STUB */ }

// 0x0806c1d0  dbfuncs.c:67
bool PlayerDataExists(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x0806c240  dbfuncs.c:78
bool LoadPlayerData(TPlayerData * /*Slot*/) { abort(); /* STUB */ }

// 0x0806d210  dbfuncs.c:290
void SavePlayerData(TPlayerData * /*Slot*/) { abort(); /* STUB */ }

// 0x0806de50  dbfuncs.c:426
void UnlinkPlayerData(uint32_t /*CharacterID*/) { abort(); /* STUB */ }
