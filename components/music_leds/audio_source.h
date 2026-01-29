#pragma once

#include <stddef.h>

#include "esphome/components/i2s_audio/microphone/i2s_audio_microphone.h"

namespace esphome {
namespace music_leds {

class AudioSource : public i2s_audio::I2SAudioMicrophone {
 public:
  using i2s_audio::I2SAudioMicrophone::read_;
};

}  // namespace music_leds
}  // namespace esphome
