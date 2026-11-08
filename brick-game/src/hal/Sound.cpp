#include "Sound.h"
#include <Arduino.h>

void Sound::begin() {
    pinMode(kAmpEnablePin, OUTPUT);
    digitalWrite(kAmpEnablePin, HIGH);   // amp off: no hiss until the codec is driven
}

void Sound::play(Sfx s) {
#ifdef SOUND_LOG
    Serial.printf("[sfx] %u\n", unsigned(s));
#else
    (void)s;
#endif
}
