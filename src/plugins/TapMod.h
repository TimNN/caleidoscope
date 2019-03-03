#pragma once

#include <kaleidoscope/plugin.h>
#include <Kaleidoscope-Ranges.h>
#include <kaleidoscope/key_defs.h>

#define Key_TapMod01 Key(kaleidoscope::ranges::KALEIDOSCOPE_SAFE_START + 1)
#define Key_TapMod02 Key(kaleidoscope::ranges::KALEIDOSCOPE_SAFE_START + 2)
#define Key_TapMod03 Key(kaleidoscope::ranges::KALEIDOSCOPE_SAFE_START + 3)
#define Key_TapMod04 Key(kaleidoscope::ranges::KALEIDOSCOPE_SAFE_START + 4)

namespace custom {

using namespace kaleidoscope;

typedef unsigned long ts_millis_t;

class TapMod : public Plugin {
  friend class TapModTest;

  public:
    static void setActual(size_t idx, Key actual);

    EventHandlerResult beforeEachCycle();
    EventHandlerResult onKeyswitchEvent(Key &mappedKey, uint8_t row, uint8_t col, uint8_t keyState);
    EventHandlerResult beforeReportingState();

    enum class State : uint8_t {
      /// Inactive, waiting for press.
      IDLE = 0,
      /// Actual key pressed, waiting for key release or timeout.
      PRESSED_IDLE,
      /// Actual key, followed by one real key pressed, waiting for key release or timeout.
      PRESSED_PRE_QUEUE,
      /// Key tapped (already released), but no actual key pressed yet.
      PRESSED_DELAYED,
      /// Key was really pressed (longer than tap timeout).
      PRESSED_REAL,
      /// Key should be released in this cycle.
      RELEASE_THIS_CYCLE,
      /// Key presses are being queued for later.
      QUEUING,
    };

    struct Entry {
      ts_millis_t pressed_ts;
      Key actual_key;
      uint8_t src_row;
      uint8_t src_col;
      State state;
    };

    struct QueueItem {
      Key key;
      uint8_t pos_addr;
      uint8_t key_state;
    };

  private:
    static const size_t QUEUE_MAX = 16;
    static const size_t ENTRY_CNT = 4;

    static const ts_millis_t TAP_TIME_MS = 180;
    static const ts_millis_t ACTIVE_TIME_MAX_MS = 320;

    static Entry entries[ENTRY_CNT];

    static QueueItem queue[QUEUE_MAX];
    static size_t queue_head;
    static size_t queue_len;

    static boolean real_key_down_this_cycle;
    /// Listening for a real key down.
    static bool listening;
    /// Waiting for a timeout.
    static bool waiting;
    /// Injecting keys.
    static bool injecting;
    static uint8_t queuing;


    static bool isTapModKey(Key key);

    static bool shouldSkipKey(Key key);

    static uint8_t find_last_queue_state(Key key, uint8_t pos_addr);

    static void queue_key(Key key, uint8_t pos_addr, uint8_t key_state);

    // For friendly test.
    static void reset();
};

}

extern custom::TapMod TapMod;



