#pragma once

// Physical wiring of the Ideaspark ESP32 1.9-inch ST7789 display.
// IMPORTANT: 5 V addressable LEDs only, NOT analog RGB LEDs.
constexpr int PIN_PIXEL_DATA = 27;
constexpr int PIXEL_COUNT = 8;    // Default; can be changed from phone setup page.
constexpr int MAX_LED_COUNT = 32; // Limit draw on the shared 5V/3A supply.
constexpr int PIN_BOOT_BUTTON = 0; // Built-in BOOT (hold only after device has booted).
constexpr int PIN_TFT_BACKLIGHT = 32;
constexpr int PIN_TFT_CS = 15;
constexpr int PIN_TFT_DC = 2;
constexpr int PIN_TFT_RST = 4;
constexpr int PIN_TFT_SCLK = 18;
constexpr int PIN_TFT_MOSI = 23;

constexpr uint32_t SCREEN_SLEEP_MS = 10000;
constexpr uint32_t SELECTION_DELAY_MS = 500;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;
constexpr uint32_t FRAME_MS = 35;
constexpr uint32_t SAVE_DELAY_MS = 3500;
constexpr uint8_t MAX_BRIGHTNESS = 190; // Cap output while battery-operated (0..255).
constexpr char SETUP_AP_PASSWORD[] = "PumpkinFX2026"; // Change if desired (>= 8 chars).

// Password-protected ArduinoOTA enabled after Wi-Fi joins. Change before
// flashing if your LAN is not trusted. This is for future code updates,
// not required to change Wi-Fi or Sinric credentials.
constexpr char OTA_PASSWORD[] = "PumpkinOTA2026"; // Change using phone setup portal.
// Configure Sinric Pro Mode capability with this exact InstanceId.
constexpr char CLOUD_MODE_INSTANCE[] = "pumpkin_effect";

// Optional: embed credentials at compile time instead of using the phone portal.
// Leave empty to provision over the 'Pumpkin-FX-Setup' Wi-Fi network on first boot.
// Do not publish this header if you put your real passwords here.
constexpr char PRESET_WIFI_SSID[] = "";
constexpr char PRESET_WIFI_PASSWORD[] = "";
// Optional Sinric Pro credentials if already known, else enter on setup page.
constexpr char PRESET_SINRIC_KEY[] = "";
constexpr char PRESET_SINRIC_SECRET[] = "";
constexpr char PRESET_SINRIC_DEVICE[] = "";