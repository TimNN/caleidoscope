#pragma once

#include <kaleidoscope/plugin.h>
#include <kaleidoscope/key_defs.h>
#include <Kaleidoscope.h>

#define try_eh(EXP) do { ::kaleidoscope::EventHandlerResult _res = EXP; if (res != ::kaleidoscope::EventHandlerResult::OK) return _res;  } while (0)

namespace custom {

using namespace kaleidoscope;

typedef bool (*IQueueShouldStop)(Key key, uint8_t row, uint8_t col, uint8_t keyState);

typedef unsigned long ts_millis_t;

typedef uint16_t millis_offset_t;

class IQueue : public Plugin {
  friend class IQueueTest;

  public:
    EventHandlerResult beforeEachCycle();
    EventHandlerResult onKeyswitchEvent(Key &mappedKey, uint8_t row, uint8_t col, uint8_t keyState);

    static EventHandlerResult start_queue(millis_offset_t timeout, IQueueShouldStop stop);

    enum class State : uint8_t {
        IDLE = 0,
        PREPARING,
        RECORD,
        REPLAY,
    };

    /// Each cycle that needs to replayed starts with a 15bit timestamp, relative to
    /// recoding start. If the high-bit is set in the word containing the timestamp,
    /// this cycle does not have any explicit updates and the next timestamp follows
    /// immediately afterwards. Otherwise, a number of updates follow. These contain
    /// mainly a 7bit index representing the key position of this update and an 8bit
    /// representation of the key state. The remaining bit is set to mark the end of
    /// the current cycle. A timestamp follows afterwards.
    union QWord {
      uint16_t raw;
      struct {
        millis_offset_t millis_offset : 15;
        bool no_explicit_updates : 1;
      };
      struct {
        uint8_t pos_idx : 7;
        bool is_last_update : 1;
        uint8_t key_state;
      };
      struct {
        uint8_t raw_pos_idx;
        uint8_t raw_key_state;
      };
    };

  private:
    static constexpr size_t QUEUE_SIZE = 32;

    static QWord queue[QUEUE_SIZE];
    static size_t queue_head;
    static size_t queue_len;

    static uint8_t flags[ROWS * COLS];
    static Key key_overrides[ROWS * COLS];

    static State state;
    static IQueueShouldStop stop_fn;
    static ts_millis_t deadline;
    static bool should_stop;
    static bool did_update;
    static bool should_record_cycle;

    static void q_push(QWord item) {
      if (queue_len < QUEUE_SIZE) {
        queue[(queue_head + queue_len) % QUEUE_SIZE] = item;
        queue_len += 1;
      }
    }

    static QWord q_pop() {
      if (queue_len > 0) {
        QWord item = queue[queue_head];
        queue_head = (queue_head + 1) % QUEUE_SIZE;
        queue_len -= 1;
        return item;
      }

      // FIXME: This is an error.
      return { 0 };
    }

    static QWord& q_peek(size_t idx = 0) {
      if (idx < queue_len) {
        return queue[(queue_head + idx) % QUEUE_SIZE];
      }

      // FIXME: This should _really_ return an error :(
      return queue[0];
    }

    static QWord& q_peek_last() {
      // FIXME: Ugh. This works for len=0 because unsigned types underflow...
      return q_peek(queue_len - 1);
    }

#ifdef CAL_TEST
    static bool stop_after_record;

    // For friendly test.
    static void reset();
#endif
};

static_assert (sizeof(IQueue::QWord) == 2, "Expected IQueue::QWord to have size 2.");
static_assert (WAS_PRESSED == 0x01, "Expected WAS_PRESSED at bit[0].");
static_assert (IS_PRESSED == 0x02, "Expected IS_PRESSED at bit[1].");
static_assert (IS_PRESSED == 0x02, "Expected IS_PRESSED at bit[1].");
static_assert ((uint32_t)ROWS * (uint32_t)COLS <= 0x7F, "Too many keys.");

}

extern custom::IQueue IQueue;
