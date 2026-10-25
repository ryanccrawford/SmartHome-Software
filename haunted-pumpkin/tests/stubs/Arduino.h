#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
using std::min;
using std::max;
template<typename T> T constrain(T value,T minval,T maxval){return max(minval,min(maxval,value));}
inline uint32_t esp_random() {static uint32_t seed = 1; return seed = seed * 1664525u + 1013904223u;}