#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>
#include <SinricPro.h>
#include "pumpkin_cloud.h"
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "config.h"
#include "effects.h"
#include "ui.h"

namespace {
Adafruit_ST7789 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
Adafruit_NeoPixel pixels(PIXEL_COUNT, PIN_PIXEL_DATA, NEO_GRB + NEO_KHZ800);
pumpkin::Effects animations(pixels);
Preferences nvs;
WebServer server(80);
SinricProPumpkin *cloudLight = nullptr;
String wifiSsid, wifiPassword, sinricKey, sinricSecret, sinricDevice, otaPassword;
uint8_t brightness = 60; // 1-100% (also capped by MAX_BRIGHTNESS).
int selectedEffect = 0;     // Saved/running selection.
int pendingEffect = 0;      // The one shown while the user is scrolling.
bool ledsOn = true;
bool customColor = false;
uint32_t customRgb = 0;
bool screenAwake = true, setupMode = false, cloudReady = false, wifiStarted = false;
bool dirty = false, pending = false, modeReportPending = false, otaStarted = false;
uint32_t lastInput = 0, lastFrame = 0, changeStarted = 0, dirtyAt = 0, lastWiFiAttempt = 0, lastModeReportAttempt = 0;
bool buttonLastRaw = false, buttonStable = false, buttonHeld = false;
uint32_t buttonChangedAt = 0, buttonDownAt = 0;


void markDirty() { dirty = true; dirtyAt = millis(); }

void saveStateIfDue(uint32_t now) {
  if (!dirty || uint32_t(now-dirtyAt) < SAVE_DELAY_MS) return;
  nvs.putUChar("effect", static_cast<uint8_t>(selectedEffect));
  nvs.putUChar("brightness", brightness);
  nvs.putBool("on", ledsOn);
  nvs.putBool("custom", customColor);
  nvs.putUInt("rgb", customRgb);
  dirty = false;
}

void screenOn() {
  screenAwake = true;
  digitalWrite(PIN_TFT_BACKLIGHT, HIGH);
  lastInput = millis();
}

void drawScreen() {
  if (!screenAwake) return;
  constexpr uint16_t background=0x0821, orange=0xFD20, muted=0xA534;
  tft.fillScreen(background);
  tft.setTextWrap(false);
  tft.setTextSize(2);
  tft.setTextColor(orange);
  tft.setCursor(10, 9); tft.print("PUMPKIN FX");
  tft.setTextSize(1); tft.setTextColor(muted);
  tft.setCursor(257, 13); tft.printf("%02d/20", pendingEffect+1);
  tft.drawFastHLine(8, 32, 304, 0x61A7);
  tft.setTextSize(2); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(12, 52); tft.print(customColor && !pending ? "Custom Color" : pumpkin::EFFECT_NAMES[pendingEffect]);
  tft.setTextSize(2);
  tft.setTextColor(ledsOn ? ST77XX_GREEN : ST77XX_RED);
  tft.setCursor(12, 96); tft.print(ledsOn ? "LEDS ON" : "LEDS OFF");
  tft.setTextColor(muted); tft.setCursor(175, 96); tft.printf("%d%%",brightness);
  tft.drawFastHLine(8, 129, 304, 0x61A7);
  tft.setTextColor(0xBDF7); tft.setTextSize(1);
  tft.setCursor(11, 143);
  tft.print(pending ? "Selecting..." : "Press BOOT for next effect");
  tft.setCursor(242, 158); tft.print("v1.0");
}

void setEffect(int index, bool defer, bool reportMode = true) {
  if (index < 0 || index >= pumpkin::EFFECT_COUNT) return;
  pendingEffect = index;
  pending = defer;
  if (defer) changeStarted = millis();
  else {
    selectedEffect = index;
    customColor = false;
    if (reportMode) modeReportPending = true;
    animations.start(index, millis());
    markDirty();
  }
  if (screenAwake) drawScreen();
}

void updatePower(bool value) {
  ledsOn = value;
  markDirty();
  if (screenAwake) drawScreen();
}

void publishCloudPower() {
  if (cloudReady && cloudLight) cloudLight->sendPowerStateEvent(ledsOn);
}

void publishCloudBrightness() {
  if (cloudReady && cloudLight) cloudLight->sendBrightnessEvent(brightness);
}

// Report changes made by the physical button or local phone UI to Sinric Pro.
// Defer/rate-limit this so quickly browsing effects does not flood the cloud.
void publishModeIfDue(uint32_t now) {
  if (!modeReportPending || !cloudReady || !cloudLight || !SinricPro.isConnected()) return;
  if (uint32_t(now-lastModeReportAttempt) < 1200) return;
  lastModeReportAttempt = now;
  if (cloudLight->sendModeEvent(String(CLOUD_MODE_INSTANCE), String(pumpkin::EFFECT_NAMES[selectedEffect]), String("PHYSICAL_INTERACTION"))) {
    modeReportPending = false;
  }
}

// Mode values must match the 20 values configured in the Sinric Pro portal.
// Case, spaces, punctuation and underscores are normalized for resilience.
String canonicalizeMode(String text) {
  text.trim();
  text.toLowerCase();
  String cleaned;
  cleaned.reserve(text.length());
  for (size_t i=0; i<text.length(); ++i) {
    const char c = text[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) cleaned += c;
  }
  return cleaned;
}

bool onSetPumpkinMode(const String &, String &mode) {
  const String normalized = canonicalizeMode(mode);
  for (int i=0; i<pumpkin::EFFECT_COUNT; ++i) {
    if (normalized == canonicalizeMode(String(pumpkin::EFFECT_NAMES[i]))) {
      // Voice should select the effect without automatically turning power on.
      // ON later resumes the remembered selection.
      setEffect(i, false, false);
      mode = pumpkin::EFFECT_NAMES[i];
      modeReportPending = false;
      return true;
    }
  }
  Serial.printf("Unknown cloud mode: %s\n", mode.c_str());
  return false;
}

bool onPowerState(const String &, bool &value) {
  updatePower(value); return true;
}
bool onBrightness(const String &, int &value) {
  brightness = constrain(value, 1, 100);
  value = brightness; markDirty(); if(screenAwake)drawScreen(); return true;
}
bool onAdjustBrightness(const String &, int &delta) {
  brightness = constrain(int(brightness)+delta, 1, 100);
  delta = brightness;
  markDirty(); if(screenAwake)drawScreen(); return true;
}
bool onColor(const String &, byte &r, byte &g, byte &b) {
  customColor = true;
  customRgb = (uint32_t(r)<<16)|(uint32_t(g)<<8)|uint32_t(b);
  markDirty(); if(screenAwake)drawScreen(); return true;
}

void beginCloud() {
  if (cloudReady || WiFi.status()!=WL_CONNECTED || sinricKey.isEmpty() ||
      sinricSecret.isEmpty() || sinricDevice.isEmpty()) return;
  SinricProPumpkin &light = SinricPro[sinricDevice.c_str()];
  cloudLight = &light;
  light.onPowerState(onPowerState);
  light.onBrightness(onBrightness);
  light.onAdjustBrightness(onAdjustBrightness);
  light.onColor(onColor); // Optional fourth portal capability, not required.
  light.onSetMode(onSetPumpkinMode); // Fallback for integrations without an instance ID.
  light.onSetMode(String(CLOUD_MODE_INSTANCE),
      [](const String &deviceId, const String &, String &mode) {
        return onSetPumpkinMode(deviceId, mode);
      });
  SinricPro.begin(sinricKey.c_str(), sinricSecret.c_str());
  modeReportPending = true;
  cloudReady = true;
  Serial.println("Sinric Pro configured");
}

String stateJson() {
  JsonDocument doc;
  doc["on"] = ledsOn;
  doc["brightness"] = brightness;
  doc["effect"] = selectedEffect;
  doc["pendingEffect"] = pendingEffect;
  doc["customColor"] = customColor;
  doc["currentMode"] = pumpkin::EFFECT_NAMES[selectedEffect];
  doc["wifi"] = WiFi.status() == WL_CONNECTED;
  doc["ledCount"] = pixels.numPixels();
  JsonArray arr = doc["effects"].to<JsonArray>();
  for (int i=0; i<pumpkin::EFFECT_COUNT; i++) arr.add(pumpkin::EFFECT_NAMES[i]);
  String result;
  serializeJson(doc,result);
  return result;
}

void handleControl() {
  bool cloudPowerChanged = false, cloudBrightnessChanged = false;
  if (server.hasArg("effect")) {
    String text=server.arg("effect");
    bool valid = !text.isEmpty();
    for (size_t i=0; i<text.length(); ++i) {
      if (text[i] < '0' || text[i] > '9') { valid=false; break; }
    }
    const int value=text.toInt();
    if (valid && value>=0 && value<pumpkin::EFFECT_COUNT) {
      setEffect(value, true);
    } else {server.send(400,"text/plain","Invalid effect");return;}
  }
  if (server.hasArg("power")) {
    String value=server.arg("power");
    if (value=="on" || value=="off") {
      updatePower(value=="on"); cloudPowerChanged=true;
    } else {server.send(400,"text/plain","Invalid power");return;}
  }
  if (server.hasArg("brightness")) {
    const String value=server.arg("brightness");
    bool valid=!value.isEmpty();
    for (size_t i=0; i<value.length(); ++i) {
      if (value[i] < '0' || value[i] > '9') {valid=false; break;}
    }
    if (!valid || value.toInt()<1 || value.toInt()>100) {
      server.send(400,"text/plain","Invalid brightness");return;
    }
    brightness=static_cast<uint8_t>(value.toInt());
    markDirty(); cloudBrightnessChanged=true;
    if(screenAwake)drawScreen();
  }
  server.send(200,"application/json",stateJson());
  if (cloudPowerChanged) publishCloudPower();
  if (cloudBrightnessChanged) publishCloudBrightness();
}

void setupWeb() {
  server.on("/",HTTP_GET,[](){server.send_P(200,"text/html",PUMPKIN_HTML);});
  server.on("/api/state",HTTP_GET,[](){server.send(200,"application/json",stateJson());});
  server.on("/api/set",HTTP_POST,handleControl);
  server.on("/setup",HTTP_GET,[](){
    if(!setupMode){server.send(403,"text/plain","Hold BOOT for 6 seconds to open setup mode.");return;}
    server.send_P(200,"text/html",PUMPKIN_SETUP_HTML);
  });
  server.on("/save",HTTP_POST,[](){
    if (!setupMode){server.send(403,"text/plain","Setup mode required");return;}
    const String ssid=server.arg("ssid"), pass=server.arg("password");
    const String key=server.arg("sinric_key");
    const String secret=server.arg("sinric_secret");
    const String device=server.arg("sinric_device");
    const String ota=server.arg("ota_password");
    const String countText=server.arg("led_count");
    const int count=countText.toInt();
    if(ssid.length()>32 || pass.length()>64 ||
       (wifiSsid.isEmpty() && ssid.isEmpty()) ||
       (!countText.isEmpty() && (count<1 || count>MAX_LED_COUNT)) ||
       (!ota.isEmpty() && (ota.length()<8 || ota.length()>64)) ||
       key.length()>80 || secret.length()>96 || device.length()>40){
      server.send(400,"text/plain","Invalid configuration");return;
    }
    // Blank fields preserve existing values, so LED count or OTA credentials
    // can be changed without retyping long Wi-Fi/Sinric credentials.
    if (!ssid.isEmpty()) nvs.putString("ssid",ssid);
    if (!pass.isEmpty()) nvs.putString("pass",pass);
    if (!key.isEmpty()) nvs.putString("skey",key);
    if (!secret.isEmpty()) nvs.putString("ssecret",secret);
    if (!device.isEmpty()) nvs.putString("sdevice",device);
    if (!ota.isEmpty()) nvs.putString("otapass",ota);
    if (!countText.isEmpty()) nvs.putUShort("leds",static_cast<uint16_t>(count));
    server.send(200,"text/html","<h1>Saved. Pumpkin is restarting. &#127875;</h1>");
    delay(250); ESP.restart();
  });
}

void enterSetup() {
  if (setupMode) return;
  setupMode = true;
  WiFi.mode(WIFI_AP_STA);
  // Separate setup network from home routers (including 192.168.4.1).
  const IPAddress setupIp(192, 168, 77, 1);
  const IPAddress setupSubnet(255, 255, 255, 0);
  if (!WiFi.softAPConfig(setupIp, setupIp, setupSubnet)) {
    Serial.println("WARNING: Unable to configure the setup network IP");
  }
  if (!WiFi.softAP("Pumpkin-FX-Setup", SETUP_AP_PASSWORD)) {
    Serial.println("ERROR: Failed to start Pumpkin-FX-Setup network");
  }
  Serial.print("Setup AP ready, browse to http://");
  Serial.println(WiFi.softAPIP());
  screenOn(); drawScreen();
}

void onShortPress() {
  if (!screenAwake) {screenOn();drawScreen();return;}
  setEffect((pendingEffect+1)%pumpkin::EFFECT_COUNT,true);
  screenOn();
}

void handleButton(uint32_t now) {
  bool raw = digitalRead(PIN_BOOT_BUTTON)==LOW;
  if (raw!=buttonLastRaw){buttonLastRaw=raw;buttonChangedAt=now;}
  if (uint32_t(now-buttonChangedAt)<BUTTON_DEBOUNCE_MS || raw==buttonStable) return;
  buttonStable=raw;
  if (raw) {buttonDownAt=now;buttonHeld=false;return;}
  uint32_t duration=now-buttonDownAt;
  if (duration>=6000) enterSetup();
  else if (duration<1500) onShortPress();
}

void startWiFi(uint32_t now) {
  if (wifiSsid.isEmpty()) {enterSetup();return;}
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  wifiStarted=true;
  lastWiFiAttempt=now;
}

void wifiLoop(uint32_t now) {
  if (setupMode || !wifiStarted || WiFi.status()==WL_CONNECTED) return;
  if (uint32_t(now-lastWiFiAttempt)>15000) {
    lastWiFiAttempt=now;
    WiFi.reconnect();
  }
}

void startOtaWhenReady() {
  if (otaStarted || WiFi.status() != WL_CONNECTED || setupMode) return;
  ArduinoOTA.setHostname("haunted-pumpkin");
  // Change this default passphrase before deploying on an untrusted network.
  ArduinoOTA.setPassword(otaPassword.c_str());
  ArduinoOTA.begin();
  otaStarted = true;
}

void handleTimer(uint32_t now) {
  if (pending && uint32_t(now-changeStarted)>=SELECTION_DELAY_MS) {
    pending = false;
    setEffect(pendingEffect,false);
  }
  if(screenAwake && uint32_t(now-lastInput)>=SCREEN_SLEEP_MS) {
    digitalWrite(PIN_TFT_BACKLIGHT,LOW);
    screenAwake=false;
  }
  if(uint32_t(now-lastFrame)>=FRAME_MS) {
    lastFrame=now;
    pixels.setBrightness(uint8_t((unsigned(brightness)*MAX_BRIGHTNESS)/100));
    animations.tick(now,ledsOn, uint8_t((unsigned(brightness)*MAX_BRIGHTNESS)/100),
      customColor, customRgb);
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOOT_BUTTON,INPUT_PULLUP);
  pinMode(PIN_TFT_BACKLIGHT,OUTPUT);
  digitalWrite(PIN_TFT_BACKLIGHT,HIGH);

  nvs.begin("pumpkinfx",false);
  const uint16_t ledCount=nvs.getUShort("leds",PIXEL_COUNT);
  pixels.updateLength(constrain(int(ledCount),1,MAX_LED_COUNT));
  pixels.begin(); pixels.clear(); pixels.show();
  SPI.begin(PIN_TFT_SCLK,-1,PIN_TFT_MOSI,PIN_TFT_CS);
  tft.init(170,320);
  tft.setRotation(1);
  tft.setTextWrap(false);

  selectedEffect=constrain(int(nvs.getUChar("effect",0)),0,pumpkin::EFFECT_COUNT-1);
  pendingEffect=selectedEffect;
  brightness=constrain(int(nvs.getUChar("brightness",60)),1,100);
  ledsOn=nvs.getBool("on",true);
  customColor=nvs.getBool("custom",false);
  customRgb=nvs.getUInt("rgb",0);
  wifiSsid=nvs.getString("ssid",PRESET_WIFI_SSID);
  wifiPassword=nvs.getString("pass",PRESET_WIFI_PASSWORD);
  sinricKey=nvs.getString("skey",PRESET_SINRIC_KEY);
  sinricSecret=nvs.getString("ssecret",PRESET_SINRIC_SECRET);
  sinricDevice=nvs.getString("sdevice",PRESET_SINRIC_DEVICE);
  otaPassword=nvs.getString("otapass",OTA_PASSWORD);
  animations.start(selectedEffect,millis());
  screenOn();drawScreen();
  setupWeb();
  startWiFi(millis());
  server.begin();
  Serial.println("Haunted Pumpkin FX booted");
}

void loop() {
  uint32_t now=millis();
  handleButton(now);
  wifiLoop(now);
  server.handleClient();
  if (WiFi.status()==WL_CONNECTED) {
    startOtaWhenReady();
    if (otaStarted) ArduinoOTA.handle();
    if (!cloudReady) beginCloud();
    if (cloudReady) {
      SinricPro.handle();
      publishModeIfDue(now);
    }
  }
  handleTimer(now);
  saveStateIfDue(now);
}