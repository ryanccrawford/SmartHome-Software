#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <esp_system.h>

namespace pumpkin {

constexpr int EFFECT_COUNT = 20;
static const char * const EFFECT_NAMES[EFFECT_COUNT] = {
    "Haunted Candle", "Twin Flames", "Dying Embers", "Hellfire",
    "Witch's Cauldron", "Ghostly Glow", "Lightning Storm", "Blood Moon",
    "Pumpkin Heartbeat", "Spectral Fade", "Possessed Flicker", "Toxic Bubbles",
    "Graveyard Mist", "Bat Swarm", "Jack-o'-Lantern", "Solid Orange",
    "Vampire Red", "Phantom Purple", "Candy Corn", "The Awakening"
};

inline uint8_t scale(float value) {
    if (value < 0.f) return 0;
    if (value > 255.f) return 255;
    return static_cast<uint8_t>(value);
}
inline float wave(float value) { return 0.5f + 0.5f * sinf(value); }
inline uint32_t rand32() { return esp_random(); }
inline float random01() { return float(rand32() & 0xffffu) / 65535.f; }

class Effects {
public:
    explicit Effects(Adafruit_NeoPixel &strip) : strip_(strip) {}

    void start(int effect, uint32_t now) {
        active_ = constrain(effect, 0, EFFECT_COUNT - 1);
        flashEnd_ = 0;
        nextBolt_ = now + 900 + (rand32() % 5000);
        nextFlash_ = 0;
        flashesRemaining_ = 0;
        for (auto &n : noise_) n = random01();
    }

    // ~28fps at most. Each LED gets its own animation; nothing ever sleeps the CPU.
    void tick(uint32_t now, bool enabled, uint8_t brightness, bool customColor = false, uint32_t solidOverride = 0) {
        if (!enabled) {
            strip_.clear();
            strip_.show();
            return;
        }
        strip_.setBrightness(brightness);
        float t = now * 0.001f;
        bool flash = false;
        if (active_ == 6) {
            if (!flashesRemaining_ && (int32_t)(now - nextBolt_) >= 0) {
                flashesRemaining_ = 1 + (rand32() % 3);
                nextFlash_ = now;
                nextBolt_ = now + 1800 + (rand32() % 8200);
            }
            if (flashEnd_ && (int32_t)(now - flashEnd_) >= 0) {
                flashEnd_ = 0;
                if (flashesRemaining_) --flashesRemaining_;
                nextFlash_ = now + 55 + (rand32() % 150);
            }
            if (!flashEnd_ && flashesRemaining_ && (int32_t)(now - nextFlash_) >= 0)
                flashEnd_ = now + 40 + (rand32() % 180);
            flash = flashEnd_ != 0;
        }
        for (uint16_t i = 0; i < strip_.numPixels(); i++) {
            float pos = float(i) / max(1, int(strip_.numPixels()));
            float rand = random01();
            float &smooth = noise_[i % 256];
            smooth = smooth * 0.75f + rand * 0.25f;
            float x = wave(t * 1.7f + pos * 6.28f);
            uint8_t r = 0, g = 0, b = 0;
            switch (active_) {
                case 0: r=scale(145+110*smooth); g=scale(22+122*smooth); b=scale(3+6*smooth); break;
                case 1: r=245; g=scale(22+155*(0.5f*smooth+0.5f*wave(t*5+(i%2)*3))); break;
                case 2: r=scale(45+165*powf(wave(t*0.7f+pos*5), 5)); g=scale(r*0.22f); break;
                case 3: r=scale(175+80*smooth); g=scale(5+155*wave(t*6+pos*12)*smooth); break;
                case 4: r=scale(6+70*x); g=scale(42+170*x); b=scale(4+18*x); break;
                case 5: r=scale(8+54*x); g=scale(38+138*x); b=scale(77+170*x); break;
                case 6: r=flash?255:1; g=flash?255:1; b=flash?255:scale(5+8*x); break;
                case 7: r=scale(60+145*wave(t*0.7f)); g=5; break;
                case 8: {float phase = fmodf(t/2.0f,1.0f); float beat = max(0.f,1.f-fabsf(phase-0.12f)/0.1f)+0.6f*max(0.f,1.f-fabsf(phase-0.3f)/0.08f); r=scale(32+213*beat);g=scale(2+90*beat);break;}
                case 9: r=scale(20+155*wave(t*.65f+pos));g=scale(10+110*wave(t*.65f+pos+2.09f));b=scale(72+170*wave(t*.65f+pos+4.19f));break;
                case 10: r=scale(25+190*smooth);g=scale(4+8*rand);b=scale(60+170*smooth);break;
                case 11: r=scale(4+35*(rand>.97f));g=scale(37+215*(rand>.97f?1:x*.4f));b=scale(2+35*x);break;
                case 12: r=scale(5+54*x);g=scale(14+70*x);b=scale(24+160*x);break;
                case 13: {float p = wave(t*3.7f-pos*15);r=scale(6+90*p);g=0;b=scale(7+120*p);break;}
                case 14: r=255;g=scale(75+15*wave(t+pos));b=2;break;
                case 15: r=255;g=83;b=0;break;
                case 16: r=235;g=0;b=0;break;
                case 17: r=125;g=0;b=210;break;
                case 18: if(i%3==0){r=255;g=68;}else if(i%3==1){r=255;g=165;}else{r=255;g=238;b=180;}break;
                case 19: {float phase = wave(t*0.6f+pos*2);r=scale(35+215*phase);g=scale(45*phase*phase);b=scale(90*(1-phase));break;}
            }
            if (customColor) {r=uint8_t(solidOverride>>16); g=uint8_t(solidOverride>>8);b=uint8_t(solidOverride);}
            strip_.setPixelColor(i, strip_.Color(r,g,b));
        }
        strip_.show();
    }
private:
    Adafruit_NeoPixel &strip_;
    int active_ = 0;
    float noise_[256] = {};
    uint32_t nextBolt_ = 0, flashEnd_ = 0, nextFlash_ = 0;
    uint8_t flashesRemaining_ = 0;
};
}