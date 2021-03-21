#include "rts.h"

#include <stdint.h>
#include <string.h>

namespace rts {

namespace {

// Returns the checksum of the Frame serialized in '*payload'. The checksum
// field must be set to 0 before calling this function.
uint8_t Checksum(const uint8_t* const payload) {
  uint8_t checksum = 0;
  // XOR all nibbles.
  for (int i = 0; i < Frame::kPayloadLength; ++i) {
    checksum ^= payload[i] ^ (payload[i] >> 4);
  }

  // The resulting checksum is also 4 bits.
  checksum &= 0xF;
  return checksum;
}

// Obfuscate the serialized Frame bytes in '*payload' by XORing every nth byte
// with the (n-1)th byte, n > 0.
void Obfuscate(uint8_t* const payload) {
  for (int i = 1; i < Frame::kPayloadLength; ++i) {
    payload[i] ^= payload[i - 1];
  }
}

// Reverse obfuscate.
void Deobfuscate(uint8_t* const payload) {
  for (int i = Frame::kPayloadLength - 1; i > 0; --i) {
    payload[i] ^= payload[i - 1];
  }
}

}  // namespace

Frame::Frame(const uint32_t address) { set_address(address); }

void Frame::set_counter(const uint8_t counter) { counter_ = counter & 0xF; }

void Frame::set_control_code(const ControlCode ctrl) { control_code_ = ctrl; }

void Frame::set_rolling_code(const uint16_t rolling_code) {
  rolling_code_ = rolling_code;
}

void Frame::set_address(const uint32_t address) {
  address_ = address & 0xFFFFFF;
}

//   byte
//    0       1        2       3       4       5       6
// |-------|--------|-------|-------|-------|-------|-------|
// |  key  |ctrl|cks|  Rolling Code |   Address(A0|A1|A3)   |
// |-------|--------|-------|-------|-------|-------|-------|
//
// References:
// - https://pushstack.wordpress.com/somfy-rts-protocol/
// - United States patent US8189620B2
void SerializeFrame(const Frame& frame, uint8_t* const payload) {
  // The upper 4 bits are always 0xA.
  payload[0] = (0xA << 4) | (frame.counter() & 0xF);

  // Write the control code first, but leave the checksum as 0 for now.
  payload[1] = static_cast<int>(frame.control_code()) << 4;

  // Rolling code (big endian).
  payload[2] = (frame.rolling_code() & 0xFF00) >> 8;
  payload[3] = (frame.rolling_code() & 0x00FF);

  // Sender address (little endian). Patent US8189620 doesn't make the
  // endianness for the address clear, but on a Telis 4 RTS remote with 5
  // channels, the addresses for each channel are contiguous if this field is
  // treated as little endian.
  payload[4] = (frame.address() & 0x0000FF);
  payload[5] = (frame.address() & 0x00FF00) >> 8;
  payload[6] = (frame.address() & 0xFF0000) >> 16;

  // Compute and update the checksum field.
  payload[1] |= Checksum(payload);

  // Finally, obfuscate the bytes, per the RTS protocol.
  Obfuscate(payload);
}

bool DeserializeFrame(const uint8_t* const payload, Frame* const frame) {
  // Deobfuscate the payload into buf.
  uint8_t buf[Frame::kPayloadLength];
  memcpy(buf, payload, Frame::kPayloadLength);
  Deobfuscate(buf);

  if (Checksum(buf) != 0) {
    return false;
  }

  frame->set_counter(buf[0] & 0xF);
  frame->set_control_code(static_cast<ControlCode>(buf[1] >> 4));
  frame->set_rolling_code((static_cast<uint16_t>(buf[2]) << 8) | buf[3]);
  uint32_t address = static_cast<uint32_t>(buf[6]) << 16;
  address |= static_cast<uint32_t>(buf[5]) << 8;
  address |= buf[4];
  frame->set_address(address);

  return true;
}

}  // namespace rts
