#pragma once
#include <stdint.h>

// Logical buttons. The enum value is also the PCF8574 pin (P0..P7), see HARDWARE.md §3.
enum class Button : uint8_t { Up, Down, Left, Right, A, B, Start, Select };
constexpr uint8_t kButtonCount = 8;

enum class ButtonEvent : uint8_t { Press, Repeat, Release };

constexpr uint8_t buttonBit(Button b) { return uint8_t(1u << uint8_t(b)); }
