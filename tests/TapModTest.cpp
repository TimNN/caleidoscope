#include <gtest/gtest.h>
#include <TapMod.h>
#include <kaleidoscope/key_defs.h>
#include <kaleidoscope/addr.h>

// Need a named namespace for friendliness.
namespace custom {

// Test base class with most function definitions.
class TapModTest : public ::testing::Test {
  public:
    void SetUp() override {
      TapMod::reset();
      TapMod::setActual(0, Key_E);
      TapMod::setActual(3, Key_I);
    }

    static ts_millis_t current_millis;

    static void handleKeyswitchEvent(Key mappedKey, byte row, byte col, uint8_t keyState) {
      ::TapMod.onKeyswitchEvent(mappedKey, row, col, keyState);
    }

  protected:
    // Friendliness is not inherited :(.
    static TapMod::Entry (&entries())[TapMod::ENTRY_CNT] {
      return TapMod::entries;
    }

    static void D(Key key) {
      handleKeyswitchEvent(key, addr::row(key.keyCode), addr::col(key.keyCode), IS_PRESSED);
    }
};

ts_millis_t TapModTest::current_millis = 0;

// Hook up millis().
extern "C" {
  custom::ts_millis_t millis() {
    return TapModTest::current_millis;
  }
}

}

// Need to define this outside the namespace, but after the class definition.
void handleKeyswitchEvent(Key mappedKey, byte row, byte col, uint8_t keyState) {
  custom::TapModTest::handleKeyswitchEvent(mappedKey, row, col, keyState);
}

namespace custom {

TEST_F(TapModTest, Demo) {
  EXPECT_EQ(entries()[0].state, TapMod::IDLE);
  EXPECT_EQ(entries()[3].state, TapMod::IDLE);
  D(Key_TapMod01);
  EXPECT_EQ(entries()[0].state, TapMod::PRESSED_IDLE);
  EXPECT_EQ(entries()[3].state, TapMod::IDLE);
}

}
