#pragma once
#include "core/key_store.h"
// Save full card dump to SD card in Proxmark3-compatible .mfd binary format
// Returns: path of saved file, or nullptr on failure
const char* dump_save(const CardDump* dump);
// Load a previously saved dump (for re-analysis without re-reading card)
bool dump_load(const char* path, CardDump* dump);
// Print dump in human-readable hex table to Serial
void dump_print(const CardDump* dump);
