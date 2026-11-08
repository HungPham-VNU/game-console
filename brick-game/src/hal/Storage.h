#pragma once
#include <stdint.h>

// High scores in NVS (Preferences, namespace "brick").
class Storage {
public:
    uint32_t loadHiScore(const char* key);
    void saveHiScore(const char* key, uint32_t value);
};
