#include <kaleidoscope/addr.h>
#include <Kaleidoscope.h>
#include "IQueue.h"

using namespace kaleidoscope;

namespace custom {

IQueue::QWord IQueue::queue[QUEUE_SIZE] =  { 0 };
size_t IQueue::queue_head = 0;
size_t IQueue::queue_len = 0;

IQueue::Flag IQueue::flags[ROWS * COLS];
Key IQueue::key_overrides[ROWS * COLS];

IQueue::State IQueue::state = IQueue::State::IDLE;
IQueueShouldStop IQueue::stop_fn = nullptr;
ts_millis_t IQueue::deadline = 0;
bool IQueue::should_stop = false;
bool IQueue::did_update = false;
bool IQueue::should_record_cycle = false;

#ifdef CAL_TEST
bool IQueue::stop_after_record = false;
#endif

EventHandlerResult IQueue::beforeEachCycle() {
  switch (state) {
    case State::IDLE:
    case State::REPLAY:
      return EventHandlerResult::OK;
    case State::PREPARING:
      break;
    default:
      return EventHandlerResult::ERROR;
  }

  state = State::RECORD;
  memset(flags, 0, sizeof(flags));

  ts_millis_t base_ts = millis();

  bool needs_new_ts = true;

  while (true) {
    ts_millis_t ts = millis();
    if (ts > deadline) { break; }

    if (needs_new_ts) {
      q_push({ .raw = (millis_offset_t)(ts - base_ts) });
    } else {
      q_peek_last().raw = (millis_offset_t)(ts - base_ts);
    }

    should_stop = false;
    did_update = false;
    should_record_cycle = false;

    // Will update global state.
    KeyboardHardware.scanMatrix();

    if (should_record_cycle) {
      if (did_update) {
        // There was at least one update, so set the flag on that.
        q_peek_last().is_last_update = true;
      } else {
        // No explicit updates, set the flag.
        q_peek_last().no_explicit_updates = true;
      }
      // In any case, a cycle was recorded so need a new ts.
      needs_new_ts = true;
    } else {
      // Cycle should not be recorded, reuse the ts.
      needs_new_ts = false;
    }

    if (should_stop) {
      break;
    }
  }

#ifdef CAL_TEST
  if (stop_after_record) {
    return EventHandlerResult::OK;
  }
#endif

  memset(flags, 0, sizeof(flags));

  state = State::REPLAY;

  while (queue_len > 0) {
    QWord cycle_info = q_pop();
    Kaleidoscope_::setMillisAtCycleStart(base_ts + cycle_info.millis_offset);

    kaleidoscope::Hooks::beforeEachCycle();

    // Perform any explicit updates.
    if (!cycle_info.no_explicit_updates) {
      while (true) {
        QWord update = q_pop();

        Flag& flag = flags[update.pos_idx];
        flag.replay_key_state = update.key_state;

        // Once we have the first key down event, we stop using key overrides.
        if (keyToggledOn(update.key_state)) {
          flag.replay_no_key_override = true;
        }

        if (update.is_last_update) {
          break;
        }
      }
    }

    // Trigger the key events.
    for (uint8_t idx = 0; idx < ROWS * COLS; idx++) {
      Flag& flag = flags[idx];

      if (flag.raw != 0 && flag.replay_key_state != 0) {
        handleKeyswitchEvent(
            flag.replay_no_key_override ? Key_NoKey : key_overrides[idx],
            kaleidoscope::addr::row(idx), kaleidoscope::addr::col(idx),
            flag.replay_key_state);

        if (keyToggledOn(flag.replay_key_state)) {
          flag.replay_key_state = WAS_PRESSED | IS_PRESSED;
        } else if (keyToggledOff(flag.replay_key_state)) {
          flag.replay_key_state = 0;
        }
      }
    }

    kaleidoscope::Hooks::beforeReportingState();

    kaleidoscope::hid::sendKeyboardReport();
    kaleidoscope::hid::releaseAllKeys();

    kaleidoscope::Hooks::afterEachCycle();
  }

  state = State::IDLE;

  // Continue with an actual cycle.
  Kaleidoscope_::setMillisAtCycleStart(millis());
  return EventHandlerResult::OK;
}

EventHandlerResult IQueue::onKeyswitchEvent(kaleidoscope::Key &mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  switch (state) {
    case State::IDLE:
    case State::REPLAY:
    case State::PREPARING:
      return EventHandlerResult::OK;
    case State::RECORD:
      break;
    default:
      return EventHandlerResult::ERROR;
  }

  should_stop |= stop_fn(mappedKey, row, col, keyState);

  uint8_t pos_idx = kaleidoscope::addr::addr(row, col);

  Flag& flag = flags[pos_idx];

  if (flag.record_key_state == 0 && keyWasPressed(keyState)) {
    key_overrides[pos_idx] = mappedKey;
  }

  if (keyState != flag.record_key_state) {
    // We only need to record the key if:
    // (1) it toggled off.
    // (2) it previously wasn't pressed.
    if (!keyIsPressed(keyState) || !keyIsPressed(flag.record_key_state)) {
      q_push({ .raw_pos_idx = pos_idx, .raw_key_state = keyState });
      did_update = true;
    }

    should_record_cycle = true;
    flag.record_key_state = keyState;
  }

  return EventHandlerResult::EVENT_CONSUMED;
}

EventHandlerResult IQueue::start_queue(millis_offset_t timeout, IQueueShouldStop stop) {
  switch (state) {
    case State::IDLE:
      stop_fn = stop;
      deadline = Kaleidoscope_::millisAtCycleStart() + timeout;
      state = State::PREPARING;
      return EventHandlerResult::OK;
    case State::PREPARING:
      // start_queue was called twice in the same cycle.
      if (stop_fn == stop) {
        // If it is by the same plugin, ignore the second
        // call (we'll assume timeout was the same).
        return EventHandlerResult::OK;
      } else {
        // ERROR
        return EventHandlerResult::ERROR;
      }
    case State::REPLAY:
      // TODO
      return EventHandlerResult::ERROR;
    default:
      return EventHandlerResult::ERROR;
  }
}

#ifdef CAL_TEST
void IQueue::reset() {
  queue_head = 0;
  queue_len = 0;
  state = State::IDLE;
  stop_after_record = false;
}
#endif

}

custom::IQueue IQueue;
