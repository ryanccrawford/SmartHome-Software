#pragma once
#include "Arduino.h"
#include <vector>
class Adafruit_NeoPixel {
public:
 explicit Adafruit_NeoPixel(uint16_t count, int, int): pixels(count){}
 uint16_t numPixels()const{return pixels.size();}
 uint32_t Color(uint8_t r,uint8_t g,uint8_t b)const{return uint32_t(r)<<16|uint32_t(g)<<8|b;}
 void setBrightness(uint8_t){}
 void setPixelColor(size_t i,uint32_t c){pixels.at(i)=c;}
 void clear(){std::fill(pixels.begin(),pixels.end(),0);}
 void show(){}
 bool nonBlack()const {return std::any_of(pixels.begin(),pixels.end(),[](uint32_t c){return c!=0;});}
private:
 std::vector<uint32_t> pixels;
};