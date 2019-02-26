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

  public:
    void SetUp() override {
      FakeKeyboardBaseTest::SetUp();
      FakeKeyboardBaseTest::add_keyswitch_handler(TapModTest::tap_mod_on_keyswitch);

      TapMod::reset();
      TapMod::setActual(0, Key_E);
      TapMod::setActual(3, Key_I);
    }

  protected:
    static const PosKey tm1;

    // Friendliness is not inherited :(.
    static TapMod::Entry (&entries())[TapMod::ENTRY_CNT] {
      return TapMod::entries;
    }
};

const PosKey TapModTest::tm1 = PosKey { Key_TapMod01, 1, 1 };

TEST_F(TapModTest, Demo) {
  EXPECT_EQ(entries()[0].state, TapMod::IDLE);
  EXPECT_EQ(entries()[3].state, TapMod::IDLE);
  cycle({D(tm1)});
  EXPECT_EQ(entries()[0].state, TapMod::PRESSED_IDLE);
  EXPECT_EQ(entries()[3].state, TapMod::IDLE);
  verify({ED(Key_E)});
}

}
