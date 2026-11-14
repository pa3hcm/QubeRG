#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <map>
#define LOW 0
#define HIGH 1
#define INPUT_PULLUP 2
extern uint32_t g_ms; extern int g_pin[64];
inline uint32_t millis(){return g_ms;} inline uint32_t micros(){return g_ms*1000u;}
inline int digitalRead(int p){return g_pin[p];} inline void pinMode(int,int){}
inline void delay(uint32_t ms){g_ms+=ms;}
struct SerialC{void begin(int){} void println(const char*s){printf("[serial] %s\n",s);} } ; extern SerialC Serial;
