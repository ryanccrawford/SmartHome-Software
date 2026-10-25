# Before you flash it (phone-friendly checklist)

**This ZIP is source code, not a compiled firmware .bin.** First compile the code successfully in PlatformIO or using the included GitHub Actions workflow.

1. Inspect the ESP32 board model and ensure the pins match the Ideaspark ESP-WROOM-32 ST7789; verify the LEDs are **5V addressable WS2812-compatible**, and that the strip input is DIN.
2. Configure Sinric Pro online: custom device template of **Light** type (if the portal supports it) with **Power + Brightness + Mode**. For Mode set name **Effect**, InstanceId **pumpkin_effect**, and all twenty mode names from `GOOGLE_VOICE_MODES.txt`.
3. Create **Haunted Pumpkin** from the custom template, link Sinric Pro to Google Home, and verify the device is discoverable. The real device remains offline until you flash it.
4. Compile with the GitHub Actions workflow and ensure it succeeds. If it fails, do **not** flash. Download the full flash binary (not only the app-only `firmware.bin`).
5. USB-flash the compiled full image. For first flash use ESP32 address **0x000000** and a capable Android OTG flashing tool or a Windows/Linux computer. Android USB serial compatibility is not guaranteed.
6. Connect your phone to **Pumpkin-FX-Setup** / **PumpkinFX2026**; open **http://192.168.77.1/setup**, paste Wi-Fi + Sinric keys/Device ID, pick the LED count, and change the OTA password. Save. All these settings require **zero reflashes**.
7. Use Google Nest voice commands for power, brightness, and modes. Verify named commands experimentally; Google recognition is third-party behavior and cannot be guaranteed without hardware/account tests.

If your cloud service rejects your custom Light+Mode template or the firmware does not compile, do not flash this package. The local button/effects work independently of Sinric Pro, but your requirement is voice mode control too.