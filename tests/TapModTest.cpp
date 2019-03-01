#include <gtest/gtest.h>
#include <TapMod.h>
#include <kaleidoscope/key_defs.h>
#include <kaleidoscope/addr.h>
#include <FakeKeyboardBaseTest.h>

// Need a named namespace for friendliness.
namespace custom {

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
    static const PosKey tm1;

    static void verify_state(TapMod::State s0, TapMod::State s3) {
      ASSERT_EQ(TapMod::entries[0].state, s0);
      ASSERT_EQ(TapMod::entries[3].state, s3);
    }
};

const PosKey TapModTest::tm1 = PosKey { Key_TapMod01, 1, 1 };

TEST_F(TapModTest, tmKeyDown_actualKeyDown) {
  verify_state(TapMod::IDLE, TapMod::IDLE);
  cycle({D(tm1)});
  verify({ED(Key_E)});
  verify_state(TapMod::PRESSED_IDLE, TapMod::IDLE);
}

TEST_F(TapModTest, tmKeyHeld_actualKeyHeld) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  cycle({H(tm1)});
  verify({EH(Key_E)});
  verify_state(TapMod::PRESSED_IDLE, TapMod::IDLE);
}

TEST_F(TapModTest, tmKeyReleasedShort_statePressedDelayed) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  cycle({U(tm1)});
  verify({Consumed, EH(Key_E)});
  verify_state(TapMod::PRESSED_DELAYED, TapMod::IDLE);
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
  verify_state(TapMod::IDLE, TapMod::IDLE);

}

TEST_F(TapModTest, tmKeyHeldLong_statePressedReal) {
  cycle({D(tm1)});
  verify({ED(Key_E)});
  inc_millis(200);
  cycle({H(tm1)});
  verify({EH(Key_E)});
  verify_state(TapMod::PRESSED_REAL, TapMod::IDLE);
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
  verify_state(TapMod::IDLE, TapMod::IDLE);
}

}
