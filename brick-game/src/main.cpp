// Pocket Brick Console – Brick Game firmware for the ES3C28P / ES3N28P (ESP32-S3 N16R8).
#include <Arduino.h>
#include <Wire.h>
#include <esp_random.h>

#include "core/App.h"
#include "core/Platform.h"
#include "games/Tetris.h"
#include "hal/Display.h"
#include "hal/Input.h"
#include "hal/Sound.h"
#include "hal/Storage.h"
#include "render/LcdRenderer.h"

#ifndef BOOT_COLOR_TEST
#define BOOT_COLOR_TEST 1   // red/green/blue/white/black at boot, to check invert/rgb_order
#endif

namespace {

constexpr int kI2cSda = 16;
constexpr int kI2cScl = 15;
constexpr uint32_t kI2cHz = 400000;

class EspPlatform : public Platform {
public:
    EspPlatform(Storage& storage, Sound& sound) : storage_(storage), sound_(sound) {}
    uint32_t loadHiScore(const char* key) override { return storage_.loadHiScore(key); }
    void saveHiScore(const char* key, uint32_t v) override {
        storage_.saveHiScore(key, v);
        Serial.printf("[app] new high score %s = %lu\n", key, (unsigned long)v);
    }
    void play(Sfx s) override { sound_.play(s); }
    uint32_t randomSeed() override { return esp_random(); }

private:
    Storage& storage_;
    Sound& sound_;
};

Display display;
LcdRenderer renderer(display);
Input input;
Sound sound;
Storage storage;
EspPlatform platform(storage, sound);

Tetris tetris;
Game* const kGames[] = {&tetris};
App app(platform, kGames, sizeof(kGames) / sizeof(kGames[0]));
Frame frame;

const char* i2cName(uint8_t addr) {
    switch (addr) {
    case 0x18: return "ES8311 codec";
    case 0x20: return "PCF8574 buttons";
    case 0x38: return "FT6336G touch (or a PCF8574A - don't use one!)";
    default: return "?";
    }
}

void printDiagnostics() {
    Serial.println();
    Serial.println("=== Pocket Brick Console: bring-up ===");
    Serial.printf("Chip      : %s rev %u, %u cores, %lu MHz\n", ESP.getChipModel(), ESP.getChipRevision(),
                  ESP.getChipCores(), (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("Flash     : %lu MB\n", (unsigned long)(ESP.getFlashChipSize() / (1024 * 1024)));
    Serial.printf("PSRAM     : %lu KB total, %lu KB free %s\n", (unsigned long)(ESP.getPsramSize() / 1024),
                  (unsigned long)(ESP.getFreePsram() / 1024), ESP.getPsramSize() ? "(OK)" : "(MISSING!)");
    Serial.printf("Heap      : %lu KB free\n", (unsigned long)(ESP.getFreeHeap() / 1024));

    Serial.println("I2C scan (SDA 16, SCL 15):");
    int found = 0;
    for (uint8_t a = 1; a < 0x7F; ++a) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  0x%02X  %s\n", a, i2cName(a));
            ++found;
        }
    }
    if (!found) Serial.println("  (nothing found)");
    Serial.printf("Buttons   : %s\n", input.hasExpander() ? "PCF8574 @0x20" : "PCF8574 not found -> BOOT button = A");
    Serial.println("======================================");
}

void colorTest() {
    struct Step { uint8_t r, g, b; const char* name; };
    const Step steps[] = {{255, 0, 0, "RED"}, {0, 255, 0, "GREEN"}, {0, 0, 255, "BLUE"},
                          {255, 255, 255, "WHITE"}, {0, 0, 0, "BLACK"}};
    for (const auto& s : steps) {
        const uint16_t c = display.color565(s.r, s.g, s.b);
        display.fillScreen(c);
        display.setFont(&fonts::Font4);
        display.setTextDatum(textdatum_t::middle_center);
        display.setTextColor(s.r + s.g + s.b > 384 ? TFT_BLACK : TFT_WHITE, c);
        display.drawString(s.name, display.width() / 2, display.height() / 2);
        display.drawString("TOP", display.width() / 2, 20);
        Serial.printf("[lcd] %s\n", s.name);
        delay(500);
    }
}

} // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 1500) delay(10);   // give USB-CDC a moment, never block

    sound.begin();
    Wire.begin(kI2cSda, kI2cScl, kI2cHz);
    input.begin();

    display.init();
    display.setRotation(LCD_ROTATION);
    display.setBrightness(200);

    printDiagnostics();
#if BOOT_COLOR_TEST
    colorTest();
#endif

    renderer.begin();
    app.begin(millis());
}

void loop() {
    const uint32_t now = millis();

    Input::Event ev[16];
    const uint8_t n = input.poll(now, ev, 16);
    for (uint8_t i = 0; i < n; ++i) app.onButton(ev[i].button, ev[i].type, now);

    app.update(now);
    app.draw(frame);
    renderer.present(frame);

    delay(1);
}
