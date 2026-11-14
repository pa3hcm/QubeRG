#pragma once
#include <cstdint>
#include <cstdio>
enum si5351_clock{SI5351_CLK0,SI5351_CLK1,SI5351_CLK2}; enum{SI5351_CRYSTAL_LOAD_8PF=0}; enum si5351_drive{SI5351_DRIVE_8MA};
extern uint64_t g_freq; extern int g_en;
struct Si5351{ bool init(int,uint32_t,int32_t){return true;} void drive_strength(si5351_clock,si5351_drive){}
 uint8_t set_freq(uint64_t f,si5351_clock){g_freq=f/100;return 0;} void output_enable(si5351_clock c,uint8_t e){if(c==SI5351_CLK0)g_en=e;} };
