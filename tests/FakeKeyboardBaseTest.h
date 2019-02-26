#pragma once

#include <gtest/gtest.h>
#include <initializer_list>
#include <kaleidoscope/key_defs.h>
#include <kaleidoscope/event_handler_result.h>
#include <kaleidoscope/keyswitch_state.h>
#include <optional>
#include <utility>
#include <vector>

using namespace kaleidoscope;

typedef unsigned long ts_millis_t;

typedef EventHandlerResult (*PluginOnKeyswitch)(Key& mappedKey, uint8_t row, uint8_t col, uint8_t keyState);

typedef std::pair<uint8_t, uint8_t> RCPair;

struct FakeKeyEvent {
  Key key;
  uint8_t row;
  uint8_t col;
  uint8_t keyState;
};

struct FakeKeyEventResult {
  FakeKeyEvent oev;
  Key mappedKey;
  EventHandlerResult result;
};

struct PosKey {
  Key key;
  uint8_t row;
  uint8_t col;
};

struct FakeKeyEventResultExpectation {
  EventHandlerResult result;
  std::optional<Key> mappedKey;
  std::optional<Key> originalKey;
  std::optional<uint8_t> keyState;
  std::optional<RCPair> pos;
};

class FakeKeyboardBaseTest : public ::testing::Test {
  public:
  protected:
    void SetUp() override;

    static void add_keyswitch_handler(PluginOnKeyswitch handler);

    static void cycle(std::initializer_list<FakeKeyEvent> events, ts_millis_t total_millis = 10);

    static void verify(std::initializer_list<FakeKeyEventResultExpectation> expectations);

    /// Down
    static FakeKeyEvent D(PosKey key) {
      return FakeKeyEvent { key.key, key.row, key.col, IS_PRESSED };
    }

    /// Hold
    static FakeKeyEvent H(PosKey key) {
      return FakeKeyEvent { key.key, key.row, key.col, IS_PRESSED | WAS_PRESSED };
    }

    /// Up
    static FakeKeyEvent U(PosKey key) {
      return FakeKeyEvent { key.key, key.row, key.col, WAS_PRESSED };
    }

    static FakeKeyEventResultExpectation ED(Key key) {
      return FakeKeyEventResultExpectation {
        EventHandlerResult::OK, key, std::nullopt, IS_PRESSED, std::nullopt
      };
    }

  private:
    const static ts_millis_t INITIAL_MILLIS = 100;

    static ts_millis_t current_millis;
    static std::vector<FakeKeyEventResult> key_events;
    static std::vector<PluginOnKeyswitch> on_keyswitch_handlers;

    static void handle_keyswitch_internal(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState);

    friend ts_millis_t millis_internal();
    friend void handleKeyswitchEvent(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState);
};


