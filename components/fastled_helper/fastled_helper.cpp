#include "esphome.h"

namespace esphome {
namespace fastled_helper {

void FastledHelper::setup() {}  // setup()

#ifdef PALETTES
void FastledHelper::set_current_palette(int index) {
#ifdef MUSIC_LEDS
  if (index >= 0 && index < ARRAY_SIZE(paletteArr) + 4)
#else
  if (index >= 0 && index < ARRAY_SIZE(paletteArr))
#endif
  {
    current_palette = index;
  }
}  // set_script()
#endif

void FastledHelper::dump_config() {
  ESP_LOGCONFIG(TAG, "Fastled Helper version: %s", FASTLED_HELPER_VERSION);
#ifdef PALETTES
  ESP_LOGCONFIG(TAG, "              Palettes: %d", ARRAY_SIZE(paletteArr));
#else
  ESP_LOGCONFIG(TAG, "              Palettes: No");
#endif
#ifdef MUSIC_LEDS
  ESP_LOGCONFIG(TAG, "    Music Leds support: Yes");
#else
  ESP_LOGCONFIG(TAG, "    Music Leds support: No");
#endif
}  // dump_config()

}  // namespace fastled_helper
}  // namespace esphome
