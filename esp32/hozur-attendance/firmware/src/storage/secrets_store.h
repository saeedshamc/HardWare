#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** Load MASTER_KEY: NVS namespace "hozour" key "master" if present, else secrets.h. */
bool secrets_load_master_key(uint8_t* out, size_t outCap, size_t* outLen);

/** Persist master key into NVS (optional provisioning). */
bool secrets_save_master_key(const uint8_t* key, size_t len);
