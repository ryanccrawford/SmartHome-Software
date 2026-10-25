# Haunted Pumpkin FX 🎃

**Standalone ESP32 firmware** for the Ideaspark ESP-WROOM-32 with built-in 1.9-inch 170×320 ST7789 LCD. No Home Assistant server and no Raspberry Pi are required. This is an Arduino / PlatformIO **source project**. It is not a compiled `.bin` until built.

## What it does

- 20 Halloween effects rendered independently per pixel, including realistic candle flicker, twin flames, embers, lightning with randomized flash and pause intervals, green cauldron, spectral fades, solid orange/red/purple, and candy corn.
- BOOT button (GPIO0): first press after display sleep wakes it; subsequent presses scroll through effects, wrapping 20 → 1. A 500 ms quiet period commits the new effect; button debounce 35 ms.
- Built-in ST7789 shows selection, brightness, on/off. Its backlight switches off after 10 s, **not the controller/LED power**.
- Local LED on/off independent of microcontroller power. Off means LEDs stay black, Wi-Fi and screen/button still work. On resumes saved selection.
- Stores effect, on/off and brightness in ESP32 nonvolatile storage (writes coalesced after 3.5 s).
- Phone web UI with all 20 effects plus power and brightness, hosted **on the ESP32**. After Wi-Fi joins, open its IP address on your home network.
- **Google Home through Sinric Pro custom Mode**: light on/off, brightness and **all 20 named effect modes**. The firmware handles `setMode` requests and reports button/phone mode changes back to Sinric Pro. Voice recognition and pairing still require live testing on your specific Nest display.
- Solid colors remain available as named effects (Solid Orange, Vampire Red, Phantom Purple). A 4th optional Color capability can be added to the Sinric Pro device template when a 4-capability subscription is available. The free custom-template allowance provides 3 capabilities: Power, Brightness and Mode. (Sinric Pro service limits may change.)
- Setup Wi-Fi from a phone without typing the password: **paste** the password in its private setup access point. If desired, embed credentials in `include/config.h` *before compilation* instead.
- Nonblocking LED effects and Wi-Fi retries; when cloud or Wi-Fi is down, LEDs and one-button controls still work.

## Files

- `src/main.cpp`: LCD, ESP32 button, web configuration, saved state, SinricPro callbacks, runtime loop.
- `include/effects.h`: the complete 20-effect rendering engine.
- `include/ui.h`: phone UI and setup portal, works without internet.
- `include/config.h`: all configurable pins, default LED count, brightness ceiling, optional credentials and OTA password.
- `include/pumpkin_cloud.h`: one custom Sinric Pro device combining Power, Brightness, Mode and optional Color callbacks.
- `GOOGLE_VOICE_MODES.txt`: exact 20 names, mode name and InstanceId for the Sinric Pro portal.
- `platformio.ini`: dependencies and build target.
- `.github/workflows/build.yml`: GitHub Actions cloud compile, including a merged all-in-one `.bin` for first-time flashing (build is performed in your GitHub repository).
- `tests`: C++ host-side LED render smoke test.

## Initial Wi-Fi provisioning from your phone

1. **First flash the firmware** to the ESP32 once using USB and a compiled image (see build instructions below).
2. Power the board. It advertises a Wi-Fi network named **`Pumpkin-FX-Setup`** if no Wi-Fi is configured.
3. On your Android phone, connect to `Pumpkin-FX-Setup`, password **`PumpkinFX2026`**. Your phone may say **No internet**; choose **Stay connected**.
4. Open **http://192.168.4.1/setup** in the phone browser.
5. Paste your home's **2.4 GHz** Wi-Fi SSID and password. You can paste your long password, with no repeated typing.
6. Optionally add Sinric Pro App Key, App Secret and Light Device ID from the Sinric Pro portal, or leave blank if using local-only control. Tap **Save & Restart**.
7. On Wi-Fi, find the ESP32's assigned IP from your router's device list (or serial log) and open `http://DEVICE_IP/` on the phone for effect selection.
8. You may also configure the **number of LEDs, 1–32**, and an OTA password in the setup form without recompiling. Blank fields keep the previously saved credentials.
9. To change settings later, **hold BOOT for six seconds while the firmware is running**. The setup AP reappears. Do not hold BOOT while powering up/resetting, or the ESP32 will boot into firmware flashing mode instead.

