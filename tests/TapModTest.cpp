#include <gtest/gtest.h>
#include <TapMod.h>
#include <kaleidoscope/key_defs.h>
#include <kaleidoscope/addr.h>

extern "C" {
  custom::ts_millis_t millis() {
    return 0;
  }
}

void handleKeyswitchEvent(Key mappedKey, byte row, byte col, uint8_t keyState) {
  TapMod.onKeyswitchEvent(mappedKey, row, col, keyState);
}

namespace custom {

class TapModTest : public ::testing::Test {
  public:
    void SetUp() override {
      TapMod::reset();
      TapMod::setActual(0, Key_E);
      TapMod::setActual(3, Key_I);
    }

  protected:
    static TapMod::Entry (&entries())[TapMod::ENTRY_CNT] {
      return TapMod::entries;
    }

    static void D(Key key) {
      handleKeyswitchEvent(key, addr::row(key.keyCode), addr::col(key.keyCode), IS_PRESSED);
    }
};

TEST_F(TapModTest, Demo) {
  EXPECT_EQ(entries()[0].state, TapMod::IDLE);
  EXPECT_EQ(entries()[3].state, TapMod::IDLE);
  D(Key_TapMod01);
  EXPECT_EQ(entries()[0].state, TapMod::PRESSED_IDLE);
  EXPECT_EQ(entries()[3].state, TapMod::IDLE);
}

}
