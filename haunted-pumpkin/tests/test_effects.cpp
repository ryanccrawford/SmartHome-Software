// Host-side smoke test for the 20 LED effects.
// Build with: g++ -std=c++17 -Itests/stubs -Iinclude tests/test_effects.cpp -o /tmp/effects && /tmp/effects
#include <cassert>
#include <iostream>
#include "effects.h"
int main() {
  for(int i=0;i<pumpkin::EFFECT_COUNT;++i){
    Adafruit_NeoPixel leds(8,27,0);
    pumpkin::Effects fx(leds);
    fx.start(i,10000);
    fx.tick(15000,true,150);
    assert(leds.nonBlack());
  }
  std::cout << "PASS: 20 named effects emit nonblack pixels\n";
}