#include "rts.h"

#include <stdint.h>
#include <string.h>

namespace rts {

namespace {

// Length of a symbol for the RTS protocol.
//
// The analysis on https://pushstack.wordpress.com/somfy-rts-protocol/ lists the
// symbol width of 1208us (not 1280us). The width of 1280us is from a Telis 4
// RTS remote (FCC ID DWNTELIS4), observed with a HackRF. 1280us is the value
// cited in patent US8189620.
constexpr int kSymbolUs = 1280;

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

void Pulse(const int us, TransmitInterface* const tx) {
  tx->SetHigh();
  tx->DelayMicroseconds(us);
  tx->SetLow();
}

void WakeupPulse(TransmitInterface* const tx) {
  Pulse(/*us=*/10000, tx);
  tx->DelayMicroseconds(38000);
}

void HardwareSync(int iterations, TransmitInterface* const tx) {
  for (; iterations > 0; --iterations) {
    Pulse(2500, tx);
    tx->DelayMicroseconds(2500);
  }
}

void SoftwareSync(TransmitInterface* const tx) {
  Pulse(4800, tx);
  tx->DelayMicroseconds(kSymbolUs / 2);
}

// Pulses 'byte' to kRfPin with one bit per symbol and Manchester encoding, MSB
// first.
void ShiftOutByte(const uint8_t byte, TransmitInterface* const tx) {
  for (int i = 7; i >= 0; --i) {
    const int bit = (byte >> i) & 0x1;
    if (bit == 1) {
      tx->SetLow();
      tx->DelayMicroseconds(kSymbolUs / 2);
      tx->SetHigh();
      tx->DelayMicroseconds(kSymbolUs / 2);
    } else {
      tx->SetHigh();
      tx->DelayMicroseconds(kSymbolUs / 2);
      tx->SetLow();
      tx->DelayMicroseconds(kSymbolUs / 2);
    }
  }
  tx->SetLow();
}

void ShiftOutFrame(const Frame& frame, TransmitInterface* const tx) {
  uint8_t payload[Frame::kPayloadLength];
  SerializeFrame(frame, payload);

  // Payload, Manchester encoded, with one bit per kSymbolUs.
  //
  //   Zero: half-symbol high, half-symbol low.
  //    One: half-symbol low, half-symbol high.
  for (unsigned int i = 0; i < sizeof(payload); ++i) {
    ShiftOutByte(payload[i], tx);
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

void TransmitFrame(const Frame& frame, TransmitInterface* const tx) {
  WakeupPulse(tx);

  // Initial frame.
  HardwareSync(/*iterations=*/2, tx);
  SoftwareSync(tx);
  ShiftOutFrame(frame, tx);

  // Repeated frames.
  for (int i = 0; i < 5; ++i) {
    // ~34ms of silence before the next hardware sync according to US8189620B2.
    tx->DelayMicroseconds(34000);
    HardwareSync(/*iterations=*/6, tx);
    SoftwareSync(tx);
    ShiftOutFrame(frame, tx);
  }

  tx->DelayMicroseconds(34000);
}

}  // namespace rts
