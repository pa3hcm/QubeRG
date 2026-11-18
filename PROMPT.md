# PROMPT

This project was generated with the help of an AI model.

- **AI model:** Claude Opus 5.5 (model ID `claude-opus-5-5`) by Anthropic
- **Tool:** Claude Code (Claude desktop app, Code tab)
- **Date:** 2026-11-11

## Full prompt

> I want to create an RF signal generator, using the following components:
>
> * ESP32 devboard
> * SI5351 dds board with 25MHz crystal and 3 SMA outputs and I2C connection
> * GM009605v4 0.96 inch oled display with I2C connection
> * Small board with 4 pushbuttons with a common ground connection
>
> The display should start saying "sigblock v0.1b" and "(c) 2026 PA3HCM".
> After 2 seconds it should display "10.000.000", indicating the output is 10 MHz exactly. Just below the 1 should be a cursor, indicating that this digit is currently selected.
>
> The 4 buttons are numbered 1 to 4.
>
> * Button 1: increase the selected digit. If going from 9 to 10/0, upper the next digit 1 as well, for example, upping the right digit of '19' will not go back to '10' but to '20'.
> * Button 2: decrease the selected digit. It work the same as button 1, but then counting down.
> * Button 3: select the next digit. When on the last digit, move back to the first digit.
> * Button 4: Not used yet, will work on this later.
>
> Whenever the number on the display changes, the output frequency of the SI5351 should be changed to the displayed value.
>
> Create an image that displays the wiring between the boards.
>
> All boards are running on 3.3 volt.
>
> Store all code in a folder named 'cubegen'. Also create a file PROMPT.md with the used AI model and version and a full prompt representing this conversation. I will use VScode + PlatformIO to program the ESP32.

### Follow-up 1

> The cursor indicates the selected digit.

(Confirmed: this was already implemented, no change.)

### Follow-up 2

> Button 4 sets the mode. The following modes are available:
>
> * The default mode is `CW` which sends a continuous carrier
> * The next mode is `CW-keyed`, which sends a CW signal -.-.-. etc...
> * The next mode is `FSK470`, where the signal is alternating 500ms on the given frequency and 500ms on a frequency that is 470Hz higher
> * The next mode is `FSK850`, same as FSK470 but with 850Hz shift
> * The next mode is `FSK1K`, same as FSK470 bith with 1kHz shift
> * FInal mode is `OFF`, were no signal is generated.
>
> The display should show 'Mode: ' followed by the selected mode

### Follow-up 3

> Add a new mode `FSK10K`with 10kHz shift.

### Follow-up 4

> Change the default frequency from 10.000MHz to 10.100MHz. Set the default mode to `FSK10K`

### Follow-up 5

> How fast can you switch the SI5351 to another frequency? I try to key it every 100ms, but I don't know how realistic that is?

(Answered only, no code change: below 100 MHz a frequency change only rewrites the multisynth registers, < 1 ms.)

### Follow-up 6

> Change any reference to 'sigblock' to 'cubeGen'

(The original prompt above is kept verbatim; the splash screen then read "cubeGen v0.1b", later renamed to "QubeRG v0.1b".)

### Follow-up 7

> Set the default mode to FSK1K

### Follow-up 8

> Set the default frequency to 10.2 MHz

### Follow-up 9

> Set de default mode to CWkeyed

### Follow-up 10

> Set the default mode to `OFF`and the default frequency to 10.000 MHz.

### Follow-up 11

> Create a new repository on my Github (account: 'pa3hcm'). Push all code to this repository. You can use the github client on my laptop.

