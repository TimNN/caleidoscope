#include "FakeKeyboardBaseTest.h"

ts_millis_t FakeKeyboardBaseTest::current_millis = FakeKeyboardBaseTest::INITIAL_MILLIS;

std::vector<FakeKeyEventResult> FakeKeyboardBaseTest::key_events = std::vector<FakeKeyEventResult>();

std::vector<PluginOnKeyswitch> FakeKeyboardBaseTest::on_keyswitch_handlers = std::vector<PluginOnKeyswitch>();

void FakeKeyboardBaseTest::SetUp() {
  Test::SetUp();
  current_millis = INITIAL_MILLIS;
  key_events = std::vector<FakeKeyEventResult>();
  on_keyswitch_handlers = std::vector<PluginOnKeyswitch>();
}

void FakeKeyboardBaseTest::add_keyswitch_handler(PluginOnKeyswitch handler) {
  on_keyswitch_handlers.push_back(handler);
}

ts_millis_t millis_internal() {
  return FakeKeyboardBaseTest::current_millis;
}

void FakeKeyboardBaseTest::cycle(std::initializer_list<FakeKeyEvent> events, ts_millis_t total_millis) {
  ASSERT_TRUE(total_millis % 2 == 0) << "total_millis (" << total_millis << ") divisible by 2";
  ts_millis_t inc = total_millis / 2;

  current_millis += inc;
  for (auto ev : events) { handle_keyswitch_internal(ev.key, ev.row, ev.col, ev.keyState); }
  current_millis += inc;
}

void FakeKeyboardBaseTest::verify(std::initializer_list<FakeKeyEventResultExpectation> expectations) {
  ASSERT_EQ(key_events.size(), expectations.size());

  auto ev = key_events.begin();
  auto ex = expectations.begin();

  for (; ev != key_events.end() && ex != expectations.end(); ev++, ex++) {
    ASSERT_EQ(ev->result, ex->result);
    if (auto mappedKey = ex->mappedKey) { ASSERT_EQ(ev->mappedKey, mappedKey ); }
    if (auto originalKey = ex->originalKey) { ASSERT_EQ(ev->oev.key, originalKey ); }
    if (auto keyState = ex->keyState) { ASSERT_EQ(ev->oev.keyState, keyState ); }
    if (auto pos = ex->pos) {
      ASSERT_EQ(ev->oev.row, pos->first);
      ASSERT_EQ(ev->oev.col, pos->second);
    }
  }

  // Sanity check.
  ASSERT_TRUE(ev == key_events.end() && ex == expectations.end());
}

void FakeKeyboardBaseTest::handle_keyswitch_internal(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  FakeKeyEvent orig = FakeKeyEvent { mappedKey, row, col, keyState };
  EventHandlerResult result = EventHandlerResult::OK;

  for (auto& handler : on_keyswitch_handlers) {
    result = handler(mappedKey, row, col, keyState);
  }

  key_events.push_back(FakeKeyEventResult { orig, mappedKey, result });
}

extern "C" {
  ts_millis_t millis() {
    return millis_internal();
  }
}

void handleKeyswitchEvent(Key mappedKey, uint8_t row, uint8_t col, uint8_t keyState) {
  FakeKeyboardBaseTest::handle_keyswitch_internal(mappedKey, row, col, keyState);
}