#include <gtest/gtest.h>
#include <TapMod.h>
#include <kaleidoscope/key_defs.h>
#include <kaleidoscope/addr.h>
#include <FakeKeyboardBaseTest.h>

// Need a named namespace for friendliness.
namespace custom {

using State = TapMod::State;

// Test base class with most function definitions.
class TapModTest : public FakeKeyboardBaseTest {
  private:
    static EventHandlerResult tap_mod_on_keyswitch(Key& mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
      return ::TapMod.onKeyswitchEvent(mappedKey, row, col, keyState);
    }

    static EventHandlerResult tap_mod_before_reporting() {
      return ::TapMod.beforeReportingState();
    }

    static EventHandlerResult tap_mod_before_cycle() {
      return ::TapMod.beforeEachCycle();
    }

    // Friendliness is not inherited :(.
    static TapMod::Entry (&entries())[TapMod::ENTRY_CNT] {
      return TapMod::entries;
    }

  public:
    void SetUp() override {
      FakeKeyboardBaseTest::SetUp();
      FakeKeyboardBaseTest::add_keyswitch_handler(tap_mod_on_keyswitch);
      FakeKeyboardBaseTest::add_before_reporting_handler(tap_mod_before_reporting);
      FakeKeyboardBaseTest::add_before_cycle_handler(tap_mod_before_cycle);

      TapMod::reset();
      TapMod::setActual(0, Key_E);
      TapMod::setActual(3, Key_I);
    }

  protected:
    static constexpr PosKey tm1 = PosKey { Key_TapMod01, 1, 1 };
    static constexpr PosKey kn1 = PosKey { Key_C, 2, 1 };

    static void verify_state(TapMod::State s0, TapMod::State s3) {
      ASSERT_EQ(TapMod::entries[0].state, s0);
      ASSERT_EQ(TapMod::entries[3].state, s3);
    }
};

TEST_F(TapModTest, tmKeyDown_actualKeyDown) {
  verify_state(State::IDLE, State::IDLE);
  cycle({D(tm1)});
  verify({ED(Key_E)});
  verify_state(State::PRESSED_IDLE, State::IDLE);
}

TEST_F(TapModTest, tmKeyHeld_actualKeyHeld) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  cycle({H(tm1)});
  verify({EH(Key_E)});
  verify_state(State::PRESSED_IDLE, State::IDLE);
}

TEST_F(TapModTest, tmKeyReleasedShort_statePressedDelayed) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  cycle({U(tm1)});
  verify({Consumed, EH(Key_E)});
  verify_state(State::PRESSED_DELAYED, State::IDLE);
}

TEST_F(TapModTest, pressedDelayed_keyHeldUntilTimeout) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  cycle({U(tm1)});
  verify({Consumed, EH(Key_E)});
  cycle({});
  verify({EH(Key_E)});
  inc_millis(300);
  cycle({});
  verify({EU(Key_E)});
  verify_state(State::IDLE, State::IDLE);
}

TEST_F(TapModTest, pressedDelayed_accectsNextReal) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  cycle({U(tm1)});
  verify({Consumed, EH(Key_E)});
  cycle({D(kn1)});
  verify_state(State::IDLE, State::IDLE);
  cycle({U(kn1)});
  verify({ED(Key_C), EH(Key_E), ReportSent, EU(Key_E), ReportSent, EU(Key_C)});
}

TEST_F(TapModTest, tmKeyHeldLong_statePressedReal) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  inc_millis(200);
  cycle({H(tm1)});
  verify({EH(Key_E)});
  verify_state(State::PRESSED_REAL, State::IDLE);
}

TEST_F(TapModTest, pressedReal_keyHeldUntilReleased) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  inc_millis(200);
  cycle({H(tm1)});
  verify({EH(Key_E)});
  cycle({H(tm1)});
  verify({EH(Key_E)});
  cycle({U(tm1)});
  verify({EU(Key_E)});
  verify_state(State::IDLE, State::IDLE);
}

TEST_F(TapModTest, tmKeyReleasedShort_realKeySameCycle) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  cycle({U(tm1), D(kn1)});
  verify({Consumed, ED(Key_C), EH(Key_E), ReportSent, EU(Key_E)});
  verify_state(State::IDLE, State::IDLE);
}

TEST_F(TapModTest, pressedIdle_realKeyPressed_statePreQueue) {
  cycle({D(tm1)});
  cycle({H(tm1), D(kn1)});
  verify({ED(Key_E), ReportSent, EH(Key_E), ED(Key_C)});
  verify_state(State::PRESSED_PRE_QUEUE, State::IDLE);
}

TEST_F(TapModTest, preQueue_keyHeldLong_statePressedReal) {
  cycle({D(tm1)});
  cycle({H(tm1), D(kn1)});
  verify({ED(Key_E), ReportSent, EH(Key_E), ED(Key_C)});
  cycle({H(tm1), H(kn1)});
  verify({EH(Key_E), EH(Key_C)});
  inc_millis(200);
  cycle({H(tm1), H(kn1)});
  verify({EH(Key_E), EH(Key_C)});
  verify_state(State::PRESSED_REAL, State::IDLE);
}

TEST_F(TapModTest, preQueue_realKeyReleased_statePreQueue) {
  cycle({D(tm1)});
  cycle({H(tm1), D(kn1)});
  verify({ED(Key_E), ReportSent, EH(Key_E), ED(Key_C)});
  cycle({H(tm1), U(kn1)});
  verify({EH(Key_E), EU(Key_C)});
  verify_state(State::PRESSED_PRE_QUEUE, State::IDLE);
}

TEST_F(TapModTest, preQueue_tmKeyReleasedShort_stateIdle) {
  cycle({D(tm1)});
  cycle({H(tm1), D(kn1)});
  verify({ED(Key_E), ReportSent, EH(Key_E), ED(Key_C)});
  cycle({U(tm1), H(kn1)});
  verify({EU(Key_E), EH(Key_C)});
  verify_state(State::IDLE, State::IDLE);
}

}
