#include <stdint.h>
#include <string.h>
#include <unity.h>

#include "rts.h"

void TestSerializeFrame() {
  static constexpr uint32_t kAddress = 0xC0FFEE;
  rts::Frame frame(kAddress);
  frame.set_counter(7);
  frame.set_control_code(rts::ControlCode::kProgram);
  frame.set_rolling_code(51);

  // Expected serialized, checksummed, obfuscated frame.
  const uint8_t kExpected[] = {0xA7, 0x2E, 0x2E, 0x1D, 0xF3, 0x0C, 0xCC};

  uint8_t payload[rts::Frame::kPayloadLength];
  rts::SerializeFrame(frame, payload);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(kExpected, payload, sizeof(payload));
}

void TestDeserializeFrame_Valid() {
  static const uint8_t kPayload[] = {0xA7, 0x2E, 0x2E, 0x1D, 0xF3, 0x0C, 0xCC};

  rts::Frame frame;
  TEST_ASSERT_TRUE(DeserializeFrame(kPayload, &frame));

  TEST_ASSERT_EQUAL(7, frame.counter());
  TEST_ASSERT_EQUAL(rts::ControlCode::kProgram, frame.control_code());
  TEST_ASSERT_EQUAL(51, frame.rolling_code());
  TEST_ASSERT_EQUAL_HEX(0xC0FFEE, frame.address());
}

void TestDeserializeFrame_BadChecksum() {
  static const uint8_t kPayload[] = {0xFF, 0xDE, 0x73, 0x8C, 0x4C, 0x6D, 0x2F};

  rts::Frame frame;
  TEST_ASSERT_FALSE(DeserializeFrame(kPayload, &frame));
}

void TestSerializeDeserialize() {
  static constexpr uint32_t kAddress = 0xC0FFEE;
  rts::Frame expected_frame(kAddress);
  expected_frame.set_counter(15);
  expected_frame.set_control_code(rts::ControlCode::kMy);
  expected_frame.set_rolling_code(1000);

  uint8_t payload[rts::Frame::kPayloadLength];
  SerializeFrame(expected_frame, payload);

  rts::Frame deserialized;
  TEST_ASSERT_TRUE(DeserializeFrame(payload, &deserialized));

  TEST_ASSERT_EQUAL(expected_frame.counter(), deserialized.counter());
  TEST_ASSERT_EQUAL(expected_frame.control_code(), deserialized.control_code());
  TEST_ASSERT_EQUAL(expected_frame.rolling_code(), deserialized.rolling_code());
  TEST_ASSERT_EQUAL_HEX(expected_frame.address(), deserialized.address());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(TestSerializeFrame);
  RUN_TEST(TestDeserializeFrame_Valid);
  RUN_TEST(TestDeserializeFrame_BadChecksum);
  RUN_TEST(TestSerializeDeserialize);

  UNITY_END();
  return 0;
}
