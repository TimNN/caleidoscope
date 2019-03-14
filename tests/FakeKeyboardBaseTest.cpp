#include <kaleidoscope/Kaleidoscope.h>
// Kaleidoscope.h includes Arduino.h, which provides some annoying #defines
#undef abs
#undef round

#include "FakeKeyboardBaseTest.h"

ts_millis_t FakeKeyboardBaseTest::current_millis = FakeKeyboardBaseTest::INITIAL_MILLIS;

std::deque<ScanQueueEntry> FakeKeyboardBaseTest::scan_event_queue = std::deque<ScanQueueEntry>();
std::vector<FakeKeyEventResult> FakeKeyboardBaseTest::key_events = std::vector<FakeKeyEventResult>();

std::vector<PluginOnKeyswitch> FakeKeyboardBaseTest::on_keyswitch_handlers = std::vector<PluginOnKeyswitch>();
std::vector<PluginBeforeReporting> FakeKeyboardBaseTest::before_reporting_handlers = std::vector<PluginBeforeReporting>();
std::vector<PluginBeforeCycle> FakeKeyboardBaseTest::before_cycle_handlers = std::vector<PluginBeforeCycle>();

void FakeKeyboardBaseTest::SetUp() {
  Test::SetUp();
  current_millis = INITIAL_MILLIS;
  key_events = std::vector<FakeKeyEventResult>();
  on_keyswitch_handlers = std::vector<PluginOnKeyswitch>();
  before_reporting_handlers = std::vector<PluginBeforeReporting>();
  before_cycle_handlers = std::vector<PluginBeforeCycle>();
}

void FakeKeyboardBaseTest::add_keyswitch_handler(PluginOnKeyswitch handler) {
  on_keyswitch_handlers.push_back(handler);
}

void FakeKeyboardBaseTest::add_before_reporting_handler(PluginBeforeReporting handler) {
  before_reporting_handlers.push_back(handler);
}

void FakeKeyboardBaseTest::add_before_cycle_handler(PluginBeforeCycle handler) {
  before_cycle_handlers.push_back(handler);
}

ts_millis_t millis_internal() {
  return FakeKeyboardBaseTest::current_millis;
}

void FakeKeyboardBaseTest::queue_scan(std::initializer_list<FakeKeyEvent> events, ts_millis_t millis_post_increment) {
  scan_event_queue.push_back(ScanQueueEntry {
    std::vector<FakeKeyEvent>(events),
    millis_post_increment,
  });
}

void FakeKeyboardBaseTest::cycle(std::initializer_list<FakeKeyEvent> events, ts_millis_t total_millis) {
  ASSERT_TRUE(total_millis % 4 == 0) << "total_millis (" << total_millis << ") divisible by 4";
  ts_millis_t inc = total_millis / 4;

  current_millis += inc;
  kaleidoscope::Kaleidoscope_::setMillisAtCycleStart(current_millis);
  before_cycle_internal();
  current_millis += inc;
  for (auto ev : events) { handle_keyswitch_internal(ev.key, ev.row, ev.col, ev.keyState); }
  current_millis += inc;
  before_reporting_internal();
  current_millis += inc;
  send_report_internal();
}

void FakeKeyboardBaseTest::verify(std::initializer_list<FakeKeyEventResultExpectation> raw_expectations) {
  std::vector<FakeKeyEventResultExpectation> expectations(raw_expectations);
  expectations.push_back(ReportSent);

  ASSERT_EQ(key_events.size(), expectations.size());

  auto ev = key_events.begin();
  auto ex = expectations.begin();

  for (; ev != key_events.end() && ex != expectations.end(); ev++, ex++) {
    ASSERT_EQ(ev->result, ex->result);
    if (auto mappedKey = ex->mappedKey) { ASSERT_EQ(ev->mappedKey, *mappedKey ); }
    if (auto originalKey = ex->originalKey) { ASSERT_EQ(ev->oev.key, *originalKey ); }
    if (auto keyState = ex->keyState) { ASSERT_EQ(ev->oev.keyState, *keyState ); }
    if (auto pos = ex->pos) {
      ASSERT_EQ(ev->oev.row, pos->first);
      ASSERT_EQ(ev->oev.col, pos->second);
    }
  }

  // Sanity check.
  ASSERT_TRUE(ev == key_events.end() && ex == expectations.end());

  key_events.clear();
}

void FakeKeyboardBaseTest::inc_millis(ts_millis_t amount) {
  current_millis += amount;
}

void FakeKeyboardBaseTest::handle_keyswitch_internal(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  FakeKeyEvent orig = FakeKeyEvent { mappedKey, row, col, keyState };
  EventHandlerResult result = EventHandlerResult::OK;

  for (auto& handler : on_keyswitch_handlers) {
    result = handler(mappedKey, row, col, keyState);
    ASSERT_TRUE(result == EventHandlerResult::OK || result == EventHandlerResult::EVENT_CONSUMED)
        << "Invalid event handler result: " << mys(result);
    if (result == EventHandlerResult::EVENT_CONSUMED) {
      break;
    }
  }

  key_events.push_back(FakeKeyEventResult { orig, mappedKey, result, false });
}

void FakeKeyboardBaseTest::before_reporting_internal() {
  for (auto& handler : before_reporting_handlers) {
    EventHandlerResult result = handler();
    ASSERT_TRUE(result == EventHandlerResult::OK || result == EventHandlerResult::EVENT_CONSUMED)
                  << "Invalid event handler result: " << mys(result);
  }
}

void FakeKeyboardBaseTest::before_cycle_internal() {
  for (auto& handler : before_cycle_handlers) {
    EventHandlerResult result = handler();
    ASSERT_TRUE(result == EventHandlerResult::OK || result == EventHandlerResult::EVENT_CONSUMED)
                  << "Invalid event handler result: " << mys(result);
  }
}

void FakeKeyboardBaseTest::send_report_internal() {
  FakeKeyEventResult expect = {};
  expect.is_send_report_marker = true;
  key_events.push_back(expect);
}

void FakeKeyboardBaseTest::act_on_matrix_scan_internal() {
  ASSERT_FALSE(scan_event_queue.empty());

  for (auto ev : scan_event_queue.front().events) { handle_keyswitch_internal(ev.key, ev.row, ev.col, ev.keyState); }

  current_millis += scan_event_queue.front().millis_post_increment;

  scan_event_queue.pop_front();
}

extern "C" {
  ts_millis_t millis() {
    return millis_internal();
  }
}

void handleKeyswitchEvent(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  FakeKeyboardBaseTest::handle_keyswitch_internal(mappedKey, row, col, keyState);
}

namespace kaleidoscope {

uint32_t Kaleidoscope_::millis_at_cycle_start_ = 0;

}

namespace kaleidoscope::hid {

void sendKeyboardReport() {
  FakeKeyboardBaseTest::send_report_internal();
}

}

Virtual::Virtual() {}

void Virtual::readMatrix() {}

void Virtual::actOnMatrixScan() {
  FakeKeyboardBaseTest::act_on_matrix_scan_internal();
}

HARDWARE_IMPLEMENTATION KeyboardHardware;
