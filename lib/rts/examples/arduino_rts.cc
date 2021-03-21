#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#include "rts.h"

// Data pin of the RF transmitter.
static constexpr int kRfPin = 5;

namespace {

// Same as delayMicroseconds(), but accurate for delays longer than 16ms.
void LongDelayMicroseconds(int32_t us) {
  // The documentation for delayMicroseconds() says, "the largest value that
  // will produce an accurate delay is 16383".
  static constexpr int kThresholdUs = 16000;
  if (us < 0) {
    return;
  }
  if (us <= kThresholdUs) {
    delayMicroseconds(us);
    return;
  }
  delayMicroseconds(kThresholdUs);
  LongDelayMicroseconds(us - kThresholdUs);
}

class Transmitter : public rts::TransmitInterface {
 public:
  explicit Transmitter(const int pin) : pin_(pin) {}

  void SetHigh() override { digitalWrite(pin_, HIGH); }
  void SetLow() override { digitalWrite(pin_, LOW); }
  void DelayMicroseconds(const uint32_t us) override {
    LongDelayMicroseconds(us);
  }

 private:
  const int pin_;
};

class RollingCodeStorage : public rts::RollingCodeInterface {
 public:
  uint16_t Read() const override {
    uint16_t rolling_code;
    EEPROM.get(0, rolling_code);
    return rolling_code;
  }

  void Write(uint16_t rolling_code) override {
    EEPROM.put(0, rolling_code);
  }
};

// Globals.
RollingCodeStorage g_rc;
Transmitter g_tx(kRfPin);
rts::Controller g_controller(/*address=*/0xC0FFEE, &g_rc, &g_tx);

} // namespace

void setup() {
  pinMode(kRfPin, OUTPUT);

  // Send a Program command. Before this command is sent, put the shade into
  // programming mode by holding the program button on an *existing* remote
  // control until the shade jogs. To unregister, repeat this process.
  g_controller.SendControlCode(rts::ControlCode::kProgram);
}

void loop() {
  // Send an Up command every 10 seconds.
  g_controller.SendControlCode(rts::ControlCode::kUp);
  delay(10000);
}