The setup AP uses a known initial password from the code. Change it in `include/config.h` before compiling if you don't want the default. The HTTP control page has **no user authentication** and is intended for a trusted local network; never forward port 80 from your router to the internet. Sinric Pro credentials are stored in the device's NVS and aren't sent back by the settings page, but physical access to the board can compromise stored secrets.

## Google Home / Google Nest display setup: all 20 effects included

**No Home Assistant server is needed.** The Google Home integration requires a Sinric Pro account, a custom device template, and the correct mode names/InstanceId before voice control can work. The ESP32 firmware already includes the mode callback; **setting up the portal does not require re-flashing**.

1. Create an account at **https://portal.sinric.pro/**.
2. Go to **Device Templates → Add Device Template**. Name it **Haunted Pumpkin**. Choose the **Light** device type if offered and compatible with Google Home. Sinric Pro warns that Google has limited compatibility with some custom device types. This must be confirmed in your portal before flashing.
3. Add these **three capabilities**: **Power**, **Brightness**, and **Mode**. The free allowance supports up to three capabilities on custom templates; additional ones require more device licenses. You don't need the optional Color capability for the 20 preset lighting effects.
4. Configure the **Mode** capability:
   - **Mode name:** `Effect`
   - **InstanceId:** `pumpkin_effect` (must match `CLOUD_MODE_INSTANCE` in `include/config.h`).
   - **Locale:** English (US).
   - **Non Controllable:** disabled / unchecked.
   - **Modes:** enter the **20 values**, one by one, from `GOOGLE_VOICE_MODES.txt`, in the same spelling. Google Home should then see the available lighting modes.
