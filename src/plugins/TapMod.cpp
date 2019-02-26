#include <kaleidoscope/keyswitch_state.h>
#include <TapMod.h>
#include <kaleidoscope/addr.h>

using namespace kaleidoscope;

namespace custom {

TapMod::Entry TapMod::entries[ENTRY_CNT] = { 0 };
TapMod::QueueItem TapMod::queue[QUEUE_MAX] = { 0 };
size_t TapMod::queue_len = 0;
size_t TapMod::queue_head = 0;
uint8_t TapMod::real_key_waiters = 0;
uint8_t TapMod::queuing_entries = 0;

void TapMod::setActual(size_t idx, Key actual) {
  if (idx < ENTRY_CNT) {
    entries[idx].actual_key = actual;
  }
}

EventHandlerResult TapMod::onKeyswitchEvent(Key &mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  if (isTapModKey(mappedKey)) {
    size_t entry_idx = mappedKey.raw - Key_TapMod01.raw;
    TapMod::Entry& entry = entries[entry_idx];

    if (keyToggledOn(keyState)) {
      // TODO: not idle
      // TODO: if queueing

      entry.state = PRESSED_IDLE;
      real_key_waiters |= 1u << entry_idx;

      entry.pressed_ts = millis();
      mappedKey = entry.actual_key;
      return EventHandlerResult::OK;
    }

    if (keyToggledOff(keyState)) {

    }

    // TODO: return
  }

  if (queuing_entries != 0) {
    byte pos_addr = addr::addr(row, col);

    uint8_t q_key_state = find_last_queue_state(mappedKey, pos_addr);

    if (q_key_state != keyState) {
      queue_key(mappedKey, pos_addr, keyState);
    }

    return EventHandlerResult::EVENT_CONSUMED;
  }

  if (real_key_waiters != 0 && keyToggledOn(keyState) && !shouldSkipKey(mappedKey)) {
      // TODO: switch states
  }

  return EventHandlerResult::OK;
}

EventHandlerResult TapMod::beforeReportingState() {
  for (auto entry : entries) {
    if (entry.state == PRESSED_DELAYED) {
      handleKeyswitchEvent(entry.actual_key, entry.src_row, entry.src_col, IS_PRESSED | WAS_PRESSED);
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
  real_key_waiters = 0;
  queuing_entries = 0;
}

}

custom::TapMod TapMod;