#pragma once

#include "esphome/components/light/addressable_light_effect.h"

namespace esphome {
namespace music_leds {

class MusicLeds;

class MusicLedsLightEffect : public light::AddressableLightEffect {
 public:
  MusicLedsLightEffect(const char *name);

  void start() override;
  void stop() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
  void set_mode(uint8_t mode) { this->mode_ = mode; }
  void set_music_leds(MusicLeds *music_leds) { this->music_leds_ = music_leds; }

 protected:
  uint8_t mode_{0};
  MusicLeds *music_leds_{nullptr};
};

}  // namespace music_leds
}  // namespace esphome
