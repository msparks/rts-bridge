#include <stdint.h>
#include <string.h>
#include <unity.h>

#include "rts.h"

// FakeTransmitter is a simple (and dumb) implementation of
// rts::TransmitInterface. It operates in virtual time and simulates a virtual
// data pin, and attempts to decode RTS data.
class FakeTransmitter : public rts::TransmitInterface {
 public:
  FakeTransmitter() {
    memset(payload_, 0, sizeof(payload_));
  }

  void SetHigh() override { pin_ = true; }
  void SetLow() override { pin_ = false; }

  void DelayMicroseconds(uint32_t us) override {
    switch (state_) {
      case State::kUnknown:
        if (!pin_ && us >= 30000) {
          // Wakeup pulse or an inter-frame spacing.
          state_ = State::kWakeup;
        }
        break;

      case State::kWakeup:
      case State::kHwSync:
        if (!pin_ && us == 2500) {
          // Delay after a hardware sync pulse.
          state_ = State::kHwSync;
        } else if (!pin_ && us == kSymbolUs / 2) {
          // Delay after a software sync pulse.
          state_ = State::kSwSync;
        }
        break;

      case State::kSwSync:
        if (us == kSymbolUs / 2) {
          // Received first half of a payload bit. The payload is
          // Manchester-encoded and edge-triggered, so we need to wait for the
          // second half of the symbol to read the bit.
          state_ = State::kPayload;
        } else if (!pin_ && us >= 30000) {
          // Inter-frame spacing. Reset state machine to read the next frame.
          state_ = State::kWakeup;
        }
        break;

      case State::kPayload:
        if (us == kSymbolUs / 2) {
          // Received the second half of a payload bit.

          if (bits_read_ >= sizeof(payload_) * 8) {
            // Probably reading a repeated frame; reset.
            bits_read_ = 0;
          }

          const uint8_t bit = pin_ ? 1 : 0;
          const int index = bits_read_ / 8;
          payload_[index] = (payload_[index] << 1) | bit;
          ++bits_read_;

          // Reset to kSwSync to read the next payload bit.
          state_ = State::kSwSync;
        }
        break;
    }

    time_ += us;
  }

  // Returns the pointer to the last captured payload. If multiple frames are
  // transmitted at once, only the last payload is returned.
  uint8_t* payload() { return payload_; }

  // Number of bits read for the most recent payload.
  unsigned int bits_read() const { return bits_read_; }

 private:
  enum class State {
    kUnknown,
    kWakeup,
    kHwSync,
    kSwSync,
    kPayload,
  };

  static constexpr int kSymbolUs = 1280;
  uint8_t payload_[rts::Frame::kPayloadLength];
  // How many bits of the payload have been received.
  unsigned int bits_read_ = 0;
  // The last completed part of the transmission.
  State state_ = State::kUnknown;
  // Virtual time in microseconds.
  uint32_t time_ = 0;
  // Whether the transmitter is enabled (high: true) or disabled (low: false).
  bool pin_ = false;
};

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

void TestTransmitFrame() {
  FakeTransmitter tx;

  static constexpr uint32_t kAddress = 0xC0FFEE;
  rts::Frame expected_frame(kAddress);
  expected_frame.set_counter(15);
  expected_frame.set_control_code(rts::ControlCode::kMy);
  expected_frame.set_rolling_code(1000);

  TransmitFrame(expected_frame, &tx);
  TEST_ASSERT_EQUAL(rts::Frame::kPayloadLength * 8, tx.bits_read());

  rts::Frame deserialized;
  TEST_ASSERT_TRUE(DeserializeFrame(tx.payload(), &deserialized));

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
  RUN_TEST(TestTransmitFrame);

  UNITY_END();
  return 0;
}
