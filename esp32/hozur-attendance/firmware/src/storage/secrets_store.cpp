#include "secrets_store.h"

#include <Preferences.h>
#include <string.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#define MASTER_KEY "DEV_ONLY_CHANGE_ME"
#endif

static Preferences prefs;

bool secrets_load_master_key(uint8_t* out, size_t outCap, size_t* outLen) {
  if (!out || outCap == 0 || !outLen) {
    return false;
  }

  if (prefs.begin("hozour", true)) {
    const size_t n = prefs.getBytesLength("master");
    if (n > 0 && n <= outCap) {
      prefs.getBytes("master", out, n);
      prefs.end();
      *outLen = n;
      return true;
    }
    prefs.end();
  }

  const char* mk = MASTER_KEY;
  const size_t n = strlen(mk);
  if (n == 0 || n > outCap) {
    return false;
  }
  memcpy(out, mk, n);
  *outLen = n;
  return true;
}

bool secrets_save_master_key(const uint8_t* key, size_t len) {
  if (!key || len == 0) {
    return false;
  }
  if (!prefs.begin("hozour", false)) {
    return false;
  }
  const size_t written = prefs.putBytes("master", key, len);
  prefs.end();
  return written == len;
}
