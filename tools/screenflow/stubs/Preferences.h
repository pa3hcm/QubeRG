#pragma once
#include <map>
#include <string>
extern std::map<std::string,uint32_t> g_nvs;
struct Preferences{ bool begin(const char*,bool){return true;} void end(){}
 uint8_t getUChar(const char*k,uint8_t d){auto i=g_nvs.find(k);return i==g_nvs.end()?d:i->second;}
 uint32_t getUInt(const char*k,uint32_t d){auto i=g_nvs.find(k);return i==g_nvs.end()?d:i->second;}
 void putUChar(const char*k,uint8_t v){g_nvs[k]=v;} void putUInt(const char*k,uint32_t v){g_nvs[k]=v;} };
