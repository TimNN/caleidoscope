#include <gtest/gtest.h>
#include <IQueue.h>
#include <FakeKeyboardBaseTest.h>

// Need a named namespace for friendliness.
namespace custom {

using State = IQueue::State;
using QWord = IQueue::QWord;

// Test base class with most function definitions.
class IQueueTest : public FakeKeyboardBaseTest {
  protected:
    static bool should_start_queuing;

  private:
    static EventHandlerResult iqueue_on_keyswitch(Key& mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
      return ::IQueue.onKeyswitchEvent(mappedKey, row, col, keyState);
    }

    static EventHandlerResult iqueue_before_cycle() {
      return ::IQueue.beforeEachCycle();
    }

    static bool should_stop_queuing(Key key, uint8_t row, uint8_t col, uint8_t keyState) {
      return key == Key_Z;
    }

    static EventHandlerResult inject_on_keyswitch(Key& mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
      if (should_start_queuing) {
        should_start_queuing = false;
        return IQueue::start_queue(400, should_stop_queuing);
      }

      return EventHandlerResult::OK;
    }

  public:
    void SetUp() override {
      FakeKeyboardBaseTest::SetUp();
      FakeKeyboardBaseTest::add_keyswitch_handler(iqueue_on_keyswitch);
      FakeKeyboardBaseTest::add_before_cycle_handler(iqueue_before_cycle);
      FakeKeyboardBaseTest::add_keyswitch_handler(inject_on_keyswitch);

      IQueue::reset();

      should_start_queuing = false;
    }

  protected:
    static constexpr PosKey kA = PosKey { Key_A, 1, 1 };
    static constexpr PosKey kB = PosKey { Key_B, 1, 2 };
    static constexpr PosKey kC = PosKey { Key_C, 1, 3 };
    static constexpr PosKey kStop = PosKey { Key_Z, 2, 1 };

    static void verify_state(State s) {
      ASSERT_EQ(IQueue::state, s);
    }

    static void verify_queue(std::initializer_list<QWord> raw_items) {
      std::vector<QWord> items(raw_items);
      ASSERT_EQ(IQueue::queue_len, items.size());

      for (size_t i = 0; i < items.size(); i++) {
        ASSERT_EQ(IQueue::q_peek(i).raw, items.at(i).raw);
      }
    }

    static void stop_after_record() {
      IQueue::stop_after_record = true;
    }
};

bool IQueueTest::should_start_queuing = false;

TEST_F(IQueueTest, idle_passesThrough) {
  cycle({});
  verify({});
  cycle({D(kA), U(kB), H(kC)});
  verify({ED(Key_A), EU(Key_B), EH(Key_C)});
}

TEST_F(IQueueTest, startQueing_statePreparing) {
  should_start_queuing = true;
  cycle({D(kA), U(kB), H(kC)});
  verify({ED(Key_A), EU(Key_B), EH(Key_C)});
  verify_state(State::PREPARING);
}

TEST_F(IQueueTest, recordStuff) {
  should_start_queuing = true;
  stop_after_record();
  cycle({U(kA)}); // Need a dummy event to trigger queuing.
  verify({EU(Key_A)});
  verify_state(State::PREPARING);
  // Start with one of each. [=> all should be recorded]
  queue_scan({D(kA), U(kB), H(kC)});
  // Hold A & C. [=> implicit update]
  queue_scan({H(kA), H(kC)});
  // Hold A & C. [=> no update]
  queue_scan({H(kA), H(kC)});
  // Release A & press B. [=> explicit update for A and B (and Z)]
  queue_scan({U(kA), D(kB), H(kC), D(kStop)});
  cycle({});
  verify({Consumed, Consumed, Consumed,
          Consumed, Consumed,
          Consumed, Consumed,
          Consumed, Consumed, Consumed, Consumed});
  verify_state(State::RECORD);
  verify_queue({
    { .millis_offset = 0 },
      { .pos_idx = kA.pos(), .is_last_update = false, .key_state = IS_PRESSED },
      { .pos_idx = kB.pos(), .is_last_update = false, .key_state = WAS_PRESSED },
      { .pos_idx = kC.pos(), .is_last_update = true, .key_state = IS_PRESSED | WAS_PRESSED },
    { .millis_offset = 10, .no_explicit_updates = true },
    { .millis_offset = 30 },
      { .pos_idx = kA.pos(), .is_last_update = false, .key_state = WAS_PRESSED },
      { .pos_idx = kB.pos(), .is_last_update = false, .key_state = IS_PRESSED },
      { .pos_idx = kStop.pos(), .is_last_update = true, .key_state = IS_PRESSED }});
}

}
