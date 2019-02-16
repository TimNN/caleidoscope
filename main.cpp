#include <Kaleidoscope.h>
#include <Kaleidoscope-HostPowerManagement.h>
#include <Kaleidoscope-LEDControl.h>
#include <Kaleidoscope-LEDEffect-SolidColor.h>

enum { DVORAK, SPECIAL };

// FUTURE: Key_Escape, Key_Enter, Key_Tab

/* *INDENT-OFF* */
KEYMAPS(
[DVORAK] = KEYMAP_STACKED
(
        Key_NoKey, Key_NoKey,     Key_NoKey, Key_NoKey,  Key_NoKey, Key_NoKey, Key_LEDEffectNext,
        Key_NoKey, Key_Quote,     Key_Comma, Key_Period, Key_P,     Key_Y,     Key_NoKey,
        Key_Tab,   Key_A,         Key_O,     Key_E,      Key_U,     Key_I,
        Key_NoKey, Key_Semicolon, Key_Q,     Key_J,      Key_K,     Key_X,     Key_NoKey,

        Key_NoKey, Key_LeftShift, Key_Backspace, Key_NoKey,
        Key_LeftShift,
        // Key_NoKey, OSM(LeftShift), Key_Backspace, Key_NoKey,
        // OSM(LeftShift),

        Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey,
        Key_NoKey, Key_F,     Key_G,     Key_C,     Key_R,     Key_L,     Key_NoKey,
        Key_D,     Key_H,     Key_T,     Key_N,     Key_S,     Key_Enter,
        Key_NoKey, Key_B,     Key_M,     Key_W,     Key_V,     Key_Z,     Key_NoKey,

        Key_NoKey, ShiftToLayer(SPECIAL), Key_Spacebar, Key_NoKey,
        ShiftToLayer(SPECIAL)
        // Key_NoKey, OSL(SPECIAL), Key_Spacebar, Key_NoKey,
        // OSL(SPECIAL)
),
[SPECIAL] = KEYMAP_STACKED
(
        Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey,     Key_NoKey,
        Key_NoKey, ___,       ___,       ___,       Key_Slash, Key_Backslash, Key_NoKey,
        ___,       Key_1,     Key_2,     Key_3,     Key_4,     Key_5,
        Key_NoKey, ___,       Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey,     Key_NoKey,

        Key_NoKey, ___, ___, Key_NoKey,
        Key_NoKey,

        Key_NoKey, Key_NoKey,    Key_NoKey, Key_NoKey,  Key_NoKey,       Key_NoKey,        Key_NoKey,
        Key_NoKey, Key_Backtick, Key_Minus, Key_Equals, Key_LeftBracket, Key_RightBracket, Key_NoKey,
        Key_6,        Key_7,     Key_8,      Key_9,           Key_0,            ___,
        Key_NoKey, Key_NoKey,    Key_NoKey, Key_NoKey,  Key_NoKey,       Key_NoKey,        Key_NoKey,

        Key_NoKey, Key_NoKey, ___, Key_NoKey,
        ___
),
)
/* *INDENT-ON* */

static kaleidoscope::plugin::LEDSolidColor ledSolid(255, 0, 219);

void hostPowerManagementEventHandler(kaleidoscope::plugin::HostPowerManagement::Event event) {
    switch (event) {
        case kaleidoscope::plugin::HostPowerManagement::Suspend:
            kaleidoscope::plugin::LEDControl::set_all_leds_to({0, 0, 0});
            kaleidoscope::plugin::LEDControl::syncLeds();
            kaleidoscope::plugin::LEDControl::paused = true;
            break;
        case kaleidoscope::plugin::HostPowerManagement::Resume:
            kaleidoscope::plugin::LEDControl::paused = false;
            kaleidoscope::plugin::LEDControl::refreshAll();
            break;
        case kaleidoscope::plugin::HostPowerManagement::Sleep:
            break;
    }
}


KALEIDOSCOPE_INIT_PLUGINS(
        LEDControl, HostPowerManagement, LEDOff, ledSolid);

void setup() {
    Kaleidoscope.setup();

    // ledSolid.activate();
}

void loop() {
    Kaleidoscope.loop();
}