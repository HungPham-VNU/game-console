#include "Storage.h"
#include <Preferences.h>

static constexpr const char* kNamespace = "brick";

uint32_t Storage::loadHiScore(const char* key) {
    Preferences p;
    // Opened read-write: a read-only open logs an error until the namespace exists.
    if (!p.begin(kNamespace, false)) return 0;
    const uint32_t v = p.getUInt(key, 0);
    p.end();
    return v;
}

void Storage::saveHiScore(const char* key, uint32_t value) {
    Preferences p;
    if (!p.begin(kNamespace, false)) return;
    p.putUInt(key, value);
    p.end();
}
