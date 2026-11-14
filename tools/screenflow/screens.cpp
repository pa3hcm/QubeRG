// Runs the real firmware (src/main.cpp) on the PC with simulated buttons and
// writes every screen of the user interface as a PGM image.
#include "main.cpp"
#include <string>
uint32_t g_ms = 0; int g_pin[64]; SerialC Serial; TwoWire Wire;
std::map<std::string, uint32_t> g_nvs; uint64_t g_freq = 0; int g_en = -1;
Frame g_last; std::vector<Frame> g_frames;
static const int P[4] = {32, 33, 25, 26};
static std::string outdir;
void run(uint32_t ms) { for (uint32_t i = 0; i < ms; i++) { g_ms++; loop(); } }
void press(int b, uint32_t hold) { g_pin[P[b]] = LOW; run(hold); g_pin[P[b]] = HIGH; run(60); }
void dump(const char *name, const Frame &f) {
  FILE *fp = fopen((outdir + "/" + name + ".pgm").c_str(), "wb");
  fprintf(fp, "P5\n128 64\n255\n");
  for (int y = 0; y < 64; y++) for (int x = 0; x < 128; x++) fputc(f.px[y][x] ? 255 : 0, fp);
  fclose(fp);
}
int main(int argc, char **argv) {
  outdir = argc > 1 ? argv[1] : ".";
  for (int i = 0; i < 64; i++) g_pin[i] = HIGH;
  setup();
  if (beaconEnabled) {               // beacon variant: only the Beacon screen
    for (int i = 0; i < 4; i++) press(BTN_MODE, 100);
    dump("beacon", g_last);
    return 0;
  }
  dump("splash", g_frames.front());
  dump("off", g_last);
  press(BTN_MODE, 100); dump("cw", g_last);
  press(BTN_MODE, 100); dump("cwkeyed", g_last);
  press(BTN_MODE, 100); dump("fsk", g_last);
  press(BTN_MODE, 1200); dump("shift170", g_last);
  press(BTN_MODE, 100); dump("shift425", g_last);
  press(BTN_MODE, 100); dump("shift850", g_last);
  press(BTN_MODE, 100); dump("shiftvar", g_last);
  press(BTN_DIGIT, 1200); dump("saved", g_last);
  run(1600);
  press(BTN_MODE, 1200); dump("fskvar", g_last);
  press(BTN_DIGIT, 100); press(BTN_DIGIT, 100); press(BTN_DIGIT, 100); dump("digit", g_last);
  press(BTN_UP, 100); dump("digitup", g_last);
  return 0;
}
