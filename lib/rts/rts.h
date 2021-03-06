#ifndef RTS_H_
#define RTS_H_

#include <stdint.h>

namespace rts {

enum class ControlCode {
  // Stop or move to favorite position.
  kMy = 0x1,
  // Move up.
  kUp = 0x2,
  // kMy + kUp
  kMyUp = 0x3,
  // Move down.
  kDown = 0x4,
  // kMy + kDown.
  kMyDown = 0x5,
  // kUp + kDown.
  kUpDown = 0x6,
  // Register or unregister remote.
  kProgram = 0x8,
  // Enable sun and wind detector (SUN and FLAG symbols on Telis Soliris).
  kSunFlag = 0x9,
  // Disable sun detector (FLAG symbol on Telis Soliris).
  kFlag = 0xA,
};

// Frame is an abstraction around the data frame in the Somfy RTS protocol.
//
// References:
// - https://pushstack.wordpress.com/somfy-rts-protocol/
// - United States patent US8189620B2
class Frame {
 public:
  // The total length of the data payload, in bytes.
  static constexpr int kPayloadLength = 7;

  // Initializes an empty frame with 'address' as the 24-bit source address.
  explicit Frame(uint32_t address);
  // Initializes an empty frame.
  Frame() {}

  // Returns the 4-bit counter field. The counter is part of the "key"
  // field. Some RTS receivers will accept a constant value for the counter, but
  // on official remotes, this value increments in lockstep with the rolling
  // code.
  uint8_t counter() const { return counter_; }

  // Sets the 4-bit counter field.
  void set_counter(uint8_t counter);

  // Returns the 4-bit control code. The control code is the RTS command, i.e.,
  // the button that was pushed.
  ControlCode control_code() const { return control_code_; }

  // Sets the 4-bit control code.
  void set_control_code(ControlCode ctrl);

  // Returns the 16-bit rolling code. The rolling code enables commands to be
  // idempotent: once an RTS receiver has seen a rolling code from a given
  // address, it will ignore future commands with the same or lower code. RTS
  // senders broadcast many repeated frames for one command, with the same
  // rolling code, to overcome RF interference. The receiver will execute the
  // command at most once.
  //
  // The rolling code must be incremented for every new command.
  uint16_t rolling_code() const { return rolling_code_; }

  // Sets the 16-bit rolling code.
  void set_rolling_code(uint16_t rolling_code);

  // Returns the 24-bit address of the sender. During the pairing (programming)
  // process, the RTS sender broadcasts a kProgram command to a listening
  // receiver. The receiver remembers the address and the associated rolling
  // code. Future commands from the same address will update the rolling code in
  // the receiver.
  uint32_t address() const { return address_; }

  // Sets the 24-bit address.
  void set_address(uint32_t address);

 private:
  // The counter for the "encryption" key field. 4 bits.
  uint8_t counter_ = 0;

  // Command (up, down, etc).
  ControlCode control_code_ = ControlCode::kMy;

  // The rolling code counter; increased with each command.
  uint16_t rolling_code_ = 0;

  // Address of the sending device. 24 bits.
  uint32_t address_ = 0;
};

// Interface for sending data with an RF transmitter. RTS receivers expect
// ASK-modulated data at 433.42MHz.
class TransmitInterface {
 public:
  // Enable the transmitter. In most implementations, this will set the data pin
  // high.
  virtual void SetHigh() = 0;
  // Disable the transmitter. In most implementations, this will set the data
  // pin low.
  virtual void SetLow() = 0;
  // Wait for 'us' microseconds. Implementations must be accurate to within 10%
  // or so to ensure successful decoding at the receiver.
  virtual void DelayMicroseconds(uint32_t us) = 0;
};

// Interface for reading and writing the rolling code from/to persistent
// storage. The rolling code needs to be persisted because RTS receiver will
// ignore codes they've already seen.
class RollingCodeInterface {
 public:
  // Returns the rolling code stored in persistent storage.
  virtual uint16_t Read() const = 0;
  // Writes a new rolling code to persistent storage.
  virtual void Write(uint16_t rolling_code) = 0;
};

// Controller is a high-level interface for sending RTS protocol commands. It
// deals with the particulars of loading, incrementing, and storing rolling
// codes.
class Controller {
 public:
  // Initializes a controller with the given sender address and interfaces for
  // storage and RF transmission.
  //
  // 'rc' and 'tx' must remain valid for the lifetime of this object.
  Controller(uint32_t address, RollingCodeInterface* rc, TransmitInterface* tx);

  // Sends a single command using the RTS protocol to the RF transmitter.
  void SendControlCode(ControlCode code);

 private:
  Frame frame_;
  RollingCodeInterface* const rc_;  // Not owned.
  TransmitInterface* const tx_;  // Not owned.
};

// Writes a checksummed and obfuscated data frame to '*payload'. '*payload' must
// be at least Frame::kPayloadLength bytes.
void SerializeFrame(const Frame& frame, uint8_t* payload);

// Deserializes a Frame from '*payload' into '*frame' and returns true if
// successful. '*payload' must be at least Frame::kPayloadLength bytes.
bool DeserializeFrame(const uint8_t* payload, Frame* frame);

// Sends a single Frame to an RF transmitter. The transmitted frame is preceded
// by a brief wakeup pulse and the hardware and software synchronization pulses,
// as required by the RTS protocol.
void TransmitFrame(const Frame& frame, TransmitInterface* tx);

}  // namespace rts

#endif  // RTS_H_
