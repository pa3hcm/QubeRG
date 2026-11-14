/*
 * QubeRG v0.1b - simple RF signal generator
 * (c) 2026 PA3HCM
 *
 * Licensed under the Creative Commons Attribution-NonCommercial 4.0
 * International License (CC BY-NC 4.0). See the LICENSE file or
 * https://creativecommons.org/licenses/by-nc/4.0/
 *
 * Hardware:
 *   - ESP32 devboard (esp32dev)
 *   - Si5351 DDS board, 25 MHz crystal, I2C address 0x60, output on CLK0
 *   - GM009605v4 0.96" OLED (SSD1306, 128x64), I2C address 0x3C
 *   - 4 push buttons to GND (internal pull-ups used)
 *
 * Wiring (all 3.3 V):
 *   I2C SDA  -> GPIO21 (Si5351 SDA + OLED SDA)
 *   I2C SCL  -> GPIO22 (Si5351 SCL + OLED SCL)
 *   Button 1 -> GPIO32  UP:    increase selected digit / FSK shift
 *   Button 2 -> GPIO33  DOWN:  decrease selected digit / FSK shift
 *   Button 3 -> GPIO25  DIGIT: select next digit (wraps around)
 *                              hold 1 s: save the FSK shift
 *   Button 4 -> GPIO26  MODE:  select next mode (CW, CW-keyed, FSK, Beacon, OFF)
 *                              hold 1 s in FSK: open / close the FSK shift setting
 *   Button COM -> GND
 *
 * FSK shift setting (hold MODE while in FSK mode):
 *   MODE        next shift: 170, 425, 850, VAR
 *   UP / DOWN   VAR only: shift +/- 1 Hz, hold for 10 Hz steps (100 Hz/s)
 *   hold DIGIT  save the shift (survives power off)
 *   hold MODE   back to normal operation
 *
 * Beacon mode: set CALLSIGN below to your own callsign to enable it.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <si5351.h>

// ---------------------------------------------------------------- pins
static const uint8_t PIN_SDA = 21;
static const uint8_t PIN_SCL = 22;

static const uint8_t NUM_BUTTONS = 4;
static const uint8_t BUTTON_PINS[NUM_BUTTONS] = {32, 33, 25, 26};
enum ButtonId : uint8_t { BTN_UP = 0, BTN_DOWN, BTN_DIGIT, BTN_MODE };

// ---------------------------------------------------------------- display
static const uint8_t SCREEN_WIDTH = 128;
static const uint8_t SCREEN_HEIGHT = 64;
static const uint8_t OLED_ADDR = 0x3C;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------------------------------------------------------- Si5351
Si5351 si5351;                       // default I2C address 0x60
static const uint32_t XTAL_FREQ = 25000000UL;
static const int32_t XTAL_CORRECTION = 0;   // in parts per billion, calibrate later
bool si5351Present = false;

// ---------------------------------------------------------------- frequency
static const uint8_t NUM_DIGITS = 8;                 // "10.000.000" -> 8 digits
static const uint32_t FREQ_MIN = 8000UL;             // Si5351 lower limit ~8 kHz
static const uint32_t FREQ_MAX = 99999999UL;         // max that fits in 8 digits
static const uint32_t FREQ_START = 10000000UL;       // 10 MHz

uint32_t frequency = FREQ_START;
uint8_t selectedDigit = 0;     // 0 = left-most digit (10 MHz position)

// ---------------------------------------------------------------- beacon
// Set your own callsign here to enable the Beacon mode.
// With the default "N0CALL" the Beacon mode is skipped.
static const char *const CALLSIGN = "N0CALL";
static const uint32_t BEACON_CARRIER_MS = 60000UL;   // carrier after the callsign

// ---------------------------------------------------------------- modes
enum Mode : uint8_t {
  MODE_CW = 0,      // continuous carrier
  MODE_CW_KEYED,    // keyed carrier: -.-.-. ...
  MODE_FSK,         // alternates f / f + shift at 1 Hz (500 ms each)
  MODE_BEACON,      // callsign in morse, 60 s carrier, repeat
  MODE_OFF,         // no output
  NUM_MODES
};
const char *const MODE_NAMES[NUM_MODES] = {"CW", "CW-keyed", "FSK", "Beacon", "OFF"};

Mode mode = MODE_OFF;              // default mode at power-up

// ---------------------------------------------------------------- FSK shift
enum ShiftSel : uint8_t { SHIFT_170 = 0, SHIFT_425, SHIFT_850, SHIFT_VAR, NUM_SHIFTS };
const uint32_t SHIFT_PRESETS[3] = {170, 425, 850};
static const uint32_t SHIFT_MIN = 1;
static const uint32_t SHIFT_MAX = 10000;
static const uint32_t VAR_SHIFT_START = 1000;

uint8_t shiftSel = SHIFT_170;            // selected shift (preset or VAR)
uint32_t varShift = VAR_SHIFT_START;     // VAR shift in Hz

Preferences prefs;                       // ESP32 non-volatile storage
static const char *const PREFS_NS = "quberg";

uint32_t currentShift() {
  return shiftSel == SHIFT_VAR ? varShift : SHIFT_PRESETS[shiftSel];
}

// ---------------------------------------------------------------- user interface
enum UiState : uint8_t { UI_NORMAL = 0, UI_SHIFT };
UiState ui = UI_NORMAL;
uint32_t messageUntil = 0;              // show a short message until this millis()
const char *message = nullptr;

// ---------------------------------------------------------------- step patterns
// Every timed mode is a list of steps that is played in a loop.
struct Step {
  bool on;          // output enabled
  bool mark;        // true = displayed frequency + FSK shift
  uint32_t ms;      // duration
};

static const uint32_t CW_UNIT_MS = 100;         // morse dot length: 1200 / 12 WPM
static const uint32_t FSK_STEP_MS = 500;        // 1 Hz alternation

// CW-keyed: "-." repeated, gives -.-.-.-.
const Step CW_KEYED_STEPS[] = {
  {true, false, 3 * CW_UNIT_MS}, {false, false, CW_UNIT_MS},
  {true, false, CW_UNIT_MS},     {false, false, CW_UNIT_MS},
};

// FSK: 500 ms on f, 500 ms on f + shift
const Step FSK_STEPS[] = {
  {true, false, FSK_STEP_MS}, {true, true, FSK_STEP_MS},
};

// Beacon: callsign in morse, word gap, carrier, word gap. Built in buildPatterns().
static const uint16_t BEACON_MAX_STEPS = 200;
Step beaconSteps[BEACON_MAX_STEPS];
uint16_t beaconLen = 0;
bool beaconEnabled = false;

const Step *patternSteps = nullptr;   // steps of the current mode, nullptr = static
uint16_t patternLen = 0;
uint16_t patternStep = 0;             // current step
uint32_t patternStepStart = 0;        // micros() at start of current step

// what the Si5351 is currently doing (to avoid needless I2C traffic)
uint32_t outFreq = 0;
bool outEnabled = false;

// ---------------------------------------------------------------- buttons
static const uint32_t DEBOUNCE_MS = 25;
static const uint32_t LONG_PRESS_MS = 1000;    // hold time for DIGIT / MODE
static const uint32_t REPEAT_DELAY_MS = 500;   // hold time before auto-repeat
static const uint32_t REPEAT_RATE_MS = 120;    // auto-repeat interval (frequency)
static const uint32_t SHIFT_RATE_MS = 100;     // auto-repeat interval (shift, 10 Hz steps)
static const uint32_t SHIFT_FAST_STEP = 10;

struct Button {
  uint8_t pin;
  bool stableState;      // true = pressed
  bool lastReading;
  uint32_t lastChange;
  uint32_t pressStart;
  uint32_t nextRepeat;
  bool longFired;
};
Button buttons[NUM_BUTTONS];

// ---------------------------------------------------------------- helpers

// Weight of digit 'd' (0 = left-most): 10^(NUM_DIGITS-1-d)
uint32_t digitWeight(uint8_t d) {
  uint32_t w = 1;
  for (uint8_t i = 0; i < NUM_DIGITS - 1 - d; i++) w *= 10;
  return w;
}

// Character column of digit 'd' in the string "XX.XXX.XXX"
uint8_t digitCharPos(uint8_t d) {
  return d + (d >= 2 ? 1 : 0) + (d >= 5 ? 1 : 0);
}

void formatFrequency(uint32_t f, char *buf) {
  // buf must hold at least 16 chars
  snprintf(buf, 16, "%02lu.%03lu.%03lu",
           (unsigned long)(f / 1000000UL),
           (unsigned long)((f / 1000UL) % 1000UL),
           (unsigned long)(f % 1000UL));
}

// Program the Si5351 CLK0 output; only talks I2C when something changes
void setOutput(bool enabled, uint32_t f) {
  if (!si5351Present) return;
  if (f != outFreq) {
    // set_freq takes the frequency in units of 0.01 Hz
    si5351.set_freq((uint64_t)f * 100ULL, SI5351_CLK0);
    outFreq = f;
    si5351.output_enable(SI5351_CLK0, enabled ? 1 : 0);
    outEnabled = enabled;
  } else if (enabled != outEnabled) {
    si5351.output_enable(SI5351_CLK0, enabled ? 1 : 0);
    outEnabled = enabled;
  }
}

// Morse code for A-Z, 0-9 and '/', as dots and dashes
const char *morseFor(char c) {
  static const char *const LETTERS[26] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--.."
  };
  static const char *const DIGITS[10] = {
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----."
  };
  if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  if (c >= 'A' && c <= 'Z') return LETTERS[c - 'A'];
  if (c >= '0' && c <= '9') return DIGITS[c - '0'];
  if (c == '/') return "-..-.";
  return nullptr;
}

void addBeaconStep(bool on, uint32_t ms) {
  if (beaconLen < BEACON_MAX_STEPS) beaconSteps[beaconLen++] = {on, false, ms};
}

void buildPatterns() {
  beaconEnabled = strcmp(CALLSIGN, "N0CALL") != 0;
  beaconLen = 0;
  for (const char *p = CALLSIGN; *p; p++) {
    const char *code = morseFor(*p);
    if (!code) continue;
    for (const char *e = code; *e; e++) {
      addBeaconStep(true, (*e == '-' ? 3 : 1) * CW_UNIT_MS);
      addBeaconStep(false, CW_UNIT_MS);                          // gap between elements
    }
    beaconSteps[beaconLen - 1].ms = 3 * CW_UNIT_MS;              // gap between characters
  }
  if (beaconLen > 0) beaconSteps[beaconLen - 1].ms = 7 * CW_UNIT_MS;   // word gap
  addBeaconStep(true, BEACON_CARRIER_MS);                        // long carrier
  addBeaconStep(false, 7 * CW_UNIT_MS);                          // word gap before repeat
}

// Select the step list for the current mode
void selectPattern() {
  switch (mode) {
    case MODE_CW_KEYED:
      patternSteps = CW_KEYED_STEPS;
      patternLen = sizeof(CW_KEYED_STEPS) / sizeof(CW_KEYED_STEPS[0]);
      break;
    case MODE_FSK:
      patternSteps = FSK_STEPS;
      patternLen = sizeof(FSK_STEPS) / sizeof(FSK_STEPS[0]);
      break;
    case MODE_BEACON:
      patternSteps = beaconSteps;
      patternLen = beaconLen;
      break;
    default:   // CW and OFF are static
      patternSteps = nullptr;
      patternLen = 0;
      break;
  }
}

// Set the output for the current mode and pattern step
void applyOutput() {
  if (patternSteps) {
    const Step &st = patternSteps[patternStep];
    setOutput(st.on, frequency + (st.mark ? currentShift() : 0));
  } else {
    setOutput(mode == MODE_CW, frequency);
  }
}

void restartPattern() {
  selectPattern();
  patternStep = 0;
  patternStepStart = micros();
  applyOutput();
}

// Advance the step pattern; call as often as possible from loop()
void updatePattern() {
  if (!patternSteps) return;
  uint32_t d = patternSteps[patternStep].ms * 1000UL;
  uint32_t now = micros();
  if (now - patternStepStart >= d) {
    // keep a steady rhythm, but resync if we fell far behind
    patternStepStart = (now - patternStepStart >= d + 100000UL) ? now : patternStepStart + d;
    patternStep = (patternStep + 1) % patternLen;
    applyOutput();
  }
}

// ---------------------------------------------------------------- preferences

void loadShift() {
  prefs.begin(PREFS_NS, true);
  shiftSel = prefs.getUChar("fskSel", SHIFT_170);
  varShift = prefs.getUInt("fskVar", VAR_SHIFT_START);
  prefs.end();
  if (shiftSel >= NUM_SHIFTS) shiftSel = SHIFT_170;
  if (varShift < SHIFT_MIN || varShift > SHIFT_MAX) varShift = VAR_SHIFT_START;
}

void saveShift() {
  prefs.begin(PREFS_NS, false);
  prefs.putUChar("fskSel", shiftSel);
  prefs.putUInt("fskVar", varShift);
  prefs.end();
}

// ---------------------------------------------------------------- display

void printCentered(const char *s, int16_t y, uint8_t size) {
  display.setTextSize(size);
  int16_t w = strlen(s) * 6 * size;
  display.setCursor(w < SCREEN_WIDTH ? (SCREEN_WIDTH - w) / 2 : 0, y);
  display.print(s);
}

// Text for the shift, e.g. "170 Hz" or "VAR 1000 Hz"
void formatShift(char *buf, size_t len, bool withVar) {
  if (shiftSel == SHIFT_VAR && withVar) {
    snprintf(buf, len, "VAR %luHz", (unsigned long)varShift);
  } else {
    snprintf(buf, len, "%lu Hz", (unsigned long)currentShift());
  }
}

void drawNormalScreen() {
  char buf[24];
  formatFrequency(frequency, buf);

  const int16_t charW = 12;
  const int16_t textX = (SCREEN_WIDTH - 10 * charW) / 2;   // 10 characters
  const int16_t textY = 22;

  display.setTextSize(2);                 // 12 x 16 px per character
  display.setCursor(textX, textY);
  display.print(buf);

  // cursor: a bar just below the selected digit
  int16_t cx = textX + digitCharPos(selectedDigit) * charW;
  display.fillRect(cx, textY + 18, charW - 2, 2, SSD1306_WHITE);

  // bottom line: mode (and FSK shift)
  char line[32];
  if (mode == MODE_FSK) {
    char sh[20];
    formatShift(sh, sizeof(sh), true);
    snprintf(line, sizeof(line), "Mode: FSK %s", sh);
  } else {
    snprintf(line, sizeof(line), "Mode: %s", MODE_NAMES[mode]);
  }
  display.setTextSize(1);
  int16_t w = strlen(line) * 6;
  display.setCursor(w > SCREEN_WIDTH - textX ? SCREEN_WIDTH - w : textX, 52);
  display.print(line);

  if (!si5351Present) {
    display.setCursor(textX, 4);
    display.print("Si5351 not found!");
  }
}

void drawShiftScreen() {
  char buf[24];
  printCentered(shiftSel == SHIFT_VAR ? "FSK shift: VAR" : "FSK shift", 2, 1);
  formatShift(buf, sizeof(buf), false);
  printCentered(buf, 20, 2);
  printCentered(shiftSel == SHIFT_VAR ? "UP/DOWN: change" : "MODE: next shift", 42, 1);
  printCentered(shiftSel == SHIFT_VAR ? "MODE: next, hold: OK" : "hold MODE: OK", 54, 1);
}

void drawScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  if (ui == UI_SHIFT) {
    drawShiftScreen();
  } else {
    drawNormalScreen();
  }
  if (message && (int32_t)(messageUntil - millis()) > 0) {
    // short message over the bottom line
    display.fillRect(0, 50, SCREEN_WIDTH, 14, SSD1306_BLACK);
    printCentered(message, 54, 1);
  }
  display.display();
}

void showMessage(const char *msg, uint32_t ms) {
  message = msg;
  messageUntil = millis() + ms;
  drawScreen();
}

// Remove an expired message from the screen
void updateMessage() {
  if (message && (int32_t)(messageUntil - millis()) <= 0) {
    message = nullptr;
    drawScreen();
  }
}

void showSplash() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  printCentered("QubeRG v0.1b", 20, 1);
  printCentered("(c) 2026 PA3HCM", 36, 1);
  display.display();
}

// ---------------------------------------------------------------- actions

void changeFrequency(bool up) {
  uint32_t w = digitWeight(selectedDigit);
  // Adding/subtracting the weight carries into the next digit (19 -> 20)
  if (up && frequency <= FREQ_MAX - w) {
    frequency += w;
  } else if (!up && frequency >= FREQ_MIN + w) {
    frequency -= w;
  } else {
    return;
  }
  applyOutput();
  drawScreen();
}

// VAR shift: single press 1 Hz, held 10 Hz steps on multiples of 10
uint32_t nextShiftValue(uint32_t v, bool up, bool fast) {
  if (fast) {
    v = up ? (v / SHIFT_FAST_STEP + 1) * SHIFT_FAST_STEP
           : (v > SHIFT_FAST_STEP ? ((v - 1) / SHIFT_FAST_STEP) * SHIFT_FAST_STEP : SHIFT_MIN);
  } else {
    v = up ? v + 1 : (v > SHIFT_MIN ? v - 1 : SHIFT_MIN);
  }
  if (v < SHIFT_MIN) v = SHIFT_MIN;
  if (v > SHIFT_MAX) v = SHIFT_MAX;
  return v;
}

void changeShift(bool up, bool fast) {
  uint32_t v = nextShiftValue(varShift, up, fast);
  if (v != varShift) {
    varShift = v;
    applyOutput();
    drawScreen();
  }
}

void nextMode() {
  mode = (Mode)((mode + 1) % NUM_MODES);
  if (mode == MODE_BEACON && !beaconEnabled) mode = (Mode)((mode + 1) % NUM_MODES);
  restartPattern();
  drawScreen();
}

void nextShift() {
  shiftSel = (shiftSel + 1) % NUM_SHIFTS;
  applyOutput();
  drawScreen();
}

// UP / DOWN: on press (repeat = false) and while held (repeat = true)
void onUpDown(uint8_t id, bool repeat) {
  bool up = (id == BTN_UP);
  if (ui == UI_SHIFT) {
    if (shiftSel == SHIFT_VAR) changeShift(up, repeat);
  } else {
    changeFrequency(up);
  }
}

// DIGIT / MODE: released before LONG_PRESS_MS
void onShortPress(uint8_t id) {
  if (id == BTN_DIGIT) {
    if (ui == UI_NORMAL) {
      selectedDigit = (selectedDigit + 1) % NUM_DIGITS;
      drawScreen();
    }
  } else {   // BTN_MODE
    if (ui == UI_SHIFT) nextShift();
    else nextMode();
  }
}

// DIGIT / MODE: held for LONG_PRESS_MS
void onLongPress(uint8_t id) {
  if (id == BTN_DIGIT) {
    saveShift();
    showMessage("FSK shift saved", 1500);
  } else {   // BTN_MODE
    if (ui == UI_SHIFT) {
      ui = UI_NORMAL;
      drawScreen();
    } else if (mode == MODE_FSK) {
      ui = UI_SHIFT;
      drawScreen();
    }
  }
}

void pollButtons() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    Button &b = buttons[i];
    bool reading = (digitalRead(b.pin) == LOW);   // active low
    bool upDown = (i == BTN_UP || i == BTN_DOWN);

    if (reading != b.lastReading) {
      b.lastReading = reading;
      b.lastChange = now;
    }

    if ((now - b.lastChange) >= DEBOUNCE_MS && reading != b.stableState) {
      b.stableState = reading;
      if (b.stableState) {                       // pressed
        b.pressStart = now;
        b.longFired = false;
        b.nextRepeat = now + REPEAT_DELAY_MS;
        if (upDown) onUpDown(i, false);
      } else if (!upDown && !b.longFired) {      // released after a short press
        onShortPress(i);
      }
    }

    if (!b.stableState) continue;

    if (upDown) {
      // auto-repeat while held
      if ((int32_t)(now - b.nextRepeat) >= 0) {
        onUpDown(i, true);
        b.nextRepeat = now + (ui == UI_SHIFT ? SHIFT_RATE_MS : REPEAT_RATE_MS);
      }
    } else if (!b.longFired && now - b.pressStart >= LONG_PRESS_MS) {
      b.longFired = true;
      onLongPress(i);
    }
  }
}

// ---------------------------------------------------------------- setup / loop

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i] = {BUTTON_PINS[i], false, false, 0, 0, 0, false};
  }

  buildPatterns();
  if (mode == MODE_BEACON && !beaconEnabled) mode = MODE_OFF;
  loadShift();

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 not found");
  }
  showSplash();

  si5351Present = si5351.init(SI5351_CRYSTAL_LOAD_8PF, XTAL_FREQ, XTAL_CORRECTION);
  if (si5351Present) {
    si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
    si5351.output_enable(SI5351_CLK1, 0);
    si5351.output_enable(SI5351_CLK2, 0);
    restartPattern();
  } else {
    Serial.println("Si5351 not found");
  }

  delay(2000);
  drawScreen();
}

void loop() {
  pollButtons();
  updatePattern();   // no delay here, keeps the keying timing accurate
  updateMessage();
}