(Created public repository https://github.com/pa3hcm/cubegen, later renamed to https://github.com/pa3hcm/QubeRG.)

### Follow-up 12

> Apply the following license: Creative Commons Attribution-NonCommercial 4.0 International License

### Follow-up 13

> The software is ok for now. The next thing to do is designing a case. I have a 3D-printer (FDM) and want to print a case with PETG. The case has the shape of a cube. You should be able to open it, maybe the cube is in 2 halfs which you can click together. On top there will be the RF output, based on an SMA thru-hole screw connector F-F. The SI5351 board is screwed on the inside, using a M-M SMA adaptor. On one of the sides is the .96' display, on the opposite side are the buttons. On the bottom of the cube lays the ESP32 board, in such a way that the USB connector can be reached from the outside to power the whole unit. All boards should be mounted in such a way that they "click" in place or get locked in by closing the cube. Please generate the STL's and add them to the repository.

### Follow-up 14

> Rename the repository (and the current folder where it is stored) to `QubeRG`.

### Follow-up 15

> Also render a image to show the product as it would look like when built. Include this image in the introduction text of the README.md.

### Follow-up 16

> Search all code, text, drawings, etc for the name 'CubeGen' and replace that name with `'QubeRG'`

(Quoted prompts in this file are kept verbatim.)

### Follow-up 17

> The SMA connector should be in the center of the top panel

### Follow-up 18

> Also mark the buttons. Put up and down symbols above button 1 and 2, 'D' above button 3 (for 'Digit') and 'M' above button 4 (for 'Mode').

### Follow-up 19

> What are the dimensions of the box?

(Answered: 90 x 90 x 90 mm outside, 3 mm walls.)

### Follow-up 20

> I think that's too big. can you make it 70 x 70 x 70?

(Tested: 70 mm does not fit because the Si5351 hangs on the SMA chain right above the ESP32; 80 mm only fits with wires soldered to the ESP32.)

### Follow-up 21

> Keep 90 mm

### Follow-up 22

> Push

### Follow-up 23

> Write a nice introduction that explains what this device is, what its applications are and why I choose this name (cube shape, QRG Q-code for frequency). Add a mode "beacon", this mode transmits a callsign (default value 'N0CALL') and should be enabled in the code by changing the callsign to any other value. The mode allows the device to act as a beacon transmitter, sending the callsign in morse code (12 wpm) and then sends a carrier for 60 seconds before repeating the call and another carrier (loop). So also update the code.
> The FSK470 and FSK850 should be changed to a 50 baud RTTY signal, sending 'RYRYRYRYRY' continuously. The FSK1K and FSK10K modes should iterate on 1 Hz.

### Follow-up 24

> The FSK470 is wrong. This should be FSK170, which has a 170 Hz spacing. Then there is FSK425, the same but now 425 Hz spacing. Forget the RTTY modulation I mentioned earlier, apply the 1Hz alternating instead.

### Follow-up 25

> Actually I don't really know what are useful FSK spacings, apart from the RTTY shifts of 170, 425 and 850. Any other suggestions for radio testing/measurement purposes?

(Answered with suggestions; no change.)

### Follow-up 26

> Let's change the interface. You simply choose FSK with the MODE button. When holding the MODE button for a second, you can switch between 170, 425, 850 and VAR. When VAR is selected, the UP/DOWN buttons are used to change the shift. The initial shift is 1000 Hz. Holding the UP/DOWN keys increase/decrease the frequency fast, about 100Hz/second in 10 Hz steps. Hold MODE again for normal operation. The shift can be set in Hz, from 1 to 10000 Hz. Holding the DIGIT button will store the selected shift in the 'Preferences' memory of the ESP32, so it survives an power outage or reboot. When in FSK VAR mode, the spacing is displayed as well.

### Follow-up 27

> Can you generate an image that shows screenshots of all possible modes and settings, and arrows between them with instructions how to get to the next screen (or go back to the main "loop").

### Follow-up 28

> The button board is 38 x 16 mm, the top of each button is 8.0 mm above the board.

### Follow-up 29

> the centre-to-centre spacing of the buttons is 7.3 mm

### Follow-up 30

> The SI5351 board dimensions are 30.2 x 21.6 mm.

### Follow-up 31

> Can you make a view of how the SI5351 board is mounted on the top lid using the SMA thru-hole connector and a M-M adaptor?

### Follow-up 32

> On the new picture CLK1 is connected, however we will use CLK0. Please change this.

### Follow-up 33

> Text, lines and diagrams are covering each other, can you fix that?

### Follow-up 34

> Keep the exploded view, but remove the assembled view.

### Follow-up 35

> The whole in the lid for the SMA thruhole connector must be changed. It has a diameter of 7.9 mm. It has two flat sides, the distance between them is 7.5mm. These connectors come in different shapes and sizes, so maybe it's better you can change the script to generate it so you can simply regenerate the lid stl.

### Follow-up 36

> Since you already add clearance, fix the current sizes of the sma hole in the lid to 7.8mm diameter and 7.4mm between the flat sides.

### Follow-up 37

> Commit and push

### Follow-up 38

> The size of the ESP32 board (PCB) is 51.5 x 28.4 mm.

### Follow-up 39

> The OLED display board (PCB) is 27.3 x 24.8 mm

### Follow-up 40

> The SI5351 board and the M-M sma adaptor together measure 46.6 mm

### Follow-up 41

> The distance between the lid and the top of the m-m sma adaptor is about 8mm

### Follow-up 42

> Can you update docs/ubeRG.png to include a small rubberduck antenna on top of it, maybe 8 cm high, just for illustration purpose.

(docs/qubeRG.png was meant.)

### Follow-up 43

> In README.md, align qubeRG.png to the right, so it gets positioned along the intro text

## Design decisions made by the AI

- I2C bus shared by Si5351 (0x60) and OLED (SSD1306, 0x3C) on GPIO21 (SDA) / GPIO22 (SCL).
- Buttons on GPIO32, GPIO33, GPIO25, GPIO26 using the ESP32 internal pull-ups (buttons switch to GND).
- RF output on Si5351 CLK0; CLK1 and CLK2 disabled.
- Display shows 8 digits (`XX.XXX.XXX` Hz), so the range is limited to 8 kHz … 99.999.999 Hz.
  Steps that would leave this range are ignored.
- Buttons 1 and 2 auto-repeat when held down.
- Power-up defaults: 10.000.000 Hz, mode OFF, cursor on the first digit.
- Button 4 cycles CW → CW-keyed → FSK → Beacon → OFF → CW.
- CW-keyed repeats "-." (dash 300 ms, gap 100 ms, dot 100 ms, gap 100 ms), i.e. 12 WPM.
- FSK modes start on the displayed frequency, then the shifted (higher) frequency.
- Keying is done with the Si5351 output enable, so the oscillator keeps running.
- "Mode: <mode>" is shown on the bottom line of the display; a missing Si5351 warning moved to the top line.
- License: CC BY-NC 4.0; official legal code in `LICENSE`, notice in README, `main.cpp` and `platformio.ini`.
- Case: 90 mm cube, generated by a parametric Python script (`case/qube_case.py`, manifold3d) because OpenSCAD was not available; bottom part + top part with tongue and snap ridges.
- Case: ESP32 lies component side up with pins down, 20 mm above the floor for dupont plugs, held by 4 snap hooks at the PCB corners; USB on the left wall.
- Case: display and button board slide into rails and are held down by fingers on the lid; printed button caps with a flange.
- Case: SMA bulkhead in the centre of the top; the Si5351 hangs below it parallel to the front wall, about 3 mm above the ESP32 module.
- Case: buttons engraved ▲ ▼ D M (button 1..4).
- Case: all board sizes are measured now: display 27.3 x 24.8 mm (position of the visible area still estimated); the ESP32 is measured: 51.5 x 28.4 mm; the Si5351 is measured: 30.2 x 21.6 mm (fit checked for CLK0 on any of the 3 SMA positions); the button board is measured: 38 x 16 mm, switch top 8.0 mm above the PCB, 7.3 mm spacing.
- Case: button caps 5 mm with a rectangular 6.9 x 9 mm flange (round 9 mm flanges would overlap at 7.3 mm spacing), holes 5.6 mm.
- Product render and inside view rendered with matplotlib (`case/qube_preview.py`).
- All timed modes are step tables played from loop() with micros() timing (no drift; no delay() in loop).
- One FSK mode, alternating at 1 Hz (500 ms on f, 500 ms on f + shift). Shift 170 / 425 / 850 / VAR (1–10000 Hz, start 1000). (RTTY was tried and removed again.)
- Shift setting: hold MODE (1 s) in FSK to open; short MODE cycles the shifts; UP/DOWN change VAR by 1 Hz, held 10 Hz steps every 100 ms on multiples of 10; hold MODE to close. The FSK signal keeps running while setting.
- DIGIT and MODE act on release (short) or after 1 s (long); hold DIGIT saves shift selection and VAR value in ESP32 Preferences (namespace "quberg"), also outside the shift setting. Holding MODE outside FSK does nothing.
- Beacon: callsign in morse at 12 WPM (100 ms dot), word gap, 60 s carrier, word gap, repeat. Mode order CW, CW-keyed, FSK, Beacon, OFF; Beacon is skipped while CALLSIGN is "N0CALL".
- Screen diagram (`docs/screens.png`): the firmware runs on the PC with stand-in hardware and the real Adafruit GFX 5x7 font, so the screenshots match the real display (`tools/screenflow/`).
- SMA mount view (`docs/sma_mount.png`, `case/qube_sma_view.py`): exploded view only, labels placed automatically in a column beside the parts so text and leader lines never overlap; connector sizes are typical values. CLK0 is the jack at one end of the Si5351 board (10 mm pitch), so the board hangs off-centre under the SMA; the fit check passes with CLK0 at either end.
- SMA hole: round with two flats (7.8 mm, 7.4 mm across the flats) + 0.2 mm clearance, so 8.0 x 7.6 mm, flats parallel to the front. Settings SMA_HOLE_D / _FLATS / _CLEAR / _ANGLE, also as command-line options (`--sma-d`, `--sma-flats`, `--sma-clear`, `--sma-angle`), with `--only top` to regenerate just the lid.
- Si5351 hanging height: board bottom edge to top of the M-M adaptor = 46.6 mm (measured, SI_STACK) plus about 8 mm from the inside of the top to the adaptor (measured, SMA_ADAPTOR_GAP). The board edge hangs 33 mm below the top, 4.6 mm above the ESP32; the fit check passes for gaps up to 10 mm.
- Product image shows an illustrative 8 cm rubber duck antenna on the SMA connector (render only, not part of the case).
- Libraries: Etherkit Si5351, Adafruit SSD1306, Adafruit GFX.
