#include <kaleidoscope/addr.h>
#include <Kaleidoscope.h>
#include "IQueue.h"

using namespace kaleidoscope;

namespace custom {

IQueue::QWord IQueue::queue[QUEUE_SIZE] =  { 0 };
size_t IQueue::queue_head = 0;
size_t IQueue::queue_len = 0;

uint8_t IQueue::flags[ROWS * COLS];
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

  // TODO
  return EventHandlerResult::ERROR;
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

  uint8_t& flag = flags[pos_idx];

  if (flag == 0 && keyWasPressed(keyState)) {
    key_overrides[pos_idx] = mappedKey;
  }

  if (keyState != flag) {
    // We only need to record the key if:
    // (1) it toggled off.
    // (2) it previously wasn't pressed.
    if (!keyIsPressed(keyState) || !keyIsPressed(flag)) {
      q_push({ .raw_pos_idx = pos_idx, .raw_key_state = keyState });
      did_update = true;
    }

    should_record_cycle = true;
    flag = keyState;
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
