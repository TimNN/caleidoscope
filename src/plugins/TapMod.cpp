#include <kaleidoscope/keyswitch_state.h>
#include <TapMod.h>
#include <kaleidoscope/addr.h>

using namespace kaleidoscope;

namespace custom {

TapMod::Entry TapMod::entries[ENTRY_CNT] = { 0 };
TapMod::QueueItem TapMod::queue[QUEUE_MAX] = { 0 };
size_t TapMod::queue_len = 0;
size_t TapMod::queue_head = 0;
uint8_t TapMod::waiting = 0;
uint8_t TapMod::queuing_entries = 0;

void TapMod::setActual(size_t idx, Key actual) {
  if (idx < ENTRY_CNT) {
    entries[idx].actual_key = actual;
  }
}

EventHandlerResult TapMod::beforeEachCycle() {
  if (waiting == 0) {
    return EventHandlerResult::OK;
  }

  // Require entries to re-register to be more resilient.
  waiting = 0;

  ts_millis_t ms = millis();

  for (size_t entry_idx = 0; entry_idx < ENTRY_CNT; entry_idx++) {
    TapMod::Entry& entry = entries[entry_idx];

    switch (entry.state) {
      case PRESSED_IDLE:
        if (ms - entry.pressed_ts <= TapMod::TAP_TIME_MS) {
          waiting |= 1U << entry_idx;
        } else {
          entry.state = PRESSED_REAL;
        }
        break;
      case PRESSED_DELAYED:
        if (ms - entry.pressed_ts <= TapMod::ACTIVE_TIME_MAX_MS) {
          waiting |= 1U << entry_idx;
        } else {
          entry.state = RELEASE_NEXT_00;
        }
      default:
        break;
    }
  }

  return EventHandlerResult::OK;
}

EventHandlerResult TapMod::onKeyswitchEvent(Key &mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  if (queuing_entries == 0) {
    if (isTapModKey(mappedKey)) {
      size_t entry_idx = mappedKey.raw - Key_TapMod01.raw;
      TapMod::Entry& entry = entries[entry_idx];

      if (keyToggledOn(keyState)) {
        switch (entry.state) {
          case IDLE:
            entry.state = PRESSED_IDLE;
            waiting |= 1u << entry_idx;

            entry.pressed_ts = millis();
            mappedKey = entry.actual_key;
            return EventHandlerResult::OK;
          default:
            return EventHandlerResult::ERROR;
        }
      }

      if (keyToggledOff(keyState)) {
        switch (entry.state) {
          case PRESSED_IDLE:
            // Time-based state transitions are handled earlier.
            entry.state = TapMod::PRESSED_DELAYED;
            return EventHandlerResult::EVENT_CONSUMED;
          case PRESSED_REAL:
            entry.state = TapMod::IDLE;
            mappedKey = entry.actual_key;
            return EventHandlerResult::OK;
          default:
            return EventHandlerResult::ERROR;
        }
      }

      if (keyWasPressed(keyState) && keyIsPressed(keyState)) {
        switch (entry.state) {
          case PRESSED_IDLE:
          case PRESSED_REAL:
            mappedKey = entry.actual_key;
            return EventHandlerResult::OK;
          default:
            return EventHandlerResult::ERROR;
        }
      }

      return EventHandlerResult::ERROR;
    }

    // TODO: FIXME properly
    return EventHandlerResult::OK;
  }

  return EventHandlerResult::ERROR;
}

EventHandlerResult TapMod::beforeReportingState() {
  for (auto& entry : entries) {
    switch (entry.state) {
      case PRESSED_DELAYED:
        handleKeyswitchEvent(entry.actual_key, entry.src_row, entry.src_col, IS_PRESSED | WAS_PRESSED);
        break;
      case RELEASE_NEXT_00:
        handleKeyswitchEvent(entry.actual_key, entry.src_row, entry.src_col,  WAS_PRESSED);
        entry.state = IDLE;
        break;
      default:
        break;
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
  waiting = 0;
  queuing_entries = 0;
}

}

custom::TapMod TapMod;