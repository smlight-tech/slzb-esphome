#include "music_leds.h"
#include "music_leds_effect.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace music_leds {

MusicLedsLightEffect::MusicLedsLightEffect(const char *name) : AddressableLightEffect(name) {}

void MusicLedsLightEffect::start() {
  ESP_LOGD(TAG, "Effect: %s", this->get_name());

  if (this->music_leds_) {
    this->music_leds_->StartFrame();
  }
  AddressableLightEffect::start();
}

void MusicLedsLightEffect::stop() { AddressableLightEffect::stop(); }

void MusicLedsLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
  if (this->music_leds_) {
    this->music_leds_->ShowFrame((PLAYMODE) this->mode_, current_color, &it);
    it.schedule_show();
  }
}

}  // namespace music_leds
}  // namespace esphome
