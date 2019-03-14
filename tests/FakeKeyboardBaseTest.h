#pragma once

#include <deque>
#include <gtest/gtest.h>
#include <initializer_list>
#include <kaleidoscope/event_handler_result.h>
#include <kaleidoscope/addr.h>
#include <kaleidoscope/hid.h>
#include <kaleidoscope/key_defs.h>
#include <kaleidoscope/keyswitch_state.h>
#include <optional>
#include <utility>
#include <vector>
#include <Kaleidoscope-Hardware-Virtual.h>

using namespace kaleidoscope;

typedef unsigned long ts_millis_t;

typedef EventHandlerResult (*PluginOnKeyswitch)(Key& mappedKey, uint8_t row, uint8_t col, uint8_t keyState);
typedef EventHandlerResult (*PluginBeforeReporting)();
typedef EventHandlerResult (*PluginBeforeCycle)();

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
  bool is_send_report_marker;
};

struct ScanQueueEntry {
  std::vector<FakeKeyEvent> events;
  ts_millis_t millis_post_increment;
};

struct PosKey {
  Key key;
  uint8_t row;
  uint8_t col;

  uint8_t pos() const {
    return kaleidoscope::addr::addr(row, col);
  }

  PosKey noKey() const {
    return PosKey { Key_NoKey, row, col };
  }
};

struct FakeKeyEventResultExpectation {
  EventHandlerResult result;
  std::optional<Key> mappedKey;
  std::optional<Key> originalKey;
  std::optional<uint8_t> keyState;
  std::optional<RCPair> pos;
  bool is_send_report_marker;
};

class FakeKeyboardBaseTest : public ::testing::Test {
  public:
  protected:
    void SetUp() override;

    static void add_keyswitch_handler(PluginOnKeyswitch handler);
    static void add_before_reporting_handler(PluginBeforeReporting handler);
    static void add_before_cycle_handler(PluginBeforeCycle handler);

    static void queue_scan(std::initializer_list<FakeKeyEvent> event, ts_millis_t millis_post_increments = 10);

    static void cycle(std::initializer_list<FakeKeyEvent> events, ts_millis_t total_millis = 20);

    static void verify(std::initializer_list<FakeKeyEventResultExpectation> expectations);

    static void inc_millis(ts_millis_t amount);

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

    /// Expect Down
    static FakeKeyEventResultExpectation ED(Key key) {
      return FakeKeyEventResultExpectation {
        EventHandlerResult::OK, key, std::nullopt, IS_PRESSED, std::nullopt, false
      };
    }

    /// Expect Down
    static FakeKeyEventResultExpectation ED(PosKey key) {
      return FakeKeyEventResultExpectation {
        EventHandlerResult::OK, key.key, std::nullopt, IS_PRESSED, std::make_pair(key.row, key.col), false
      };
    }

    /// Expect Hold
    static FakeKeyEventResultExpectation EH(Key key) {
      return FakeKeyEventResultExpectation {
        EventHandlerResult::OK, key, std::nullopt, IS_PRESSED | WAS_PRESSED, std::nullopt, false
      };
    }

    /// Expect Hold
    static FakeKeyEventResultExpectation EH(PosKey key) {
      return FakeKeyEventResultExpectation {
        EventHandlerResult::OK, key.key, std::nullopt, IS_PRESSED | WAS_PRESSED, std::make_pair(key.row, key.col), false
      };
    }

    /// Expect Up
    static FakeKeyEventResultExpectation EU(Key key) {
      return FakeKeyEventResultExpectation {
        EventHandlerResult::OK, key, std::nullopt, WAS_PRESSED, std::nullopt, false
      };
    }

    /// Expect Up
    static FakeKeyEventResultExpectation EU(PosKey key) {
      return FakeKeyEventResultExpectation {
        EventHandlerResult::OK, key.key, std::nullopt, WAS_PRESSED, std::make_pair(key.row, key.col), false
      };
    }

    static constexpr FakeKeyEventResultExpectation Consumed = FakeKeyEventResultExpectation {
      EventHandlerResult::EVENT_CONSUMED, std::nullopt, std::nullopt, std::nullopt, std::nullopt, false
    };

    static constexpr FakeKeyEventResultExpectation ReportSent = FakeKeyEventResultExpectation {
      EventHandlerResult::OK, std::nullopt, std::nullopt, std::nullopt, std::nullopt, false
    };

  private:
    const static ts_millis_t INITIAL_MILLIS = 100;

    static ts_millis_t current_millis;
    static std::deque<ScanQueueEntry> scan_event_queue;
    static std::vector<FakeKeyEventResult> key_events;
    static std::vector<PluginOnKeyswitch> on_keyswitch_handlers;
    static std::vector<PluginBeforeReporting> before_reporting_handlers;
    static std::vector<PluginBeforeCycle> before_cycle_handlers;

    static void handle_keyswitch_internal(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState);
    static void before_reporting_internal();
    static void before_cycle_internal();

    static void send_report_internal();
    static void act_on_matrix_scan_internal();

    friend class kaleidoscope::Hooks;
    friend ts_millis_t millis_internal();
    friend void handleKeyswitchEvent(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState);
    friend void kaleidoscope::hid::sendKeyboardReport();
    friend void ::Virtual::actOnMatrixScan();

    static std::string mys(EventHandlerResult e) {
      switch (e) {
        case EventHandlerResult::OK:
          return "OK";
        case EventHandlerResult::EVENT_CONSUMED:
          return "EVENT_CONSUMED";
        case EventHandlerResult::ERROR:
          return "ERROR";
      }
    }
};