5. Save the device template; create a **device using your new template** (not a stock Sinric Pro Light device). Give it a voice-friendly name: **Haunted Pumpkin**.
6. Copy its **App Key**, **App Secret**, and **Device ID** into the phone setup form (hold BOOT for six seconds, reconnect to `Pumpkin-FX-Setup`, then visit http://192.168.4.1/setup). Tap Save and let it restart. You **do not** need to rebuild or re-flash.
7. In the **Google Home app**, open **Add → Device → Works with Google Home**, find **Sinric Pro**, link your account, and assign **Haunted Pumpkin** to a room.
8. After linking, try these utterances:
   - “Hey Google, turn on Haunted Pumpkin.”
   - “Hey Google, turn off Haunted Pumpkin.”
   - “Hey Google, set Haunted Pumpkin to forty percent brightness.”
   - “Hey Google, set the Haunted Pumpkin effect to Haunted Candle.”
   - “Hey Google, set Haunted Pumpkin to Lightning Storm mode.”
   - “Hey Google, set Haunted Pumpkin to Ghostly Glow.”

The last three are **illustrative utterances, not verified promises**. Sinric Pro documents support for the Google Home Modes trait, and Google documents arbitrary named modes, but recognition of a specific name and whether the device type/template is exposed depend on live Sinric Pro and Google Home behavior. If pairing fails, the device will still work fully with the physical BOOT button, 20 effects, and the ESP32's local phone page. Avoid flashing until you confirm the template is accepted and the firmware compiles.

The mode callback accepts the exact mode names plus case/punctuation/underscore variants, updates the selected effect without turning LEDs on if they're currently off, and retains the selection in flash. The local button and web UI report committed mode changes back to Sinric Pro.

Sinric Pro is a third-party cloud service. Internet access is needed for voice or remote Google Home commands, while the ESP32 local button/effects remain functional offline.

## Update without plugging in USB again

**ArduinoOTA is built in**. After first flashing and joining Wi-Fi, the device advertises itself as `haunted-pumpkin` for password-protected OTA upgrades via Arduino IDE / PlatformIO. Initial default OTA password is `PumpkinOTA2026`. **Change it using the phone setup form**; it is saved in nonvolatile storage. This allows future bug fixes without connecting a USB cable, though compiling an updated firmware image still requires a build environment. OTA can fail if the device loses power during installation, so keep the battery well charged.

You can change Wi-Fi/Sinric credentials, number of LEDs, and OTA password through the phone setup form **without updating firmware at all**.

## Wiring (buck converter, 5V LED strip)

```
12V nominal LiPo battery
  -> inline fuse + battery protection / low-voltage cutoff
  -> 5V/3A regulated buck converter
        +5V -> ESP32 5V/VIN   AND strip +5V
        GND -> ESP32 GND      AND strip GND

ESP32 GPIO27 -> (3.3V-to-5V AHCT level shifter recommended)
              -> 330 ohm series resistor -> LED strip DIN
```

Check the LED strip **pad markings** for +5V, GND and DIN. **Wire color is not sufficient identification.** The strip in the photo appears three-wire and likely WS2812-compatible (GRB), but confirm the actual strip type and voltage first. The arrow on a strip points in the data-flow direction; connect at **DIN**, not DOUT.

Add a **500–1000 µF capacitor** across LED +5V/GND near the strip, observing polarity. No 12V supply is connected to the ESP32 or strip. The buck converter must tolerate the **fully charged** LiPo pack voltage (e.g. a 3S pack reaches ~12.6 V), and the pack needs appropriate low-voltage protection. A 5V/3A source provides 15W total. Approximate worst-case 5V RGB LED draw is 60mA per pixel at full RGB white, plus the controller. **`PIXEL_COUNT` defaults to 8**, but you can change the count from your phone on the ESP32 setup page after first flash. The firmware permits 1–32 LEDs to stay within the 5V/3A power budget at its capped output brightness.

The ESP32 has 3.3V GPIO logic; for reliable data into a 5V strip, use a 74AHCT125/74HCT245 or similar 5V-compatible logic level shifter. Do **not** connect the LED data line to 5V directly. The microcontroller draws from the 5V input through its onboard voltage regulator; never inject 5V into the **3V3** pin.

**USB flashing:** Disconnect the buck/battery power from the ESP32's 5V/VIN while programming over USB unless you've verified the board's power isolation. Do not put an actual candle flame next to the electronics or LiPo.

### Ideaspark display pins

| Role | GPIO |
| --- | --- |
| LCD MOSI / SCLK | 23 / 18 |
| LCD CS / DC / RESET | 15 / 2 / 4 |
| LCD backlight | 32 |
| One BOOT button | 0 |
| LED data | 27 (external strip) |

If the display looks sideways or the backlight does not illuminate, board revisions may need `tft.setRotation(...)` or backlight polarity changes. GPIO0 is a bootstrapping pin; **never hold the button while resetting** unless you intend to flash.

## Build the firmware

### Option A: Desktop with PlatformIO

1. Install VS Code and PlatformIO.
2. Open this folder, check `include/config.h`, and run **PlatformIO: Build**.
3. The compiled application is `.pio/build/ideaspark-esp32/firmware.bin`. For a blank ESP32, also flash the bootloader and partitions at their standard offsets, or use **PlatformIO: Upload** over USB and let it handle that automatically.

### Option B: Cloud build from your phone using GitHub Actions

1. Upload these **project files, including `.github/workflows/build.yml`**, to a GitHub repository you control, keeping the same folder structure.
2. In GitHub on your phone, open **Actions → Build Pumpkin Firmware → Run workflow**.
3. Once the run **succeeds**, download the `pumpkin-firmware-full-flash` artifact, containing **`pumpkin-fx-full-flash.bin`**. This image combines the bootloader, partition table, boot_app0 and firmware for flashing at **address 0x00000** on an ESP32. An ordinary app-only `firmware.bin` is instead flashed at **0x10000** on an already-prepared device.
4. To flash using Android: you need a **USB-C data cable/OTG adapter** and an Android ESP32 flashing application that supports the classic ESP32 CH340 USB serial adapter **and** writing a bin file at the specified address. Android compatibility is **not guaranteed**. If Android USB serial flashing doesn't work, use a Windows/Linux computer for the first flash.
5. After first flash, configure Wi-Fi via the captive setup AP from your phone (above).

The Actions workflow does **not** contain your home Wi-Fi password or Sinric secret: they are entered on the device's setup page instead. **Do not check real credentials into a public GitHub repository.**

## Development verification and limitations

The source and platform configuration are included as actual files, and all twenty renderers passed a host-side C++ smoke test with stub hardware. This environment could **not download the PlatformIO compiler/packages**, so a real ESP32 compile and physical hardware flash were not performed. Google Home Mode integration is implemented in the source but **has not been compiled against the target ESP32 toolchain or tested with your actual Sinric Pro portal and Nest display**. The 20 effects are verified only using a host-side stub test, not hardware. This is a source release, not a guaranteed one-flash finished binary. Verify the first build and device behavior before leaving it unattended.