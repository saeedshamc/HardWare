#include "storage/sd_helper.h"
#include <SD.h>
#include "config.h"

static bool _sd_ok = false;

bool sd_init(void) {
    if (_sd_ok) return true;
    _sd_ok = SD.begin(SD_CS_PIN);
    return _sd_ok;
}

bool sd_is_available(void) { return _sd_ok; }
