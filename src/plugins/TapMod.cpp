#include <kaleidoscope/keyswitch_state.h>
#include <kaleidoscope/addr.h>
#include <kaleidoscope/hid.h>
#include "TapMod.h"

using namespace kaleidoscope;

namespace custom {

TapMod::Entry TapMod::entries[ENTRY_CNT] = { 0 };
TapMod::QueueItem TapMod::queue[QUEUE_MAX] = { 0 };
size_t TapMod::queue_len = 0;
size_t TapMod::queue_head = 0;
bool TapMod::real_key_down_this_cycle = false;
bool TapMod::listening = false;
bool TapMod::waiting = false;
bool TapMod::injecting = false;
uint8_t TapMod::queuing = 0;

void TapMod::setActual(size_t idx, Key actual) {
  if (idx < ENTRY_CNT) {
    entries[idx].actual_key = actual;
  }
}

EventHandlerResult TapMod::beforeEachCycle() {
  real_key_down_this_cycle = false;

  if (!waiting) {
    return EventHandlerResult::OK;
  }

  waiting = false;

  ts_millis_t ms = Kaleidoscope_::millisAtCycleStart();

  for (size_t entry_idx = 0; entry_idx < ENTRY_CNT; entry_idx++) {
    Entry& entry = entries[entry_idx];

    switch (entry.state) {
      case State::PRESSED_IDLE:
      case State::PRESSED_PRE_QUEUE:
        if (ms - entry.pressed_ts <= TAP_TIME_MS) {
          waiting = true;
        } else {
          entry.state = State::PRESSED_REAL;
        }
        break;
      case State::PRESSED_DELAYED:
        if (ms - entry.pressed_ts <= ACTIVE_TIME_MAX_MS) {
          waiting = true;
        } else {
          entry.state = State::RELEASE_THIS_CYCLE;
        }
      default:
        break;
    }
  }

  return EventHandlerResult::OK;
}

EventHandlerResult TapMod::onKeyswitchEvent(Key &mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  if (queuing == 0) {
    if (isTapModKey(mappedKey)) {
      size_t entry_idx = mappedKey.raw - Key_TapMod01.raw;
      Entry& entry = entries[entry_idx];

      if (keyToggledOn(keyState)) {
        switch (entry.state) {
          case State::IDLE:
            entry.state = State::PRESSED_IDLE;
            listening = true;
            waiting = true;

            entry.pressed_ts = Kaleidoscope_::millisAtCycleStart();
            mappedKey = entry.actual_key;
            return EventHandlerResult::OK;
          default:
            return EventHandlerResult::ERROR;
        }
      }

      if (keyToggledOff(keyState)) {
        switch (entry.state) {
          case State::PRESSED_IDLE:
            // Time-based state transitions are handled earlier.
            entry.state = State::PRESSED_DELAYED;
            injecting = true;
            return EventHandlerResult::EVENT_CONSUMED;
          case State::PRESSED_PRE_QUEUE:
            // Time-based state transitions are handled earlier, so this is the same as
            // PRESSED_REAL.
          case State::PRESSED_REAL:
            entry.state = State::IDLE;
            mappedKey = entry.actual_key;
            return EventHandlerResult::OK;
          default:
            return EventHandlerResult::ERROR;
        }
      }

      if (keyWasPressed(keyState) && keyIsPressed(keyState)) {
        switch (entry.state) {
          case State::PRESSED_IDLE:
          case State::PRESSED_PRE_QUEUE:
          case State::PRESSED_REAL:
            mappedKey = entry.actual_key;
            return EventHandlerResult::OK;
          default:
            return EventHandlerResult::ERROR;
        }
      }

      return EventHandlerResult::ERROR;
    }

    if (listening && keyToggledOn(keyState) && !shouldSkipKey(mappedKey)) {
      real_key_down_this_cycle = true;
    }
    return EventHandlerResult::OK;
  }

  return EventHandlerResult::ERROR;
}

EventHandlerResult TapMod::beforeReportingState() {
  if (injecting) {
    injecting = false;

    for (size_t entry_idx = 0; entry_idx < ENTRY_CNT; entry_idx++) {
      Entry& entry = entries[entry_idx];

      switch (entry.state) {
        case State::PRESSED_DELAYED:
          handleKeyswitchEvent(entry.actual_key, entry.src_row, entry.src_col, IS_PRESSED | WAS_PRESSED);
          injecting = true;
          break;
        case State::RELEASE_THIS_CYCLE:
          handleKeyswitchEvent(entry.actual_key, entry.src_row, entry.src_col, WAS_PRESSED);
          entry.state = State::IDLE;
          break;
        default:
          break;
      }
    }
  }

  if (listening && real_key_down_this_cycle) {
    listening = false;

    for (size_t entry_idx = 0; entry_idx < ENTRY_CNT; entry_idx++) {
      Entry& entry = entries[entry_idx];

      switch (entry.state) {
        case State::PRESSED_IDLE:
          entry.state = State::PRESSED_PRE_QUEUE;
          break;
        case State::PRESSED_PRE_QUEUE:
          return EventHandlerResult::ERROR;
        case State::PRESSED_DELAYED:
          hid::sendKeyboardReport();
          handleKeyswitchEvent(entry.actual_key, entry.src_row, entry.src_col, WAS_PRESSED);
          entry.state = State::IDLE;
          break;
        default:
          break;
      }
    }
  }

  return EventHandlerResult::OK;
}

bool TapMod::isTapModKey(Key key) {
  return (Key_TapMod01 <= key && key <= Key_TapMod04);
}

bool TapMod::shouldSkipKey(Key _key) {
  return false;
}

uint8_t TapMod::find_last_queue_state(Key key, uint8_t pos_addr) {
  for (size_t i = queue_len; i > 0;) {
    i -= 1;
    QueueItem& item = queue[(queue_head + i) % QUEUE_MAX];
    if (item.key == key && item.pos_addr == pos_addr) {
      return item.key_state;
    }
  }
  return 0;
}

void TapMod::queue_key(Key key, uint8_t pos_addr, uint8_t key_state) {
  if (queue_len >= QUEUE_MAX) return;
  QueueItem& item = queue[(queue_head + queue_len) % QUEUE_MAX];
  item.key = key;
  item.pos_addr = pos_addr;
  item.key_state = key_state;
  queue_len += 1;
}

void TapMod::reset() {
  memset(entries, 0, sizeof(entries));
  memset(queue, 0, sizeof(queue));
  queue_len = 0;
  queue_head = 0;
  listening = 0;
  waiting = 0;
  injecting = 0;
  queuing = 0;
}

}

custom::TapMod TapMod;
